/*******************************************************************************
 * factory_client.h
 * Author : Naresh K
 */

#include<stdio.h>
#define SYNC_FILE 1
#define NOT_SYNC_FILE 0
#define BUF_LEN 128
#define VBUF_LEN 4
#define EXTRA_MSG_FILE "/tmp/mptool/mptool_extra_msg.txt"

typedef enum {
  MP_OK = 0,
  MP_WAIT,
  MP_ERROR,
  MP_CLOSED,
  MP_BUSY,
  MP_INTERNAL_ERROR,
  MP_NO_RUN,
  MP_NO_IMPL,
  MP_OS_ERROR,
  MP_HW_ERROR,
  MP_IO_ERROR,
  MP_TIMEOUT,
  MP_NO_MEMORY,
  MP_TOO_MANY,

  MP_NOT_EXIST,
  MP_NOT_SUPPORT,
  MP_NO_INTERFACE,
  MP_BAD_STATE,
  MP_BAD_PARAM,
  MP_BAD_COMMAND,

  MP_UNKNOWN_FMT,
} am_mp_err_t;

typedef enum {
  AM_MP_ETHERNET = 0,
  AM_MP_BUTTON,
  AM_MP_LED,
  AM_MP_IRLED,
  AM_MP_LIGHT_SENSOR,
  AM_MP_MAC_ADDR,
  AM_MP_WIFI_STATION,
  AM_MP_MIC,
  AM_MP_SPEAKER,
  AM_MP_SENSOR,
  AM_MP_LENS_FOCUS,
  AM_MP_LENS_SHADING_CALIBRATION,
  AM_MP_BAD_PIXEL_CALIBRATION,
  AM_MP_FW_INFO,
  AM_MP_SDCARD,
  AM_MP_IRCUT,
  /*MCU commands*/
  AM_MP_MCU_TESTS,
  AM_MP_PIR,
  AM_MP_GSENS,
  AM_MP_VOICE_DECT,
  AM_MP_BATTERY,
  AM_MP_USB,
  AM_MP_BLUETOOTH,
  AM_MP_LOAD_KEY,
  AM_MP_STANDBY,
  AM_MP_MAX_ITEM_NUM, //all added item must before this item
  AM_MP_COMLETE_TEST,
  AM_MP_RESET_ALL,
  AM_MP_TYPE_INIT = 255,
} am_mp_msg_type_t;

typedef struct {
    am_mp_msg_type_t type;
    am_mp_err_t ret;
} am_mp_result_t;

typedef struct{
    //int32_t stage;
    int stage;
    am_mp_result_t result;
    char msg[256];
} am_mp_msg_t;

typedef am_mp_result_t (*factory_handler) (int , char**, int);

am_mp_err_t mptool_save_data(am_mp_msg_type_t mp_type,
                             am_mp_err_t mp_result,
                             //int32_t sync_flag);
                             int sync_flag);
