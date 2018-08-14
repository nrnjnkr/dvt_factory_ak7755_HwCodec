#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "gpio.h"


                                                                                
#define GPIO_MAX_FILENAME_LENGTH 128                                            
#define GPIO_MAX_BUFFER_LENGTH 10                                               
                                                                                
const char gsGPIO_GpioExportPath[] = "/sys/class/gpio/export";                  
const char gsGPIO_GpioUnexportPath[] = "/sys/class/gpio/unexport";              
                                                                                
static const uint32_t snBootGpio[3] = { 92, 91, 38 };                           
static const uint32_t snPowerOffGpio = 112;                                     
static const int snNumBootGpios = 3; 

int gGPIO_SetDirection( uint32_t anGpioNum, int anDirection )
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
        fprintf( stderr, "open of gpio direction file failed\n" );
        return nFd;
    }

   /*
    * GPIO Direction values 
    * In  = 0x00
    * Out = 0x01
    */
    snprintf( sBuf, GPIO_MAX_BUFFER_LENGTH,
            ( anDirection == 0x00 ) ? "in" : "out" );
    nRet = write( nFd, sBuf, strlen( sBuf ) );

    if( nRet < strlen( sBuf ) )
    {
        fprintf( stderr, "setting direction for gpio %d failed\n",
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
        fprintf( stderr, "open of %s failed\n", sFileName );
        return nFd;
    }

    snprintf( sBuf, GPIO_MAX_BUFFER_LENGTH, "%d", anGpioNum );
    nRet = write( nFd, sBuf, strlen( sBuf ) );
    if( nRet < strlen( sBuf ) )
    {
        fprintf( stderr, "export for gpio %d failed\n", anGpioNum );
        return nRet;
    }

    close( nFd );

    return 0;
}

int gGPIO_Initialize( uint32_t anGpioNum, int anDirection )
{
    int nRet;

    nRet = gGPIO_Export( anGpioNum, true );
    if( nRet < 0 )
    {
        fprintf( stderr, "gGPIO_export failed for gpio %d\n", anGpioNum );
        return nRet;
    }

    nRet = gGPIO_SetDirection( anGpioNum, anDirection );
    if( nRet < 0 )
    {
        fprintf( stderr, "gGPIO_SetDirection failed for gpio %d\n",
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
        fprintf( stderr, "gGPIO_export failed for gpio %d\n", anGpioNum );
        return nRet;
    }

    return 0;
}


int gGPIO_Get( uint32_t anGpioNum )
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
        fprintf( stderr, "open of gpio value file failed\n" );
        return nFd;
    }

    nRet = read( nFd, sBuf, 1 );

    if( nRet < 1 )
    {
        fprintf( stderr, "read of gpio %d failed\n", anGpioNum );
        return ( nRet < 0 ) ? nRet : -1;
    }

    close( nFd );

    return ( ( sBuf[0] == '0' ) ? 0 : 1 );
}

int gGPIO_Set( uint32_t anGpioNum, int aVal )
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
        fprintf( stderr, "open of gpio value file failed\n" );
        return nFd;
    }

    nRet = write( nFd, &cValue, 1 );

    if( nRet < 1 )
    {
        fprintf( stderr, "read of gpio %d failed\n", anGpioNum );
        return ( nRet < 0 ) ? nRet : -1;
    }

    close( nFd );

    return 0;
}
