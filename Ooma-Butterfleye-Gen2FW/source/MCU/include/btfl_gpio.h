// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef BTFL_GPIO_H_
#define BTFL_GPIO_H_

typedef struct
{
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} GpioUnit;

typedef enum {

  gpioGroup_I2C_SDA = 0,    //PA0<-->SDA
  gpioGroup_I2C_SCL,        //PA1<-->SCL

  gpioGroup_BT_RST,            //PA2

  gpioGroup_BootMode_1_PIN,       //PB7

  /* In DVT Bootmode2 pin to  BT_HOST_WAKEUP
   * gpioGroup_BootMode_2_PIN, //PB8
   */
  gpioGroup_BT_HOST_WAKEUP, //PB8
  gpioGroup_WIFI_EN,             //PB11

  /* In DVT PB13 changed to PIR sensitivity set
   * gpioGroup_ACCELEROMETER_INTERRUPT, //PB13
   */
  gpioGroup_PIR_SENS_CONTROL,      //PB13

  /* In DVT BT host exchanged with BootMode 2 pin (GPIO38)
   * gpioGroup_BT_HOST_WAKEUP, //PB14
   */

  gpioGroup_BootMode_2_PIN,      //PB14

  /* In DVT PC0 changed to G-Sensor wake up
   * gpioGroup_PIR_SENS_CONTROL, //PC0
   */

  gpioGroup_ACCELEROMETER_INTERRUPT, //PC0

  gpioGroup_BootMode_0_PIN, //PC1

  gpioGroup_SWT_DET,        //PC13, irq  // KEY2
  gpioGroup_VOICE_WAKEUP,     //PC14, irq
  gpioGroup_RGB_EN,  //PC15

  gpioGroup_UART_TX,        //PD4
  gpioGroup_UART_RX,        //PD5

  // gpioGroup_ADC,            //PD6
  gpioGroup_PIR_Wakeup,     //PD6

  gpioGroup_ReadyOff,       //PD7, irq, from S2l

  gpioGroup_WIFI_Mode,      //PE10   exception wake
  gpioGroup_WIFI_Wakeup,    //PE11, irq  cloud wake

  gpioGroup_BatterAlert,    //PE12

  gpioGroup_PowerCtrl,      //PE13, to S2l

  gpioGroup_VOICE_DETECT_PRESET,    //PF0
  gpioGroup_SW_DIO_32K_CLK,         //PF1

  gpioGroup_PWR_EN,         //PF2 wifi power

  gpioGroup_Total           = gpioGroup_PWR_EN + 1,

  gpioGroup_LED,
  gpioGroup_I2C,
  gpioGroup_BootMode,
  gpioGroup_UART,
} Gpio_Group_TypeDef;

static const GpioUnit GPIO_GROUP[gpioGroup_Total] = {
    {gpioPortA,  0}, //gpioGroup_I2C->SDA
    {gpioPortA,  1}, //gpioGroup_I2C->SCL

    // {gpioPortA,  2}, //gpioGroup_LED_Green
    {gpioPortA,  2}, //gpioGroup_BT_RST

    {gpioPortB,  7}, //gpioGroup_BootMode_1_PIN
    {gpioPortB,  8}, //gpioGroup_BootMode_2_PIN

    {gpioPortB, 11}, //gpioGroup_WIFI_EN

    //
    {gpioPortB, 13}, //gpioGroup_ACCELEROMETER_INTERRUPT

    {gpioPortB, 14}, //gpioGroup_BT_HOST_WAKEUP

    {gpioPortC,  0}, //gpioGroup_PIR_SENS_CONTROL

    {gpioPortC,  1}, //gpioGroup_BootMode_0_PIN

    //
    {gpioPortC, 13}, //gpioGroup_SWT_DET
    {gpioPortC, 14}, //gpioGroup_VOICE_WAKEUP
    {gpioPortC, 15}, //gpioGroup_RGB_EN

    {gpioPortD,  4}, //gpioGroup_UART_TX
    {gpioPortD,  5}, //gpioGroup_UART_RX

    // Changed
    {gpioPortD, 6}, //gpioGroup_PIR_Wakeup

    {gpioPortD,  7}, //gpioGroup_ReadyOff

    {gpioPortE, 10}, //gpioGroup_WIFI_Mode
    {gpioPortE, 11}, //gpioGroup_WIFI_Wakeup

    {gpioPortE, 12}, //gpioGroup_BatterAlert

    {gpioPortE, 13}, //gpioGroup_PowerCtrl

    {gpioPortF,  0},  //gpioGroup_VOICE_DETECT_PRESET
    {gpioPortF,  1},  //gpioGroup_SW_DIO_32K_CLK,

    {gpioPortF,  2}  //gpioGroup_PWR_EN
};

//gpio basic function
void GpioSetup(uint8_t gpio, bool enable);
void EnableGpioIRQ(uint8_t gpio);
void DisableGpioIRQ(uint8_t gpio);
void GpioIRQEven(bool enable);
void GpioIRQOdd(bool enable);
void ConfigS2lmBootMode(uint8_t mode);

#endif /* AM_GPIO_H_ */
