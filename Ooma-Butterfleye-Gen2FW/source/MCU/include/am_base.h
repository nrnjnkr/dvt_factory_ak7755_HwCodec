/*
 * /s2lm_elektra_project/include/am_base.h
 *
 *  Created on: May 7, 2015
 *      Author: ChuChen
 *
 *  Copyright (C) 2015-2018, Ambarella, Inc.
 *  All rights reserved. No Part of this file may be reproduced, stored
 *  in a retrieval system, or transmitted, in any form, or by any means,
 *  electronic, mechanical, photocopying, recording, or otherwise,
 *  without the prior consent of Ambarella, Inc.
 */

#ifndef AM_BASE_H_
#define AM_BASE_H_

#define ELEKTRA_MCU_VERSION "20180404_1.0.0"

#define GPIO_LOW_LEVEL                  (0)
#define GPIO_HIGH_LEVEL                 (1)

#define Delay_Time_After_Trigger        0//2000 //ms
#define Delay_Time_After_S2L_Bootup     200 //ms
#define Delay_Time_Pir_Trigger          3//s

#define Delay_Time_Trigger_Check        100 //ms

#define TRIGGER_DEFAULT_ENABLE_MASK     (0)
#define TRIGGER_PIR_ENABLE_MASK         (1 << 0)
#define TRIGGER_WIFI_ENABLE_MASK        (1 << 1)

#define AMBA_ADC_RESOLUTION             (12) //adcRes12Bit
#define WLAN_RECONNECT_TIME_GAP         (300)
#define ENABLE_I2C_DEBUG
#define ENABLE_UART_PRINT
#define ENABLE_MCU_SLEEP_MODE
#define DC_IRQ_ENABLE
#define SWT_IRQ_ENABLE
//#define PWR_MODE_ENABLE
//rtc use 1kHz clock
//#define RTC_USE_ULFRCO
//enable low battery mode
//#define LOW_BATTERY_MODE_ENABLE
//#define WIFI_RECONNECT_ENABLE
//#define ENABLE_VCA_DEBUG

//#define ELEKTRA_V3

// Project definition
// #define PROJ_AMBA_ELEKTRA
#define PROJ_TEKNIQUE_OOMA

typedef enum {
    mcuStatus_Init = 0,
    mcuStatus_Running,
    mcuStatus_Wait_Bootup_trigger,
    mcuStatus_Wait_ReadyOff_trigger,
    mcuStatus_Handle_PIR_trigger,
    mcuStatus_Handle_WIFI_trigger,
    mcuStatus_Handle_PWR_Trigger,
    mcuStatus_Handle_ReadyOff_trigger,
    mcuStatus_Handle_NotifyMode_trigger,
    mcuStatus_Handle_VOICE_trigger,
    mcuStatus_Handle_BUTTON_trigger,
    mcuStatus_Handle_ACCEL_trigger,
    mcuStatus_LowBattery,
    mcuStatus_Poweron_trigger,
	mcuStatus_Handle_StandBy,
} MCU_Status_TypeDef;

// should match to MCU_TRIGGER_TYPE in bpi
typedef enum {
  triggerMode_Null = 0,
  triggerMode_PIR,
  triggerMode_Wifi,
  triggerMode_Wifi_Reconnect,
  triggerMode_PWR,
  triggerMode_PWR_2S,
  triggerMode_Power_Empty,
  triggerMode_Accel,
  triggerMode_Button,
  triggerMode_Voice,
  triggerMode_Bt,
} Trigger_Mode_TypeDef;

typedef enum {
  triggerStatus_Invalid = 0,
  triggerStatus_Init,
  triggerStatus_Set,
  triggerStatus_Clear,
  triggerStatus_Off,
} Trigger_Status_TypeDef;

typedef enum {
  pirStatus_Active = 0,
  pirStatus_Inactive,
} PIR_Status_TypeDef;

typedef enum {
  dcStatus_No = 0,
  dcStatus_Plugin,
  dcStatus_Unplugged,
} DC_Status_TypeDef;

typedef enum {
  chrgStatus_Charging = 0,
  chrgStatus_Full,
  chrgStatus_Discharging,
} CHRG_Status_TypeDef;

typedef enum {
  s2lmbootupMode_Config = 0x00, //(0b000) bootup to config mode(config wlan, cloud server...)
  s2lmbootupMode_WlanReconnect = 0x01, //(0b001) bootup to wlan reconnect mode, just try to connect wlan
  s2lmbootupMode_Notify = 0x02, //(0b010) bootup to notify msg to cloud/app
  s2lmbootupMode_rebuild_hiber = 0x03, //(0b011) bootup to rebuild hiber
  s2lmbootupMode_Recording = 0x04, //(0b100) bootup to recording mode
  s2lmbootupMode_Streaming = 0x05, //(0b101) bootup to streaming mode
  s2lmbootupMode_MpMode = 0x07, //(0b111) bootup to Mp mode to run Mptool
  s2lmbootupMode_Init = 0xFF,
} S2LM_Bootup_Mode_TypeDef;

typedef enum {
  s2lmStatus_PowerOn = 0,
  s2lmStatus_PowerOff,
  s2lmStatus_Off_to_On_Transfer
} S2LM_Status_Typedef;

typedef enum {
  msgToS2L_Base = 0x00,
  msgToS2L_PirOn = msgToS2L_Base,
  msgToS2L_PirOff, //0x81
  msgToS2L_LowBattery, //0x82
  msgToS2l_NormalBattery, //0x83
  msgToS2L_Discharging, //0x84
  msgToS2L_Charging, //0x85
  msgToS2L_ChargFull, //0x86
  msgToS2L_NeedChrg, //0x87
  msgToS2L_DcPlugin,  //0x88
  msgToS2L_DcUnplugged,  //0x89
  msgToS2L_WifiWakeup,
  msgToS2L_WifiReconnect,
  msgToS2L_WifiResetDone,

  msgFromS2L_Base = msgToS2L_Base + 0x20, //0x20
  msgFromS2L_LED_Red_On = msgFromS2L_Base,
  msgFromS2L_LED_Red_Off,
  msgFromS2L_LED_Green_On,
  msgFromS2L_LED_Green_Off,

  msgFromS2L_WifiConnected,
  msgFromS2L_WifiDisconnected,
  msgFromS2L_ResetWifi,
  msgFromS2L_PowerOffS2L,

  msgFromS2L_UART_DEBUG_MODE = 0x30,
  msgFromS2L_UART_COMMUNICATION_MODE,
  msgFromS2L_SCAN_I2C,
} MCU_Msg;

typedef enum {
  powerManager_I2C = 0,
  powerManager_ADC = 1,
} Power_Manager_TypeDef;

typedef enum {
  timeGap_1s = 0,
  timeGap_2s,
  timeGap_4s,
  timeGap_8s,
  timeGap_16s,
  timeGap_32s,
} Time_Gap_TypeDef;

typedef struct {
  uint8_t enable;
  uint8_t timer;//timer * 1s
  uint8_t count;
  uint8_t trigger;
} Timer_Wlan_Up;

typedef enum {
  batteryStatus_Normal = 1 << 0,
  batteryStatus_NeedChrg = 1 << 1,
  batteryStatus_Low = 1 << 2,
} Battery_Status_TypeDef;

typedef enum {
  pwrMode_Invalid = 0, //mode invalid
  pwrMode_TriggerOn, //mode to open trigger
  pwrMode_TriggerOff, //mode to close trigger
  pwrMode_WifiSetup, //mode to config wifi
  pwrMode_FwUpgrade,
  pwrMode_UsbDownload,
  pwrMode_MpMode,
  pwrMode_UartWorkingMode,
} PWR_Button_Mode_TypeDef;

typedef enum {
  mcuMode_Sleeping = 0, //all triggers is closed
  mcuMode_Working,
} MCU_Mode_TypeDef;

#endif /* AM_BASE_H_ */
