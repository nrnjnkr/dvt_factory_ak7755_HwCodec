#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"
#include "utils.h"

#define GPIO_MAX_FILENAME_LENGTH 128
#define GPIO_MAX_BUFFER_LENGTH 10

const char gsGPIO_GpioExportPath[] = "/sys/class/gpio/export";
const char gsGPIO_GpioUnexportPath[] = "/sys/class/gpio/unexport";

int gGPIO_SetDirection( uint32_t anGpioNum, eGPIO_Direction anDirection )
{
    int nFd;
    int nRet;
    char sFileName[GPIO_MAX_FILENAME_LENGTH];
    char sBuf[GPIO_MAX_BUFFER_LENGTH];

    /* set gpio direction */
    snprintf( sFileName, GPIO_MAX_FILENAME_LENGTH,
            "/sys/class/gpio/gpio%d/direction", anGpioNum );

    nFd = open( sFileName, O_WRONLY | O_SYNC );
    if( nFd < 0 )
    {
        gLOG_Log( kLOG_Error, "open of gpio direction file failed\n" );
        return nFd;
    }

    snprintf( sBuf, GPIO_MAX_BUFFER_LENGTH,
            ( anDirection == kGPIO_DirectionIn ) ? "in" : "out" );
    nRet = write( nFd, sBuf, strlen( sBuf ) );

    if( nRet < strlen( sBuf ) )
    {
        gLOG_Log( kLOG_Error, "setting direction for gpio %d failed\n",
                anGpioNum );
        return ( nRet < 0 ) ? nRet : -1;
    }

    close( nFd );

    return 0;
}

int gGPIO_Export( uint32_t anGpioNum, bool export )
{
    int nFd;
    int nRet;
    char sFileName[GPIO_MAX_FILENAME_LENGTH];
    char sBuf[GPIO_MAX_BUFFER_LENGTH];

    /* export gpio */
    snprintf( sFileName, GPIO_MAX_FILENAME_LENGTH,
            export ? gsGPIO_GpioExportPath : gsGPIO_GpioUnexportPath );

    nFd = open( sFileName, O_WRONLY | O_SYNC );
    if( nFd < 0 )
    {
        gLOG_Log( kLOG_Error, "open of %s failed\n", sFileName );
        return nFd;
    }

    snprintf( sBuf, GPIO_MAX_BUFFER_LENGTH, "%d", anGpioNum );
    nRet = write( nFd, sBuf, strlen( sBuf ) );
    if( nRet < strlen( sBuf ) )
    {
        gLOG_Log( kLOG_Error, "export for gpio %d failed\n", anGpioNum );
        return nRet;
    }

    close( nFd );

    return 0;
}

int gGPIO_Initialize( uint32_t anGpioNum, eGPIO_Direction anDirection )
{
    int nRet;

    nRet = gGPIO_Export( anGpioNum, true );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "gGPIO_export failed for gpio %d\n", anGpioNum );
        return nRet;
    }

    nRet = gGPIO_SetDirection( anGpioNum, anDirection );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "gGPIO_SetDirection failed for gpio %d\n",
                anGpioNum );
        return nRet;
    }

    return 0;
}

int gGPIO_DeInitialize( uint32_t anGpioNum )
{
    int nRet;

    nRet = gGPIO_Export( anGpioNum, false );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "gGPIO_export failed for gpio %d\n", anGpioNum );
        return nRet;
    }

    return 0;
}


tGPIO_GpioVal gGPIO_Get( uint32_t anGpioNum )
{
    int nFd;
    int nRet;
    char sFileName[GPIO_MAX_FILENAME_LENGTH];
    char sBuf[GPIO_MAX_BUFFER_LENGTH];

    snprintf( sFileName, GPIO_MAX_FILENAME_LENGTH,
            "/sys/class/gpio/gpio%d/value", anGpioNum );

    nFd = open( sFileName, O_RDONLY | O_SYNC );
    if( nFd < 0 )
    {
        gLOG_Log( kLOG_Error, "open of gpio value file failed\n" );
        return nFd;
    }

    nRet = read( nFd, sBuf, 1 );

    if( nRet < 1 )
    {
        gLOG_Log( kLOG_Error, "read of gpio %d failed\n", anGpioNum );
        return ( nRet < 0 ) ? nRet : -1;
    }

    close( nFd );

    return ( ( sBuf[0] == '0' ) ? 0 : 1 );
}

int gGPIO_Set( uint32_t anGpioNum, tGPIO_GpioVal aVal )
{
    int nFd;
    int nRet;
    char sFileName[GPIO_MAX_FILENAME_LENGTH];
    char cValue;

    cValue = ( aVal == 0 ) ? '0' : '1';

    snprintf( sFileName, GPIO_MAX_FILENAME_LENGTH,
            "/sys/class/gpio/gpio%d/value", anGpioNum );

    nFd = open( sFileName, O_WRONLY | O_SYNC );
    if( nFd < 0 )
    {
        gLOG_Log( kLOG_Error, "open of gpio value file failed\n" );
        return nFd;
    }

    nRet = write( nFd, &cValue, 1 );

    if( nRet < 1 )
    {
        gLOG_Log( kLOG_Error, "read of gpio %d failed\n", anGpioNum );
        return ( nRet < 0 ) ? nRet : -1;
    }

    close( nFd );

    return 0;
}
