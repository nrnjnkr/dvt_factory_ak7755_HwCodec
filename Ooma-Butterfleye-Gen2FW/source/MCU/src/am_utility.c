/*
 * /s2lm_elektra_project/src/am_utility.c
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

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "config.h"
#include "am_base.h"
#include "am_log.h"
#include "am_led.h"
#include "am_common.h"
#include "btfl_gpio.h"
//////////////////////////////////////////////////////////////////////////////////
static void _poweroff_wifi_module()
{
  GPIO_PinOutClear(GPIO_GROUP[gpioGroup_WIFI_EN].port, GPIO_GROUP[gpioGroup_WIFI_EN].pin);
  GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BT_RST].port, GPIO_GROUP[gpioGroup_BT_RST].pin);
  GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PWR_EN].port, GPIO_GROUP[gpioGroup_PWR_EN].pin);
}

static void _poweron_wifi_module()
{
  GPIO_PinOutSet(GPIO_GROUP[gpioGroup_PWR_EN].port, GPIO_GROUP[gpioGroup_PWR_EN].pin);
  GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BT_RST].port, GPIO_GROUP[gpioGroup_BT_RST].pin);
  GPIO_PinOutSet(GPIO_GROUP[gpioGroup_WIFI_EN].port, GPIO_GROUP[gpioGroup_WIFI_EN].pin);
}
//////////////////////////////////////////////////////////////////////////////////

void PowerWifiModule(bool on)
{
  if (on) {
    _poweron_wifi_module();
  } else {
    _poweroff_wifi_module();
  }
}

//check wifi-module switch status
bool UpdateWifiModuleStatus(void)
{
  unsigned int status = GPIO_PinInGet(GPIO_GROUP[gpioGroup_SWT_DET].port, GPIO_GROUP[gpioGroup_SWT_DET].pin);
  return (status == 0)? false:true;
}

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)


uint8_t UpdateDCIRQStatus(uint8_t dcStatus)
{
  unsigned int status = GPIO_PinInGet(GPIO_GROUP[gpioGroup_DC_OK].port, GPIO_GROUP[gpioGroup_DC_OK].pin);
  if (status == GPIO_HIGH_LEVEL) {
    dcStatus = dcStatus_Unplugged;
  } else {
    dcStatus = dcStatus_Plugin;
  }

  return dcStatus;
}

#elif defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)

#endif

uint8_t CheckWlanMode(void)
{
  unsigned int wifiMode = GPIO_PinInGet(GPIO_GROUP[gpioGroup_WIFI_Mode].port, GPIO_GROUP[gpioGroup_WIFI_Mode].pin);
  //0601 enable wifi-reconnect in release ver1.0.0
  //return s2lmbootupMode_Streaming;
  if (wifiMode == GPIO_LOW_LEVEL) {
    LOG_DEBUG("s2lmbootupMode_WlanReconnect\n");
    return s2lmbootupMode_WlanReconnect;
  } else {
    LOG_DEBUG("s2lmbootupMode_Streaming\n");
    return s2lmbootupMode_Streaming;
  }
}

/*
 *  Deinitialize the LED's and power off the Wifi Chip
 */
void before_sleep_do()
{
    led_deinit();
    //As of now shutting down the complete Wifi chip

    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_WIFI_EN].port, GPIO_GROUP[gpioGroup_WIFI_EN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BT_RST].port, GPIO_GROUP[gpioGroup_BT_RST].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PWR_EN].port, GPIO_GROUP[gpioGroup_PWR_EN].pin);

}

void after_wakeup_do()
{
    led_init();
    //GpioSetup(gpioGroup_WIFI_EN, 0);
}

void mcu_enter_sleep(void)
{
	//only goes to the sleep mode when power is low
  //if ((_mcuLowEnergyModeEnable == true) && (_timerWlanReconnect.enable == 0) && (_battery_low_quantity() == true)) {
    //maybe we need to modify the rtc's clock always tobe ULFRCO
    //Delay(500);

    // OOMA's board can not wake up from EM3 mode by GPIO IRQ
    // Using EM2 for now: RTC is working
    EMU_EnterEM2(false);
    // EMU_EnterEM3(true);
    // EMU_EnterEM4();
    // UartSetup(false);
  //} else {
  //  //DC plugin, don't enter low energy mode
  // Delay(50);
 // }
}

