#include <stdio.h>
#include "factory_client.h"

//am_mp_result_t mptool_result[AM_MP_MAX_ITEM_NUM];

am_mp_result_t test(int fd, char *cmd_name, char *argv[], int argc);

am_mp_result_t mptool_eth_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_button_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_led_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_ir_led_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_light_sensor_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_mac_addr_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_wifi_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_mic_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_speaker_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_sensor_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_lens_focus_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_lens_shad_cal_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_bad_pixel_calibration(int fd, char *argv[], int argc);
am_mp_result_t mptool_get_fw_info_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_sd_test_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_ircut_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_complete_test_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_reset_all_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_mcu_init_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_pir_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_gsensor_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_voice_detect_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_battery_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_usb_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_bluetooth_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_downloadkey_test(int fd, char *argv[], int argc);
am_mp_result_t mptool_standby_mode_test(int fd, char *argv[], int argc);

factory_handler test_cmd[] = {
  mptool_eth_test, //AM_MP_ETHERNET
  mptool_button_test, //AM_MP_BUTTON
  mptool_led_test,//AM_MP_LED
  mptool_ir_led_test,//AM_MP_IRLED
  mptool_light_sensor_test,//AM_MP_LIGHT_SENSOR
  mptool_mac_addr_test,//AM_MP_MAC_ADDR
  mptool_wifi_test,//AM_MP_WIFI_STATION
  mptool_mic_test,//AM_MP_MIC
  mptool_speaker_test,//AM_MP_SPEAKER
  mptool_sensor_test,//AM_MP_SENSOR
  mptool_lens_focus_test,//AM_MP_LENS_FOCUS
  mptool_lens_shad_cal_test,//AM_MP_LENS_SHADING_CALIBRATION
  mptool_bad_pixel_calibration,//AM_MP_BAD_PIXEL_CALIBRATION
  mptool_get_fw_info_test,//AM_MP_FW_INFO
  mptool_sd_test_test,//AM_MP_SDCARD
  mptool_ircut_test,//AM_MP_IRCUT,
  mptool_mcu_init_test, //AM_MP_MCU_TESTS
  mptool_pir_test, //AM_MP_PIR
  mptool_gsensor_test, // AM_MP_GSENS
  mptool_voice_detect_test,//AM_MP_VOICE_DECT
  mptool_battery_test,//AM_MP_BATTERY
  mptool_usb_test,//AM_MP_USB
  mptool_bluetooth_test, //AM_MP_BLUETOOTH
  mptool_downloadkey_test, //AM_MP_KEY_DOWNLOAD
  mptool_standby_mode_test, //AM_MP_STANDBY
  mptool_complete_test_test,//AM_MP_COMLETE_TEST
  mptool_reset_all_test,//AM_MP_RESET_ALL
};
