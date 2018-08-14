#include <stdio.h>
#include <fcntl.h>                                                              
#include <assert.h>                                                             
#include <termios.h> 
#include <unistd.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_mcu.h"

int McuHandler = INIT;
int uart_fd = 0;

int connect_mcu() {
    struct termios options;                                                 
    uart_fd = open(UART_PORT_NAME, O_RDWR | O_NOCTTY | O_NONBLOCK);       
    if (uart_fd < 0) {                                                    
        fprintf(stderr,"Open serial port fail!\n");                              
       return -1;
    }                                                                       
                                                                                
    tcflush(uart_fd, TCIOFLUSH);                                          
    if (tcgetattr(uart_fd, &options) < 0) {                               
       fprintf(stderr,"Get port options fail\n");                               
       return -1;
    }                                                                       
                                                                                
    //use raw mode                                                          
    cfmakeraw(&options);                                                    
    options.c_cflag |= CLOCAL;                                              
    //enable/disable RTS/CTS                                                
    if (UART_PORT_FLOWCTRL) {                                               
       options.c_cflag |= CRTSCTS;                                         
    } else {                                                                
       options.c_cflag &= ~CRTSCTS;                                        
    }                                                                       
                                                                                
    cfsetospeed(&options, UART_PORT_BAUDRATE);                              
    cfsetispeed(&options, UART_PORT_BAUDRATE);                              
                                                                                
    if (tcsetattr(uart_fd, TCSANOW, &options) < 0) {                      
       fprintf(stderr,"Set port options fail\n");                               
       return -1;
    }                                                                       
    tcflush(uart_fd, TCIOFLUSH);                                          
    fprintf(stderr, "Mcu uart is connected \n");
    return 0;
}

char* readfrommcu() {

    return NULL;
}

tMcu_cmd_rsp sendcommand2mcu(char cmd) {

    /*
     * Fomart for command to mcu is
     * First Byte 'F' (Factory test started)
     * Seconsd Byte cmd
     */
    tMcu_cmd_rsp resp;
    char FirstByte = 'F';

    tcflush(uart_fd, TCIOFLUSH);

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(uart_fd, &wfds);
    //Wait forever
    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &FirstByte, 1);

    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &cmd, 1);

    /*
     *  Response for command from mcu will be
     *  First byte : 'R' (Response)
     *  Second byte : 'S' or 'F' (Success or Fail)
     */

    fd_set rfds;
    struct timeval tv;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(uart_fd, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    // Wait for 5sec to get reply from mcu
    tcflush(uart_fd, TCIOFLUSH);
    ret = select(uart_fd+1, &rfds, NULL, NULL, &tv);
    if(ret >= 1) {
        read(uart_fd, &resp.resp[0], sizeof(char));
        if(resp.resp[0] == 'R')  {
            read(uart_fd, &resp.resp[1], sizeof(char));
        }
   } else { //Time out

       resp.resp[0] = 'R';
       resp.resp[1] = 'F';
   }

   return resp;
}

int disconnect_mcu() {
  return close(uart_fd);
}

am_mp_err_t mptool_mcu_init_handler(am_mp_msg_t *from_msg,
        am_mp_msg_t *to_msg) {


    to_msg->result.ret = MP_ERROR;
    fprintf(stderr, "MCU Test %s \n", __func__);
    int ret = -1;
    switch(from_msg->stage) {
        case CONNECT:
            if(McuHandler == CONNECT) {
                to_msg->result.ret = MP_BAD_STATE;
                memcpy(to_msg->msg, "Already connected",
                        strlen("Already connected"));
                return MP_OK;
            }
            ret = connect_mcu();
            if(ret == -1) {
                to_msg->result.ret = MP_ERROR;
                memcpy(to_msg->msg, CONNECT_ERROR,
                        strlen(CONNECT_ERROR));
            } else {
                McuHandler = CONNECT;
                to_msg->result.ret = MP_OK;
            }
            break;
        case DISCONNECT:
            if(McuHandler == DISCONNECT) {
                to_msg->result.ret = MP_BAD_STATE;
                memcpy(to_msg->msg, "Already Disconnected",
                        strlen("Already Disconnected"));
                return MP_OK;
            }
            ret = disconnect_mcu();
            if(ret == -1) {
                to_msg->result.ret = MP_ERROR;
                memcpy(to_msg->msg, DISCONNECT_ERROR,
                        strlen(DISCONNECT_ERROR));
            } else {
                to_msg->result.ret = MP_OK;
                McuHandler = DISCONNECT;
            }
            break;
        default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;

    }
    return MP_OK;
}

am_mp_err_t mptool_pir_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    fprintf(stderr, "MCU Test %s \n", __func__);
    to_msg->result.ret = MP_ERROR;
    tMcu_cmd_rsp resp;
    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch(from_msg->stage) {
        case MCU_CMD_TEST_SET:
            //tcflush(uart_fd, TCIOFLUSH);
            resp = sendcommand2mcu(CMD_PIR);
            if(resp.resp[1] == 'S') {
                to_msg->result.ret = MP_OK;
            } else
                to_msg->result.ret = MP_ERROR;

            break;

       case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret, SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;

       default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}

am_mp_err_t mptool_gsensor_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    to_msg->result.ret = MP_ERROR;
    tMcu_cmd_rsp resp;

    fprintf(stderr, "MCU Test %s \n", __func__);
    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch(from_msg->stage) {
        case MCU_CMD_TEST_SET:
            //tcflush(uart_fd, TCIOFLUSH);
            resp = sendcommand2mcu(CMD_GSEN);
            if(resp.resp[1] == 'S') {
                to_msg->result.ret = MP_OK;
            } else
                to_msg->result.ret = MP_ERROR;

            break;
       case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret, SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;
       default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}


am_mp_err_t mptool_voice_detect_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    to_msg->result.ret = MP_ERROR;
    tMcu_cmd_rsp resp;
    fprintf(stderr, "MCU Test %s \n", __func__);

    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch(from_msg->stage) {
        case MCU_CMD_TEST_SET:
            //tcflush(uart_fd, TCIOFLUSH);
            resp = sendcommand2mcu(CMD_VOICE);
            if(resp.resp[1] == 'S') {
                to_msg->result.ret = MP_OK;
            } else
                to_msg->result.ret = MP_ERROR;

            break;
       case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret, SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;
       default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}

typedef struct battery {
    int mvol;
    int parcentage;
}tBattery;

//Don't expect response as it's shutting down the S2LM.
void sendStandBy() {

    //tMcu_cmd_rsp resp;
    char FirstByte = 'F';
    char cmd = CMD_STANDBY;
    tcflush(uart_fd, TCIOFLUSH);

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(uart_fd, &wfds);
    //Wait forever
    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &FirstByte, 1);

    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &cmd, 1);

}

void getBatteryparcentage(tBattery *bat, tMcu_cmd_rsp *resp) {

    //tMcu_cmd_rsp resp;
    char FirstByte = 'F';
    char cmd = CMD_BAT;
    tcflush(uart_fd, TCIOFLUSH);

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(uart_fd, &wfds);
    //Wait forever
    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &FirstByte, 1);

    select(uart_fd+1, NULL, &wfds, NULL, NULL);
    write(uart_fd, &cmd, 1);

    fd_set rfds;
    struct timeval tv;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(uart_fd, &rfds);
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    int timeout = 1;
    // Wait for 5sec to get reply from mcu
    tcflush(uart_fd, TCIOFLUSH);

    while(timeout) {
        ret = select(uart_fd+1, &rfds, NULL, NULL, &tv);
        if(ret >= 1) {
            read(uart_fd, &resp->resp[0], sizeof(char));
            if(resp->resp[0] == 'R')  { //MCU is sending response
                read(uart_fd, &bat->mvol, sizeof(int));
                read(uart_fd, &bat->parcentage, sizeof(int));
                break;
            }
        }
        ++timeout;
    }
}

am_mp_err_t mptool_battery_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    to_msg->result.ret = MP_ERROR;
    tMcu_cmd_rsp resp;
    tBattery bat;

    fprintf(stderr, "MCU Test %s \n", __func__);

    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch(from_msg->stage) {
        case MCU_CMD_TEST_SET:
            //tcflush(uart_fd, TCIOFLUSH);
            getBatteryparcentage(&bat, &resp);
            if(resp.resp[0] == 'R') {
                to_msg->result.ret = MP_OK;
                memcpy(to_msg->msg, &bat, sizeof(tBattery));
            } else
                to_msg->result.ret = MP_ERROR;

            break;
       case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret, SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;
       default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}


am_mp_err_t mptool_usb_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    to_msg->result.ret = MP_ERROR;
    tMcu_cmd_rsp resp;
    fprintf(stderr, "MCU Test %s \n", __func__);

    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }


    switch(from_msg->stage) {
        case MCU_CMD_TEST_SET:
            //tcflush(uart_fd, TCIOFLUSH);
            resp = sendcommand2mcu(CMD_USB);
            if(resp.resp[1] == 'S') {
                to_msg->result.ret = MP_OK;
            } else
                to_msg->result.ret = MP_ERROR;

            break;
       case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret, SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;
       default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}

am_mp_err_t mptool_led_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{

  mp_led_msg led_msg;
  to_msg->result.ret = MP_OK;
  tMcu_cmd_rsp resp;
    fprintf(stderr, "MCU Test %s McuHandler %d\n", __func__, McuHandler);

  if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
  }


  switch (from_msg->stage) {
    case MCU_CMD_TEST_SET:
      memcpy(&led_msg, from_msg->msg, sizeof(led_msg));

      /*
       * MPTOOL written for the board where different LED's used for
       * REG/GREEN/BLUE and accessible through GPIO's but in OOMA board
       * LED's can be controlled from MCU only.
       *
       *  As of now sending 1-byte command for type of LED's and same
       * implemented in MCU side as well, if mcu side protocol is changed
       * please change here too.
       */
      fprintf(stderr, "GPIO %d \n", led_msg.gpio_id);

       if(led_msg.gpio_id == 11 )
           resp = sendcommand2mcu(CMD_LED_RED); //RED
       else if(led_msg.gpio_id == 12)
           resp = sendcommand2mcu(CMD_LED_GREEN);  //GREEN
       else if(led_msg.gpio_id == 35)
           resp = sendcommand2mcu(CMD_LED_BLUE);  //BLUE

       if(resp.resp[1] == 'S') {
           to_msg->result.ret = MP_OK;
       } else
           to_msg->result.ret = MP_ERROR;
      break;

    case MCU_CMD_RESULT_SAVE:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }
  return MP_OK;
}


am_mp_err_t mptool_button_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
    tMcu_cmd_rsp resp;

    fprintf(stderr, "MCU Test %s \n", __func__);
    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch (from_msg->stage) {
        case MCU_CMD_TEST_SET:
            resp = sendcommand2mcu(CMD_KEY);
            if(resp.resp[1] == 'S') {
                to_msg->result.ret = MP_OK;
            } else
                to_msg->result.ret = MP_ERROR;

            break;
        case MCU_CMD_RESULT_SAVE:
            if (mptool_save_data(from_msg->result.type,
                        from_msg->result.ret,
                        SYNC_FILE) != MP_OK) {
                to_msg->result.ret = MP_ERROR;
            }
            break;
        default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}

am_mp_err_t mptool_stanby_mode_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg) {

    if(McuHandler != CONNECT) {
        to_msg->result.ret = MP_BAD_STATE;
        memcpy(to_msg->msg, TRY_CONNECT, strlen(TRY_CONNECT));
        return MP_OK;
    }

    switch (from_msg->stage) {
        case 0:
            sendStandBy();
            to_msg->result.ret = MP_OK;
            break;
        default:
            to_msg->result.ret = MP_NOT_SUPPORT;
            break;
    }
    return MP_OK;
}
