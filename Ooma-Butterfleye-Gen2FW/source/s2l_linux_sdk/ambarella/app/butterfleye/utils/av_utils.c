#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include <iav_ucode_ioctl.h>

#include "av_utils.h"

#define MAX_ENCODE_STREAM_NUM 1

void* rtsp_server(void*);

int gIAV_StartEncoding()
{
    struct iav_stream_info streaminfo;
    int fd_iav;
    int i;
    int streamid = 1;

    // open the device
    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        return -1;
    }

    for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
        if (streamid & (1 << i)) {
            streaminfo.id = i;

            if( ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &streaminfo) < 0 )
            {
                perror("Could not get info for stream\n");
            }


            if (streaminfo.state == IAV_STREAM_STATE_ENCODING) {
                streamid &= ~(1 << i);
            }
        }
    }

    if (streamid == 0) {
        printf("already in encoding, nothing to do \n");
        close( fd_iav );
        return 0;
    }

    if( ioctl(fd_iav, IAV_IOC_START_ENCODE, streamid) < 0 )
    {
        perror("Could not start encoding for stream\n");
        close( fd_iav );
        return -1;
    }

    printf("Start encoding for stream 0x%x successfully\n", streamid);

    close( fd_iav );

    return 0;
}

int gIAV_StopEncoding()
{
    struct iav_stream_info streaminfo;
    int fd_iav;
    int i;
    int streamid = 1;

    // open the device
    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        return -1;
    }

    for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
        if (streamid & (1 << i)) {
            streaminfo.id = i;

            if( ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &streaminfo) < 0 )
            {
                perror("Could not get info for stream\n");
            }


            if (streaminfo.state != IAV_STREAM_STATE_ENCODING) {
                streamid &= ~(1 << i);
            }
        }
    }

    if (streamid == 0) {
        printf("not encoding, nothing to do \n");
        close( fd_iav );
        return 0;
    }

    if( ioctl(fd_iav, IAV_IOC_STOP_ENCODE, streamid) < 0 )
    {
        perror("Could not stop encoding for stream\n");
        close( fd_iav );
        return -1;
    }

    printf("Stopped encoding for stream 0x%x successfully\n", streamid);

    close( fd_iav );

    return 0;
}

static pthread_t rtsp_server_thread;

int gRTSP_StartRtspServer()
{
    int ret;

    ret = pthread_create( &rtsp_server_thread, NULL, rtsp_server, NULL );
    if( ret < 0 )
    {
        printf( "Failed to create rtsp server" );
        return -1;
    }

    return 0;
}
int gRTSP_StopRtspServer()
{
    return -1;
}
