/*
 * /s2lm_elektra_project/src/main.c
 *
 *  Created on: May 6, 2015
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
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_pcnt.h"
#include "em_rtc.h"
#include "em_leuart.h"
#include "em_i2c.h"
#include "em_timer.h"
#include "em_adc.h"

#include "config.h"
#include "am_base.h"
#include "am_log.h"
#include "am_common.h"
#include "am_rtc.h"
#include "am_adc.h"
#include "am_i2c.h"
#include "am_battery.h"
#include "am_led.h"
#include "am_timer.h"
#include "am_uart.h"
#include "am_utility.h"
#include "am_device.h"
#include "am_mfg.h"
#include "pir.h"
#include "tek_msg.h"
#include "potentiometer.h"
#include "accelerometer.h"
#include "factory_test.h"

extern tFactoryTests factory;

//////////////////////////////////////////////////////////////////////////////////


void PowerOnS2lm(uint8_t s2lmbootupMode)
{
  //TODO
  GpioSetup(gpioGroup_BootMode, true);
  ConfigS2lmBootMode(s2lmbootupMode);
  if (/*s2lmbootupMode == s2lmbootupMode_Config || */ s2lmbootupMode == s2lmbootupMode_rebuild_hiber ) {
  	GPIO_PinOutClear(GPIO_GROUP[gpioGroup_WIFI_EN].port, GPIO_GROUP[gpioGroup_WIFI_EN].pin);
  	GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PWR_EN].port, GPIO_GROUP[gpioGroup_PWR_EN].pin);
  }
  GPIO_PinOutSet(GPIO_GROUP[gpioGroup_PowerCtrl].port, GPIO_GROUP[gpioGroup_PowerCtrl].pin);
  
  _s2lmStatus = s2lmStatus_Off_to_On_Transfer;
}


void PowerOffS2lm(void)
{
  GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PowerCtrl].port, GPIO_GROUP[gpioGroup_PowerCtrl].pin);
  GpioSetup(gpioGroup_BootMode, false);
  ConfigLEDStatus(ledType_All, ledStatus_Off, 0);
  _s2lmStatus = s2lmStatus_PowerOff;
}

void mcu_mode_change(MCU_Mode_TypeDef new_mode)
{
    _mcuMode = new_mode;
}

void mcu_uart_sleep_trigger()
{
    _UartMcuSleepTrigger = triggerStatus_Set;
}

uint8_t mcu_get_wakeup_trigger()
{
    return _oldTriggerMode;
}

/*
 * Function to disable all GPIO IRQ's
 */
void DisableTriggers() {
	/*
	 * TO-DO : Need to think about the Button trigger
	 */
	DisableGpioIRQ(gpioGroup_PIR_Wakeup);
	DisableGpioIRQ(gpioGroup_ACCELEROMETER_INTERRUPT);
	DisableGpioIRQ(gpioGroup_SWT_DET);
	DisableGpioIRQ(gpioGroup_VOICE_WAKEUP);
	DisableGpioIRQ(gpioGroup_ReadyOff);
}

void EnableTriggers() {

    /*
     * Actually all interrupts should be enabled once MCU goes to sleep or in factory test mode.
     */

    //PIR
    EnableGpioIRQ(gpioGroup_PIR_Wakeup);
    // G-Sensor
    EnableGpioIRQ(gpioGroup_ACCELEROMETER_INTERRUPT);
    // Wake-on WiFi
    EnableGpioIRQ(gpioGroup_WIFI_Wakeup);
    //Button
    EnableGpioIRQ(gpioGroup_SWT_DET);
    /*
     *  Pull down Voice detect preset to low to enable voice detect
     */
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_VOICE_DETECT_PRESET].port, GPIO_GROUP[gpioGroup_VOICE_DETECT_PRESET].pin, gpioModePushPull, 0)
    Delay(100);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_VOICE_DETECT_PRESET].port, GPIO_GROUP[gpioGroup_VOICE_DETECT_PRESET].pin);
    Delay(100);
    EnableGpioIRQ(gpioGroup_VOICE_WAKEUP);
}

void _prepare(void)
{
  // Clock Init
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_VCMP, false);
  CMU_ClockEnable(cmuClock_AES, false);
  CMU_ClockEnable(cmuClock_DMA, false);

  //setup sys tick
  SetupSysTick();
  UartSetup(true);

  _show_mcu_version_info();

  // GPIO input / output config
  //power control
  GpioSetup(gpioGroup_PowerCtrl, true);
  PowerOffS2lm();
  /*
   * Configuring GPIO's but not enabling the interrupt lines by default.
   * Use EnableGpioIRQ for enabling interrupt for the gpio's.
   */
  GpioSetup(gpioGroup_PWR_EN, false);
  GpioSetup(gpioGroup_WIFI_EN, false);
  GpioSetup(gpioGroup_SWT_DET, false);
  GpioSetup(gpioGroup_PIR_Wakeup, false);
  GpioSetup(gpioGroup_RGB_EN, false);
  GpioSetup(gpioGroup_PIR_SENS_CONTROL, false);
  GpioSetup(gpioGroup_ReadyOff, false);
  GpioSetup(gpioGroup_WIFI_Wakeup, false);
  GpioSetup(gpioGroup_WIFI_Mode, false);
  GpioSetup(gpioGroup_BootMode, false);
  GpioSetup(gpioGroup_BT_RST, false);
  GpioSetup(gpioGroup_ACCELEROMETER_INTERRUPT, false);
  GpioSetup(gpioGroup_BT_HOST_WAKEUP, false);
  GpioSetup(gpioGroup_VOICE_WAKEUP, false);
  GpioSetup(gpioGroup_BatterAlert, false);

  //enable GPIO IRQ
  GpioIRQEven(true);
  GpioIRQOdd(true);

  // Initialize the all trigger to Off State
  _offTrigger = triggerStatus_Off;
  _pirTrigger = triggerStatus_Off;
  _wifiTrigger = triggerStatus_Off;
  _AccelDetectTrigger = triggerStatus_Off;
  _ButtonTrigger = triggerStatus_Off;
  _VoiceDetectTrigger = triggerStatus_Off;

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)
  GpioSetup(gpioGroup_PWR_Button, true);
#endif
  //power manager
  if (0 != SetupPowerManager(_powerManagerflag)) {
    LOG_ERROR("SetupPowerManager fail\n");
  }

  //setup RTC, clock 32.768/4, timer is 1s
  //RTCSetup(cmuSelect_LFRCO, cmuClkDiv_4, 1);
  // change timer to 100ms, resolution is 122 micro senconds
  //RTC_CompareSet(0, 818);
#ifndef PWR_MODE_ENABLE
  //update swt button status
  _update_wifi_trigger();
#endif

  //dc init
  _init_dc_chrg();
  //update battery status
  _batteryStatus = UpdateBatStatus(_batteryStatus);

  // Initialize LED's
  led_init();


  pir_sensitivity_set(50);

  potentiometer_set(23); //0x7b 0x68

  //Initialize G-sensor
  accelerometer_init();
  accelerometer_probe();
  accelerometer_range_set(1); //2G

  //Probing Fuel gauge
  fuelgauge_probe();

  return;
}

/*
 * Function to check the whether triggers are set and handle the trigger if it's set.
 */

uint8_t performStatusWaitBootupTrigger(void)
{

	// Wake up from sleeping mode, should disable all interrupt in the future.

    if (_pirTrigger == triggerStatus_Set) {
        LOG_DEBUG("get pir trigger\n");
        _pirTrigger = triggerStatus_Clear;
        return mcuStatus_Handle_PIR_trigger;
    }

    if (_VoiceDetectTrigger == triggerStatus_Set)
    {
        LOG_DEBUG("get voice detect trigger\n");
        _VoiceDetectTrigger = triggerStatus_Clear;
        return mcuStatus_Handle_VOICE_trigger;
    }

    if (_ButtonTrigger == triggerStatus_Set)
    {
        LOG_DEBUG("get button trigger\n");
        _ButtonTrigger = triggerStatus_Clear;
        return mcuStatus_Handle_BUTTON_trigger;
    }

    if (_AccelDetectTrigger == triggerStatus_Set)
    {
        LOG_DEBUG("get accelerometer trigger\n");
        _AccelDetectTrigger = triggerStatus_Clear;
        return mcuStatus_Handle_ACCEL_trigger;
    }
    
    if (_wifiTrigger == triggerStatus_Set)
    {
        LOG_DEBUG("get wifi trigger\n");
        _wifiTrigger = triggerStatus_Clear;
        return mcuStatus_Handle_WIFI_trigger;
    }


    return mcuStatus_Wait_Bootup_trigger;
}

/*
 *  Function to wait for boot up the S2LM
 */
uint8_t performStatusWaitReadyoffTrigger(void)
{

  /*
   * As soon as S2LM boots up and informs MCU about successful power on by
   * setting ReadyOff GPIO High and setting LOW after 50ms delay.
   * This is how we can confirm S2LM is successfully powered On.
   * Below code does the detection of S2LM power on.
   */

  if ((GPIO_PinInGet(GPIO_GROUP[gpioGroup_ReadyOff].port,
                    GPIO_GROUP[gpioGroup_ReadyOff].pin) == GPIO_HIGH_LEVEL) 
        || (_offTrigger == triggerStatus_Set))
  {
	  Delay(50);
      if (GPIO_PinInGet(GPIO_GROUP[gpioGroup_ReadyOff].port,
                        GPIO_GROUP[gpioGroup_ReadyOff].pin) == GPIO_LOW_LEVEL)
      {
          _offTrigger = triggerStatus_Clear;
          _s2lmStatus = s2lmStatus_PowerOn;
          return mcuStatus_Running;
      }
  }

  return mcuStatus_Wait_ReadyOff_trigger;
}

/*
 *  Function to detect the powering off the S2LM
 */
uint8_t performStatusRunning(void)
{
    LOG_DEBUG("performStatusRunning\n");

    if ((GPIO_PinInGet(GPIO_GROUP[gpioGroup_ReadyOff].port,
                    GPIO_GROUP[gpioGroup_ReadyOff].pin) == GPIO_HIGH_LEVEL) 
        || (_offTrigger == triggerStatus_Set))
    {
        // Enter sleeping mode
        PowerOffS2lm();
        //De-initializes the LEDs and power off WiFi Chip
        before_sleep_do();

        _offTrigger = triggerStatus_Clear;
        _UartMcuSleepTrigger = triggerStatus_Clear;
        _mcuMode = mcuMode_Sleeping;
        _rtcCount = 0;
        //EnableTriggers();
        /*
         *  We have enabled all interrupts starting of the main application
         *  and we are skipping handling them if s2lm is powered On.
         *  One more reason enabling all interrupts at starting is factory test.
         *  TO-DO : idly all GPIO interrupts should enabled before going to sleep.
         */
         mcu_enter_sleep();
         return mcuStatus_Wait_Bootup_trigger;
    }

    return mcuStatus_Running;
}



//Handler for PIR trigger
uint8_t performStatusHandlePirTrigger(void)
{
  LOG_DEBUG("mcustatus_Handle_PIR_trigger\n");

  _oldTriggerMode = triggerMode_PIR;
  _s2lmStatus = s2lmStatus_Off_to_On_Transfer;
  _rtcCount = 0;
  //Disable all  the interrupts ( Kind of masking) as S2LM is going to power on.
  //DisableTriggers();
  after_wakeup_do();
  //PowerOnS2lm(s2lmbootupMode_Recording);
  PowerOnS2lm(s2lmbootupMode_Config);
  Delay(Delay_Time_After_S2L_Bootup);
  UartRxIntEnable();
  _mcuMode = mcuMode_Working;

  return mcuStatus_Wait_ReadyOff_trigger;
}

//Handler for Voice detect trigger.
uint8_t performStatusHandleVoiceTrigger(void)
{
    LOG_DEBUG("mcustatus_Handle_VOICE_trigger\n");

    _oldTriggerMode = triggerMode_Voice;
    _s2lmStatus = s2lmStatus_Off_to_On_Transfer;
    _rtcCount = 0;
    //DisableTriggers();
    after_wakeup_do();
    //PowerOnS2lm(s2lmbootupMode_Recording);
    PowerOnS2lm(s2lmbootupMode_Config);
    Delay(Delay_Time_After_S2L_Bootup);
    UartRxIntEnable();
    _mcuMode = mcuMode_Working;

    return mcuStatus_Wait_ReadyOff_trigger;
}

//Handler for G-Sensor trigger
uint8_t performStatusHandleAccelTrigger(void)
{
	LOG_DEBUG("mcustatus_Handle_ACCEL_trigger\n");

    uint32_t int_status;
    _oldTriggerMode = triggerMode_Accel;
    _s2lmStatus = s2lmStatus_Off_to_On_Transfer;
    _rtcCount = 0;
    //DisableTriggers();
    after_wakeup_do();

    // Get accel interrupt status
    accelerometer_int_status(&int_status);

    //PowerOnS2lm(s2lmbootupMode_Recording);
    PowerOnS2lm(s2lmbootupMode_Config);
    Delay(Delay_Time_After_S2L_Bootup);
    UartRxIntEnable();
    _mcuMode = mcuMode_Working;

    return mcuStatus_Wait_ReadyOff_trigger;
}

//Handler for Button trigger
uint8_t performStatusHandleButtonTrigger(void)
{
    LOG_DEBUG("mcustatus_Handle_Button_trigger\n");

    _oldTriggerMode = triggerMode_Button;
    _s2lmStatus = s2lmStatus_Off_to_On_Transfer;
    _rtcCount = 0;
    after_wakeup_do();

    //PowerOnS2lm(s2lmbootupMode_Recording);
    PowerOnS2lm(s2lmbootupMode_Config);
    Delay(Delay_Time_After_S2L_Bootup);
    UartRxIntEnable();
    _mcuMode = mcuMode_Working;

    return mcuStatus_Wait_ReadyOff_trigger;
}

//Handler for WiFi trigger
uint8_t performStatusHandleWifiTrigger(void)
{
  LOG_DEBUG("mcustatus_Handle_WIFI_trigger\n");

  _oldTriggerMode = triggerMode_Wifi;

  uint8_t bootupMode = s2lmbootupMode_Streaming;

  if (_wifiReconnect) {
	  bootupMode = s2lmbootupMode_WlanReconnect;
	  _wifiTrigger = triggerStatus_Clear;
  } else if (_wifiTrigger == triggerStatus_Set) {
    _wifiTrigger = triggerStatus_Clear;
    bootupMode = CheckWlanMode();
  } else if (_timerWlanReconnect.trigger == triggerStatus_Set) {
    _timerWlanReconnect.trigger = triggerStatus_Clear;
    bootupMode = s2lmbootupMode_WlanReconnect;
  } else {
    LOG_ERROR("Handle Wifi Trigger, but no valid trigger! Check Me!\n");
  }

  _s2lmStatus = s2lmStatus_Off_to_On_Transfer;

  _rtcCount = 0;
  bootupMode = s2lmbootupMode_Config;
  PowerOnS2lm(bootupMode);
  Delay(Delay_Time_After_S2L_Bootup);
  UartRxIntEnable();
  _mcuMode = mcuMode_Working;
  _wifiReconnect = 0;

  return mcuStatus_Wait_ReadyOff_trigger;
}

uint8_t performStatusStandBy(void) {
  UartRxIntDisable();
  DisableTriggers();
  PowerOffS2lm();
  //De-initializes the LEDs and power off WiFi Chip
  before_sleep_do();
  mcu_enter_sleep();
  return mcuStatus_Wait_Bootup_trigger;

}

/*
 *  MCU main application
 */
int main(void)
{
  uint8_t mcuStatus = mcuStatus_Wait_Bootup_trigger;
  /* Chip errata */
  CHIP_Init();

  /* MCU working state */
  _mcuMode = mcuMode_Working;

  /* Here all modules will be Initialized like Uart, I2c device G-sensor, Loud noise device. */
  _prepare();

  /* Power On WiFi and Bluetooth modules. */
   PowerWifiModule(true);

   /* Start of the MCU first state will be config mode */
   uint8_t bootMode = s2lmbootupMode_Config;
   _oldTriggerMode = triggerMode_PWR;


   EnableGpioIRQ(gpioGroup_ReadyOff);
   mcuStatus = mcuStatus_Wait_ReadyOff_trigger;

   /*
    *  By default booting the s2lm,
    *  Note : TO-DO we should able to power on S2LM only when button triggers for the first time.
    */
   PowerOnS2lm(bootMode);
   /*
    * Initializing factory test cases.
    * Note : TO-DO, Need to remove in release branch
    */
   Factory_init();
   /*
    *  Enabling all the triggers as now.
    */
   EnableTriggers();

   while (1) {

	   UartHandler();

	   if(factory.standby.state == TEST_ON) {
		   mcuStatus = mcuStatus_Handle_StandBy;
		   factory.standby.state = TEST_OFF;
	   }

	   switch (mcuStatus) {

	   case mcuStatus_Wait_Bootup_trigger: // MCU sleeping mode
		   mcuStatus = performStatusWaitBootupTrigger();
		   break;
	   case mcuStatus_Wait_ReadyOff_trigger: // wait S2L bootup done
		   mcuStatus = performStatusWaitReadyoffTrigger();
		   break;
	   case mcuStatus_Running: // MCU and S2L running status
		   mcuStatus = performStatusRunning();
		   break;
	   case mcuStatus_Handle_PIR_trigger:
		   mcuStatus = performStatusHandlePirTrigger();
		   break;
	   case mcuStatus_Handle_VOICE_trigger:
		   mcuStatus = performStatusHandleVoiceTrigger();
		   break;
	   case mcuStatus_Handle_BUTTON_trigger:
		   mcuStatus = performStatusHandleButtonTrigger();
		   break;
	   case mcuStatus_Handle_ACCEL_trigger:
		   mcuStatus = performStatusHandleAccelTrigger();
		   break;
	   case mcuStatus_Handle_WIFI_trigger:
		   mcuStatus = performStatusHandleWifiTrigger();
		   break;
	   case mcuStatus_Handle_StandBy:
		   mcuStatus = performStatusStandBy();
		   break;
	   default:
			break;
    }
  }

  return 0;
}
