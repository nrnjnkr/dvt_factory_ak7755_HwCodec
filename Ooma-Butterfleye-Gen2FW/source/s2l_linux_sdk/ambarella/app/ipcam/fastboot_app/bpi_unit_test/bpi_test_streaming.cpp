/*
 * bpi_test_streaming.cpp
 *
 *
 * History:
 *       2017/05/17 - [jyi] created file
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#include "bpi_utils.h"
#include "bpi_typedefs.h"
#include "bpi_uploader.h"
#include "bpi_app_config.h"
#include "streaming_rtp_over_udp.h"
#include "config.h"

static char dest_ip[64] = {0};
static char overlay_tag[64] = {0};
static unsigned int streaming_duration = 0;
static int enable_audio = 0;
static int target_bitrate = 0;
static int stop_flag = 0;

static int test_streaming()
{
    int ret = 0;
    char *audio_codec_name;
    int audio_payload_type;
#if defined(AMBOOT_AUDIO_48000)
    audio_codec_name = (char*)"aac-48k";
    audio_payload_type = 97;
#elif defined(AMBOOT_AUDIO_16000)
    audio_codec_name = (char*)"aac-16k";
    audio_payload_type = 97;
#elif defined(AMBOOT_AUDIO_8000)
    audio_codec_name = (char*)"g711-8k";
    audio_payload_type = 8;
#else
    audio_codec_name = NULL;
    audio_payload_type = 0;
    enable_audio = 0;
#endif
    transport_info_t transport_info;
    bpi_streamer_info streamer_info;

    snprintf(transport_info.ipaddress, sizeof(transport_info.ipaddress), "%s",
             dest_ip);
    transport_info.is_ipv4 = 1;
    transport_info.video.enable = 1;
    transport_info.video.port = 8554;
    transport_info.video.payload_type = 96;
    transport_info.video.ssrc = 0xfeadbeaf;
    transport_info.audio.enable = enable_audio;
    transport_info.audio.port = 8556;
    transport_info.audio.payload_type = audio_payload_type;
    transport_info.audio.ssrc = 0xfeadbea1;
    streamer_info.stream_id = 0;
    streamer_info.enable_lbr = (target_bitrate == 0);
    streamer_info.target_bitrate = target_bitrate;
    streamer_info.audio_codec_name = audio_codec_name;
    streamer_info.transport_info = &transport_info;
    streamer_info.osd_label = overlay_tag;
    ret = streaming_initialize(&streamer_info);
    if(!ret){
        if(streaming_duration){
            while(streaming_duration || !stop_flag){
                sleep(1);
                streaming_duration--;
            }
        }else{
            while(!stop_flag) sleep(1);
        }
        ret = streaming_finalize();
    }

    return ret;
}

static void signal_interrupt(int sig)
{
    stop_flag = 1;
}

static int init_param(int argc, char **argv)
{
    int ret = -1;
    const char *short_options = "s:d:r:o:ah";
    struct option long_options[] = {
        {"streaming_dest_ip", 1, 0, 's'},
        {"duration", 1, 0, 'd'},
        {"bitrate_cotrol", 1, 0, 'r'},
        {"enable_audio", 0, 0, 'a'},
        {"overlay_tag", 1, 0, 'o'},
        {"help", 0, 0, 'h'}
    };
    int ch = 0;
    int option_index = 0;

    while(1){
        ch = getopt_long(argc, argv, short_options, long_options, &option_index);
        if(ch == -1 || ret > 0) break;
        switch(ch){
        case 's':
            snprintf(dest_ip, sizeof(dest_ip), "%s", optarg);
            ret = 0; //mandatory
            break;
        case 'd':
            streaming_duration = atoi(optarg);
            break;
        case 'a':
            enable_audio = 1;
            break;
        case 'r':
            target_bitrate = atoi(optarg);
            break;
        case 'o':
            snprintf(overlay_tag, sizeof(overlay_tag), "%s", optarg);
            break;
        case 'h':
            ret = 1;
            break;
        default:
            LOG_ERROR("catch default opt: %c\n", (char)ch);
            ret = 1;
            break;
        }
    }

    return ret;
}

static void usage()
{
    printf(
        "save this .sdp on computer:\n"
        "\tm=video 8554 RTP/AVP 96\n"
        "\ta=rtpmap:96 H264/90000\n"
        "\n"
        "Usage: bpi_test_streaming [option] [arguments]\n"
        "-- Mandatory --\n"
        "\t-s, --streaming_dest_ip <ip>:        appoint destination ip\n"
        "-- Optional --\n"
        "\t-d, --duration <duration_s>:           set streaming duration, or endless\n"
        "\t-r, --bitrate_control <bitrate_bps>:     if not set, use lbr\n"
        "\t-o, --overlay_tag <string>:          set overlay to video\n"
        "\t-a, --enable_audio:                  audio .sdp has following content:\n"
        "\t\tm=audio 8556 RTP/AVP 97\n"
        "\t\ta=rtpmap:97 mpeg4-generic/48000/1\n"
        "\t-h, --help: show this help content\n"
        "-- MAKE SURE DESTINATION IP IS ACCESSIBLE AND DSP IS ENCODING --\n"
        );
}

int main(int argc, char **argv)
{
    int ret = 1;
    int log_options = BPI_LOG_ENABLE_DEBUG_LOG;

    signal(SIGINT, signal_interrupt);
    signal(SIGQUIT, signal_interrupt);
    signal(SIGTERM, signal_interrupt);
    do{
        if(argc < 2 || init_param(argc, argv) != 0){
            usage();
            break;
        }
        set_log_options(log_options);

        if(test_streaming()){
            LOG_ERROR("streaming failed\n");
            break;
        }
        ret = 0;
    }while(0);

    return ret;
}
