#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/msg.h>

/* Warning: If you change any enums in this file, make corresponding change in
 * utils/enum_print.c */
extern const char * gGPIO_Direction[];
extern const char * gBOOT_GpioBootVal[];
extern const char * gBOOT_BootReason[];
extern const char * gAPP_ThreadId[];
extern const char * gMSGQ_Message[];

#define ENUM_TO_STRING_GPIO_DIR( x ) ( gGPIO_Direction[x] )
#define ENUM_TO_STRING_GPIO_BOOTVAL( x ) ( gBOOT_GpioBootVal[x] )
#define ENUM_TO_STRING_BOOTREASON( x ) ( gBOOT_BootReason[x] )
#define ENUM_TO_STRING_APP_THREADID( x ) ( gAPP_ThreadId[x] )
#define ENUM_TO_STRING_MSGQ_MESSAGE( x ) ( gMSGQ_Message[x] )

typedef uint8_t tGPIO_GpioVal;

typedef enum
{
    kGPIO_DirectionIn = 0x0,
    kGPIO_DirectionOut = 0x1
} eGPIO_Direction;


int gGPIO_SetDirection( uint32_t anGpioNum, eGPIO_Direction anDirection );
int gGPIO_Export( uint32_t anGpioNum, bool export );
int gGPIO_Initialize( uint32_t anGpioNum, eGPIO_Direction anDirection );
int gGPIO_DeInitialize( uint32_t anGpioNum );
tGPIO_GpioVal gGPIO_Get( uint32_t anGpioNum );
int gGPIO_Set( uint32_t anGpioNum, tGPIO_GpioVal aVal );

/*
 boot mode settings in mcu 
 s2lmbootupMode_Config = 0x00;                                                  
 s2lmbootupMode_PIR = 0x01;                                                     
 s2lmbootupMode_ACC = 0x02;                                                     
 s2lmbootupMode_VOICE = 0x03                                                    
 s2lmbootupMode_WIFI = 0x04                                                     
 s2lmbootupMode_BUTTON = 0x05;                                                  
 s2lmbootupMode_Init = 0xFF; 
 */
typedef enum
{
    kBOOT_Config = 0x00,
    kBOOT_PirTrigger = 0x01,
    kBOOT_AccelerometerTrigger = 0x02,
    kBOOT_LoudNoiseTrigger = 0x03,
    kBOOT_WiFiWakeup = 0x04,
    kBOOT_Button = 0x05,
    kBOOT_Unknown = 0xFF,
} eBOOT_GpioBootVal;

typedef enum
{
    kBOOT_BootReasonConfig, 
    kBOOT_BootReasonPir,
    kBOOT_BootReasonAccelerometer,
    kBOOT_BootReasonLoudNoise,
    kBOOT_BootReasonWifiWakeupCloud,
    kBOOT_BootReasonButton,
    kBOOT_BootReasonUnknown,
} eBOOT_BootReason;

eBOOT_BootReason gBOOT_CheckBootReason( void );
int gBOOT_PowerOff( void );

typedef enum
{
    kAPP_NotStarted,
    kAPP_Running,
    kAPP_Stopped,
} eAPP_ThreadState;

typedef enum
{
    kWIFI_StateUnknown,
    kWIFI_StateConnecting,
    kWIFI_StateConnected,
    kWIFI_StateDisconnected,
} eWIFI_WifiState;

typedef enum
{
    kCLOUD_StateUnknown,
    kCLOUD_StateConnecting,
    kCLOUD_StateConnected,
    kCLOUD_StateDisconnected,
} eCLOUD_CloudState;

typedef enum
{
    kAPP_InvalidThreadId = 0,
    kAPP_MainAppThread,
    kAPP_UartRxThread,
    kAPP_UartTxThread,
    kAPP_EventManagerThread,
    kAPP_FileUploadThread,
    kAPP_RecordingStreamingThread,
    kAPP_CloudThread,
    kAPP_WifiThread,
    kAPP_BluetoothThread,
    kAPP_KeepAliveToMcuThread,
    kAPP_McuLoggerThread,
} eAPP_ThreadId;

#define MAX_PAYLOAD_SZ 10

typedef enum
{
    kMSGQ_CmdNoneStatusNone = 0,
    kMSGQ_StatusBootReason,

    kMSGQ_StatusThreadStarted,
    kMSGQ_StatusThreadExited,
    kMSGQ_CmdExitThread,

    kMSGQ_CmdSendMsgToMcu,
    kMSGQ_StatusMsgReceivedFromMcu,

    kMSGQ_CmdStartFileUpload,
    kMSGQ_CmdStopFileUpload,
    kMSGQ_StatusFileUploadStarted,
    kMSGQ_StatusFileUploadStopped,

    kMSGQ_CmdStartRecording,
    kMSGQ_CmdRecordingDone,
    kMSGQ_CmdStopRecording,
    kMSGQ_CmdStartStreaming,
    kMSGQ_CmdStopStreaming,
    kMSGQ_StatusRecordingStarted,
    kMSGQ_StatusRecordingStopped,
    kMSGQ_StatusStreamingStarted,
    kMSGQ_StatusStreamingStopped,

    kMSGQ_CmdCloudCheckConnection,
    kMSGQ_StatusCloudConnectionAlive,
    kMSGQ_StatusCloudConnectionLost,
    kMSGQ_CmdCloudCheckWakeupReason,
    kMSGQ_StatusCloudWakeupStartStream,
    kMSGQ_CmdGetBatteryStatus,
    kMSGQ_StatusCloudBatteryStatus,

    kMSGQ_CmdWifiSetupConnection,
    kMSGQ_CmdWifiCheckConnection,
    kMSGQ_StatusWifiConnected,
    kMSGQ_StatusWifiDisconnected,

} eMSGQ_Message;

typedef struct
{
    long nReceiver;
    eAPP_ThreadId nSender;
    eMSGQ_Message nMessage;
    size_t nPayloadSize;
    char sPayload[MAX_PAYLOAD_SZ];
} tMessage;

int gMSGQ_Initialize( void );
int gMSGQ_Deinitialize( void );
int gMSGQ_Send( tMessage *apMessage );
int gMSGQ_Receive( tMessage *apMessage );

#define MESSAGE_API( sModule, nThreadId ) \
static int s ## sModule ## _SendMessage( eAPP_ThreadId anReceiver,\
        eMSGQ_Message anMessage,\
        char *asPayload, size_t anSize) \
{ \
    int nRet; \
    tMessage Message; \
    \
    Message.nReceiver = anReceiver; \
    Message.nSender = nThreadId; \
    Message.nMessage = anMessage; \
    Message.nPayloadSize \
        = ( anSize > MAX_PAYLOAD_SZ ) ? MAX_PAYLOAD_SZ : anSize; \
    if( anSize > MAX_PAYLOAD_SZ ) \
    { \
        gLOG_Log( kLOG_Warn, "Payload size %d is larger than %d -- truncating\n", \
            anSize, MAX_PAYLOAD_SZ ); \
    } \
    if( asPayload && Message.nPayloadSize ) \
    { \
        memcpy( Message.sPayload, asPayload, Message.nPayloadSize ); \
    }\
    \
    nRet = gMSGQ_Send( &Message ); \
    \
    return nRet; \
} \
\
static int s ## sModule ## _SendMessageToEventManager( eMSGQ_Message anMessage,\
        char *asPayload, size_t anSize ) \
{ \
    return s ## sModule ## _SendMessage( kAPP_EventManagerThread, anMessage, \
            asPayload, anSize ); \
} \
\
static int s ## sModule ## _ReceiveMessage( eAPP_ThreadId *apnSender, \
        eMSGQ_Message * apnMessage,\
        char *asPayload, size_t * apnSize) \
{ \
    int nRet; \
    tMessage Message; \
    \
    Message.nReceiver = nThreadId; \
    nRet = gMSGQ_Receive( &Message ); \
    if( nRet < 0 ) \
    { \
        return nRet; \
    } \
    \
    *apnSender = Message.nSender; \
    *apnMessage = Message.nMessage; \
    *apnSize = Message.nPayloadSize; \
    if( asPayload && Message.nPayloadSize ) \
    { \
        memcpy( asPayload, Message.sPayload, Message.nPayloadSize ); \
    }\
    \
    return nRet; \
}

#endif
