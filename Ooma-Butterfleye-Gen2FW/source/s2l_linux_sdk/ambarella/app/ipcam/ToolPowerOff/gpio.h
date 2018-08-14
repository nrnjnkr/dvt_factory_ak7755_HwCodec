#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int gGPIO_SetDirection( uint32_t anGpioNum, int anDirection );

int gGPIO_Export( uint32_t anGpioNum, bool export );

int gGPIO_Initialize( uint32_t anGpioNum, int anDirection );

int gGPIO_DeInitialize( uint32_t anGpioNum );

int  gGPIO_Get( uint32_t anGpioNum );

int gGPIO_Set( uint32_t anGpioNum, int aVal );
