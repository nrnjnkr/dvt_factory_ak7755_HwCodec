#include "utils.h"
#include "log.h"

static const uint32_t snBootGpio[3] = { 92, 91, 38 };
static const uint32_t snPowerOffGpio = 112;
static const int snNumBootGpios = 3;

eBOOT_BootReason sBOOT_FindWifiWakeupReason( void )
{
    // TODO: Find wifi wakeup reason
    return kBOOT_BootReasonWifiWakeupCloud;
}

eBOOT_BootReason gBOOT_CheckBootReason( void )
{
    int nRet;
    eBOOT_BootReason nBootReason;
    eBOOT_GpioBootVal nBootGpioValue;
    tGPIO_GpioVal GpioVal[3];
    int i;

    /* Read boot reason GPIOs set by MCU */

    nBootGpioValue = 0;

    for( i = 0; i < snNumBootGpios; i++ )
    {
        nRet = gGPIO_Initialize( snBootGpio[i], kGPIO_DirectionIn );
        if( nRet < 0 )
        {
            gLOG_Log( kLOG_Error, "%s : Initializing GPIO %d failed\n", __func__,
                    snBootGpio[i] );
            return kBOOT_BootReasonUnknown;
        }


        GpioVal[i] = gGPIO_Get( snBootGpio[i] );
        if( GpioVal[i] < 0 )
        {
            gLOG_Log( kLOG_Error, "%s : Initializing GPIO %d failed\n", __func__,
                    snBootGpio[i] );
            return kBOOT_BootReasonUnknown;
        }

        nBootGpioValue |= ( GpioVal[i] << i );
    }

    /* Find boot reason based on GPIO values */

    nBootReason = kBOOT_BootReasonUnknown;

    switch( nBootGpioValue )
    {

    case kBOOT_Config: 
   	nBootReason = kBOOT_BootReasonConfig;
	break;

    case kBOOT_PirTrigger:
        nBootReason = kBOOT_BootReasonPir;
        break;

    case kBOOT_AccelerometerTrigger:
        nBootReason = kBOOT_BootReasonAccelerometer;
        break;

    case kBOOT_LoudNoiseTrigger:
        nBootReason = kBOOT_BootReasonLoudNoise;
        break;

    case kBOOT_WiFiWakeup:
        nBootReason = sBOOT_FindWifiWakeupReason();
        break;

    case kBOOT_Button:
        nBootReason = kBOOT_BootReasonButton;
 	break; 

    default:
        nBootReason = kBOOT_BootReasonUnknown;
        break;
    }

    return nBootReason;
}

int gBOOT_PowerOff( void )
{
    int nRet = 0;

    nRet = gGPIO_Initialize( snPowerOffGpio, kGPIO_DirectionOut );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s : Initializing GPIO %d failed\n", __func__,
                snPowerOffGpio );
        return nRet;
    }

    nRet = gGPIO_Set( snPowerOffGpio, 1 );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s : Setting GPIO %d failed\n", __func__,
                snPowerOffGpio );
    }

    return nRet;
}
