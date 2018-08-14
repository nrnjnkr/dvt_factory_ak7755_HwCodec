/*
 * streaming_rtp_over_udp.h
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
#ifndef _STREAMING_RTP_OVER_UDP_H_
#define _STREAMING_RTP_OVER_UDP_H_

#include "am_export_if.h"
#include "bpi_oryx_export.h"

typedef struct _av_info{
    unsigned short port;
    int payload_type;
    unsigned int ssrc;
    int enable;
}av_info_t;

typedef struct transport_info_t{
    char ipaddress[128];
    int is_ipv4;
    av_info_t video;
    av_info_t audio;
}transport_info_t;

struct bpi_streamer_info {
    int stream_id;
    int target_bitrate;
    int enable_lbr;
    char *audio_codec_name;
    const char *osd_label;
    transport_info_t *transport_info;
};

int streaming_initialize(bpi_streamer_info *streamer_info);
int streaming_finalize(void);

#endif//_STREAMING_RTP_OVER_UDP_H_

