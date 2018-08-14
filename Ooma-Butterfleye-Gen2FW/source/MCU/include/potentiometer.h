// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

extern int potentiometer_init(void);
extern int potentiometer_test(void);
extern uint8_t potentiometer_get(void);
extern uint8_t potentiometer_set(uint8_t percentage);

// Get current resistance
// extern uint8_t get_potentiometer_resis(void);

#endif
