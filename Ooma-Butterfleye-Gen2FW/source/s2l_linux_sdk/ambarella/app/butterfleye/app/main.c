#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include "butterfleye.h"
#include "log.h"
#include "utils.h"

#if DEBUG
eLOG_LogLevel gnLogLevel = kLOG_Debug;
#else
eLOG_LogLevel gnLogLevel = kLOG_Info;
#endif

//MESSAGE_API( APP, kAPP_MainAppThread );

static int sAPP_SendMessage( eAPP_ThreadId anReceiver,
        eMSGQ_Message anMessage,
        char *asPayload, size_t anSize) 
{ 
    int nRet; 
    tMessage Message; 
    
    Message.nReceiver = anReceiver; 
    Message.nSender = kAPP_MainAppThread; 
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
    
    nRet = gMSGQ_Send( &Message ); 
    
    return nRet; 
} 


static int sAPP_ReceiveMessage( eAPP_ThreadId *apnSender, 
        eMSGQ_Message * apnMessage,
        char *asPayload, size_t * apnSize) 
{ 
    int nRet; 
    tMessage Message; 
    
    gLOG_Log(kLOG_Info, "Main App receiveer thread queue\n");
    Message.nReceiver = kAPP_MainAppThread; 
    nRet = gMSGQ_Receive( &Message ); 
    if( nRet < 0 ) 
    { 
        return nRet; 
    } 
    
    *apnSender = Message.nSender; 
    *apnMessage = Message.nMessage; 
    *apnSize = Message.nPayloadSize; 
    gLOG_Log(kLOG_Info, "App receiveer thread\n");
    if( asPayload && Message.nPayloadSize ) 
    { 
        memcpy( asPayload, Message.sPayload, Message.nPayloadSize ); 
    }
 	
    gLOG_Log(kLOG_Info, "End of receiver thread\n");
    return nRet;   
}

typedef struct {
    pthread_t Ref;
    void * ( * fMainLoop ) ( void * );
    const char * sName;
} tAPP_ThreadInfo;

// This is the order in which the threads will start, so preserve this order
static tAPP_ThreadInfo sAPP_ThreadInfo[] = {
    {
        .fMainLoop = gRECORDSTREAM_MainLoop,
        .sName = "RecordingStreamingThread",
    },

    {
        .fMainLoop = gEVENTMANAGER_MainLoop,
        .sName = "EventManagerThread",
    },

    {
        .fMainLoop = gWIFI_MainLoop,
        .sName = "WiFiThread",
    },

    {
        .fMainLoop = gBT_MainLoop,
        .sName = "BluetoothThread",
    },

    {
        .fMainLoop = gUARTRX_MainLoop,
        .sName = "UartRxThread",
    },

    {
        .fMainLoop = gUARTTX_MainLoop,
        .sName = "UartTxThread",
    },

    {
        .fMainLoop = gCLOUD_MainLoop,
        .sName = "CloudThread",
    },

    {
        .fMainLoop = gFILEUPLOAD_MainLoop,
        .sName = "FileUploaderThread",
    },

    {
        .fMainLoop = gKEEPALIVE_MainLoop,
        .sName = "KeepAliveToMCUThread",
    },

    {
        .fMainLoop = gMCULOGGER_MainLoop,
        .sName = "MCULoggerThread",
    },
};

static bool sbExitApp = false;

static void sAPP_InitializeAppState( void )
{
}

static int sAPP_StartAllThreads( void )
{
    int nRet;
    int i;

    for( i = 0; i < ( sizeof( sAPP_ThreadInfo ) / sizeof( tAPP_ThreadInfo ) ); i++ )
    {
        nRet = pthread_create( &sAPP_ThreadInfo[i].Ref, NULL,
                sAPP_ThreadInfo[i].fMainLoop, NULL );
        if( nRet != 0 )
        {
            gLOG_Log( kLOG_Error, "Failed to start thread : %s\n",
                    sAPP_ThreadInfo[i].sName );
            return nRet;
        }
    }

    return nRet;
}

static int sAPP_JoinAllThreads( void )
{
    int i;

    for( i = 0; i < ( sizeof( sAPP_ThreadInfo ) / sizeof( tAPP_ThreadInfo ) ); i++ )
    {
        pthread_join( sAPP_ThreadInfo[i].Ref, NULL );
    }

    return 0;
}

int main( int argc, char** argv )
{
    int nRet = 0;
    eBOOT_BootReason nBootReason;
    eAPP_ThreadId nSender;
    eMSGQ_Message nMessage;

    gLOG_Log( kLOG_Info,
            "*******************************************************\n" );
    gLOG_Log( kLOG_Info, "Welcome to Butterfleye JR Main Application\n" );
    gLOG_Log( kLOG_Info,
            "*******************************************************\n" );

    sAPP_InitializeAppState();

    nRet = gMSGQ_Initialize();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Message queue init failed .. exiting\n" );
        exit( 1 );
    }

    nBootReason = gBOOT_CheckBootReason();
    gLOG_Log( kLOG_Info, "Boot reason is %x %s\n", nBootReason,
            ENUM_TO_STRING_BOOTREASON( nBootReason ) );
    if( ( nBootReason < 0 ) || ( nBootReason == kBOOT_BootReasonUnknown ) )
    {
        gLOG_Log( kLOG_Error, "Could not determine boot reason\n" );
        //exit( 1 );
    }
    if(nBootReason == 0x00) {

	fprintf(stderr, "Skipping as of now\n");
    } else {

 /*   nRet = sAPP_SendMessageToEventManager (kMSGQ_StatusBootReason,
            ( char * ) &nBootReason, sizeof( nBootReason ) );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Sending boot reason to event manager failed\n" );
        exit( 1 );
    } */

    nRet = sAPP_StartAllThreads();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Failed to start all application threads\n" );
        exit( 1 );
    }


    nRet = sAPP_SendMessage (kAPP_EventManagerThread, kMSGQ_StatusThreadStarted, NULL, 0 );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Sending message to event manager failed\n" );
    }

    nRet = sAPP_SendMessage (kAPP_EventManagerThread, kMSGQ_StatusBootReason,
            ( char * ) &nBootReason, sizeof( nBootReason ) );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Sending boot reason to event manager failed\n" );
        exit( 1 );
    }

    gLOG_Log(kLOG_Error, "In Main after starting and informing boot mode\n");

    size_t nRecvdMsgPayloadSize;
    char sRecvdMsgPayload[ MAX_PAYLOAD_SZ ];
    while( !sbExitApp )
    {
        nRet = sAPP_ReceiveMessage( &nSender, &nMessage, sRecvdMsgPayload, &nRecvdMsgPayloadSize );
        if( nRet < 0 )
        {
            gLOG_Log( kLOG_Error, "Error receiving message from message queue\n" );
            continue;
        }

        switch( nMessage )
        {
        case kMSGQ_CmdExitThread:
	     gLOG_Log(kLOG_Info, "Received thread exit\n");
            sbExitApp = true;
            break;

        default:
            gLOG_Log( kLOG_Warn, "Received unknown message : %d\n", nMessage );
            break;
        }
    }

    gLOG_Log(kLOG_Info, "Sending event thread status thread exited\n");
    nRet = sAPP_SendMessage (kAPP_EventManagerThread, kMSGQ_StatusThreadExited, NULL, 0 );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Sending message to event manager failed\n" );
    }

    gLOG_Log(kLOG_Info, "Sending event thread exit\n");
    nRet = sAPP_SendMessage (kAPP_EventManagerThread, kMSGQ_CmdExitThread, NULL, 0 );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Sending message to event manager failed\n" );
    }
    gLOG_Log(kLOG_Info, "before joining thread \n");

    nRet = sAPP_JoinAllThreads();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Failed to stop all application threads\n" );
        exit( 1 );
    }
  }
    nRet = gBOOT_PowerOff();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "Could not send power off message to MCU" );
        exit( 1 );
    }

    exit( 0 );
}
