/*
 * streaming_audio_playback.c
 *
 * History:
 *       2017/03/15 - [Jian Liu] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>          /* for printf, fprintf */
#include <stdlib.h>         /* for atoi()          */
#include <errno.h>
#include <unistd.h>         /* for close()         */
#include <string.h>         /* for strncpy()       */
#include <time.h>           /* for usleep()        */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include "streaming_audio_playback.h"
#include "audio_play.h"

/*It would be much better use AM_PLAYBACK_URI_RTP,
*    but at this moment, AM_PLAYBACK_URI_RTP is not ready
*/
#define USE_UNIX_DOMAIN_URI 1

//-------------------------------------------------------------------------
// interface implementation
//  ---- TODO:  check rtp packets and handle jitter.
//-------------------------------------------------------------------------
typedef struct  _context_aplay_t
{
    AMPlaybackRtpUri rtp_uri;
    pthread_t thread;
    volatile int thread_running;
}context_aplay_t;

static context_aplay_t s_aplay_context;

static void *aplay_thread_routine(void *arg);
int  audio_play_initialize(AMPlaybackRtpUri &uri){
    memset(&s_aplay_context,0,sizeof(context_aplay_t));
    s_aplay_context.rtp_uri = uri;
    s_aplay_context.thread_running = 1;
    int ret = pthread_create(&s_aplay_context.thread, NULL, aplay_thread_routine, (void*)&s_aplay_context);
    if (ret != 0) {
        printf("faile to spawn  audio playback thread\n");
        audio_play_finalize();
        return -1;
    }
    return 0;
}

int  audio_play_finalize(void)
{
    s_aplay_context.thread_running = 0;
    if(s_aplay_context.thread){
        pthread_join(s_aplay_context.thread, NULL);
    }
    return 0;
}



#ifndef   USE_UNIX_DOMAIN_URI
static void *aplay_thread_routine(void *arg){
    context_aplay_t* context = (context_aplay_t*)arg;
    AMIPlaybackPtr playback  = nullptr;
    AMPlaybackUri uri;

    load_audio_playback_driver();

    signal(SIGPIPE, SIG_IGN);
    playback = AMIPlayback::create();
    if(!playback) {
        goto err_exit;
    }
    if (!playback->init()) {
        goto err_exit;
    }
    uri.type = AM_PLAYBACK_URI_RTP;
    uri.media.rtp = context->rtp_uri;
    if (!playback->add_uri(uri)) {
        ERROR("Failed to add uri to  play list!");
        goto err_exit;
    }
    playback->play();
    while(context->thread_running){
        usleep(1000 * 10);
    }
    playback->stop();
err_exit:
    return (void*)NULL;
}

#else //UNIX_DOMAIN_URI

#define AUDIO_PLAYBACK_DOMAIN_NAME "/tmp/audio_playback0"

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netinet/in.h>
static inline int ff_network_wait_fd(int fd, int write)
{
    int ret;
    int ev = write ? POLLOUT : POLLIN;
    struct pollfd p;
    p.fd = fd;
    p.events = ev;
    p.revents = 0;
    while((ret = poll(&p, 1, 100) )< 0 && (errno == EINTR));
    return ret < 0 ? (-errno) : p.revents & ev ? 0 : (-EAGAIN);
}

static inline int socket_write(int fd, void *buf, int size)
{
    int ret = ff_network_wait_fd(fd, 1);
    if (ret < 0){
        return ret;
    }
    while((ret = send(fd, buf, size, 0) ) < 0 && (errno == EINTR));
    return (ret < 0) ? -errno : ret;
}

static int retry_transfer_wrapper(int fd, void *buf_, int size)
{
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
    unsigned char *buf = (unsigned char *)buf_;
    int len;
    int fast_retries = 5;
    len = 0;
    while (len < size){
        int ret = socket_write(fd, buf+len, size-len);
        if (ret == (-EAGAIN)){
            ret = 0;
            if (fast_retries)
                fast_retries--;
            //else
                //my_msleep(1);
        }else if (ret < 1){
            return ret < 0 ? ret : len;
        }
        if (ret){
            fast_retries = FFMAX(fast_retries, 2);
        }
        len += ret;
    }
    return len;
}

static int create_domain_client(char *path)
{
    struct sockaddr_un addr;
    int ret,count = 1000;

    if (access (path, F_OK) != 0){
        ERROR ("Server doesn't run: %s doesn't exist!",path);
        return -1;
    }

    int sock_fd = socket (PF_UNIX, SOCK_STREAM, 0);
    if(sock_fd < 0){
        return -1;
    }
    memset (&addr, 0, sizeof (struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    snprintf (addr.sun_path,sizeof(addr.sun_path),"%s",path);
retry:
    while( (ret = connect (sock_fd, (struct sockaddr *)&addr,sizeof (struct sockaddr_un))) < 0 && errno == EINTR);
    if ( ret < 0){
        if(errno == ECONNREFUSED){
            /*retry, maybe the server is not ready yet for our case*/
            --count;
            if(count <= 0){
                goto exit;
            }
            usleep(5000);
            goto retry;
        }
        ERROR("Failed to connect domain server [%s],errno = %d\n",path,errno);
        goto exit;
    }
    return sock_fd;
exit:
    close(sock_fd);
    return -1;
}

static void *aplay_thread_routine(void *arg){
    context_aplay_t* context = (context_aplay_t*)arg;
    AMIPlaybackPtr playback  = nullptr;
    AMPlaybackUri uri;
    struct sockaddr *addr = NULL;
    struct sockaddr_in addr4 = { 0 };
    //struct sockaddr_in6 addr6 = { 0 };
    socklen_t socket_len = sizeof(struct sockaddr);
    int domain_sock = -1;
    int rtp_sock = -1;
    unsigned char  send_buf[2048];
    unsigned char *recv_buf = &send_buf[8];
    int recv_buf_len = 2040;
    send_buf[0] = 'R';
    send_buf[1] = 'T';
    send_buf[2] = 'P';
    send_buf[3] = ' ';
    load_audio_playback_driver();

    bool play_start = false;
    signal(SIGPIPE, SIG_IGN);

    //create rtp socket, TODO, at this moment, IPV4 only
    if(context->rtp_uri.ip_domain != AM_PLAYBACK_IPV4){
        ERROR("At this moment, support only IPV4\n");
        goto err_exit;
    }
    if((rtp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        goto err_exit;
    }
    addr4.sin_family = AF_INET;
    addr4.sin_addr.s_addr = htonl(INADDR_ANY);
    addr4.sin_port = htons(context->rtp_uri.udp_port);
    addr = (struct sockaddr*)&addr4;
    socket_len = sizeof(addr4);
    if (bind(rtp_sock, addr, socket_len) < 0) {
        goto err_exit;
    }

    //create playback instance and start it
    remove(AUDIO_PLAYBACK_DOMAIN_NAME);//make sure domain-socket path has been removed
    playback = AMIPlayback::create();
    if(!playback) {
        ERROR("Failed to create player!");
        goto err_exit;
    }
    if (!playback->init()) {
        ERROR("Failed to init player!");
        goto err_exit;
    }

    uri.type = AM_PLAYBACK_URI_UNIX_DOMAIN;
    uri.media.unix_domain.audio_type = context->rtp_uri.audio_type;
    uri.media.unix_domain.channel = context->rtp_uri.channel;
    uri.media.unix_domain.sample_rate = context->rtp_uri.sample_rate;
    uri.media.unix_domain.playback_id_bit_map |= 0x00000001 << 0;
    snprintf(uri.media.unix_domain.name,sizeof(uri.media.unix_domain.name),"%s",AUDIO_PLAYBACK_DOMAIN_NAME);
    /*TODO ,
    * It is weired that player->play() will fail here, at this moment, perform this operation after at least one packet has been sent to domain-socket-server.
    */
#if 0
    if(!playback->play()){
        ERROR("Failed to player->play()!");
        goto err_exit;
    }
#endif

    //create domain socket

    while (context->thread_running) {
        fd_set fds;
        int max_fd = -1;
        FD_ZERO(&fds);
        FD_SET(rtp_sock, &fds);
        max_fd = rtp_sock;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        int ret;
        while((ret = select(max_fd + 1, &fds, NULL, NULL, &tv)) < 0 && errno == EINTR);
        if(ret == 0){
            if(play_start){
                play_start = false;
                if(domain_sock != -1){
                    close(domain_sock);
                    domain_sock = -1;
                }
                playback->stop();
            }
            continue;
        }
        if(ret < 0){
            break;
        }
        if (FD_ISSET(rtp_sock, &fds)) {
            ssize_t recv_len;
            while((recv_len = recv(rtp_sock, recv_buf,recv_buf_len, 0)) < 0  && errno == EINTR);
            if(recv_len < 0){
                goto err_exit;
            }
            /*TODO:
            *    check rtp packets and handle jitter
            */
            if(!play_start){
                remove(AUDIO_PLAYBACK_DOMAIN_NAME);//make sure domain-socket path has been removed
                if (!playback->add_uri(uri)) {
                    ERROR("Failed to add uri to  play list!");
                    goto err_exit;
                }
                domain_sock = create_domain_client((char*)AUDIO_PLAYBACK_DOMAIN_NAME);
                if (domain_sock < 0) {
                    ERROR("failed to create domain socket");
                    goto err_exit;
                }
            }

            send_buf[4] = (recv_len & 0xff000000) >> 24;
            send_buf[5] = (recv_len & 0x00ff0000) >> 16;
            send_buf[6] = (recv_len & 0x0000ff00) >>  8;
            send_buf[7] = (recv_len & 0x000000ff);;
            retry_transfer_wrapper(domain_sock, send_buf,recv_len + 8);

            if(!playback->play()){
                ERROR("Failed to player->play()!");
                break;
            }
            else{
                play_start = true;
            }
        }
    }
err_exit:
    if(play_start) playback->stop();
    if(rtp_sock != -1) close(rtp_sock);
    if(domain_sock != -1) close(domain_sock);
    return (void*)NULL;
}
#endif
