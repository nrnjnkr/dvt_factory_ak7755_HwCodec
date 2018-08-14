/*
 * /s2lm_elektra_project/src/am_common.c
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

#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "config.h"
#include "am_base.h"

static volatile uint32_t msTicks;

#define PWR_BUTTON_PRESS_MODE_0 (500)//(3000) //3s
#define PWR_BUTTON_PRESS_MODE_1 (1000)//(5000) //5s
#define PWR_BUTTON_PRESS_MODE_2 (2000)//(10000) //10s
#define PWR_BUTTON_PRESS_MODE_3 (5000) //15s

void SetupSysTick(void)
{
  /* Setup SysTick Timer for 1 msec interrupts  */
  if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1);
}

void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

bool DelayCheckGpio(uint8_t gpio, uint32_t dlyTicks, uint8_t status)
{
  uint32_t curTicks = 0;
  msTicks = curTicks;
  while (status == GPIO_PinInGet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin)) {
    if ((msTicks - curTicks) >= dlyTicks) {
      return true;
    }
  }
  return false;
}

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)

uint8_t DelayCheckPwrMode(uint8_t mcuMode)
{
#ifdef PWR_MODE_ENABLE
  uint32_t curTicks = 0;
  msTicks = curTicks;
  while (!GPIO_PinInGet(GPIO_GROUP[gpioGroup_PWR_Button].port, GPIO_GROUP[gpioGroup_PWR_Button].pin)) {
    if (msTicks >= PWR_BUTTON_PRESS_MODE_3) {
      return pwrMode_UsbDownload;
    }
  }

  if (msTicks >= PWR_BUTTON_PRESS_MODE_2) {
    return pwrMode_MpMode;
  } else if (msTicks >= PWR_BUTTON_PRESS_MODE_1) {
    return pwrMode_FwUpgrade;
  } else if (msTicks >= PWR_BUTTON_PRESS_MODE_0) {
    return (mcuMode == mcuMode_Working)? pwrMode_TriggerOff : pwrMode_WifiSetup;
  }

  return (mcuMode == mcuMode_Working)? pwrMode_Invalid : pwrMode_TriggerOn;
#else
  uint32_t curTicks = 0;
  msTicks = curTicks;
  while (!GPIO_PinInGet(GPIO_GROUP[gpioGroup_PWR_Button].port, GPIO_GROUP[gpioGroup_PWR_Button].pin)) {
    if (msTicks >= PWR_BUTTON_PRESS_MODE_3) {
      return pwrMode_UartWorkingMode;
    }
  }

  if (msTicks >= PWR_BUTTON_PRESS_MODE_2) {
    return pwrMode_MpMode;
  }
  return pwrMode_WifiSetup;
#endif
}

#elif defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// Put these drivers into a separate module
uint8_t DelayCheckPwrMode(uint8_t mcuMode)
{
    return pwrMode_Invalid;
}

#endif

bool DelayCheckGpioPIR(uint32_t dlyTicks)
{
  uint32_t curTicks = 0;
  msTicks = curTicks;
  while (GPIO_PinInGet(GPIO_GROUP[gpioGroup_PIR_Wakeup].port, GPIO_GROUP[gpioGroup_PIR_Wakeup].pin)) {
    if ((msTicks - curTicks) >= dlyTicks) {
      return true;
    }
  }
  return false;
}

uint32_t GetCurTicks()
{
  return msTicks;
}
/**************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 *****************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}
