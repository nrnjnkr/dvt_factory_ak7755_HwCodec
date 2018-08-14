/*
 * /s2lm_elektra_project/src/am_adc.c
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
#include "em_adc.h"

#include "am_log.h"
#include "am_adc.h"

#if NOT_USE
static uint32_t _adcsetup_calibration_offset(ADC_TypeDef *adc, ADC_Ref_TypeDef ref)
{
  int32_t  sample;
  uint32_t cal;

  /* Binary search variables */
  uint8_t high;
  uint8_t mid;
  uint8_t low;

  /* Reset ADC to be sure we have default settings and wait for ongoing */
  /* conversions to be complete. */
  ADC_Reset(adc);

  ADC_Init_TypeDef       init       = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

  /* Init common settings for both single conversion and scan mode */
  init.timebase = ADC_TimebaseCalc(0);
  /* Might as well finish conversion as quickly as possibly since polling */
  /* for completion. */
  /* Set ADC clock to 7 MHz, use default HFPERCLK */
  init.prescale = ADC_PrescaleCalc(7000000, 0);

  /* Set an oversampling rate for more accuracy */
  init.ovsRateSel = adcOvsRateSel4096;
  /* Leave other settings at default values */
  ADC_Init(adc, &init);

  /* Init for single conversion use, measure diff 0 with selected reference. */
  singleInit.reference = ref;
  singleInit.input     = adcSingleInpDiff0;
  singleInit.acqTime   = adcAcqTime16;
  singleInit.diff      = true;
  /* Enable oversampling rate */
  singleInit.resolution = adcResOVS;

  ADC_InitSingle(adc, &singleInit);

  /* ADC is now set up for offset calibration */
  /* Offset calibration register is a 7 bit signed 2's complement value. */
  /* Use unsigned indexes for binary search, and convert when calibration */
  /* register is written to. */
  high = 128;
  low  = 0;

  /* Do binary search for offset calibration*/
  while (low < high)
  {
    /* Calculate midpoint */
    mid = low + (high - low) / 2;

    /* Midpoint is converted to 2's complement and written to both scan and */
    /* single calibration registers */
    cal      = adc->CAL & ~(_ADC_CAL_SINGLEOFFSET_MASK | _ADC_CAL_SCANOFFSET_MASK);
    cal     |= (mid - 63) << _ADC_CAL_SINGLEOFFSET_SHIFT;
    cal     |= (mid - 63) << _ADC_CAL_SCANOFFSET_SHIFT;
    adc->CAL = cal;

    /* Do a conversion */
    ADC_Start(adc, adcStartSingle);

    /* Wait while conversion is active */
    while (adc->STATUS & ADC_STATUS_SINGLEACT) ;

    /* Get ADC result */
    sample = ADC_DataSingleGet(adc);

    /* Check result and decide in which part of to repeat search */
    /* Calibration register has negative effect on result */
    if (sample < 0)
    {
      /* Repeat search in bottom half. */
      high = mid;
    }
    else if (sample > 0)
    {
      /* Repeat search in top half. */
      low = mid + 1;
    }
    else
    {
      /* Found it, exit while loop */
      break;
    }
  }

  return adc->CAL;
}

static void _adc_calibration()
{
  uint8_t  offset_calibration_value;
  uint8_t  gain_calibration_value;
  uint32_t calibration_value = 0;

  char buffer[64] = {0};

  uint32_t old_gain_calibration_value =
    (DEVINFO->ADC0CAL0 & _DEVINFO_ADC0CAL0_2V5_GAIN_MASK)
    >> _DEVINFO_ADC0CAL0_2V5_GAIN_SHIFT;

  LOG_DEBUG("old gain %u\n", old_gain_calibration_value);

  uint32_t old_offset_calibration_value =
    (DEVINFO->ADC0CAL0 & _DEVINFO_ADC0CAL0_2V5_OFFSET_MASK)
    >> _DEVINFO_ADC0CAL0_2V5_OFFSET_SHIFT;

  LOG_DEBUG("old offset %u\n", old_offset_calibration_value);

  calibration_value = adcsetup_calibration_offset(ADC0, adcRef2V5);

  offset_calibration_value = (calibration_value & _ADC_CAL_SINGLEOFFSET_MASK) >> _ADC_CAL_SINGLEOFFSET_SHIFT;
  gain_calibration_value   = (calibration_value & _ADC_CAL_SINGLEGAIN_MASK) >> _ADC_CAL_SINGLEGAIN_SHIFT;

  LOG_DEBUG("new offset %u\n", offset_calibration_value);
  LOG_DEBUG("new gain %u\n", gain_calibration_value);

  old_gain_calibration_value =
    (DEVINFO->ADC0CAL0 & _DEVINFO_ADC0CAL0_2V5_GAIN_MASK)
    >> _DEVINFO_ADC0CAL0_2V5_GAIN_SHIFT;

  LOG_DEBUG("2 old gain %u\n", old_gain_calibration_value);

  old_offset_calibration_value =
    (DEVINFO->ADC0CAL0 & _DEVINFO_ADC0CAL0_2V5_OFFSET_MASK)
    >> _DEVINFO_ADC0CAL0_2V5_OFFSET_SHIFT;

  LOG_DEBUG("2 old offset %u\n", old_offset_calibration_value);
}
#endif
static uint32_t _adc_resolution(uint32_t res)
{
  switch (res) {
  case 12:
    return adcRes12Bit;

  case 8:
    return adcRes8Bit;

  case 6:
    return adcRes6Bit;

  default:
    return adcRes12Bit;
  }
}
////////////////////////////////////////////////
void ADCSetup(uint32_t resolution)
{
  CMU_ClockEnable(cmuClock_ADC0, true);

  ADC_Reset(ADC0);

  //adc_calibration();

  ADC_Init_TypeDef adcInit = ADC_INIT_DEFAULT;
  adcInit.timebase = ADC_TimebaseCalc(0);
  adcInit.prescale = ADC_PrescaleCalc(7000000, 0);
  adcInit.ovsRateSel = adcOvsRateSel16;

  ADC_InitSingle_TypeDef adcSingleInit = ADC_INITSINGLE_DEFAULT;
  adcSingleInit.input = adcSingleInpCh6;
  adcSingleInit.acqTime = adcAcqTime16;
  adcSingleInit.reference = adcRef2V5;
  adcSingleInit.resolution = _adc_resolution(resolution);
  adcSingleInit.rep = false;

  ADC_Init(ADC0, &adcInit);
  ADC_InitSingle(ADC0, &adcSingleInit);
  //ADC_IntEnable(ADC0, ADC_IF_SINGLE);
  //NVIC_EnableIRQ(ADC0_IRQn);
}

uint32_t ADCReadSingleData(void)
{
  ADC_Start(ADC0, adcStartSingle);
  while(ADC0->STATUS & ADC_STATUS_SINGLEACT);

  return ADC_DataSingleGet(ADC0);
}
