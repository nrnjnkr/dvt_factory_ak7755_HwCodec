#include<stdio.h>
#include "gpio.h"


/* 
 * This tool used to know the boot trigger source and 
 * send ready to power off notfication to mcu via GPIO 112
 *
 */


/*
 * As of now mcu sets gpio for below reasons,
 *
 *  dsp |  Mode
 *  2  | 1   | 0
 * PB8 | PB7 | PC1
 *  0  | 0   | 0 ===> dsp off, wlan config mode
 *  0  | 0   | 1 ===> dsp 0ff, wlan reconnect mode
 *  0  | 1   | 0 ===> dsp off, msg notification
 *  1  | 0   | 0 ===> dsp on, recording mode
 *  1  | 0   | 1 ===> dsp on, streaming mode
 */

/*
 * Note : 
 *TO-DO : Change below order according to mcu bootmode setup 
 */
enum {

	WLAN_CONFIG,
	PIR,
	STREAM,
	G_SENSOR,
	VOICE_DETECT,
	BUTTON	
};

static const uint32_t snBootGpio[3] = { 92, 91, 38 };                           
static const uint32_t snPowerOffGpio = 112;                                     
static const int snNumBootGpios = 3;  

int gBOOT_CheckBootReason( void )                                  
{                                                                               
    int nRet;                                                                   
    int GpioVal[3];                                                   
    int i;                                                                      
                                                                                
    /* Read boot reason GPIOs set by MCU */                                     
                                                                                
    int nBootGpioValue = 0;                                                         
                                                                                
    for( i = 0; i < snNumBootGpios; i++ )                                       
    {                                                                           
        //nRet = gGPIO_Initialize( snBootGpio[i], kGPIO_DirectionIn );            
        nRet = gGPIO_Initialize( snBootGpio[i], 1 );            
        if( nRet < 0 )                                                          
        {                                                                       
            fprintf(stderr, "%s : Initializing GPIO %d failed\n", __func__,
                    snBootGpio[i] );                                            
            return -1;                                     
        }                                                                       
                                                                                
                                                                                
        GpioVal[i] = gGPIO_Get( snBootGpio[i] );                                
        if( GpioVal[i] < 0 )                                                    
        {                                                                       
            fprintf(stderr, "%s : Initializing GPIO %d failed\n", __func__,
                    snBootGpio[i] );                                            
            return -1;                                     
        }                                                                       
                                                                                
        nBootGpioValue |= ( GpioVal[i] << i );                                  
    }                              
}  

int gBOOT_PowerOff( void )                                                      
{                                                                               
    int nRet = 0;                                                               
                                                                                
    //nRet = gGPIO_Initialize( snPowerOffGpio, kGPIO_DirectionOut );              
    nRet = gGPIO_Initialize( snPowerOffGpio, 0x01 );              
    if( nRet < 0 )                                                              
    {                                                                           
        fprintf( stderr, "%s : Initializing GPIO %d failed\n", __func__,   
                snPowerOffGpio );                                               
        return nRet;                                                            
    }                                                                           
                                                                                
    nRet = gGPIO_Set( snPowerOffGpio, 1 );                                      
    if( nRet < 0 )                                                              
    {                                                                           
        fprintf( stderr, "%s : Setting GPIO %d failed\n", __func__,        
                snPowerOffGpio );                                               
    }                                                                           
                                                                                
    return nRet;                                                                
}


int main() {

	int reason = 0;

	reason = gBOOT_CheckBootReason();
	printf("Boot code is %d\n", reason);
	switch(reason) {
	
		case WLAN_CONFIG: 
			printf("Boot trigger is WLAN_CONFIG\n");
			break;
		case PIR:
			printf("Boot trigger is PIR\n");
			break;
		case STREAM:
			printf("Boot trigger is STREAM/WIFI\n");
			break;
		case VOICE_DETECT:
			printf("Boot trigger is voice detect\n");
			break;
		case BUTTON:
			printf("Boot trigger is Button\n");
			break;
		case G_SENSOR:
			printf("Boot trigger is accelerometer\n");
			break;
	}

      printf("Informing mcu to power off s2lm\n");
      gBOOT_PowerOff();
	return 0;
}               
