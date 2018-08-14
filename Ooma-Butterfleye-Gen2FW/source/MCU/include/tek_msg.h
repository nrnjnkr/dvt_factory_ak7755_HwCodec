// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////
#ifndef _TEK_MSG_H_
#define _TEK_MSG_H_

#define MCU_MSG_RSP_OK                         0
#define MCU_MSG_RSP_NG                         1
#define MCU_MSG_RSP_NOT_IMPLEMENTED            2

#include <stdint.h>
#include <stddef.h>

typedef
enum _msg_type_t {
    MSG_TYPE_PING,
    MSG_TYPE_PONG,
    MSG_TYPE_REQ,
    MSG_TYPE_RSP,
    MSG_TYPE_EVT,
    MSG_TYPE_MAX
} msg_type_t;

typedef
enum _msg_sub_type_t {
    MSG_SUB_TYPE_ACCEL,
    MSG_SUB_TYPE_BT,
    MSG_SUB_TYPE_CHARGER,
    MSG_SUB_TYPE_FUELGAUGE,
    MSG_SUB_TYPE_LED,
    MSG_SUB_TYPE_MCU,
    MSG_SUB_TYPE_OTA,
    MSG_SUB_TYPE_PIR,
    MSG_SUB_TYPE_VOICE,
    MSG_SUB_TYPE_WIFI,
    MSG_SUB_TYPE_BUTTON,
    MSG_SUB_TYPE_MAX
} msg_sub_type_t;

typedef
enum _msg_accel_type_t {
    ACCEL_PROBE,
    ACCEL_OFFSET_SET,
    ACCEL_OFFSET_GET,
    ACCEL_ACC_VALUE_GET, // (uint32_t)x, (uint32_t)y, (uint32_t)z
    ACCEL_TEMP_VALUE_GET,
    ACCEL_RANGE_SET,
    ACCEL_RANGE_GET,
    ACCEL_INT_TRIG,
    ACCEL_MAX,
} msg_accel_type_t;

typedef
enum _msg_bt_type_t {
    BT_MAX,
} msg_bt_type_t;

typedef
enum _msg_charger_type_t {
    CHARGER_OFFSET_SET,
    CHARGER_MAX
} msg_charger_type_t;

typedef
enum _msg_fuelgauge_type_t {
    FUELGAUGE_PROBE,
    FUELGAUGE_DEVICE_NAME,
    FUELGAUGE_VOLTAGE,
    FUELGAUGE_FULL_CAPACITY,
    FUELGAUGE_REMAINING_CAPACITY,
    FUELGAUGE_STATE_OF_CHARGE,
    FUELGAUGE_STATE_OF_HEALTH,
    FUELGAUGE_TEMPERATURE,
    FUELGAUGE_AVERAGE_CURRENT,
    FUELGAUGE_MAX
} msg_fuelgauge_type_t;

typedef
enum _msg_led_type_t {
    LED_MAX
} msg_led_type_t;

typedef
enum _msg_mcu_type_t {
    MCU_SLEEP,
    MCU_VERSION,
    MCU_WAKEUP_TRIGGER,
    MCU_MAX
} msg_mcu_type_t;

typedef
enum _msg_pir_type_t {
    PIR_PROBE,
    PIR_SENS_SET,
    PIR_SENS_GET,
    PIR_INT_TRIG,
    PIR_MAX,
} msg_pir_type_t;

typedef
enum _msg_voice_type_t {
    VOICE_SENS_SET,
    VOICE_SENS_GET,
    VOICE_INT_TRIG,
    VOICE_MAX
} msg_voice_type_t;

typedef
enum _msg_wifi_type_t {
    WIFI_MAX
} msg_wifi_type_t;

typedef
enum _msg_button_type_t {
    BUTTON_TRIG,
    BUTTON_MAX
} msg_button_type_t;

extern int tek_msg_init(void);
extern int UartHandler(void);
extern int tek_msg_encode(msg_type_t msg_type,
                            msg_sub_type_t sub_type,
                            uint8_t sub_type1,
                            uint8_t * data,
                            uint16_t len);
#endif
