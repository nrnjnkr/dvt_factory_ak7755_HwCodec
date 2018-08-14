/*
 * /s2lm_elektra_project/include/am_led.h
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

#ifndef AM_LED_H_
#define AM_LED_H_

typedef enum
{
  ledStatus_On = 0,
  ledStatus_Off,
  ledStatus_Blink,
} LED_Status_TypeDef;

typedef enum
{
  ledType_All = 0,
  ledType_Red,
  ledType_Green,
  ledType_Interval,
} LED_Type_TypeDef;

typedef struct LED {
	int state;
}LED_t;

LED_t led_red, led_green, led_blue;

#define WORKING_MODE_NUM 6
typedef enum{
	working = 0,
	standby,
	vca,
	need_charge,
	low_power,
	reset_wifi,
}Work_Mode_Def;

void ConfigLEDStatus(LED_Type_TypeDef led, LED_Status_TypeDef status, uint8_t timer);
void LED_Toggle(void);
LED_Type_TypeDef GetLEDType(Work_Mode_Def work_mode);
LED_Status_TypeDef GetLEDStatus(Work_Mode_Def work_mode);
int led_flash(uint8_t color);

int led_set_pwm(uint8_t color, uint8_t pwm);

void led_init(void);
void led_deinit(void);
int led_test(void);

#endif /* AM_LED_H_ */
