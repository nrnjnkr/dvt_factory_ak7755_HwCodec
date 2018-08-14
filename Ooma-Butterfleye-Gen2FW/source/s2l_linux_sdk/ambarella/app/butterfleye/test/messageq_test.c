#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "utils.h"

eLOG_LogLevel gnLogLevel = kLOG_Debug;

MESSAGE_API( MAINAPP, kAPP_MainAppThread );
MESSAGE_API( UARTRX, kAPP_UartRxThread );
MESSAGE_API( UARTTX, kAPP_UartTxThread );
MESSAGE_API( EVENTMANAGER, kAPP_EventManagerThread );
MESSAGE_API( FILEUPLOAD, kAPP_FileUploadThread );
MESSAGE_API( RECORD, kAPP_RecordingStreamingThread );
MESSAGE_API( CLOUD, kAPP_CloudThread );
MESSAGE_API( WIFI, kAPP_WifiThread );
MESSAGE_API( BT, kAPP_BluetoothThread );
MESSAGE_API( KEEPALIVE, kAPP_KeepAliveToMcuThread );
MESSAGE_API( MCULOG, kAPP_McuLoggerThread );


void main()
{
    eAPP_ThreadId sender;
    eAPP_ThreadId receiver;
    eMSGQ_Message message;
    char payload[ MAX_PAYLOAD_SZ + 10 ];
    size_t payload_size;
    int ret;

    gMSGQ_Initialize();

    system( "ipcs -q" );

#if 0
    /* Receive a message - no message on queue -- blocks */
    payload_size = 0;
    ret = sEVENTMANAGER_ReceiveMessage( &sender, &message, payload, &payload_size );
    printf( "Receive message - no message on queue ret val = %d\n", ret );
#endif

    /* Send a message - no payload */
    message = 0x1;
    receiver = kAPP_UartTxThread;
    ret = sMAINAPP_SendMessage( receiver, message, NULL, 0 );
    printf( "Send Message - no payload ret = %d\n", ret );

    system( "ipcs -q" );

    /* Send a message - payload size < MAX_PAYLOAD_SZ */
    message = kMSGQ_CmdStartRecording;
    payload_size = MAX_PAYLOAD_SZ - 4;
    memset( payload, 'A', payload_size );
    ret = sWIFI_SendMessageToEventManager( message, payload, payload_size );
    printf( "Send message - payload size < MAX_PAYLOAD_SZ ret = %d\n", ret );
    system( "ipcs -q" );

    /* Send a message - payload size = MAX_PAYLOAD_SZ */
    message = kMSGQ_CmdStopRecording;
    payload_size = MAX_PAYLOAD_SZ;
    memset( payload, 'B', payload_size );
    ret = sWIFI_SendMessageToEventManager( message, payload, payload_size );
    printf( "Send message - payload size = MAX_PAYLOAD_SZ ret = %d\n", ret );
    system( "ipcs -q" );



    /* Send a message - payload size > MAX_PAYLOAD_SZ */
    message = kMSGQ_StatusWifiConnected;
    payload_size = MAX_PAYLOAD_SZ + 10;
    memset( payload, 'C', payload_size );
    ret = sWIFI_SendMessageToEventManager( message, payload, payload_size );
    printf( "Send message - payload size > MAX_PAYLOAD_SZ ret = %d\n", ret );
    system( "ipcs -q" );


    memset( payload, 0, MAX_PAYLOAD_SZ + 10 );

    /* Receive a message */
    payload_size = MAX_PAYLOAD_SZ;
    ret = sEVENTMANAGER_ReceiveMessage( &sender, &message, payload, &payload_size );
    printf( "Receive message ret val = %d sender %d message %d payload_size %ld payload %s\n", ret, sender, message, payload_size, payload );

    /* Receive a message */
    payload_size = MAX_PAYLOAD_SZ;
    ret = sMAINAPP_ReceiveMessage( &sender, &message, payload, &payload_size );
    printf( "Receive message ret val = %d sender %d message %d payload_size %ld payload %s\n", ret, sender, message, payload_size, payload );

    /* Receive a message */
    payload_size = MAX_PAYLOAD_SZ;
    ret = sMAINAPP_ReceiveMessage( &sender, &message, payload, &payload_size );
    printf( "Receive message ret val = %d sender %d message %d payload_size %ld payload %s\n", ret, sender, message, payload_size, payload );

    /* Receive a message */
    payload_size = MAX_PAYLOAD_SZ;
    ret = sMAINAPP_ReceiveMessage( &sender, &message, payload, &payload_size );
    printf( "Receive message ret val = %d sender %d message %d payload_size %ld payload %s\n", ret, sender, message, payload_size, payload );

    system( "ipcs -q" );

    /* Receive a message */

    /* Send message until no more space */


    gMSGQ_Deinitialize();

    system( "ipcs -q" );
}
