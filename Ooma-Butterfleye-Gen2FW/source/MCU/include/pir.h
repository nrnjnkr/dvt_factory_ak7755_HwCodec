// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef PIR_H_
#define PIR_H_

extern int pir_init(void);
extern int pir_probe(void);

extern uint8_t pir_sensitivity_get(void);

extern int pir_sensitivity_set(uint8_t sensitivity);


#endif
