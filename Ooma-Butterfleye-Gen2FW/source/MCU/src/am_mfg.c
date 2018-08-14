#include <stdio.h>
#include "em_gpio.h"
#include "em_i2c.h"

#include "config.h"
#include "am_base.h"
#include "am_uart.h"
#include "am_i2c.h"
#include "am_led.h"
#include "am_log.h"
#include "accelerometer.h"
#include "fuel_gauge.h"
#include "charger.h"
#include "potentiometer.h"
#include "pir.h"

/*
 * To enable MFG test should add -DENABLE_MFG_TEST to compiler
 */

void MFG_test(void)
{
    scan_i2c_bus();
    // LED  test
    // Set LED pwm form 0 to 0xff
    if (0 == led_test())
    {
        LOG_DEBUG("LED test OK\n");
    }
    else
    {
        LOG_DEBUG("LED test failed\n");
    }
    // G-sensor read chip ID should equal to 0xFA
    if (0 == accelerometer_probe())
    {
        LOG_DEBUG("Accelerometer BMA253 test OK\n");
    }
    else
    {
        LOG_DEBUG("Accelerometer BMA253 test failed\n");
    }
    // Get fuel gauge version id and voltague
    if (0 == fuel_gauge_test())
    {
        LOG_DEBUG("Fuel gauge test OK\n");
    }
    else
    {
        LOG_DEBUG("Fuel gauge test failed\n");
    }

    // Get charger status
    if (0 == charger_test())
    {
        LOG_DEBUG("Charger test OK\n");
    }
    else
    {
        LOG_DEBUG("Charger test failed\n");
    }

    // Voice detect adjustment
    if (0 == potentiometer_test())
    {
        LOG_DEBUG("Potentiometer test OK\n");
    }
    else
    {
        LOG_DEBUG("Potentiometer test failed\n");
    }
    // Clear all status;
    //_pirTrigger = triggerStatus_Clear;
    //_VoiceDetectTrigger = triggerStatus_Clear;
    //_ButtonTrigger = triggerStatus_Clear;


    // 2.7V
    potentiometer_set(82);
}

void init_MFG_test(void)
{
#if defined(TEKNIQUE_PCB_SMT_DEBUG)
#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
  //----------------------------------------------------------
  led_init();
  potentiometer_init();
  fuel_gauge_init();
  charger_init();
  // AS084 test
  pir_init();
#endif
#endif
}

void repeat_MFG_tests(int button_trigger)
{
    // only by pressing the button to repeat the mfg test
    if (button_trigger == triggerStatus_Set)
    {
        LOG_DEBUG("-------------------------------------------------------\n");
        MFG_test();
        LOG_DEBUG("-------------------------------------------------------\n");
    }
}
