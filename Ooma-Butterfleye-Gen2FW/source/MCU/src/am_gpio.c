/*
 * /s2lm_elektra_project/src/proj_gpio.c
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
#include "em_gpio.h"
#include "em_cmu.h"

#include "config.h"
#include "am_base.h"
#include "am_log.h"

//////////////////////////////////////////////////////////////////////////////////
static void _enable_gpio_odd_irq(void)
{
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

static void _disable_gpio_odd_irq(void)
{
  NVIC_DisableIRQ(GPIO_ODD_IRQn);
}

static void _enable_gpio_even_irq(void)
{
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

static void _disable_gpio_even_irq(void)
{
  NVIC_DisableIRQ(GPIO_EVEN_IRQn);
}


/* dsp |  Mode
 *  2  | 1   | 0
 * PB8 | PB7 | PC1
 *  0  | 0   | 0 ===> dsp off, wlan config mode
 *  0  | 0   | 1 ===> dsp 0ff, wlan reconnect mode
 *  0  | 1   | 0 ===> dsp off, msg notification
 *  1  | 0   | 0 ===> dsp on, recording mode
 *  1  | 0   | 1 ===> dsp on, streaming mode
 */
static void _config_s2l_bootup_mode(uint8_t mode)
{
  switch (mode) {
  case s2lmbootupMode_Init:
    //GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    //GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    //GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_Streaming:
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_Recording:
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_Notify:
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_WlanReconnect:
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_Config:
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  case s2lmbootupMode_MpMode:
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin);
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin);
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin);
    break;

  default:
    break;
  }
  return;
}

//////////////////////////////////////////////////////////////////////////////////
void GpioSetup(uint8_t gpio, bool enable)
{
  switch (gpio) {
  case gpioGroup_SWT_DET:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
#ifdef SWT_IRQ_ENABLE
    //GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
#endif
    break;

  case gpioGroup_PIR_Wakeup:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    // GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInputPull, 1);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
    break;

  case gpioGroup_WIFI_Wakeup:
    //risingEdge
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, true, false, enable);
    break;

  case gpioGroup_WIFI_Mode:
    //TODO
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    break;

  case gpioGroup_PowerCtrl:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModePushPull, 0);
    break;

  case gpioGroup_ReadyOff:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, true, false, enable);
    break;

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)
    // For OOMA project, the LED is controlled by I2C driver.
    // TODO: split LED drivers and make it more readable
    case gpioGroup_LED:
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_LED_Red].port, GPIO_GROUP[gpioGroup_LED_Red].pin, gpioModePushPull, (enable? 0:1));
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_LED_Green].port, GPIO_GROUP[gpioGroup_LED_Green].pin, gpioModePushPull, (enable? 0:1));
    break;
#endif

  case gpioGroup_I2C:
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_I2C_SCL].port, GPIO_GROUP[gpioGroup_I2C_SCL].pin, gpioModeWiredAnd, 1);
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_I2C_SDA].port, GPIO_GROUP[gpioGroup_I2C_SDA].pin, gpioModeWiredAnd, 1);
    break;

  case gpioGroup_BootMode:
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_BootMode_0_PIN].port, GPIO_GROUP[gpioGroup_BootMode_0_PIN].pin, enable? gpioModePushPull : gpioModeDisabled, 0);
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_BootMode_1_PIN].port, GPIO_GROUP[gpioGroup_BootMode_1_PIN].pin, enable? gpioModePushPull : gpioModeDisabled, 0);
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_BootMode_2_PIN].port, GPIO_GROUP[gpioGroup_BootMode_2_PIN].pin, enable? gpioModePushPull : gpioModeDisabled, 0);
    break;

  case gpioGroup_PWR_EN:
  case gpioGroup_WIFI_EN:
  case gpioGroup_BT_RST:
    //TODO
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModePushPull, 0);
    GPIO_PinOutClear(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin);
    break;

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)
    case gpioGroup_CHRG_OK:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    //GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
    break;

  case gpioGroup_DC_OK:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
#ifdef DC_IRQ_ENABLE
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, true, true, enable);
#endif
    break;

  case gpioGroup_USB_Boot_Mode:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 0);
    break;
#endif // CONFIG_PROJECT_AMBA_ELEKTRA

  case gpioGroup_UART:
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_UART_RX].port, GPIO_GROUP[gpioGroup_UART_RX].pin, gpioModeInput, 1);
    GPIO_PinModeSet(GPIO_GROUP[gpioGroup_UART_TX].port, GPIO_GROUP[gpioGroup_UART_TX].pin, gpioModePushPull, 1);
    // GPIO_PinModeSet(GPIO_GROUP[gpioGroup_UART_TX].port, GPIO_GROUP[gpioGroup_UART_TX].pin, gpioModeWiredAndDrivePullUp, 1);
    break;

#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)
  case gpioGroup_PWR_Button:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
    break;
#endif // CONFIG_PROJECT_AMBA_ELEKTRA

  case gpioGroup_VOICE_WAKEUP:
    GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInput, 1);
    GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, true, true, enable);
    break;

  case gpioGroup_PIR_SENS_CONTROL:
      GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModePushPull, 0);
      GPIO_PinOutClear(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin);
      // GPIO_PinOutSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin);
      break;
  case gpioGroup_ACCELEROMETER_INTERRUPT:
      GPIO_PinModeSet(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, gpioModeInputPullFilter, 1);
      GPIO_IntConfig(GPIO_GROUP[gpio].port, GPIO_GROUP[gpio].pin, false, true, enable);
      break;

  default:
      LOG_DEBUG("GPIO: %d not found\n", gpio);
    break;
  }

  return;
}

void EnableGpioIRQ(uint8_t gpio)
{
  GPIO_IntClear(1 << GPIO_GROUP[gpio].pin);
  GPIO_IntEnable(1 << GPIO_GROUP[gpio].pin);
  return;
}

void DisableGpioIRQ(uint8_t gpio)
{
  GPIO_IntDisable(1 << GPIO_GROUP[gpio].pin);
  return;
}

void GpioIRQEven(bool enable)
{
  if (enable) {
    _enable_gpio_even_irq();
  } else {
    _disable_gpio_even_irq();
  }
}

void GpioIRQOdd(bool enable)
{
  if (enable) {
    _enable_gpio_odd_irq();
  } else {
    _disable_gpio_odd_irq();
  }
}

void ConfigS2lmBootMode(uint8_t mode)
{
  _config_s2l_bootup_mode(mode);
}

