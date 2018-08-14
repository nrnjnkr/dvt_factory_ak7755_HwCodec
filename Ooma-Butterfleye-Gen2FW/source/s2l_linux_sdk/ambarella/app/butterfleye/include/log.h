#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>
#include "butterfleye.h"

typedef enum
{
    kLOG_Debug = 0x0,
    kLOG_Info = 0x1,
    kLOG_Warn = 0x2,
    kLOG_Error = 0x3,
    kLOG_Fatal = 0x4
} eLOG_LogLevel;

void gLOG_Log( eLOG_LogLevel anLevel, char *apLogString, ... );

#endif
