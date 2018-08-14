#include <stdio.h>
#include <fcntl.h>                                                              
#include <assert.h>                                                             
#include <termios.h> 
#include <unistd.h>
#include "am_mp_server.h"

#define UART_PORT_NAME "/dev/ttyS0"
#define UART_PORT_BAUDRATE B9600
#define UART_PORT_FLOWCTRL 0

#define TRY_CONNECT "Please connect to mcu first, then try testing commands"
#define TRY_DISCONNECT "Please disconnect mcu then try tesitng commands"

#define CONNECT_ERROR "Failed to connect to MCU"
#define DISCONNECT_ERROR "Failed to Disconnect to MCU"

typedef enum {
    Init = 0,
    Sent,
    Received,
}States_t;

typedef struct {
    States_t state;
    int red_led;
}led_t;

typedef struct {
    States_t state;
    int bat_conn;
    int level;
}battery_t;

typedef struct {
    States_t state;
    int pir;
}pir_t;

typedef struct {
    States_t state;
    int acceler;
}accelerometer_t;

typedef struct {
    States_t state;
    int voice_dect;
}voice_dect_t;

typedef struct {
    States_t state;
    int swt_dect;
    int mcu_reset_dect;
}button_t;

typedef enum Commands {
	CMD_INIT_FACTORY_TEST = 'F',
	CMD_LED_RED = 'R',
	CMD_LED_GREEN = 'L',
	CMD_LED_BLUE = 'B',
	CMD_PIR = 'P',
	CMD_GSEN = 'S',
	CMD_KEY = 'K',
	CMD_VOICE = 'V',
	CMD_BAT = 'C',
	CMD_USB = 'U',
    CMD_STANDBY = 'Z',
}tFactoryCommand;

enum mcu_states {
    INIT = -1,
    CONNECT = 0,
    DISCONNECT = 2,
    UNKWON = 0XFF,
};

typedef struct Mcu_cmd {
    char cmd[2];
}tMcu_cmd;

typedef struct Mcu_cmd_response {
    char resp[2];
}tMcu_cmd_rsp;

enum {
    MCU_CMD_TEST_SET = 0,
    MCU_CMD_RESULT_SAVE,
};

typedef struct{                                                                 
    int32_t gpio_id;                                                            
    int32_t led_state;                                                          
}mp_led_msg;

int connect_mcu();
int disconnect_mcu();
char* readfrommcu();
tMcu_cmd_rsp sendcommand2mcu(char cmd);

// MCU Test case handlers 
am_mp_err_t mptool_mcu_init_handler(am_mp_msg_t *from_msg, 
        am_mp_msg_t *to_msg);
am_mp_err_t mptool_pir_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
am_mp_err_t mptool_gsensor_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
am_mp_err_t mptool_voice_detect_handler(am_mp_msg_t *from_msg, 
        am_mp_msg_t *to_msg);
am_mp_err_t mptool_battery_handler(am_mp_msg_t *from_msg, 
        am_mp_msg_t *to_msg);
am_mp_err_t mptool_usb_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
am_mp_err_t mptool_led_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
am_mp_err_t mptool_button_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
am_mp_err_t mptool_stanby_mode_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg);
