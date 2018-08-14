#include <stdbool.h>
#include <pthread.h>
#include "butterfleye.h"
#include "log.h"
#include "utils.h"

static bool sbExitThread = false;

static struct {
eAPP_ThreadState nMainAppThread;
eAPP_ThreadState nUartRxThread;
eAPP_ThreadState nUartTxThread;
eAPP_ThreadState nEventManagerThread;
eAPP_ThreadState nFileUploadThread;
eAPP_ThreadState nRecordingStreamingThread;
eAPP_ThreadState nCloudThread;
eAPP_ThreadState nWifiThread;
eAPP_ThreadState nBluetoothThread;
eAPP_ThreadState nKeepAliveToMcuThread;
eAPP_ThreadState nMcuLoggerThread;
} sThreadState;

static struct {
bool nFileUploadInProgress;
bool nRecordingInProgress;
bool nStreamingInProgress;
eWIFI_WifiState nWifiState;
eCLOUD_CloudState nCloudState;
} sEventManagerState;

int thread_exited = 0;

//MESSAGE_API( EVENTMANAGER, kAPP_EventManagerThread );


static int sEVENTMANAGER_SendMessage( eAPP_ThreadId anReceiver,
eMSGQ_Message anMessage,
char *asPayload, size_t anSize) 
{ 
int nRet; 
tMessage Message; 

Message.nReceiver = anReceiver; 
Message.nSender = kAPP_EventManagerThread; 
Message.nMessage = anMessage; 
Message.nPayloadSize 
= ( anSize > MAX_PAYLOAD_SZ ) ? MAX_PAYLOAD_SZ : anSize; 
if( anSize > MAX_PAYLOAD_SZ ) 
{ 
gLOG_Log( kLOG_Warn, "Payload size %d is larger than %d -- truncating\n", 
anSize, MAX_PAYLOAD_SZ ); 
} 
if( asPayload && Message.nPayloadSize ) 
{ 
memcpy( Message.sPayload, asPayload, Message.nPayloadSize ); 
}

gLOG_Log(kLOG_Info, "Sending msg from eventmanager\n");
nRet = gMSGQ_Send( &Message ); 

gLOG_Log(kLOG_Info, "Sending msg from event done\n");
return nRet; 
} 


static int sEVENTMANAGER_ReceiveMessage( eAPP_ThreadId *apnSender, 
eMSGQ_Message * apnMessage,
char *asPayload, size_t * apnSize) 
{ 
int nRet; 
tMessage Message; 

Message.nReceiver = kAPP_EventManagerThread; 
nRet = gMSGQ_Receive( &Message ); 
if( nRet < 0 ) 
{ 
return nRet; 
} 

*apnSender = Message.nSender; 
*apnMessage = Message.nMessage; 
*apnSize = Message.nPayloadSize; 
if( asPayload && Message.nPayloadSize ) 
{ 
memcpy( asPayload, Message.sPayload, Message.nPayloadSize ); 
}

return nRet;  
}

static int sEVENTMANAGER_Init( void )
{
gLOG_Log( kLOG_Info, "EventManager : Initializing thread state"
" --> setting all threads to kAPP_NotStarted \n" );

sThreadState.nMainAppThread = kAPP_NotStarted;
sThreadState.nUartRxThread = kAPP_NotStarted;
sThreadState.nUartTxThread = kAPP_NotStarted;
sThreadState.nEventManagerThread = kAPP_NotStarted;
sThreadState.nFileUploadThread = kAPP_NotStarted;
sThreadState.nRecordingStreamingThread = kAPP_NotStarted;
sThreadState.nCloudThread = kAPP_NotStarted;
sThreadState.nWifiThread = kAPP_NotStarted;
sThreadState.nBluetoothThread = kAPP_NotStarted;
sThreadState.nKeepAliveToMcuThread = kAPP_NotStarted;
sThreadState.nMcuLoggerThread = kAPP_NotStarted;

sEventManagerState.nFileUploadInProgress = false;
sEventManagerState.nRecordingInProgress = false;
sEventManagerState.nStreamingInProgress = false;
sEventManagerState.nWifiState = kWIFI_StateUnknown;
sEventManagerState.nCloudState = kCLOUD_StateUnknown;

return 0;
}

static int sEVENTMANAGER_Deinit( void )
{
return 0;
}

void StopAllThreads();
int bootmode = 0 ;
static void sEVENTMANAGER_HandleBootReason( eBOOT_BootReason anBootReason )
{

gLOG_Log(kLOG_Info, "HandleBoot Reason %d\n", anBootReason);
bootmode = anBootReason;
if(bootmode == 0x00) {
StopAllThreads();

} else if(bootmode == 0x01 || bootmode == 0x02 || bootmode == 0x03 
|| bootmode == 0x05) {
sEVENTMANAGER_SendMessage(kAPP_RecordingStreamingThread,kMSGQ_CmdStartRecording, 
NULL, 0);
}
}


void StopAllThreads() {

sEVENTMANAGER_SendMessage(kAPP_RecordingStreamingThread, 
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_UartRxThread,
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_UartTxThread, 
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_FileUploadThread, 
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_CloudThread, 
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_WifiThread,
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_BluetoothThread, 
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_KeepAliveToMcuThread,
kMSGQ_CmdExitThread, NULL, 0);
sEVENTMANAGER_SendMessage(kAPP_McuLoggerThread,
kMSGQ_CmdExitThread, NULL, 0);


//sbExitThread = true; 
//sEVENTMANAGER_SendMessage(kAPP_EventManagerThread,
//		kMSGQ_CmdExitThread, NULL, 0);
}

static void sEVENTMANAGER_ProcessMainAppThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
eBOOT_BootReason nBootReason;

switch( anMessage )
{
case kMSGQ_StatusBootReason:
memcpy( &nBootReason, apsPayload, sizeof( eBOOT_BootReason ) );
sEVENTMANAGER_HandleBootReason( nBootReason );
break;

case kMSGQ_StatusThreadStarted:
if( sThreadState.nMainAppThread == kAPP_NotStarted )
{
sThreadState.nMainAppThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "Main App thread is already running and "
" we got Main App thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nMainAppThread == kAPP_Running )
{
sThreadState.nMainAppThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "Main App thread is not running and "
" we got Main App thread exited message\n" );
}
StopAllThreads();
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from Main App\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessMcuMessage( char * apsPayload, size_t anPayloadSize )
{
}

static void sEVENTMANAGER_ProcessUartRxThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nUartRxThread == kAPP_NotStarted )
{
sThreadState.nUartRxThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "UartRx thread is already running and "
" we got UartRx thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nUartRxThread == kAPP_Running )
{
sThreadState.nUartRxThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "UartRx thread is not running and "
" we got UartRx thread exited message\n" );
}
break;

case kMSGQ_StatusMsgReceivedFromMcu:
sEVENTMANAGER_ProcessMcuMessage( apsPayload, anPayloadSize );
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from UartRx thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessUartTxThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nUartTxThread == kAPP_NotStarted )
{
sThreadState.nUartTxThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "UartTx thread is already running and "
" we got UartTx thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nUartTxThread == kAPP_Running )
{
sThreadState.nUartTxThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "UartTx thread is not running and "
" we got UartTx thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from UartTx thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessFileUploadThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nFileUploadThread == kAPP_NotStarted )
{
sThreadState.nFileUploadThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "FileUpload thread is already running and "
" we got FileUpload thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nFileUploadThread == kAPP_Running )
{
sThreadState.nFileUploadThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "FileUpload thread is not running and "
" we got FileUpload thread exited message\n" );
}
break;

case kMSGQ_StatusFileUploadStarted:
if( sThreadState.nFileUploadThread == kAPP_Running )
{
if( !sEventManagerState.nFileUploadInProgress )
{
sEventManagerState.nFileUploadInProgress = true;
}
else
{
gLOG_Log( kLOG_Error, "FileUpload is already in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "FileUpload thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;

case kMSGQ_StatusFileUploadStopped:
if( sThreadState.nFileUploadThread == kAPP_Running )
{
if( sEventManagerState.nFileUploadInProgress )
{
sEventManagerState.nFileUploadInProgress = false;
}
else
{
gLOG_Log( kLOG_Error, "FileUpload is not in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "FileUpload thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from FileUpload thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessRecordingStreamingThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nRecordingStreamingThread == kAPP_NotStarted )
{
sThreadState.nRecordingStreamingThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is already running and "
" we got RecordingStreaming thread started message again\n" );
}
break;

case kMSGQ_CmdRecordingDone: 
gLOG_Log( kLOG_Info, "Record done, going for sleep \n" );
//StopAllThreads();
sEVENTMANAGER_SendMessage(kAPP_MainAppThread,kMSGQ_CmdExitThread, NULL, 0);
gLOG_Log( kLOG_Info, "Sent main app exit start \n" );
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nRecordingStreamingThread == kAPP_Running )
{
sThreadState.nRecordingStreamingThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is not running and "
" we got RecordingStreaming thread exited message\n" );
}
break;

case kMSGQ_StatusRecordingStarted:
if( sThreadState.nRecordingStreamingThread == kAPP_Running )
{
if( sEventManagerState.nStreamingInProgress )
{
gLOG_Log( kLOG_Error, "Streaming is already in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}

if( !sEventManagerState.nRecordingInProgress )
{
sEventManagerState.nRecordingInProgress = true;
}
else
{
gLOG_Log( kLOG_Error, "Recording is already in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;

case kMSGQ_StatusRecordingStopped:
if( sThreadState.nRecordingStreamingThread == kAPP_Running )
{
if( sEventManagerState.nRecordingInProgress )
{
sEventManagerState.nRecordingInProgress = false;
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming is not in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;

case kMSGQ_StatusStreamingStarted:
if( sThreadState.nRecordingStreamingThread == kAPP_Running )
{
if( sEventManagerState.nRecordingInProgress )
{
gLOG_Log( kLOG_Error, "Recording is already in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}

if( !sEventManagerState.nStreamingInProgress )
{
sEventManagerState.nStreamingInProgress = true;
}
else
{
gLOG_Log( kLOG_Error, "Streaming is already in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;

case kMSGQ_StatusStreamingStopped:
if( sThreadState.nRecordingStreamingThread == kAPP_Running )
{
if( sEventManagerState.nStreamingInProgress )
{
sEventManagerState.nStreamingInProgress = false;
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming is not in progress and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
}
else
{
gLOG_Log( kLOG_Error, "RecordingStreaming thread is not running and "
" we got %s message\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
}
break;


default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from RecordingStreaming thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessCloudThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nCloudThread == kAPP_NotStarted )
{
sThreadState.nCloudThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "Cloud thread is already running and "
" we got Cloud thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nCloudThread == kAPP_Running )
{
sThreadState.nCloudThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "Cloud thread is not running and "
" we got Cloud thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from Cloud thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}

static void sEVENTMANAGER_ProcessWifiThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nWifiThread == kAPP_NotStarted )
{
sThreadState.nWifiThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "Wifi thread is already running and "
" we got Wifi thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nWifiThread == kAPP_Running )
{
sThreadState.nWifiThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "Wifi thread is not running and "
" we got Wifi thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from Wifi thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}


static void sEVENTMANAGER_ProcessBluetoothThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nBluetoothThread == kAPP_NotStarted )
{
sThreadState.nBluetoothThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "Bluetooth thread is already running and "
" we got Bluetooth thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nBluetoothThread == kAPP_Running )
{
sThreadState.nBluetoothThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "Bluetooth thread is not running and "
" we got Bluetooth thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from Bluetooth thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}


static void sEVENTMANAGER_ProcessKeepAliveToMcuThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nKeepAliveToMcuThread == kAPP_NotStarted )
{
sThreadState.nKeepAliveToMcuThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "KeepAliveToMcu thread is already running and "
" we got KeepAliveToMcu thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nKeepAliveToMcuThread == kAPP_Running )
{
sThreadState.nKeepAliveToMcuThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "KeepAliveToMcu thread is not running and "
" we got KeepAliveToMcu thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from KeepAliveToMcu thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}


static void sEVENTMANAGER_ProcessMcuLoggerThreadMessage( eMSGQ_Message anMessage,
size_t anPayloadSize, char * apsPayload )
{
switch( anMessage )
{
case kMSGQ_StatusThreadStarted:
if( sThreadState.nMcuLoggerThread == kAPP_NotStarted )
{
sThreadState.nMcuLoggerThread = kAPP_Running;
}
else
{
gLOG_Log( kLOG_Error, "McuLogger thread is already running and "
" we got McuLogger thread started message again\n" );
}
break;

case kMSGQ_StatusThreadExited:
if( sThreadState.nMcuLoggerThread == kAPP_Running )
{
sThreadState.nMcuLoggerThread = kAPP_Stopped;
}
else
{
gLOG_Log( kLOG_Error, "McuLogger thread is not running and "
" we got McuLogger thread exited message\n" );
}
break;

default:
gLOG_Log( kLOG_Error, "Received unexpected message %s from McuLogger thread\n",
ENUM_TO_STRING_MSGQ_MESSAGE( anMessage ) );
break;
}

return;
}


void * gEVENTMANAGER_MainLoop( void *arg )
{
int nRet;

// Received message
eAPP_ThreadId nSender;
eMSGQ_Message nRecvdMessage;
size_t nRecvdMsgPayloadSize;
char sRecvdMsgPayload[ MAX_PAYLOAD_SZ ];

// Initialize
nRet = sEVENTMANAGER_Init();
if( nRet < 0 )
{
gLOG_Log( kLOG_Error, "EventManager initialization failed\n" );
goto exit;
}

while( !sbExitThread )
{
nRecvdMessage = kMSGQ_CmdNoneStatusNone;
nRet = sEVENTMANAGER_ReceiveMessage( &nSender, &nRecvdMessage,
sRecvdMsgPayload, &nRecvdMsgPayloadSize );
if( nRet < 0 )
{
gLOG_Log( kLOG_Error, "EventManager thread error receiving message\n" );
continue;
}

if((nRecvdMessage == kMSGQ_CmdExitThread) && (nSender == kAPP_MainAppThread)) {

gLOG_Log(kLOG_Info, "kMSG CMD exit thread\n");
sbExitThread = true;
continue;
}

switch( nSender )
{
case kAPP_MainAppThread:
sEVENTMANAGER_ProcessMainAppThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_UartRxThread:
sEVENTMANAGER_ProcessUartRxThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_UartTxThread:
sEVENTMANAGER_ProcessUartTxThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_FileUploadThread:
sEVENTMANAGER_ProcessFileUploadThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_RecordingStreamingThread:
sEVENTMANAGER_ProcessRecordingStreamingThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_CloudThread:
sEVENTMANAGER_ProcessCloudThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_WifiThread:
sEVENTMANAGER_ProcessWifiThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_BluetoothThread:
sEVENTMANAGER_ProcessBluetoothThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_KeepAliveToMcuThread:
sEVENTMANAGER_ProcessKeepAliveToMcuThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

case kAPP_McuLoggerThread:
sEVENTMANAGER_ProcessMcuLoggerThreadMessage( nRecvdMessage,
nRecvdMsgPayloadSize, sRecvdMsgPayload );
break;

default:
gLOG_Log( kLOG_Error, "EventManager thread received message from"
" unknown sender\n" );
break;
}
}

// De-initialize
nRet = sEVENTMANAGER_Deinit();
if( nRet < 0 )
{
gLOG_Log( kLOG_Error, "EventManager de-initialization failed\n" );
}

gLOG_Log( kLOG_Info, "EventManager thread exited\n" );

exit:
pthread_exit( NULL );
return 0;
};
