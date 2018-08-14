/*
 * /s2lm_elektra_project/src/am_timer.c
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
#include "em_gpio.h"
#include "em_timer.h"

#include "am_log.h"
#include "am_timer.h"
#include "am_led.h"

////////////////////////////////////////////////
static uint32_t _prescale(uint32_t preScale)
{
  switch (preScale) {
  case timerPrescale1:
    return 1;

  case timerPrescale2:
    return 2;

  case timerPrescale4:
    return 4;

  case timerPrescale8:
    return 8;

  case timerPrescale16:
    return 16;

  case timerPrescale32:
    return 32;

  case timerPrescale64:
    return 64;

  case timerPrescale128:
    return 128;

  case timerPrescale256:
    return 256;

  case timerPrescale512:
    return 512;

  case timerPrescale1024:
    return 1024;

  default:
    return 1;
  }
}
////////////////////////////////////////////////
void Timer0Setup(uint32_t preScale, uint32_t num, uint32_t den)
{
  CMU_ClockEnable(cmuClock_TIMER0, true);

  TIMER_Reset(TIMER0);
  TIMER_IntClear(TIMER0,_TIMER_IFC_MASK);
  TIMER_IntEnable(TIMER0, TIMER_IEN_OF);

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;
  timerInit.prescale = preScale;
  TIMER_Init(TIMER0, &timerInit);
  TIMER_TopSet(TIMER0, (num * CMU_ClockFreqGet(cmuClock_TIMER0))/(den * _prescale(preScale)));

  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_EnableIRQ(TIMER0_IRQn);
  TIMER_Enable(TIMER0, true);
}

void Timer1Setup(uint32_t preScale, uint32_t num, uint32_t den)
{
  CMU_ClockEnable(cmuClock_TIMER1, true);

  TIMER_Reset(TIMER1);
  TIMER_IntClear(TIMER1,_TIMER_IFC_MASK);
  TIMER_IntEnable(TIMER1, TIMER_IEN_OF);

  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.enable = false;
  timerInit.prescale = preScale;
  TIMER_Init(TIMER1, &timerInit);
  TIMER_TopSet(TIMER1, (num * CMU_ClockFreqGet(cmuClock_TIMER1))/(den * _prescale(preScale)));

  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  NVIC_EnableIRQ(TIMER1_IRQn);
  TIMER_Enable(TIMER1, true);
}

void ResetTimer0()
{
  TIMER_Enable(TIMER0, false);
  NVIC_DisableIRQ(TIMER0_IRQn);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  TIMER_Reset(TIMER0);
  CMU_ClockEnable(cmuClock_TIMER0, false);
}

void ResetTimer1()
{
  TIMER_Enable(TIMER1, false);
  NVIC_DisableIRQ(TIMER1_IRQn);
  NVIC_ClearPendingIRQ(TIMER1_IRQn);
  TIMER_Reset(TIMER1);
  CMU_ClockEnable(cmuClock_TIMER1, false);
}
