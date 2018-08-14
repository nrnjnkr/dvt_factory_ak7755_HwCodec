// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////
#include "em_gpio.h"

#include "config.h"
#include "am_common.h"
#include "am_log.h"

#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// DEFINES ///////////////////////////////////////////////////////////////////

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////
static int g_sensitivity = 0;

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////

int pir_init(void)
{
    LOG_DEBUG("%s \n", __FUNCTION__);
    return 0;
}

int pir_probe(void)
{
    LOG_DEBUG("%s \n", __FUNCTION__);
    return 0;
}



// 0~100
uint8_t pir_sensitivity_get()
{
    return g_sensitivity;
}

int pir_sensitivity_set(uint8_t sensitivity)
{
    int delay_ms = 1000*sensitivity/100;
    g_sensitivity = sensitivity;
    // Clear SET pulse
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].port, GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].pin);
    Delay(500);

    // 300 ms pulse
    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].port, GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].pin);
    Delay(300);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].port, GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].pin);

    Delay(delay_ms);

    GPIO_PinOutSet(GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].port, GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].pin);
    Delay(300);
    GPIO_PinOutClear(GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].port, GPIO_GROUP[gpioGroup_PIR_SENS_CONTROL].pin);

    // State machine will turn on gpioGroup_PIR_Wakeup.
    LOG_DEBUG("%s set AS084 %d%% sensitive, pulse width: %d ms\n", __FUNCTION__, sensitivity, delay_ms);
    return 0;
}

#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////
