/*
 * streaming_rtp_over_udp.c
 *
 * History:
 *       2016/12/14 - Jian Liu] created file
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
#include <inttypes.h>
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_video_camera_if.h"
#include "am_audio_capture_if.h"
#include "am_audio_codec_if.h"
#include "am_plugin.h"
#include "am_video_types.h"
#include "am_image_quality_if.h"
#include "am_low_bitrate_control_if.h"
#include "am_motion_detect_if.h"
#include "am_encode_overlay_if.h"
#include "streaming_rtp_over_udp.h"
#include "bpi_oryx_config.h"

static unsigned long long get_current_time(void){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC,&now);
    unsigned long long mseconds = now.tv_sec * 1000+ now.tv_nsec/1000000;
    return mseconds;
}
#include <sys/time.h>
static unsigned long long current_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec/1000);
}
static void setscheduler(int priority)
{
    struct sched_param sched_param;
    int sched_policy = SCHED_RR;
    if (sched_getparam(0, &sched_param) < 0) {
        //printf("Scheduler getparam failed...\n");
        return;
    }
    sched_param.sched_priority = sched_get_priority_max(sched_policy);
    sched_param.sched_priority -= priority;
    if (!sched_setscheduler(0, sched_policy, &sched_param)) {
        //printf("Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
        return;
    }
    printf("!!!Scheduler set to Round Robin with priority %i FAILED!!!\n", sched_param.sched_priority);
}

static char* base64_encode(char const* data, unsigned length) {
  static const char s_base64_char[] =
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  unsigned char const* orig = (unsigned char const*)data;
  if (orig == NULL) return NULL;

  unsigned const num_24bit_values = length/3;
  bool have_padding = (length > num_24bit_values*3);
  bool have_padding2 = (length == num_24bit_values*3 + 2);
  unsigned const num_result_bytes = 4*(num_24bit_values + have_padding);
  char* result = new char[num_result_bytes+1]; // allow for trailing '\0'

  // Map each full group of 3 input bytes into 4 output base-64 characters:
  unsigned i;
  for (i = 0; i < num_24bit_values; ++i) {
    result[4*i+0] = s_base64_char[(orig[3*i]>>2)&0x3F];
    result[4*i+1] = s_base64_char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
    result[4*i+2] = s_base64_char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
    result[4*i+3] = s_base64_char[orig[3*i+2]&0x3F];
  }

  // Now, take padding into account.  (Note: i == num_24bit_values)
  if (have_padding) {
    result[4*i+0] = s_base64_char[(orig[3*i]>>2)&0x3F];
    if (have_padding2) {
      result[4*i+1] = s_base64_char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
      result[4*i+2] = s_base64_char[(orig[3*i+1]<<2)&0x3F];
    } else {
      result[4*i+1] = s_base64_char[((orig[3*i]&0x3)<<4)&0x3F];
      result[4*i+2] = '=';
    }
    result[4*i+3] = '=';
  }

  result[num_result_bytes] = '\0';
  return result;
}

//---------------------------------------------------------------------------------------
// rtp utils
//---------------------------------------------------------------------------------------
#ifndef WORDS_BIGENDIAN
/*
 * rtp_hdr_t represents an RTP header.  The bit-fields in
 * this structure should be declared "unsigned int" instead of
 * "unsigned char", but doing so causes the MS compiler to not
 * fully pack the bit fields.
 */

typedef struct {
    unsigned char cc : 4; /* CSRC count             */
    unsigned char x : 1; /* header extension flag  */
    unsigned char p : 1; /* padding flag           */
    unsigned char version : 2; /* protocol version    */
    unsigned char pt : 7; /* payload type           */
    unsigned char m : 1; /* marker bit             */
    uint16_t seq; /* sequence number        */
    uint32_t ts; /* timestamp              */
    uint32_t ssrc; /* synchronization source */
} rtp_hdr_t;

#else /*  BIG_ENDIAN */

typedef struct {
    unsigned char version : 2; /* protocol version    */
    unsigned char p : 1; /* padding flag           */
    unsigned char x : 1; /* header extension flag  */
    unsigned char cc : 4; /* CSRC count             */
    unsigned char m : 1; /* marker bit             */
    unsigned char pt : 7; /* payload type           */
    uint16_t seq; /* sequence number        */
    uint32_t ts; /* timestamp              */
    uint32_t ssrc; /* synchronization source */
} rtp_hdr_t;
#endif

/*
 * RTP_HEADER_LEN indicates the size of an RTP header
 */
#define RTP_HEADER_LEN   12
/*
 * RTP_MAX_BUF_LEN defines the largest RTP packet in the rtp.c implementation
 */
#define RTP_MAX_BUF_LEN  16384

typedef struct {
    rtp_hdr_t header;
    char body[RTP_MAX_BUF_LEN];
} rtp_msg_t;

typedef struct rtp_sender_ctx {
    rtp_msg_t message;
    int socket;
    struct sockaddr_in addr; /* reciever's address */
} rtp_sender_ctx_t;
typedef struct rtp_sender_ctx *rtp_sender_t;

#define ADDR_IS_MULTICAST(a) IN_MULTICAST(htonl(a))

#define stap_a_sps_pps_buf_size 1024
#define MAX_RTP_PAYLOAD_LEN 1400
typedef struct _rtp_session_t{
    rtp_sender_t sender_;
    int sock;
    struct in_addr rcvr_addr;
    struct ip_mreq mreq;
    int count_;
    unsigned int ssrc_;
    int freq;
    unsigned short port_base_;
    unsigned int fake_ts;

    //for video
    int stap_a_sps_pps_len;
    unsigned char stap_a_sps_pps[stap_a_sps_pps_buf_size];
}rtp_session_t;

static int rtp_sendto(rtp_sender_t sender, const void* msg, int len,
                      unsigned int timestamp)
{
    int octets_sent;
    int pkt_len = len + RTP_HEADER_LEN;

    /* marshal data */
    memcpy(sender->message.body, (char*)msg, len);

    /* update header */
    sender->message.header.seq = ntohs(sender->message.header.seq) + 1;
    sender->message.header.seq = htons(sender->message.header.seq);
    sender->message.header.ts = htonl(timestamp);

    octets_sent = sendto(sender->socket, (void*)&sender->message,
        pkt_len, 0, (struct sockaddr*)&sender->addr,
        sizeof(struct sockaddr_in));
    if (octets_sent != pkt_len) {
        //perror("rtp_sendto() -- sendto failed");
    }
    return octets_sent;
}

static int rtp_sendto2(rtp_sender_t sender, const void* msg, int len,
                       const void *msg2,int len2,unsigned int timestamp)
{
    int octets_sent;
    int pkt_len = len + len2 + RTP_HEADER_LEN;

    /* marshal data */
    memcpy(sender->message.body, (char*)msg, len);
    memcpy(&sender->message.body[len], (char*)msg2, len2);
    /* update header */
    sender->message.header.seq = ntohs(sender->message.header.seq) + 1;
    sender->message.header.seq = htons(sender->message.header.seq);
    sender->message.header.ts = htonl(timestamp);

    octets_sent = sendto(sender->socket, (void*)&sender->message,
        pkt_len, 0, (struct sockaddr*)&sender->addr,
        sizeof(struct sockaddr_in));

    if (octets_sent != pkt_len) {
        perror("rtp_sendto2() -- sendto failed");
    }
    return octets_sent;
}

static int rtp_sender_init(rtp_sender_t sender, int sock,
                           struct sockaddr_in addr, unsigned int ssrc,
                           int payload_type)
{
    /* set header values */
    sender->message.header.ssrc = htonl(ssrc);
    sender->message.header.ts = 0;
    sender->message.header.seq = (unsigned short)rand();
    sender->message.header.m = 0;
    sender->message.header.pt = (unsigned char)payload_type;
    sender->message.header.version = 2;
    sender->message.header.p = 0;
    sender->message.header.x = 0;
    sender->message.header.cc = 0;

    /* set other stuff */
    sender->socket = sock;
    sender->addr = addr;
    return 0;
}

static rtp_sender_t rtp_sender_alloc(void) {
  return (rtp_sender_t)malloc(sizeof(rtp_sender_ctx_t));
}

static void rtp_sender_dealloc(rtp_sender_t rtp_ctx) {
  free(rtp_ctx);
}


static pthread_mutex_t  rtp_mutex = PTHREAD_MUTEX_INITIALIZER;
static int random_initialized = 0;
static unsigned short get_port_base(void){
    static unsigned short s_port_min = 50000;
    static unsigned short s_port_max = 60000;
    static unsigned short current_port  = 50000;

    unsigned short port_base;
    pthread_mutex_lock(&rtp_mutex);
    port_base = current_port;
    current_port += 2;
    if( current_port >= s_port_max){
        current_port = s_port_min;
    }
    pthread_mutex_unlock(&rtp_mutex);
    return port_base;
}
static int create_udp_socket(unsigned short port){
    struct sockaddr_in adr_inet;
    memset(&adr_inet,0,sizeof(adr_inet));
    adr_inet.sin_family = PF_INET;
    adr_inet.sin_port   = htons(port);
    adr_inet.sin_addr.s_addr = htonl(INADDR_ANY);
    int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1){
        return -1;
    }
    if(bind (sockfd, (struct sockaddr *) &adr_inet, sizeof (adr_inet)) < 0){
        close(sockfd);
        return -1;
    }
    return sockfd;
}

static void *rtp_session_init(char *address, unsigned short port, int payload_type,unsigned int ssrc){
    rtp_session_t *session = (rtp_session_t*)malloc(sizeof(rtp_session_t));
    if(!session) return (void*)0;

    int ret = -1;
    struct sockaddr_in name = {0};
    unsigned char ttl = 5;

    pthread_mutex_lock(&rtp_mutex);
    if(!random_initialized){
        srand(time(NULL));
        random_initialized = 1;
    }
    pthread_mutex_unlock(&rtp_mutex);

    /* set address */
#ifdef HAVE_INET_ATON
    if (0 == inet_aton(address, &session->rcvr_addr)) {
        fprintf(stderr, "cannot parse IP v4 address %s\n", address);
        exit(1);
    }
    if (session->rcvr_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "address error");
        exit(1);
    }
#else
    session->rcvr_addr.s_addr = inet_addr(address);
    if (0xffffffff == session->rcvr_addr.s_addr) {
        fprintf(stderr, "cannot parse IP v4 address %s\n", address);
        exit(1);
    }
#endif
    /* open socket */
    session->sock = -1;
    int max_count =  10000;
    while(max_count--){
        session->port_base_ = get_port_base();
        session->sock = create_udp_socket(session->port_base_);
        if(session->sock > 0){
            break;
        }
        close(session->sock),session->sock = -1;
    }
    if(session->sock == -1){
        return (void*)0;
    }

    name.sin_addr   = session->rcvr_addr;
    name.sin_family = PF_INET;
    name.sin_port   = htons(port);

    if (ADDR_IS_MULTICAST(session->rcvr_addr.s_addr)) {
        ret = setsockopt(session->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        if (ret < 0) {
            fprintf(stderr, "Failed to set TTL for multicast group\n");
            perror("");
            exit(1);
        }
        //struct ip_mreq mreq;
        session->mreq.imr_multiaddr.s_addr = session->rcvr_addr.s_addr;
        session->mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        ret = setsockopt(session->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&session->mreq,sizeof(session->mreq));
        if (ret < 0) {
            fprintf(stderr, "Failed to join multicast group\n");
            perror("");
            exit(1);
        }
    }

    session->ssrc_ = ssrc;

    /* initialize sender's rtp and srtp contexts */
    session->sender_ = rtp_sender_alloc();
    if (session->sender_ == NULL) {
      fprintf(stderr, "error: malloc() failed\n");
      exit(1);
    }
    rtp_sender_init(session->sender_, session->sock, name, session->ssrc_,payload_type);
    session->count_ = 0;
    return (void*)session;
}

static int rtp_session_uninit(void *session_){
    rtp_session_t *session = (rtp_session_t*)session_;
    if(session->sock != -1){
       if(session->sender_){
           rtp_sender_dealloc(session->sender_);
           session->sender_ = NULL;
       }
        if (ADDR_IS_MULTICAST(session->rcvr_addr.s_addr)) {
            setsockopt(session->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&session->mreq,sizeof(session->mreq));
        }
       close(session->sock);
       session->sock = -1;
       free(session);
   }
   return 0;
}

//------------------------------------------------------------------------------------------
//video process
//------------------------------------------------------------------------------------------
#define NAL_NON_IDR 1
#define NAL_IDR 5

#define MAX_VIDEO_FRAME_NUM 128

typedef struct _stream_context_video_t
{
    int stream_id;
    AMQueryFrameDesc  frame_queue[MAX_VIDEO_FRAME_NUM];
    int read_index;
    int write_index;
    int frame_num;
    int idr_frame_arrived;
    int first_frame_sent;
    int target_bitrate;
    int enable_lbr;
    const char *osd_label;
    pthread_t thread;
    volatile int thread_running;
    rtp_session_t *session;
    OryxVideoModule *late_module;
}stream_context_video_t;

typedef struct{
    int i_payload;
    unsigned char *p_payload;
    int nal_type;
} h264_nal_t;

static int parse_to_get_nals(unsigned char *data, unsigned len,
                             h264_nal_t *nals, int *nal_num,
                             unsigned char last_naltype){
    unsigned char *tmp_buffer_ = data;
    unsigned tmp_pos = 0;
    int nal_idx = 0;
    unsigned char nal_type;
    int get_nal_flag = 0;
    if(!tmp_buffer_) return -1;
    if(len <= 4) return -2;

    while(1){
        unsigned pos = tmp_pos + 4;
        while(pos < len - 4){
            if(tmp_buffer_[pos] == 0x00
                && tmp_buffer_[pos + 1] == 0x00
                && tmp_buffer_[pos + 2] == 0x00
                && tmp_buffer_[pos + 3] == 0x01){
                get_nal_flag = 1;
                break;
            }
            ++pos;
        }
        if(!get_nal_flag){
            printf("parse_to_get_nals, error bitstream ---"
                   "can not find any NAL unit.\n");
            break;
        }

        if(pos - tmp_pos >= 5){
            nal_type = tmp_buffer_[tmp_pos + 4] & 0x1F;
            if(nal_type == last_naltype){
                nals[nal_idx].i_payload = len - tmp_pos;
            }else{
               nals[nal_idx].i_payload = pos - tmp_pos;
            }
            nals[nal_idx].p_payload = &tmp_buffer_[tmp_pos];
            nals[nal_idx].nal_type = nal_type;
            ++nal_idx;
            tmp_pos  = pos;
        }else{
            *nal_num = nal_idx;
            printf("parse_to_get_nals, error bitstream --- "
                   "nal_idx = %d, pos = %d, len = %d\n",
                   nal_idx, pos, len);
            if(nal_idx >= 1){
                printf("nal_type = ");
                for(int i =0; i < nal_idx; i++)
                    printf(" %d ", nals[i].nal_type);
                printf("\n");
            }
            fflush(stdout);
            break;
        }
        if(nal_type == last_naltype){
            *nal_num = nal_idx;
            break;
        }
        if(nal_idx >= *nal_num){
            return -3;
        }
    }
    return 0;
}

static int nalSend(rtp_session_t *session, const unsigned char *buf,
                   int  size, long long timestamp){

    int nal_size = size;
    int nal_type = buf[0] & 0x1f;
    //SPS/PPS/IDR/non-IDR
    if(nal_type != 7 && nal_type != 8 && nal_type != 5 && nal_type != 1){
        return 0;
    }
    if (size <= MAX_RTP_PAYLOAD_LEN) {
        if(nal_type == 7){//SPS
            session->stap_a_sps_pps_len = 0;
            session->stap_a_sps_pps[0] = 0x78;//STAP-A
            session->stap_a_sps_pps[1] = (size >> 8) & 0xff;
            session->stap_a_sps_pps[2] = (size >> 0) & 0xff;
            memcpy(&session->stap_a_sps_pps[3],buf,size);
            session->stap_a_sps_pps_len += 3 + size;
            return 0;
        }else if(nal_type == 8){//PPS
            unsigned char *buf_ =
                &session->stap_a_sps_pps[session->stap_a_sps_pps_len];
            buf_[0] = (size >> 8) & 0xff;
            buf_[1] = (size >> 0) & 0xff;
            memcpy(&buf_[2],buf,size);
            session->stap_a_sps_pps_len += 2 + size;
            session->sender_->message.header.m = 1;
            rtp_sendto(session->sender_, session->stap_a_sps_pps,
                       session->stap_a_sps_pps_len,(unsigned int)timestamp);
        }else{
            session->sender_->message.header.m = 1;
            rtp_sendto(session->sender_, buf, size,(unsigned int)timestamp);
        }
        ++session->count_;
        if(session->count_ > 10){
           session->count_ = 0;
           while(usleep(1000) < 0 && errno == EINTR);
        }
    } else {
        unsigned char type = buf[0] & 0x1F;
        unsigned char nri = buf[0] & 0x60;
        unsigned char indicator = 28 | nri;
        unsigned char  fu_header = type | (1 << 7);
        buf += 1;
        size -= 1;
        while (size - 2> MAX_RTP_PAYLOAD_LEN) {
            unsigned char rtp_buf[2];
            rtp_buf[0] =  indicator;
            rtp_buf[1] = fu_header;
            session->sender_->message.header.m = 0;
            rtp_sendto2(session->sender_, rtp_buf, 2, buf,
                        MAX_RTP_PAYLOAD_LEN - 2,
                        (unsigned int)timestamp);
            buf += MAX_RTP_PAYLOAD_LEN - 2;
            size -= MAX_RTP_PAYLOAD_LEN - 2;
            fu_header &= ~(1 << 7);

            ++session->count_;
            if(session->count_ > 10){
               session->count_ = 0;
               while(usleep(1000) < 0 && errno == EINTR);
           }
        }
        fu_header |= 1 << 6;

        unsigned char rtp_buf[2];
        rtp_buf[0] = indicator;
        rtp_buf[1] = fu_header;
        session->sender_->message.header.m = 1;
        rtp_sendto2(session->sender_, rtp_buf, 2, buf, size,
                    (unsigned int)timestamp);
        ++session->count_;
        if(session->count_ > 10){
           session->count_ = 0;
           while(usleep(1000) < 0 && errno == EINTR);
        }
    }
    return nal_size;
}

static int rtp_send_h264(void *session_, AMQueryFrameDesc *desc,
                         AMIVideoAddressPtr address){
    rtp_session_t *session =  (rtp_session_t *)session_;
    h264_nal_t nals[8];
    int nal_num = 8;
    int i;
    unsigned char last_naltype =
        (desc->video.type != AM_VIDEO_FRAME_TYPE_IDR) ? NAL_NON_IDR : NAL_IDR;
    AMAddress video_addr;
    if (address->video_addr_get(*desc, video_addr)!= AM_RESULT_OK) {
        printf("Failed to get the address.\n");
        return -1;
    }
    if((i = parse_to_get_nals(video_addr.data, desc->video.data_size,
                              nals, &nal_num, last_naltype)) < 0){
        printf("rtp_send_h264 --- "
               "failed to parse_to_get_nals, ret %d, nal_num = %d\n",
               i, nal_num);
        fflush(stdout);
        return -1;
    }
    int sent_size = 0;
    for(i = 0; i < nal_num; i++){
        sent_size += nalSend(session, nals[i].p_payload + 4,
                             nals[i].i_payload - 4, desc->pts);
    }
    return 0;
}

static inline int meet_start_byte(const unsigned char *stream_head)
{
    constexpr unsigned int start_code = 0x01000000;
    if (*(int*)stream_head == start_code) {
        return 1;
    }

    return 0;
}

/* strip off 0x00 0x00 0x00 0x01 from bit stream */
static int parse_to_get_hevc_nals(const unsigned char *stream_buf,
                                  const unsigned int stream_len,
                                  h264_nal_t *nals, int *nal_num)
{
    constexpr unsigned int NALU_START_BYTE_LEN = 4;
    unsigned int byte_pos = 0;
    unsigned int prev_nalu_pos = 0;
    int _nal_num = 0;
    int ret = -1;

    while (byte_pos < stream_len - NALU_START_BYTE_LEN) {
        if (meet_start_byte(&stream_buf[byte_pos])) {
            nals[_nal_num].p_payload =
                (unsigned char*)&stream_buf[byte_pos + NALU_START_BYTE_LEN];
            nals[_nal_num].nal_type =
                (stream_buf[byte_pos + NALU_START_BYTE_LEN] & 0x7E) >> 1;
            if (_nal_num > 0) {
                nals[_nal_num - 1].i_payload =
                    byte_pos - prev_nalu_pos - NALU_START_BYTE_LEN;
            }
            prev_nalu_pos = byte_pos;
            _nal_num++;
            byte_pos += 4;
        } else {
            byte_pos++;
            continue;
        }
    }

    if (_nal_num > 0 && _nal_num < *nal_num) {
        nals[_nal_num - 1].i_payload =
            stream_len - prev_nalu_pos - NALU_START_BYTE_LEN;
        ret = 0;
    }
    *nal_num = _nal_num;
    return ret;
}

static int send_hevc_fu(rtp_session_t *session, const unsigned char *buf,
                        int nal_size, long long timestamp, int nal_type)
{
    constexpr int FU_HDR_LEN = 3;
    constexpr int FU_PAYLOAD_LEN = MAX_RTP_PAYLOAD_LEN - FU_HDR_LEN;
    constexpr unsigned char FU_TYPE = 49;
    unsigned char fu_hdr[FU_HDR_LEN] = {0};
    int sent_size = -1;
    fu_hdr[0] = (buf[0] & 0x81) | (FU_TYPE << 1);
    fu_hdr[1] = buf[1];
    fu_hdr[2] = 0x80 | nal_type;

    buf += 2;
    nal_size -= 2;
    while (nal_size > FU_PAYLOAD_LEN) {
        session->sender_->message.header.m = 0;
        sent_size = rtp_sendto2(session->sender_, fu_hdr, FU_HDR_LEN, buf,
                          FU_PAYLOAD_LEN, (unsigned int)timestamp);
        buf += FU_PAYLOAD_LEN;
        nal_size -= FU_PAYLOAD_LEN;
        fu_hdr[2] &= 0x3F;
        ++session->count_;
        if (session->count_ > 10) {
            session->count_ = 0;
            while (usleep(1000) < 0 && errno == EINTR);
        }
    }

    fu_hdr[2] = 0x40 | nal_type;
    session->sender_->message.header.m = 1;
    sent_size = rtp_sendto2(session->sender_, fu_hdr, FU_HDR_LEN, buf, nal_size,
                      (unsigned int)timestamp);
    ++session->count_;
    if (session->count_ > 10) {
        session->count_ = 0;
        while (usleep(1000) < 0 && errno == EINTR);
    }

    return sent_size;
}

static int send_hevc_single(rtp_session_t *session, const unsigned char *buf,
                            int nal_size, long long timestamp)
{
    int sent_size = -1;
    session->sender_->message.header.m = 1;
    sent_size = rtp_sendto(session->sender_, buf, nal_size, timestamp);
    ++session->count_;

    return sent_size;
}

static int send_hevc_nal(rtp_session_t *session, h264_nal_t *nals,
                         long long timestamp)
{
    int sent_size = -1;
    if (nals->i_payload > MAX_RTP_PAYLOAD_LEN){
        sent_size = send_hevc_fu(session, nals->p_payload, nals->i_payload,
                                 timestamp, nals->nal_type);
    } else {
        sent_size = send_hevc_single(session, nals->p_payload, nals->i_payload,
                                     timestamp);
    }

    return sent_size;
}

static int rtp_send_h265(void *session_, AMQueryFrameDesc *desc,
                         AMIVideoAddressPtr address){
    constexpr int MAX_NALU_NUM = 10;
    rtp_session_t* session = (rtp_session_t*)session_;
    h264_nal_t nals[MAX_NALU_NUM] = {0};
    int nal_num = MAX_NALU_NUM;
    int sent_size = -1;

    AMAddress video_addr;
    if (address->video_addr_get(*desc, video_addr) != AM_RESULT_OK) {
        printf("Failed to get the address.\n");
        return sent_size;
    }
    if (parse_to_get_hevc_nals(video_addr.data, desc->video.data_size,
            nals, &nal_num) < 0) {
        printf("%s --- failed to parse nal, nal_num = %d\n", __func__, nal_num);
        return sent_size;
    }
    for (int i = 0; i < nal_num; i++) {
        if (nals[i].nal_type == 0x23) continue;  // jump delimiter
        sent_size += send_hevc_nal(session, &nals[i], desc->pts);
    }
    return sent_size;
}

static void video_queue_append(stream_context_video_t *context,
                               AMQueryFrameDesc *desc)
{
    if(context->frame_num == MAX_VIDEO_FRAME_NUM){
         context->idr_frame_arrived = 0;
         AMQueryFrameDesc *desc_ = &context->frame_queue[context->read_index];
         if(desc_->video.type == AM_VIDEO_FRAME_TYPE_IDR){
             context->read_index =
                 (context->read_index + 1) % MAX_VIDEO_FRAME_NUM;
             -- context->frame_num;
         }

         int frame_num = context->frame_num;
         int i;
         for(i = 0 ; i  < frame_num; i++){
             desc_ = &context->frame_queue[context->read_index];
             if(desc_->video.type != AM_VIDEO_FRAME_TYPE_IDR){
                 context->read_index =
                     (context->read_index + 1) % MAX_VIDEO_FRAME_NUM;
                 -- context->frame_num;
             }else{
                 context->idr_frame_arrived += 1;
                 break;
             }
         }
    }
    if(!context->idr_frame_arrived){
         if(desc->video.type != AM_VIDEO_FRAME_TYPE_IDR){
             return;
         }
        context->idr_frame_arrived += 1;
    }
    memcpy(&context->frame_queue[context->write_index],
           desc, sizeof(AMQueryFrameDesc));
    context->write_index = (context->write_index +1)% MAX_VIDEO_FRAME_NUM;
    ++ context->frame_num;
}

AMQueryFrameDesc *video_queue_get(stream_context_video_t *context)
{
    if(context->frame_num > 0){
         AMQueryFrameDesc *packet = &context->frame_queue[context->read_index];
         context->read_index = (context->read_index + 1) % MAX_VIDEO_FRAME_NUM;
         -- context->frame_num;
         return packet;
    }
    return NULL;
}

static int streaming_queue_frame(stream_context_video_t *context,
                                 AMQueryFrameDesc *desc)
{
    video_queue_append(context,desc);
    return 0;
}

static void show_h264_sdp_config(unsigned char *stap_a);
static int streaming_send_frames(stream_context_video_t *handle,
                                 AMIVideoAddressPtr address)
{
    int ret = 0;
    for (int i = 0; i < 2; i++) {
        AMQueryFrameDesc *desc = video_queue_get(handle);
        if (desc) {
            if (desc->video.stream_type == AM_STREAM_TYPE_H264) {
                rtp_send_h264(handle->session, desc, address);
            } else if(desc->video.stream_type == AM_STREAM_TYPE_H265) {
                rtp_send_h265(handle->session, desc, address);
            } else {
                printf("streaming_send_frames -- stream type not suppoted.\n");
                ret = -1;
                break;
            }
            if (!handle->first_frame_sent) {
                printf("streaming_send_frames --- first_frame_sent, "
                       "frame_num = %d, current_time= %llu, pts = %" PRId64 "\n",
                       handle->frame_num, get_current_time(), desc->pts);
                //to do, add show_h265_sdp_config
                if(desc->video.stream_type == AM_STREAM_TYPE_H264)
                    show_h264_sdp_config(handle->session->stap_a_sps_pps);
                handle->first_frame_sent = 1;
            }
        }
    }
    return ret;
}

static int handle_predata(stream_context_video_t *handle,
                          AMIVideoReaderPtr reader)
{
    const int MAX_PREDATA_NUM = 300;
    AMQueryFrameDesc *frame_desc_pre = new AMQueryFrameDesc[MAX_PREDATA_NUM];
    if(!frame_desc_pre){
        printf("handle_predata --- failed to alloc memory\n");
        return -1;
    }
    unsigned long long video_prev_ms = 0;
    int index = 0, start = -1;
    unsigned long long now;
    AM_RESULT result;
    while(1){
        now = current_time_ms();
        result = reader->query_video_frame(frame_desc_pre[index],0);
        if(result == AM_RESULT_OK
            && frame_desc_pre[index].video.stream_id
                == (unsigned int)handle->stream_id){
            if(frame_desc_pre[index].video.type == AM_VIDEO_FRAME_TYPE_IDR){
                start = index;
            }
            index++;
            if(index == MAX_PREDATA_NUM){
                index = 0;
                continue;
            }
            if(video_prev_ms && now - video_prev_ms > 10){
                break;
            }
            video_prev_ms = now;
        }else{
            break;
        }
    }

    if(start == -1){
        printf("handle_predata [%d frames]--- No key frame found\n", index);
        delete [] frame_desc_pre;
        return -1;
    }
    if(start < index){
        while(start < index){
            streaming_queue_frame(handle, &frame_desc_pre[start]);
            start++;
        }
    }else{
        while(start < MAX_PREDATA_NUM){
            streaming_queue_frame(handle, &frame_desc_pre[start]);
            start++;
        }
        start = 0;
        while(start < index){
            streaming_queue_frame(handle, &frame_desc_pre[start]);
            start++;
        }
    }

    delete [] frame_desc_pre;
    return 0;
}

static int start_late_modules(stream_context_video_t *param)
{
    OryxVideoModule *late_module = param->late_module;
    if (!param->enable_lbr) {
        late_module->set_bitrate(param->target_bitrate);
    }

    late_module->start_smart_avc(param->enable_lbr);
    late_module->start_overlay(param->osd_label);
    late_module->start_linux_aaa();

    return 0;
}

static int stop_late_modules(OryxVideoModule *late_module)
{
    late_module->stop_smart_avc();
    late_module->stop_overlay();
    late_module->stop_linux_aaa();
    late_module->stop_camera();

    return 0;
}

static void* video_thread_routine(void* param)
{
    stream_context_video_t *handle = (stream_context_video_t*)param;
    AM_RESULT result = AM_RESULT_OK;
    AMQueryFrameDesc frame_desc;

    setscheduler(10);
    AMIVideoReaderPtr reader;
    AMIVideoAddressPtr address;
    if (!(reader = AMIVideoReader::get_instance())) {
        printf("video_thread_routine -- Failed to create greader!\n");
        return (void*)NULL;
    }
    if (!(address = AMIVideoAddress::get_instance())) {
        printf("video_thread_routine --- Failed to get instance of VideoAddress!\n");
        return (void*)NULL;
    }
    if (handle_predata(handle,reader) < 0) {
        printf("video_thread_routine --- Failed to handle_predata!\n");
        return (void*)NULL;
    }
    signal(SIGPIPE, SIG_IGN);
    while (handle->thread_running) {
        streaming_send_frames(handle, address);
        result = reader->query_video_frame(frame_desc, 0);
        if (result == AM_RESULT_OK &&
            frame_desc.video.stream_id == (unsigned int) handle->stream_id) {
            streaming_queue_frame(handle, &frame_desc);
        } else if (result != AM_RESULT_ERR_AGAIN) {
            printf("Failed to query video frame.\n");
            break;
        }
        while(usleep(1000) < 0 && errno == EINTR);
    }

    return (void*)NULL;
}

//------------------------------------------------------------------------------------------
//audio process
//------------------------------------------------------------------------------------------
#define MAX_AUDIO_CHUNK_SIZE 2048
typedef struct _audio_frame_packet_t{
    unsigned char data[MAX_AUDIO_CHUNK_SIZE];
    int size;
    long long pts_90k;
}audio_frame_packet_t;

typedef int (*rtp_send_audio)(rtp_session_t *session,unsigned char *data,int data_size,unsigned int pts_inc);

#define MAX_AUDIO_FRAME_NUM  64
typedef struct  _stream_context_audio_t
{
    char codec_name[16];
    audio_frame_packet_t  *frame_queue;
    int read_index;
    int write_index;
    int frame_num;
    AMIAudioCapture *input;
    AMPlugin *plugin;
    AM_AUDIO_INFO codec_required_info;
    rtp_send_audio  rtp_send;
    unsigned int pts_inc;
    AMIAudioCodec *encoder;
    unsigned char result_buf[MAX_AUDIO_CHUNK_SIZE];
    pthread_t  thread;
    volatile int thread_running;
    int predata_processed;
    unsigned long long prev_ms;
    rtp_session_t *session;
}stream_context_audio_t;

static void audio_callback(AudioCapture *data);
static bool initialize_audio_encoder(stream_context_audio_t *handle);
static audio_frame_packet_t *audio_queue_get(stream_context_audio_t *context,int *frame_num);
static int rtp_send_raw(rtp_session_t *session,unsigned char *data,int data_size,unsigned int pts_inc);
static int rtp_send_aac(rtp_session_t *session,unsigned char *data,int data_size,unsigned int pts_inc);
static void* audio_thread_routine(void* param)
{
    stream_context_audio_t *handle = (stream_context_audio_t*)param;
    AM_AUDIO_INFO *info = &handle->codec_required_info;
    int first_frame_sent = 0;
    AM_AUDIO_CODEC_TYPE codec_type;

    setscheduler(10);

    printf("audio_thread_routine --start --current_time= %llu\n",get_current_time());

    handle->predata_processed = 0;
    handle->prev_ms = 0;
    if(!initialize_audio_encoder(handle)){
        printf("audio_thread_routine -- initialize_audio_encoder  failed\n");
        goto err_exit;
    }
    if(info->chunk_size > MAX_AUDIO_CHUNK_SIZE){
        printf("audio_thread_routine -- audio-chunk-size allocated is too small, %d wanted\n",info->chunk_size);
        goto err_exit;
    }

    codec_type = handle->encoder->get_codec_type();
    switch(codec_type){
    case AM_AUDIO_CODEC_AAC:
        handle->rtp_send = rtp_send_aac;
        handle->pts_inc = 1024;
        break;
    case AM_AUDIO_CODEC_G711:
    case AM_AUDIO_CODEC_G726:
    case AM_AUDIO_CODEC_OPUS:
        handle->rtp_send = rtp_send_raw;
        handle->pts_inc = info->chunk_size/info->channels/sizeof(short);//TODO
        break;
    default:
        handle->rtp_send = NULL;
        handle->pts_inc = 0;
        break;
    }
    if(!handle->rtp_send){
        printf("audio_thread_routine -- audio codec type [%d] not supported\n",info->type);
        goto err_exit;
    }
    printf("Audio INFO: channels %d, sample-rate %d, chunk_size_bytes %d,pts_inc = %u\n",info->channels,info->sample_rate,info->chunk_size,handle->pts_inc);

    handle->frame_queue = new audio_frame_packet_t[MAX_AUDIO_FRAME_NUM];
    if(!handle->frame_queue){
        printf("audio_thread_routine -- failed to alloc memory\n");
        goto err_exit;
    }
    handle->input  = create_audio_capture("raw","oryx_module_capture",handle,audio_callback);
    if(!handle->input) {
        printf("audio_thread_routine -- Failed to create audio capture with interface [raw]\n");
        goto err_exit;
    }
    if (!handle->input->set_channel(info->channels) ||
        !handle->input->set_sample_rate(info->sample_rate)||
        !handle->input->set_chunk_bytes(info->chunk_size) ||
        !handle->input->set_sample_format((AM_AUDIO_SAMPLE_FORMAT)info->sample_format /*AM_SAMPLE_S16LE*/) ){
        printf("Set audio parameter failed\n");
        goto err_exit;
    }
    if (!handle->input->start(90/*volume, not used*/)) {
        printf("Start audio capture failed\n");
        goto err_exit;
    }
    printf("audio_thread_routine --audio-capture start --current_time= %llu\n",get_current_time());

    signal(SIGPIPE, SIG_IGN);
    while(handle->thread_running){
        int frame_num;
        audio_frame_packet_t *packet = audio_queue_get(handle,&frame_num);
        if(packet){
            unsigned int out_size;
            if(!first_frame_sent){
                printf("audio_thread_routine --get first packet -- current_time = %llu\n",get_current_time());
            }
            if (handle->encoder->encode(packet->data, info->chunk_size,handle->result_buf,&out_size)) {
                (* handle->rtp_send)(handle->session,handle->result_buf,out_size,handle->pts_inc);
                if(!first_frame_sent){
                     printf("audio_thread_routine ---first_frame_sent , frame_num = %d, current_time= %llu\n",frame_num,get_current_time());
                     first_frame_sent = 1;
                }else{
                     //printf("audio_thread_routine --- frame_num = %d, current_time_ms= %llu\n",frame_num,current_time_ms());
                }
            }
        }
        //while(usleep(1000) < 0 && errno == EINTR);
    }
err_exit:
    if(handle->input){
        handle->input->stop();
        handle->input->destroy();
    }
    if(handle->encoder){
        handle->encoder->destroy(),handle->encoder = NULL;
    }
    if(handle->plugin){
        handle->plugin->destroy(),handle->plugin = NULL;
    }
    if(handle->frame_queue){
        delete [](handle->frame_queue);
    }
    return (void*)NULL;
}

#ifdef BUILD_AMBARELLA_ORYX_CODEC_DIR
#define ORYX_CODEC_DIR ((const char*)BUILD_AMBARELLA_ORYX_CODEC_DIR)
#else
#define ORYX_CODEC_DIR ((const char*)"/usr/lib/oryx/codec")
#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_CODEC_CONF_DIR \
  (const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/codec")
#else
#define ORYX_CODEC_CONF_DIR ((const char*)"/etc/oryx/stream/codec")
#endif

static bool initialize_audio_encoder(stream_context_audio_t *handle)
{
    bool ret = false;
    std::string codec = ORYX_CODEC_DIR;
    codec.append("/codec-").append(handle->codec_name).append(".so");
    handle->plugin = AMPlugin::create(codec.c_str());
    if (handle->plugin) {
        char *codec_name = handle->codec_name;
        AudioCodecNew get_audio_codec = (AudioCodecNew)handle->plugin->get_symbol(AUDIO_CODEC_NEW);
        if (get_audio_codec) {
            std::string codecConf = ORYX_CODEC_CONF_DIR;
            codecConf.append("/codec-").append(codec_name).append(".acs");
            handle->encoder = get_audio_codec(codecConf.c_str());
            if (handle->encoder) {
                /* Setup audio source parameters */
                handle->codec_required_info.channels = 1;
                handle->codec_required_info.sample_format = AM_SAMPLE_S16LE;
                handle->codec_required_info.sample_size = 2;
                if(!strcmp(codec_name, "aac-48k")){
                    handle->codec_required_info.type = AM_AUDIO_AAC;
                    handle->codec_required_info.sample_rate = 48000;
                }else if(!strcmp(codec_name, "aac-16k")){
                    handle->codec_required_info.type = AM_AUDIO_AAC;
                    handle->codec_required_info.sample_rate = 16000;
                }else if(!strcmp(codec_name, "g711-8k")){
                    handle->codec_required_info.type = AM_AUDIO_G711A;
                    handle->codec_required_info.sample_rate = 8000;
                }else{
                    printf("unknown codec name:%s\n", codec_name);
                }

                if (handle->encoder->check_encode_src_parameter(handle->codec_required_info)) {
                    handle->codec_required_info.chunk_size = handle->encoder->get_encode_required_chunk_size(handle->codec_required_info);
                    //printf("Audio codec %s is loaded!\n",codec_name);
                    if (handle->encoder->initialize(&handle->codec_required_info, AM_AUDIO_CODEC_MODE_ENCODE)) {
                        //printf("initialize codec %s, done!\n", codec_name);
                        ret = true;
                    }else{
                        printf("Failed to initialize codec %s, abort!\n", codec_name);
                    }
                } else {
                    printf("Failed to get codec %s required audio parameters!\n",codec_name);
                }
            } else {
                printf("Failed to load audio codec %s!\n", codec_name);
            }
        } else {
            printf("Failed to get symbol (%s) from %s!\n", AUDIO_CODEC_NEW, codec.c_str());
        }
    }
    return ret;
}

static int rtp_send_raw(rtp_session_t *session,unsigned char *data,int data_size,unsigned int pts_inc)
{
    session->sender_->message.header.m = 1;
    rtp_sendto(session->sender_,data,data_size,session->fake_ts);
    session->fake_ts += pts_inc;
    return 0;
}

static void show_aac_sdp_config(unsigned char *aac_adts_header);
static int rtp_send_aac(rtp_session_t *session,unsigned char *data,int data_size,unsigned int pts_inc)
{
    static int count = 0;
    if(!count){
        show_aac_sdp_config(data);
        count = 1;
    }
    int size = data_size - 7;
    unsigned char rtp_buf[4];
    rtp_buf[0] = 0x00;
    rtp_buf[1] = 0x10;
    rtp_buf[2] = (size & 0x1fe0) >> 5;
    rtp_buf[3] = (size & 0x1f) << 3;
    session->sender_->message.header.m = 1;
    rtp_sendto2(session->sender_, rtp_buf,4,data + 7, size,(unsigned int)session->fake_ts);
    session->fake_ts += pts_inc;
    //printf("rtp_send_aac ---- packet_size = %d, packet_pts = %u, curent_time = %u\n",size + 4,  session->fake_ts,get_current_time());
    return 0;
}

static pthread_mutex_t aq_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t aq_cond = PTHREAD_COND_INITIALIZER;
static audio_frame_packet_t *audio_queue_get(stream_context_audio_t *context, int *frame_num)
{
    pthread_mutex_lock(&aq_mutex);
    while (context->frame_num <= 0) {
        pthread_cond_wait(&aq_cond, &aq_mutex);
    }
    if(context->frame_num){
        audio_frame_packet_t *packet = &context->frame_queue[context->read_index];
        context->read_index = (context->read_index + 1) % MAX_AUDIO_FRAME_NUM;
        -- context->frame_num;
        *frame_num = context->frame_num;
        pthread_mutex_unlock(&aq_mutex);
        return packet;
    }
    pthread_mutex_unlock(&aq_mutex);
    return NULL;
}

static int audio_queue_frame(stream_context_audio_t *context,AudioPacket *packet)
{
    /*streaming mode, discard all the pre-captured sampels
    */
    if(!context->predata_processed){
        unsigned long long now = current_time_ms();//get_current_time();
        if(context->prev_ms  && now - context->prev_ms > 10){
            context->predata_processed  = 1;
        }
        context->prev_ms = now;
    }
    if(context->predata_processed){
        pthread_mutex_lock(&aq_mutex);
        if(context->frame_num == MAX_AUDIO_FRAME_NUM){
            context->read_index = 0;
            context->write_index = 0;
            context->frame_num = 0;
            printf("audio_queue_frame --- flush data\n");
        }
        memcpy(context->frame_queue[context->write_index].data,(void*)packet->data,packet->length);
        context->frame_queue[context->write_index].size  = packet->length;
        context->frame_queue[context->write_index].pts_90k = packet->pts;
        context->write_index = (context->write_index +1)% MAX_AUDIO_FRAME_NUM;
        ++ context->frame_num;
        pthread_cond_signal(&aq_cond);
        pthread_mutex_unlock(&aq_mutex);
    }
    return 0;
}

static void audio_callback(AudioCapture *data){
    stream_context_audio_t *handle = (stream_context_audio_t*)data->owner;
    audio_queue_frame(handle,&data->packet);
}

//-------------------------------------------------------------------------
// interface implementation
//-------------------------------------------------------------------------
static stream_context_video_t   s_stream_context_video;
static stream_context_audio_t   s_stream_context_audio;
static transport_info_t s_transport_info;

int streaming_initialize(bpi_streamer_info *streamer_info)
{
    OryxVideoModule *video_module = OryxVideoModule::get_instance();
    if (!video_module->start_camera()){
        printf("failed to start video camera.\n");
        return -1;
    }
    memset(&s_stream_context_video, 0, sizeof(s_stream_context_video));
    memset(&s_stream_context_audio, 0, sizeof(s_stream_context_audio));
    s_stream_context_video.stream_id = streamer_info->stream_id;
    s_stream_context_video.target_bitrate = streamer_info->target_bitrate;
    s_stream_context_video.enable_lbr = streamer_info->enable_lbr;
    s_stream_context_video.osd_label = streamer_info->osd_label;
    s_stream_context_video.late_module = video_module;
    snprintf(s_stream_context_audio.codec_name,
             sizeof(s_stream_context_audio.codec_name),
             "%s",
             streamer_info->audio_codec_name);
    memcpy(&s_transport_info, streamer_info->transport_info,
           sizeof(transport_info_t));

    if (streamer_info->transport_info->video.enable) {
        s_stream_context_video.session = (rtp_session_t*)
            rtp_session_init(s_transport_info.ipaddress,
                             s_transport_info.video.port,
                             s_transport_info.video.payload_type,
                             s_transport_info.video.ssrc);
        if (!s_stream_context_video.session) {
            printf("video rtp_session_init  failed 1\n");
            streaming_finalize();
            return -1;
        }
    }
    if (streamer_info->transport_info->audio.enable) {
        s_stream_context_audio.session = (rtp_session_t*)
            rtp_session_init(s_transport_info.ipaddress,
                             s_transport_info.audio.port,
                             s_transport_info.audio.payload_type,
                             s_transport_info.audio.ssrc);
        if (!s_stream_context_audio.session) {
            printf("audio rtp_session_init failed 1\n");
            streaming_finalize();
            return -1;
        }
    }

    int ret;
    if (streamer_info->transport_info->video.enable) {
        s_stream_context_video.thread_running = 1;
        ret = pthread_create(&s_stream_context_video.thread, NULL,
                             video_thread_routine,
                             (void*) &s_stream_context_video);
        if (ret != 0) {
            printf("fail to spawn video thread\n");
            streaming_finalize();
            return -1;
        }
    }
    if (streamer_info->transport_info->audio.enable) {
        s_stream_context_audio.thread_running = 1;
        ret = pthread_create(&s_stream_context_audio.thread, NULL,
                             audio_thread_routine,
                             (void*) &s_stream_context_audio);
        if (ret != 0) {
            printf("fail to spawn audio thread\n");
            streaming_finalize();
            return -1;
        }
    }
    if(s_stream_context_video.thread_running){
        start_late_modules(&s_stream_context_video);
    }

    return 0;
}

int streaming_finalize(void)
{
    printf("%s:called\n", __func__);
    s_stream_context_video.thread_running = 0;
    if(s_stream_context_video.thread){
        pthread_join(s_stream_context_video.thread, NULL);
    }
    s_stream_context_audio.thread_running = 0;
    if(s_stream_context_audio.thread){
        pthread_join(s_stream_context_audio.thread, NULL);
    }

    if(s_stream_context_video.session){
        rtp_session_uninit(s_stream_context_video.session);
    }
    if(s_stream_context_audio.session){
        rtp_session_uninit(s_stream_context_audio.session);
    }
    stop_late_modules(s_stream_context_video.late_module);

    printf("%s:done\n", __func__);
    return 0;
}

static void show_h264_sdp_config(unsigned char *stap_a){
    // Generate a new "a=fmtp:" line each time, using our SPS and PPS
    unsigned sps_size = (stap_a[1] << 8) + (stap_a[2]);
    unsigned char *sps = &stap_a[3];
    unsigned pps_size = (stap_a[3 + sps_size] << 8) + (stap_a[3 + sps_size + 1]);
    unsigned char *pps = &stap_a[3 + sps_size + 2];
    unsigned int profileLevelId = (sps[1]<<16) | (sps[2]<<8) | sps[3];
    char* sps_base64 = base64_encode((char*)sps, sps_size);
    char* pps_base64 = base64_encode((char*)pps, pps_size);
    char const* fmtp_fmt =
        "a=fmtp:%d packetization-mode=1"
        ";profile-level-id=%06X"
        ";sprop-parameter-sets=%s,%s\r\n";
    unsigned fmtp_fmt_size = strlen(fmtp_fmt)  + 3 /* max char len */ + 6 /* 3 bytes in hex */ + strlen(sps_base64) + strlen(pps_base64);
    char* fmtp = new char[fmtp_fmt_size];
    sprintf(fmtp, fmtp_fmt, s_transport_info.video.payload_type, profileLevelId, sps_base64, pps_base64);
    delete[] sps_base64;
    delete[] pps_base64;

    printf("h264_sdp_config\n");
    printf("\tm=video %d RTP/AVP %d\n",s_transport_info.video.port,s_transport_info.video.payload_type);
    printf("\ta=rtpmap:%d H264/90000\n",s_transport_info.video.payload_type);
    printf("\t%s\n",fmtp);
    delete []fmtp;
}

static void show_aac_sdp_config(unsigned char *aac_adts_header){
    unsigned char *hdr = aac_adts_header;
    unsigned int  syncword = (hdr[0] << 4) | (hdr[1] >> 4) ;// # bslbf(12)
    if(syncword != 0b111111111111){
       printf("show_aac_sdp_config -- Invalid syncword");
       exit(1);
    }
#if 0
    unsigned char ID  = (hdr[1] >> 3) & 0b1;//    # bslbf(1)
    unsigned char layer  = (hdr[1] >> 1) & 0b11;//   # uimsbf(2)
    unsigned char protection_absent  = (hdr[1]) & 0b1;//    # bslbf(1)
    unsigned char profile            = (hdr[2] >> 6) & 0b11;//   # uimsbf(2)
    unsigned char sampling_freq_idx  = (hdr[2] >> 2) & 0b1111;// # uimsbf(4)
    unsigned char private_bit        = (hdr[2] >> 1) & 0b1;//    # bslbf(1)
    unsigned char channel_cfg        = ((hdr[2] & 0b1) << 2) | (hdr[3] >> 6);//  # uimsbf(3)
    unsigned char original_copy      = (hdr[3] >> 5) & 0b1;//    # bslbf(1)
    unsigned char home               = (hdr[3] >> 4) & 0b1;//    # bslbf(1)
    //parse adts_variable_header()
    unsigned char copyright_id_bit   = (hdr[3] >> 3) & 0b1;//    # bslbf(1)
    unsigned char copyright_id_start = (hdr[3] >> 2) & 0b1;//    # bslbf(1)
    unsigned int frame_length       = ((hdr[3] & 0b11) << 11) | (hdr[4] << 3) | (hdr[5] >> 5);//  # bslbf(13)
    unsigned int adts_buf_fullness  = ((hdr[5] & 0b11111) << 6) | (hdr[6] >> 2);//  # bslbf(11)
    unsigned int num_rawdata_blocks = (hdr[6]     ) & 0b11;//   # uimsbf(2)
#else
    unsigned char profile            = (hdr[2] >> 6) & 0b11;//   # uimsbf(2)
    unsigned char sampling_freq_idx  = (hdr[2] >> 2) & 0b1111;// # uimsbf(4)
    unsigned char channel_cfg        = ((hdr[2] & 0b1) << 2) | (hdr[3] >> 6);//  # uimsbf(3)
#endif
    unsigned char audioSpecificConfig[2];
    unsigned char audioObjectType = profile + 1;
    audioSpecificConfig[0] = (audioObjectType<<3) | (sampling_freq_idx>>1);
    audioSpecificConfig[1] = (sampling_freq_idx<<7) | (channel_cfg <<3);
    char fConfigStr[32];
    sprintf(fConfigStr, "%02X%02X", audioSpecificConfig[0], audioSpecificConfig[1]);

    static unsigned const samplingFrequencyFromIndex[16] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,  16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };
    printf("aac_sdp_config\n");
    printf("\tm=audio %d RTP/AVP %d\n",s_transport_info.audio.port,s_transport_info.audio.payload_type);
    printf("\ta=rtpmap:%d mpeg4-generic/%d/%d\n",s_transport_info.audio.payload_type,samplingFrequencyFromIndex[sampling_freq_idx],channel_cfg);
    printf("\ta=fmtp:%d streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s;SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=%d\n",s_transport_info.audio.payload_type,fConfigStr,profile);
}
