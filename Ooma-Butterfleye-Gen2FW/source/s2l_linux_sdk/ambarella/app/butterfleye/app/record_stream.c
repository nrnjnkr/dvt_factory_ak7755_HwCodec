#include<stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "butterfleye.h"
#include "log.h"
#include "utils.h"
#include "av_utils.h"

static bool gbExitThread = false;

//MESSAGE_API( RECORDSTREAM, kAPP_RecordingStreamingThread );

static int sRECORDSTREAM_SendMessage( eAPP_ThreadId anReceiver,
        eMSGQ_Message anMessage,
        char *asPayload, size_t anSize) 
{ 
    int nRet; 
    tMessage Message; 
    
    Message.nReceiver = anReceiver; 
    Message.nSender = kAPP_RecordingStreamingThread; 
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


static int sRECORDSTREAM_ReceiveMessage( eAPP_ThreadId *apnSender, 
        eMSGQ_Message * apnMessage,
        char *asPayload, size_t * apnSize) 
{ 
    int nRet; 
    tMessage Message; 
    
    Message.nReceiver = kAPP_RecordingStreamingThread; 
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

static int sRECORDSTREAM_Init( void )
{
    return 0;
}

	int recordstream(int timestamp);

static int start_recording(int timestamp) {
	recordstream(timestamp);
	fprintf(stderr, "After record done\n");
	sRECORDSTREAM_SendMessage(kAPP_EventManagerThread, 
				kMSGQ_CmdRecordingDone, NULL, 0 );
	return 1;
}

static int sRECORDSTREAM_Deinit( void )
{
    return 0;
}

static int sRECORDSTREAM_StartStreaming( void )
{
    int nRet;

    // stop encode
    nRet = gIAV_StopEncoding();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Stop encoding failed\n", __func__ );
        return nRet;
    }

    // start RTSP server
    nRet = gRTSP_StartRtspServer();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Starting RTSP server failed\n", __func__ );
        return nRet;
    }

    // start encode
    nRet = gIAV_StartEncoding();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Start encoding failed\n", __func__ );
        return nRet;
    }

    return 0;
}

static int sRECORDSTREAM_StopStreaming( void )
{
    int nRet;

    // stop RTSP server
    nRet = gRTSP_StopRtspServer();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Stopping RTSP server failed\n", __func__ );
        return nRet;
    }

    // stop encode
    nRet = gIAV_StopEncoding();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Stop encoding failed\n", __func__ );
        return nRet;
    }

    return 0;
}

void * gRECORDSTREAM_MainLoop( void *arg )
{
    int nRet;

    // Received message
    eAPP_ThreadId nSender;
    eMSGQ_Message nRecvdMessage;
    size_t nRecvdMsgPayloadSize;
    char sRecvdMsgPayload[ MAX_PAYLOAD_SZ ];

    // Initialize
    nRet = sRECORDSTREAM_Init();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "RecordingStreaming initialization failed\n" );
        goto exit;
    }

    gLOG_Log( kLOG_Info, "RecordingStreaming thread started\n" );
    sRECORDSTREAM_SendMessage (kAPP_EventManagerThread, 
			kMSGQ_StatusThreadStarted, NULL, 0 );

    while( !gbExitThread )
    {
        nRecvdMessage = kMSGQ_CmdNoneStatusNone;
        nRet = sRECORDSTREAM_ReceiveMessage( &nSender, &nRecvdMessage,
                sRecvdMsgPayload, &nRecvdMsgPayloadSize );
        if( nRet < 0 )
        {
            gLOG_Log( kLOG_Error, "RecordingStreaming thread error receiving message\n" );
            continue;
        }

        switch( nRecvdMessage )
        {
        // Add other cases here
        case kMSGQ_CmdExitThread:
            gbExitThread = true;
            break;

	case kMSGQ_CmdStartRecording:
		start_recording(5);
		gLOG_Log(kLOG_Info, "Record is finished\n");
	//	sRECORDSTREAM_SendMessageToEventManager(
	//			kMSGQ_CmdRecordingDone, NULL, 0 );
		gLOG_Log(kLOG_Info, "sent Record is finished to event manager\n");
		break;
		
        case kMSGQ_CmdStartStreaming:
            nRet = sRECORDSTREAM_StartStreaming();
            if( nRet == 0 )
            {
                sRECORDSTREAM_SendMessage(kAPP_EventManagerThread, 
                        kMSGQ_StatusStreamingStarted, NULL, 0 );
            }

            break;

        case kMSGQ_CmdStopStreaming:
            nRet = sRECORDSTREAM_StopStreaming();
            if( nRet == 0 )
            {
                sRECORDSTREAM_SendMessage (kAPP_EventManagerThread, 
                        kMSGQ_StatusStreamingStopped, NULL, 0 );
            }

            break;

        default:
            gLOG_Log( kLOG_Error, "RecordingStreaming thread received unknown message\n" );
            break;
        }
    }

    // Initialize
    nRet = sRECORDSTREAM_Deinit();
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "RecordingStreaming de-initialization failed\n" );
    }

    sRECORDSTREAM_SendMessage (kAPP_EventManagerThread, kMSGQ_StatusThreadExited, NULL, 0 );

    gLOG_Log( kLOG_Info, "RecordingStreaming thread exited\n" );

exit:
    pthread_exit( NULL );
    return 0;
}
