#include<stdio.h>                                                               
#include <unistd.h>  
#include<stdlib.h>                                                              
#include<sys/types.h>                                                           
#include<sys/socket.h>                                                          
#include <errno.h>                                                              
#include<string.h>                                                              
#include <netinet/in.h>                                                         
#include <arpa/inet.h>  
#include "factory_client.h"


am_mp_result_t mptool_eth_test(int fd, char *argv[], int argc) {

    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    
    cmd_req.result.type = AM_MP_ETHERNET;
    cmd_req.stage = 0 ; //MP_ETH_HAND_SHAKE 

    fprintf(stderr, "Ethernet test \n");
    //Send the command request 
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the request from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    result = cmd_response.result;
    return result;
}


am_mp_result_t mptool_button_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Button test \n");
    am_mp_result_t result  = {0};

    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_BUTTON;
    cmd_req.stage = 0; //LED_STATE_GET

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;

    return result;
}

typedef struct{
    int32_t gpio_id;
    int32_t led_state;
}mp_led_msg;

am_mp_result_t mptool_led_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "LED test \n");
    am_mp_result_t result  = {0};

    mp_led_msg led_msg; 

    if(argc >= 4) {

        if(!strcmp(argv[2], "RED")) {
            led_msg.gpio_id = 11;
            if(!strcmp(argv[3], "On")) {
                led_msg.led_state = 1;
            } else {
                led_msg.led_state = 0;
            }
        }else if(!strcmp(argv[2], "GREEN")) {
            led_msg.gpio_id = 12;
            if(!strcmp(argv[3], "On")) {
                led_msg.led_state = 1;
            } else {
                led_msg.led_state = 0;
            }
        } else if(!strcmp(argv[2], "BLUE")) {
            led_msg.gpio_id = 35;
            if(!strcmp(argv[3], "On")) {
                led_msg.led_state = 1;
            } else {
                led_msg.led_state = 0;
            }

       }
    } else { 
        //Default case  RED will turn on
            led_msg.gpio_id = 11;
            led_msg.led_state = 1;
    } 

    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_LED;
    cmd_req.stage = 0 ; //LED_STATE

    memcpy(cmd_req.msg, &led_msg, sizeof(mp_led_msg));

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    result = cmd_response.result;
    return result;
}

typedef struct{                                                                 
    int32_t gpio_id;                  
    int32_t led_state;                
    int32_t pwm_channel;              
    int32_t bl_brightness;            
}mp_ir_led_msg;

am_mp_result_t mptool_ir_led_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "IR LED test \n");
    am_mp_result_t result  = {0};

    mp_ir_led_msg msg;

    msg.pwm_channel = 0; //Default channel for our board 
    msg.bl_brightness = (int32_t)90;  //Default bringhtness

    if(!strcmp(argv[2], "On")) {
        if(argc == 4)  {
            msg.bl_brightness = (int32_t)atoi(argv[3]);
            fprintf(stderr, "Brightness setting for %s %s %s %d\n", argv[1], 
                argv[2], argv[3],  atoi(argv[3]));
        }
    }

    fprintf(stderr, "after copy brightness\n");
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    
    cmd_req.result.type = AM_MP_IRLED;
    cmd_req.stage = 0 ; //LED_STATE_SET

    memcpy(cmd_req.msg, &msg, sizeof(mp_ir_led_msg));
    fprintf(stderr, "after copy msg\n");

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    result = cmd_response.result;
    return result;
}


typedef struct {
    int32_t channel;
    uint32_t value;
}mp_light_sensor_msg;

am_mp_result_t mptool_light_sensor_test(int fd, char *argv[], int argc){

    fprintf(stderr, "Light Sensor test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    mp_light_sensor_msg sens;


    cmd_req.result.type = AM_MP_LIGHT_SENSOR;
    cmd_req.stage = 1; //LED_STATE_GET

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    memcpy(&sens, cmd_response.msg, sizeof(mp_light_sensor_msg));

    fprintf(stderr, "Light value %d\n", sens.value);

    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_mac_addr_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "MAC Addr test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    //Send the request to serverfor the test
    int i = 0;
    cmd_req.result.type = AM_MP_MAC_ADDR;

    if(argc >= 3) {
        cmd_req.stage = 1;//MP_MAC_SET,
        memcpy(cmd_req.msg, argv[2], strlen(argv[2]));

        //Validate mac string
        for(i  =0 ; i < strlen(cmd_req.msg); i++) {
            if(((cmd_req.msg[i] >= 'a' && cmd_req.msg[i] <= 'f') ||
                    (cmd_req.msg[i] >= 'A' && cmd_req.msg[i] <= 'F')) ||
                    (cmd_req.msg[i] >= '0' && cmd_req.msg[i] <= '9')){
            } else {
                fprintf(stderr, "Given mac is "
                        "not within of 0-f range and it shoul be in format "
                        "00112233445566\n");
                cmd_response.result.ret = MP_ERROR;
                return cmd_response.result;
            }
        }
        if(strlen(cmd_req.msg) != 12) {
            fprintf(stderr, "MAC size is not proper!! Provide valid "
                    "sized mac address\n");
            cmd_response.result.ret = MP_ERROR;
            return cmd_response.result;
        }
        //Send the request to serverfor the test
        write(fd, &cmd_req, sizeof(am_mp_msg_t));
        //Wait for the result from the server
        read(fd, &cmd_response, sizeof(am_mp_msg_t));
        if(cmd_response.result.ret == MP_OK)
            fprintf(stderr, "MAC set successfully!! Please reboot camera to "
                "take effect\n");

    } else {
        fprintf(stderr, "Supply the proper mac address \n");
        cmd_response.result.ret = MP_ERROR;
    }

    result = cmd_response.result;
    return result;
}


#define BUFF_LEN 1000
#define CMD_LEN 300

enum {
    CMD_SETUP = 0,
    CMD_SCAN,
    CMD_CONNECT,
    CMD_GET_COUNTRY_CODE,
    CMD_SET_COUNTRY_CODE,
    CMD_SIGNAL_STRENGTH,
    CMD_THROUGHPUT_TEST
}WifiCommand;

enum {
    WIFI_INTF_NOT_UP = 0,
    WIFI_INTF_UP,
    WIFI_AP_CONNECTED,
};

typedef struct {
    char ssid[100];
    char psswd[100];
}tConnectAp;

typedef struct wifi_state {
    int state;
    char ssid_name[100];
    char password[100];
    char ip_addr[100];
    char netmask[100];
}tWifiState;

static void sendrequest(int fd, am_mp_msg_t *cmd_req ,am_mp_msg_t *cmd_response) {

    //Send the request to serverfor the test
    write(fd, cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, cmd_response, sizeof(am_mp_msg_t));

}

/* Handler to process request relating to wifi test cases. */
am_mp_result_t mptool_wifi_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Wifi Tests\n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_WIFI_STATION;
    if(argc >= 3 ) {
        if(!strcmp(argv[2], "setup")) {
            cmd_req.stage = CMD_SETUP;
            sendrequest(fd, &cmd_req, &cmd_response);
        } else if(!strcmp(argv[2], "scan")) {
            cmd_req.stage = CMD_SCAN;
            sendrequest(fd,&cmd_req, &cmd_response);
            if(cmd_response.result.ret == MP_ERROR)
                fprintf(stderr, "%d\n", cmd_response.msg);
        } else if(!strcmp(argv[2], "connect")) {
            if(argc >= 5) {
                tConnectAp Creds;
                strcpy(Creds.ssid, argv[3]);
                strcpy(Creds.psswd, argv[4]);
                memcpy(&cmd_req.msg, &Creds, sizeof(tConnectAp));
                cmd_req.stage = CMD_CONNECT;
                sendrequest(fd,&cmd_req, &cmd_response);
                if(cmd_response.result.ret == MP_ERROR)
                    fprintf(stderr, "%s\n", cmd_response.msg);
            } else {
                fprintf(stderr, "Please provide the AP details for connect\n");
                cmd_response.result.ret = MP_ERROR;
            }
        } else if(!strcmp(argv[2], "setcountry")) {
            if(argc >= 4) {
                strcpy(cmd_req.msg, argv[3]);
                cmd_req.stage = CMD_SET_COUNTRY_CODE;
                sendrequest(fd,&cmd_req, &cmd_response);
                if(cmd_response.result.ret == MP_ERROR)
                    fprintf(stderr, "%s\n", cmd_response.msg);
            } else {
                fprintf(stderr, "Please provide the country code\n");
                cmd_response.result.ret = MP_ERROR;
            }
        } else if(!strcmp(argv[2], "getcountry")) {
            cmd_req.stage = CMD_GET_COUNTRY_CODE;
            sendrequest(fd,&cmd_req, &cmd_response);
            fprintf(stderr, "Country set is %s\n", cmd_response.msg);
        } else if (!strcmp(argv[2], "signal_strength")) {
            cmd_req.stage = CMD_SIGNAL_STRENGTH;
            sendrequest(fd,&cmd_req, &cmd_response);
            if(cmd_response.result.ret == MP_ERROR)
                fprintf(stderr, "%s\n", cmd_response.msg);
        } else if (!strcmp(argv[2], "wifi_throughput"))  {
            cmd_req.stage = CMD_THROUGHPUT_TEST;
            sendrequest(fd,&cmd_req, &cmd_response);
            fprintf(stderr, "iperf server started at %s\n", cmd_response.msg);

        } else  {
            fprintf(stderr, "Please provide proper sub commads\n");
            cmd_response.result.ret = MP_ERROR;
        }
    } else {
        fprintf(stderr, "Please provide sub commands to test\n");
        cmd_response.result.ret = MP_ERROR;
    }
    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_mic_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Mic test \n");
    am_mp_result_t result  = {0};
    /*am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;


    cmd_req.result.type = AM_MP_MIC;
    cmd_req.stage  = 1; //MP_MIC_RECORD_THD,

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    */
    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;
}

am_mp_result_t mptool_speaker_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Speaker test \n");
    am_mp_result_t result  = {0};
    /*am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_SPEAKER;
    cmd_req.stage  = 1; //MP_SPEAKER_PLAY,
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result; */
    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;
}


am_mp_result_t mptool_sensor_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Sensor test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_SENSOR;
    cmd_req.stage  = 1; //
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));
    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
 //   fprintf(stderr, "Command not supporting right now...!");
   // result.ret =  MP_NOT_SUPPORT;
    return result;
}


typedef enum {
    LENS_FOCUS_HAND_SHAKE = 0,
    LENS_FOCUS_START_RTSP,
    LENS_FOCUS_STOP_RTSP,
    LENS_FOCUS_GET_AF_DATA,
} mp_lens_focus_stage_t;

am_mp_result_t mptool_lens_focus_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Lens focus test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    cmd_req.result.type = AM_MP_LENS_FOCUS;

    if(argc >= 3) {
        if(!strcmp(argv[2], "rtsp_start")) {
            cmd_req.stage = LENS_FOCUS_START_RTSP;
            //Send the request to serverfor the test
            write(fd, &cmd_req, sizeof(am_mp_msg_t));

            //Wait for the result from the server
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
        } else if(!strcmp(argv[2], "rtsp_stop")) {
            cmd_req.stage = LENS_FOCUS_STOP_RTSP;
            write(fd, &cmd_req, sizeof(am_mp_msg_t));
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
        } else if(!strcmp(argv[2], "get_data")) {
            cmd_req.stage = LENS_FOCUS_GET_AF_DATA;
            write(fd, &cmd_req, sizeof(am_mp_msg_t));
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
            int af_data = 0;
            memcpy(af_data, cmd_response.msg, sizeof(int));
            fprintf(stderr, "AF Data %d\n", af_data);
        } else {
            cmd_response.result.ret = MP_ERROR;
            fprintf(stderr, "Please provide proper sub command\n");
        }
    }
    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_lens_shad_cal_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Lens Shad cal test \n");
    am_mp_result_t result  = {0};

    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;
}

am_mp_result_t mptool_bad_pixel_calibration(int fd, char *argv[], int argc) {

    fprintf(stderr, "Bad pixel calibration test \n");
    am_mp_result_t result  = {0};
    //Send the request to serverfor the test

    //Wait for the result from the server
    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;
}


typedef struct {
     char os_version[16];
     char build_time[64];
     char fw_version[16];
} am_mp_fw_info_t;

am_mp_result_t mptool_get_fw_info_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Get Firmawre information test \n");
    am_mp_result_t result  = {0};
    am_mp_fw_info_t msg;
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    
    cmd_req.result.type = AM_MP_FW_INFO;
    cmd_req.stage = 0 ; //MP_GET_FW_INFO 
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    memcpy(&msg, cmd_response.msg, sizeof(am_mp_fw_info_t));

    fprintf(stderr, "OS version %s\n", msg.os_version);
    fprintf(stderr, "Build time %s\n", msg.build_time);
    fprintf(stderr, "Firmware Version %s\n", msg.fw_version);

    result = cmd_response.result;
    return result;
}


struct mp_sd_test_msg {                                                         
    int32_t read_speed;
    int32_t write_speed;
}; 

am_mp_result_t mptool_sd_test_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "SDCard test \n");
    am_mp_result_t result  = {0};
    struct mp_sd_test_msg sdcard; 
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
   
    cmd_req.result.type = AM_MP_SDCARD;
    cmd_req.stage = 0 ; //MP_GET_FW_INFO 
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t)); 

    if(cmd_response.result.ret == MP_ERROR) {
        fprintf(stderr, "%s\n", cmd_response.msg);
        result = cmd_response.result;
        return result;
    }
    memcpy(&sdcard, cmd_response.msg, sizeof(struct mp_sd_test_msg));

    fprintf(stderr, "SDCARD write speed %d Bytes/s\n", sdcard.write_speed);
    fprintf(stderr, "SDCARD read speed %d Bytes/s\n", sdcard.read_speed);

    result = cmd_response.result;
    return result;
}


enum {
    IRCUT_GPIO_STATE_SET = 0,
    IRCUT_START_RTSP     = 1,
    IRCUT_STOP_RTSP      = 2,
    IRCUT_RESULT_SAVE    = 3
};

#define IRCUT_MAX_GPIO_NUM 5 

typedef struct {
    int gpio_num;
    int gpio_id[IRCUT_MAX_GPIO_NUM];
    int gpio_state[IRCUT_MAX_GPIO_NUM];
}mp_ircut_msg;

am_mp_result_t mptool_ircut_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "IRCUT test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    mp_ircut_msg ir_msg;

    cmd_req.result.type = AM_MP_IRCUT;

    if(argc >= 3) {

        if(!strcmp(argv[2], "rtsp_start")) {
            cmd_req.stage = IRCUT_START_RTSP;
            //Send the request to serverfor the test
            write(fd, &cmd_req, sizeof(am_mp_msg_t));

            //Wait for the result from the server
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
        } else if(!strcmp(argv[2], "rtsp_stop")) {
            cmd_req.stage = IRCUT_STOP_RTSP;
            write(fd, &cmd_req, sizeof(am_mp_msg_t));
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
        } else if(!strcmp(argv[2], "enable_ircut")) {
            cmd_req.stage = IRCUT_GPIO_STATE_SET;
            write(fd, &cmd_req, sizeof(am_mp_msg_t));
            read(fd, &cmd_response, sizeof(am_mp_msg_t));
        } else {
            cmd_response.result.ret = MP_ERROR;
            fprintf(stderr, "Please provide proper sub command\n");
        }
    }

    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_complete_test_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Test all test cases \n");
    am_mp_result_t result  = {0};
    //Send the request to serverfor the test

    //Wait for the result from the server
    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;
}
 
am_mp_result_t mptool_reset_all_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Reset all test cases \n");
    am_mp_result_t result  = {0};

    //Send the request to serverfor the test

    //Wait for the result from the server
    fprintf(stderr, "Command not supporting right now...!");
    result.ret =  MP_NOT_SUPPORT;
    return result;

}


enum mcu_states {
    INIT = -1,
    CONNECT = 0,
    DISCONNECT = 2,
    UNKWON = 0XFF,
};

am_mp_result_t mptool_mcu_init_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "MCU Init test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_MCU_TESTS;

    fprintf(stderr, "mcu connect %s %s %s\n", argv[0], argv[1], argv[2]);

    if(!strcmp(argv[2], "connect")) {
        cmd_req.stage = CONNECT;
    } else if (!strcmp(argv[2], "disconnect")) {
        cmd_req.stage = DISCONNECT;
    } else {
        fprintf(stderr, "Bad parameter\n");
        cmd_response.result.ret = MP_ERROR;
        result = cmd_response.result;
        return result;

    }

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;

}

enum {
    MCU_CMD_TEST_SET = 0,
    MCU_CMD_RESULT_SAVE,
};

am_mp_result_t mptool_pir_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "PIR test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_PIR;
    cmd_req.stage = MCU_CMD_TEST_SET;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_gsensor_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "G-Sensor test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_GSENS;
    cmd_req.stage = MCU_CMD_TEST_SET;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_voice_detect_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Voice detect test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_VOICE_DECT;
    cmd_req.stage = MCU_CMD_TEST_SET;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;
}

typedef struct battery {
    int mvol;
    int parcentage;
}tBattery;

am_mp_result_t mptool_battery_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Battery test \n");
    am_mp_result_t result  = {0};
    am_mp_fw_info_t msg;
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    tBattery bat;

    cmd_req.result.type = AM_MP_BATTERY;
    cmd_req.stage = MCU_CMD_TEST_SET;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    memcpy(&bat, cmd_response.msg, sizeof(tBattery));
    fprintf(stderr, "Bateery Info\n");
    fprintf(stderr, "Voltage : %dmV\n", bat.mvol);
    fprintf(stderr, "Parcentage : %d\n", bat.parcentage);

    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_usb_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Get Firmawre information test \n");
    am_mp_result_t result  = {0};
    am_mp_fw_info_t msg;
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_USB;
    cmd_req.stage = MCU_CMD_TEST_SET;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;
}

enum {
    START_ADV = 0,
    STOP_ADV,
};
am_mp_result_t mptool_bluetooth_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Bluetooth test \n");
    am_mp_result_t result  = {0};
    am_mp_fw_info_t msg;
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_BLUETOOTH;
    if(argc >= 3) {
        if(!strcmp(argv[2], "start_adv"))
            cmd_req.stage = START_ADV;
        else if (!strcmp(argv[2], "stop_adv"))
            cmd_req.stage = STOP_ADV;
        else {
            fprintf(stderr, "Please provide proper sub commands\n");
            cmd_response.result.ret = MP_ERROR;
            return cmd_response.result;
        }

        //Send the request to serverfor the test
        write(fd, &cmd_req, sizeof(am_mp_msg_t));

        //Wait for the result from the server
        read(fd, &cmd_response, sizeof(am_mp_msg_t));

        fprintf(stderr, "%s\n", cmd_response.msg);
    }
    result = cmd_response.result;
    return result;
}

typedef struct download {
    char filename[20];
    char serverip[20];
    char md5sum[10];
}tDownloadKeys;

am_mp_result_t mptool_downloadkey_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Download Keys test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;
    tDownloadKeys keys;

    cmd_req.result.type = AM_MP_LOAD_KEY;
    cmd_req.stage = 0;

    strcpy(keys.filename, argv[3]);
    strcpy(keys.md5sum, argv[4]);
    strcpy(keys.serverip, argv[2]);

    memcpy(cmd_req.msg, &keys, sizeof(tDownloadKeys));

    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    fprintf(stderr, "KeyDownload : %s\n", cmd_response.msg);
    result = cmd_response.result;
    return result;
}

am_mp_result_t mptool_standby_mode_test(int fd, char *argv[], int argc) {

    fprintf(stderr, "Standby test \n");
    am_mp_result_t result  = {0};
    am_mp_msg_t cmd_req;
    am_mp_msg_t cmd_response;

    cmd_req.result.type = AM_MP_STANDBY;
    cmd_req.stage = 0;
    //Send the request to serverfor the test
    write(fd, &cmd_req, sizeof(am_mp_msg_t));

    //Wait for the result from the server
    read(fd, &cmd_response, sizeof(am_mp_msg_t));

    result = cmd_response.result;
    return result;
}
