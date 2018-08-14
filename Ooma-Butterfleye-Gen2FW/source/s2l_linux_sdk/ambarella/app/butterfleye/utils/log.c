#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "log.h"

extern eLOG_LogLevel gnLogLevel;

static pthread_mutex_t sLogMutex = PTHREAD_MUTEX_INITIALIZER;

void gLOG_Log( eLOG_LogLevel anLevel, char *apLogString, ... )
{
    va_list args;

    pthread_mutex_lock( &sLogMutex );

    if( anLevel >= gnLogLevel )
    {
        va_start( args, apLogString );

        switch( anLevel )
        {
        case kLOG_Debug:
            fprintf( stdout, "[DEBUG]\t" );
            vfprintf( stdout, apLogString, args );
            break;

        case kLOG_Info:
            fprintf( stdout, "[INFO]\t" );
            vfprintf( stdout, apLogString, args );
            break;

        case kLOG_Warn:
            fprintf( stderr, "[WARN]\t" );
            vfprintf( stderr, apLogString, args );
            break;

        case kLOG_Error:
            fprintf( stderr, "[ERROR]\t%s --> ", strerror( errno ) );
            vfprintf( stderr, apLogString, args );
            break;

        case kLOG_Fatal:
            fprintf( stderr, "[FATAL]\t" );
            vfprintf( stderr, apLogString, args );
            fprintf( stderr, "[FATAL]\tFatal error -- exiting application !\n\n" );
            exit( 1 );
        }

        va_end( args );
    }

    pthread_mutex_unlock( &sLogMutex );

    fflush( stdout );
    fflush( stderr );
}
