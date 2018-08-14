// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef FUEL_GAUGE_H_
#define FUEL_GAUGE_H_

#include <stdint.h>
#include <stddef.h>

extern int fuel_gauge_init(void);
extern int fuel_gauge_test(void);

int fuelgauge_probe();
const char* fuelgauge_get_name();

// voltage in mV
uint32_t fuelgauge_get_voltage_mV();

// capacity in mAh
uint32_t fuelgauge_get_full_capacity_mAh();
uint32_t fuelgauge_get_remaining_capacity_mAh();

//
uint32_t fuelgauge_get_state_of_charge_percent();
uint32_t fuelgauge_get_state_of_health_percent();

// temperature in C degree
float fuelgauge_get_temperature_c();

// current in mA
int32_t fuelgauge_get_average_current_mA();

#endif
