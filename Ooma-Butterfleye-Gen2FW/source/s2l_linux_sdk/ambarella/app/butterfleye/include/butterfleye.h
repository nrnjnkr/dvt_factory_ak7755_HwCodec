#ifndef _BUTTERFLEYE_H_
#define _BUTTERFLEYE_H_

#include "utils.h"

typedef struct
{
    eBOOT_GpioBootVal gpioValueAtBoot; /* The value of GPIOs set by MCU at boot */
    eBOOT_BootReason bootReason; /* S2LM boot reason */

    /* The below values to be read from configuration file */
} tAPP_ApplicationState;

void * gCLOUD_MainLoop( void *arg );
void * gRECORDSTREAM_MainLoop( void *arg );
void * gFILEUPLOAD_MainLoop( void *arg );
void * gUARTTX_MainLoop( void *arg );
void * gBT_MainLoop( void *arg );
void * gMCULOGGER_MainLoop( void *arg );
void * gWIFI_MainLoop( void *arg );
void * gKEEPALIVE_MainLoop( void *arg );
void * gUARTRX_MainLoop( void *arg );
void * gEVENTMANAGER_MainLoop( void *arg );

#endif
