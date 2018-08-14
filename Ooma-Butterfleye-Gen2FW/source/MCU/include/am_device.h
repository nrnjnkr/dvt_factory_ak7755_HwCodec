/*
 * /s2lm_elektra_project/include/am_device.h
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

#ifndef AM_DEVICE_H_
#define AM_DEVICE_H_

#include "factory_test.h"

extern tFactoryTests factory;

/////////////////////////////////////////////////////////////////////////////////
static volatile uint8_t _pirTrigger = triggerStatus_Init;
static volatile uint8_t _ButtonTrigger = triggerStatus_Init;
static volatile uint8_t _VoiceDetectTrigger = triggerStatus_Init;
static volatile uint8_t _AccelDetectTrigger = triggerStatus_Init;
static volatile uint8_t _UartMcuSleepTrigger = triggerStatus_Init;

static volatile uint8_t _wifiTrigger =  triggerStatus_Init;
static volatile uint8_t _pwrTrigger = triggerStatus_Init;
static volatile uint8_t _offTrigger = triggerStatus_Init;
static volatile uint8_t _batteryTrigger = triggerStatus_Init;
static volatile uint8_t _dcCheckTrigger = triggerStatus_Init;
//static volatile uint8_t _dcTrigger = triggerStatus_Init; //dc status
static volatile uint8_t _chrgTrigger = triggerStatus_Init;
static volatile uint8_t _chrgCheckTrigger = triggerStatus_Init;
static volatile uint8_t _swtTrigger = triggerStatus_Init;
static volatile uint8_t _notifyModeTrigger = triggerStatus_Init;
//TODO,trigger to notify pir status, interval is 1s now
static volatile uint8_t _pirNotifyTrigger = triggerStatus_Init;
static volatile uint8_t _pirStatus = pirStatus_Inactive;
static volatile uint8_t _dcStatus = dcStatus_No;
static volatile uint8_t _batteryStatus = batteryStatus_Normal;
static volatile uint8_t _chrgokStatus = chrgStatus_Discharging;

static volatile uint8_t _oldTriggerMode = triggerMode_Null;
/*
 * Used tracking S2LM status.
 */
static volatile uint8_t _s2lmStatus = s2lmStatus_PowerOff;

static volatile Timer_Wlan_Up _timerWlanReconnect = {0};
static volatile bool _mcuLowEnergyModeEnable = true;
static uint8_t _mcuMode = mcuMode_Sleeping;

static uint8_t _triggerEnablebits = TRIGGER_PIR_ENABLE_MASK;
static uint8_t _powerManagerflag =   powerManager_I2C;//powerManager_ADC;

static uint32_t _rtcCount = 0;
//#if 0
static uint32_t _rtcCountBatteryCheck = 0;
static uint32_t _rtcCountSwtCheck = 0;
static uint32_t _rtcCountChrgCheck = 0;

static uint32_t _swtCheckTimerInterval = 30; //s
static uint32_t _batteryCheckTimerInterval = 5; //s
static uint32_t _chrgCheckTimerInterval = 5; //s
static uint32_t _rtcTimerInterval = 1; //s
//#endif
static volatile uint8_t _wifiReconnect = 0;

//battery
//static int _soc = 0;
//static uint16_t rm_capacity = 0;
//static uint16_t voltage = 0;

//msg buffer
CircularBuffer _rxBuf = {{0}, 0, 0, 0, false};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//Internal function
//
static void _show_mcu_version_info(void)
{
  LOG_PRINT("===AMBA BTFL MCU VERSION: %s===\n", ELEKTRA_MCU_VERSION);
#ifdef ENABLE_I2C_DEBUG
  LOG_PRINT("I2C Debug Print: Enable\n");
#else
  LOG_PRINT("I2C Debug Print: Disable\n");
#endif
#ifdef ENABLE_UART_PRINT
  LOG_PRINT("UART For Debug: Enable\n");
#else
  LOG_PRINT("UART For Debug: Disable\n");
#endif
#ifdef ENABLE_MCU_SLEEP_MODE
  LOG_PRINT("MCU Low Energy Mode: Enable\n");
#else
  LOG_PRINT("MCU Low Energy Mode: Disable\n");
#endif
#ifdef PWR_MODE_ENABLE
  LOG_PRINT("Button Multi-Mode: Enable\n");
#else
  LOG_PRINT("Button Multi-Mode: Disable(only config mode)\n");
#endif
#ifdef RTC_USE_ULFRCO
  LOG_PRINT("RTC Clock: ULFRCO(1 kHz)\n");
#else
  LOG_PRINT("RTC Clock: LFRCO(32.768 kHz)\n");
#endif
#ifdef LOW_BATTERY_MODE_ENABLE
  LOG_PRINT("Low Battery Mode: Enable\n");
#else
  LOG_PRINT("Low Battery Mode: Disable\n");
#endif
#ifdef WIFI_RECONNECT_ENABLE
  LOG_PRINT("Wifi Reconnect Mode: Enable\n");
#else
  LOG_PRINT("Wifi Reconnect Mode: Disable\n");
#endif

  LOG_PRINT("DC Status Check by IRQ: Enable\n");
  LOG_PRINT("Wifi Trigger Switch by IRQ: Disable\n");
  LOG_PRINT("===Version Info Done===\n");
}

static void _update_wifi_trigger(void)
{
  if (UpdateWifiModuleStatus()) {
    PowerWifiModule(true);
    GpioSetup(gpioGroup_WIFI_Wakeup, false);
    //_wifiTrigger = triggerStatus_Clear;
    _triggerEnablebits |= TRIGGER_WIFI_ENABLE_MASK;
    LOG_DEBUG("Enable WIFI Trigger\n");
  } else {
    PowerWifiModule(false);
    _wifiTrigger = triggerStatus_Off;
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_WIFI_Wakeup].port, GPIO_GROUP[gpioGroup_WIFI_Wakeup].pin, gpioModeDisabled, 0);
    _triggerEnablebits &= (0xFF & (~TRIGGER_WIFI_ENABLE_MASK));
    LOG_DEBUG("Disable WIFI Trigger\n");
  }
}

static void _init_dc_chrg(void)
{
  // TODO: implement I2C driver
  //LOG_DEBUG("I2C DC is not implemented\n");
  return;
}


static void _off_all_trigger(void)
{
  GPIO_PinModeSet(GPIO_GROUP[gpioGroup_PIR_Wakeup].port, GPIO_GROUP[gpioGroup_PIR_Wakeup].pin, gpioModeDisabled, 0);
  GPIO_PinModeSet(GPIO_GROUP[gpioGroup_WIFI_Wakeup].port, GPIO_GROUP[gpioGroup_WIFI_Wakeup].pin, gpioModeDisabled, 0);
  GPIO_PinModeSet(GPIO_GROUP[gpioGroup_ReadyOff].port, GPIO_GROUP[gpioGroup_ReadyOff].pin, gpioModeDisabled, 0);
  _pirTrigger = triggerStatus_Off;
  _wifiTrigger = triggerStatus_Off;
  _offTrigger = triggerStatus_Off;
}

static void _notify_dcchrg_status(void)
{
  if (_dcStatus == dcStatus_Plugin) {
    SendMsg(msgToS2L_DcPlugin);
    LOG_DEBUG("notify dcStatus_Plugin\n");
    if (_chrgokStatus == chrgStatus_Full) {
      SendMsg(msgToS2L_ChargFull);
    } else {
      SendMsg(msgToS2L_Charging);
    }
  } else {
    LOG_DEBUG("notify dcStatus_Unplugged\n");
    SendMsg(msgToS2L_DcUnplugged);
    SendMsg(msgToS2L_Discharging);
  }
}

//GPIO Interrupt handler
void GPIO_EVEN_IRQHandler(void)
{

  static uint32_t cnt = 0;
  uint32_t gpio_int = GPIO_IntGetEnabled();
  _mcuMode = mcuMode_Working;
  tek_print("EVEN GPIO interrupt 0x%x\r\n", gpio_int);

  /*
   * Check if interrupt is for PIR trigger.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_PIR_Wakeup].pin)) {
	  GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_PIR_Wakeup].pin);
	  if(factory.pir.state == TEST_ON ) {
		 factory.pir.state = TEST_OFF;
		 SendCmdResponse('S');
		 return;
	  }
    LOG_DEBUG("PIR triggered, cnt %d\n", cnt++);
    if((_s2lmStatus == s2lmStatus_Off_to_On_Transfer)|| (_s2lmStatus == s2lmStatus_PowerOn) || _pirTrigger == triggerStatus_Set) {

    	LOG_DEBUG("Ignoring the PIR trigger as S2LM in power on status");
    }
    else
    	_pirTrigger = triggerStatus_Set;
  }

  /*
   * Check if interrupt is for voice detect trigger.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_VOICE_WAKEUP].pin)) {
    GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_VOICE_WAKEUP].pin);
    LOG_DEBUG("VOICE_WAKEUP triggered\n");
    if(factory.voice.state == TEST_ON ) {
    	factory.voice.state = TEST_OFF;
    	SendCmdResponse('S');
    	return;
    }

    if((_s2lmStatus == s2lmStatus_Off_to_On_Transfer)|| (_s2lmStatus == s2lmStatus_PowerOn) || _VoiceDetectTrigger == triggerStatus_Set ) {

    	LOG_DEBUG("Ignoring the VOice trigger as S2LM in power on status");
    }
    else
    	_VoiceDetectTrigger = triggerStatus_Set;

  }

  /*
   * Check, If interrupt is for G-Sensor trigger.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_ACCELEROMETER_INTERRUPT].pin)) {
    GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_ACCELEROMETER_INTERRUPT].pin);
    LOG_DEBUG("ACCEL triggered\n");
    if(factory.acl.state == TEST_ON ) {
    	factory.acl.state = TEST_OFF;
    	SendCmdResponse('S');
    	return;
    }
    if((_s2lmStatus == s2lmStatus_Off_to_On_Transfer)|| (_s2lmStatus == s2lmStatus_PowerOn)|| _AccelDetectTrigger == triggerStatus_Set) {

    	LOG_DEBUG("Ignoring the G-Sensor trigger as S2LM in power on status");
    }
    else
    	_AccelDetectTrigger = triggerStatus_Set;

  }

}

void GPIO_ODD_IRQHandler(void)
{
  uint32_t gpio_int = GPIO_IntGetEnabled();
  _mcuMode = mcuMode_Working;
  tek_print("ODD GPIO interrupt 0x%x\r\n", gpio_int);

  /*
   * Check, If interrupt is for ReadyOff trigger.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_ReadyOff].pin)) {
	  GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_ReadyOff].pin);
    LOG_DEBUG("ReadyOff triggered\n");
    _offTrigger = triggerStatus_Set;
  }

  /*
   * Check, If interrupt is for Wifi disconnect or Streaming.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_WIFI_Wakeup].pin)) {

    GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_WIFI_Wakeup].pin);
    if((_s2lmStatus == s2lmStatus_Off_to_On_Transfer)|| (_s2lmStatus == s2lmStatus_PowerOn) || _wifiTrigger == triggerStatus_Set) {

    	LOG_DEBUG("Ignoring the Wifi trigger as S2LM in power on status");
    }
    else
    	_wifiTrigger = triggerStatus_Set;
  }

  /*
   * Check, If interrupt is for Button trigger.
   */
  if (gpio_int & (1 << GPIO_GROUP[gpioGroup_SWT_DET].pin)) {
    GPIO_IntClear(1 << GPIO_GROUP[gpioGroup_SWT_DET].pin);

    if(factory.key.state == TEST_ON ) {
    	factory.key.state = TEST_OFF;
    	SendCmdResponse('S');
    	return;
    }
    LOG_DEBUG("BUTTON triggered\n");
    if((_s2lmStatus == s2lmStatus_Off_to_On_Transfer)|| (_s2lmStatus == s2lmStatus_PowerOn)|| _ButtonTrigger == triggerStatus_Set) {

    	LOG_DEBUG("Ignoring the BUTTON triggere as S2LM in power on status");
    }
    else
    	_ButtonTrigger = triggerStatus_Set;
  }

}

//LEUART0 Interrupt handler
void LEUART0_IRQHandler()
{
  uint32_t leuartIF = LEUART_IntGet(LEUART0);
  if (leuartIF & LEUART_IF_RXDATAV) {
    LEUART_IntClear(LEUART0, LEUART_IF_RXDATAV);
    /* Copy data into RX Buffer */
    uint8_t rxData = LEUART_Rx(LEUART0);
    _rxBuf.data[_rxBuf.wrI] = rxData;
    _rxBuf.wrI             = (_rxBuf.wrI + 1) % BUFFERSIZE;
    _rxBuf.pendingBytes++;

    /* Flag Rx overflow */
    if (_rxBuf.pendingBytes > BUFFERSIZE) {
      _rxBuf.overflow = true;
    }
  }

  if (leuartIF & LEUART_IF_TXBL) {
    //TODO
  }

}

// RTC Interrupt handler
void RTC_IRQHandler(void)
{
  uint32_t rtc_int = RTC_IntGet();
  if (rtc_int & RTC_IF_OF) {
    
    RTC_IntClear(RTC_IFC_OF);
  } else if (rtc_int & RTC_IF_COMP0) {
    RTC_IntClear(RTC_IFC_COMP0);
  }
}

//TIMER0 Interrupt handler
void TIMER0_IRQHandler(void)
{
  uint32_t timer0_if = TIMER_IntGet(TIMER0);
  if (timer0_if & TIMER_IEN_OF) {
    TIMER_IntClear(TIMER0, TIMER_IEN_OF);
    if (_timerWlanReconnect.enable) {
      _timerWlanReconnect.count++;
      if (_timerWlanReconnect.count >= _timerWlanReconnect.timer) {
        _timerWlanReconnect.trigger = triggerStatus_Set;
        _timerWlanReconnect.count = 0;
      }
    }
  }
}

//TIMER1 Interrupt handler
void TIMER1_IRQHandler(void)
{
  uint32_t timer1_if = TIMER_IntGet(TIMER1);
  if (timer1_if & TIMER_IEN_OF) {
    TIMER_IntClear(TIMER1, TIMER_IEN_OF);

    LED_Toggle();
  }
}

//ADC0 Interrupt handler
void ADC0_IRQHandler(void)
{
  uint32_t adc_if = ADC_IntGet(ADC0);
  if (adc_if & ADC_IF_SINGLE) {
    ADC_IntClear(ADC0, ADC_IFC_SINGLE);
  }
}

#endif /* AM_DEVICE_H_ */
