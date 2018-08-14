/*
 * /s2lm_elektra_project/src/am_rtc.c
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
#include "em_rtc.h"

#include "am_rtc.h"

void RTCSetup(CMU_Select_TypeDef clockSource, uint32_t clkDiv, uint32_t timer)
{
  CMU_ClockSelectSet(cmuClock_LFA, clockSource);
  CMU_ClockEnable(cmuClock_RTC, true);
  if (clkDiv > 0) CMU_ClockDivSet(cmuClock_RTC, clkDiv);

  uint32_t comp0 = CMU_ClockFreqGet(cmuClock_RTC);
  RTC_CompareSet(0, comp0 * timer);

  RTC_Init_TypeDef rtc_init = RTC_INIT_DEFAULT;
  rtc_init.enable = false;
  RTC_IntClear(_RTC_IFC_MASK);
  RTC_IntEnable(RTC_IEN_COMP0);
  RTC_Init(&rtc_init);

  NVIC_ClearPendingIRQ(RTC_IRQn);
  NVIC_EnableIRQ(RTC_IRQn);
  RTC_Enable(true);
}
