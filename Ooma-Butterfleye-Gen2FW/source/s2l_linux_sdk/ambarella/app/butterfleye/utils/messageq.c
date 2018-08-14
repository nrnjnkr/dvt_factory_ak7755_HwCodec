#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "utils.h"
#include "log.h"

static int gsnMsgQueueId;

int gMSGQ_Initialize( void )
{
    key_t key;
    int nId;

    nId = 'B';
    key = ftok( "/tmp", nId );

create_queue:
    gsnMsgQueueId = msgget( key, 0666 | IPC_CREAT | IPC_EXCL );

    if( gsnMsgQueueId < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Message queue creation failed \n", __func__ );
        if( errno == EEXIST )
        {
            gLOG_Log( kLOG_Info, "Deleting older instance of message queue "
                    "and creating new one\n" );
            gsnMsgQueueId = msgget( key, 0666 );
            if( gMSGQ_Deinitialize() == 0 )
            {
                goto create_queue;
            }
        }

        return gsnMsgQueueId;
    }

    gLOG_Log( kLOG_Debug, "Message queue created with id %d\n", gsnMsgQueueId );

    return 0;
}

int gMSGQ_Deinitialize( void )
{
    int nRet;

    nRet = msgctl( gsnMsgQueueId, IPC_RMID, NULL );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: Message queue deletion failed\n", __func__ );
        return nRet;
    }

    gLOG_Log( kLOG_Debug, "Message queue with id %d deleted\n", gsnMsgQueueId );

    return 0;
}

int gMSGQ_Send( tMessage *apMessage )
{
    int nRet;

    if( apMessage->nPayloadSize > MAX_PAYLOAD_SZ )
    {
        gLOG_Log( kLOG_Error, "%s: Payload size bigger than max supported size\n",
                __func__ );
        return -EMSGSIZE;
    }

    nRet = msgsnd( gsnMsgQueueId,
            apMessage,
            sizeof( tMessage ) - sizeof( long ),
            0 );
    if( nRet < 0 )
    {
        gLOG_Log( kLOG_Error, "msgqsnd failed\n" );
        return nRet;
    }

    gLOG_Log( kLOG_Debug, "%s: Sent message: "
            "Receiver = %ld Sender = %d Message = %d  Payload Size = %ld \n",
            __func__, apMessage->nReceiver, apMessage->nSender,
            apMessage->nMessage, apMessage->nPayloadSize );
    for( int i = 0; i < apMessage->nPayloadSize; i++ )
    {
        gLOG_Log( kLOG_Debug, "Message payload [%d] 0x%x\n", i, apMessage->sPayload[i] );
    }

    return 0;
}

int gMSGQ_Receive( tMessage *apMessage )
{
    size_t nReceivedMessageSize;

    gLOG_Log( kLOG_Debug, "%s: Trying to receive message of type %d\n",
            __func__, apMessage->nReceiver );

    nReceivedMessageSize = msgrcv( gsnMsgQueueId,
            apMessage,
            sizeof( tMessage ) - sizeof( long ),
            apMessage->nReceiver,
            0 );
    if( nReceivedMessageSize < 0 )
    {
        gLOG_Log( kLOG_Error, "%s: msgrcv failed\n", __func__ );
        return nReceivedMessageSize;
    }

    gLOG_Log( kLOG_Debug, "%s: Received message: "
            "Receiver = %ld Sender = %d Message = %d  Payload Size = %ld \n",
            __func__, apMessage->nReceiver, apMessage->nSender,
            apMessage->nMessage, apMessage->nPayloadSize );
    for( int i = 0; i < apMessage->nPayloadSize; i++ )
    {
        gLOG_Log( kLOG_Debug, "Message payload [%d] 0x%x\n", i, apMessage->sPayload[i] );
    }

    return 0;
}
