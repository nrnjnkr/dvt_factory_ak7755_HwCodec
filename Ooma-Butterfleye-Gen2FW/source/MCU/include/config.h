// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef PROJECT_CONFIG_H_
#define PROJECT_CONFIG_H_


// It must be one of these
// #define CONFIG_PROJECT_AMBA_ELEKTRA
#define CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA


#define TEKNIQUE_PCB_SMT_DEBUG

#define TEKNIQUE_PCB_SMT_DEBUG_LEAVE_POWER_TO_S2L


#if defined(CONFIG_PROJECT_AMBA_ELEKTRA)
    #include "am_gpio.h"
#elif defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
    #include "btfl_gpio.h"
#else
    #error no PROJ_xx is defined
#endif

#endif
