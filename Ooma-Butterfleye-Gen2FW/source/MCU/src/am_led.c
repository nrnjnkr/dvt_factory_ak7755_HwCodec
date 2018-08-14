/*
 * /s2lm_elektra_project/src/am_led.c
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
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_timer.h"

#include "config.h"

#include "am_led.h"
#include "am_timer.h"
#include "am_log.h"

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)

typedef volatile struct
{
  LED_Status_TypeDef status;
  uint8_t timer; //timer to blink
} LEDStatusUnit;

typedef struct
{
	LED_Type_TypeDef   type;
	LED_Status_TypeDef status;
}LED_Mode;

static LED_Mode mode[WORKING_MODE_NUM]= {
	{ledType_Green, ledStatus_On},//working
	{ledType_All, ledStatus_Off},//standby
	{ledType_Red,ledStatus_Blink},//vca
	{ledType_Green, ledStatus_On},//{ledType_All,ledStatus_On},//need_charge
	{ledType_Green, ledStatus_On},//{ledType_All,ledStatus_On}, //low_power
	{ledType_Green, ledStatus_Blink},//reset_wifi
};


static LEDStatusUnit redStatus = {ledStatus_Off, 0};
static LEDStatusUnit greenStatus = {ledStatus_Off, 0};
//////////////////////////////////////////////////////
static void _led_red(bool on)
{
  if (on) GPIO_PinOutClear(GPIO_GROUP[gpioGroup_LED_Red].port, GPIO_GROUP[gpioGroup_LED_Red].pin);
  else GPIO_PinOutSet(GPIO_GROUP[gpioGroup_LED_Red].port, GPIO_GROUP[gpioGroup_LED_Red].pin);
}

static void _led_green(bool on)
{
  if (on) GPIO_PinOutClear(GPIO_GROUP[gpioGroup_LED_Green].port, GPIO_GROUP[gpioGroup_LED_Green].pin);
  else GPIO_PinOutSet(GPIO_GROUP[gpioGroup_LED_Green].port, GPIO_GROUP[gpioGroup_LED_Green].pin);
}

static void _led_toggle(uint8_t led)
{
  GPIO_PinOutToggle(GPIO_GROUP[led].port, GPIO_GROUP[led].pin);
}
//////////////////////////////////////////////////////
void ConfigLEDStatus(LED_Type_TypeDef led, LED_Status_TypeDef status, uint8_t timer)
{
  switch (status) {
  case ledStatus_On:
  case ledStatus_Off: {
    bool on = (status == ledStatus_On)? true : false;
    if (led == ledType_All) {
      redStatus.status = status;
      redStatus.timer = 0;
      _led_red(on);
      greenStatus.status = status;
      greenStatus.timer = 0;
      _led_green(on);
    } else if (led == ledType_Red) {
      redStatus.status = status;
      redStatus.timer = 0;
      _led_red(on);

      greenStatus.status = ledStatus_Off;
      greenStatus.timer = 0;
      _led_green(false);
    } else if (led == ledType_Green) {
      greenStatus.status = status;
      greenStatus.timer = 0;
      _led_green(on);

      //0409
      redStatus.status = ledStatus_Off;
      redStatus.timer = 0;
      _led_red(false);
    }
  }
    break;

  case ledStatus_Blink:
    if (led == ledType_All) {
      redStatus.status = ledStatus_Blink;
      redStatus.timer = timer;
      _led_red(true);
      greenStatus.status = ledStatus_Blink;
      greenStatus.timer = timer;
      _led_green(true);
    } else if (led == ledType_Red) {
      redStatus.status = ledStatus_Blink;
      redStatus.timer = timer;
      _led_red(true);

      greenStatus.status = ledStatus_Off;
      greenStatus.timer = 0;
      _led_green(false);
    } else if (led == ledType_Green) {
      greenStatus.status = ledStatus_Blink;
      greenStatus.timer = timer;
      _led_green(true);

      redStatus.status = ledStatus_Off;
      redStatus.timer = 0;
      _led_red(false);
    } else if (led == ledType_Interval) {
      redStatus.status = ledStatus_Blink;
      redStatus.timer = timer;
      _led_red(true);

      greenStatus.status = ledStatus_Blink;
      greenStatus.timer = timer;
      _led_green(false);
    }
    Timer1Setup(timerPrescale1024, 2, 10);
    break;

  default:
    break;
  }
  return;
}

void LED_Toggle(void)
{
  if (redStatus.status == ledStatus_Blink) {
    _led_toggle(gpioGroup_LED_Red);
  }

  if (greenStatus.status == ledStatus_Blink) {
    _led_toggle(gpioGroup_LED_Green);
  }
}

LED_Type_TypeDef GetLEDType(Work_Mode_Def work_mode)
{
	int idx = (work_mode < WORKING_MODE_NUM)? work_mode: 0;
	return mode[idx].type;
}

LED_Status_TypeDef GetLEDStatus(Work_Mode_Def work_mode)
{
	int idx = (work_mode < WORKING_MODE_NUM)? work_mode: 0;
		return mode[idx].status;
}
#endif
