/*
 * Bpi_test_adc.cpp
 *
 *
 * History:
 *       2017/06/14 - [mzchen] created file
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
#include <stdio.h>
#include <getopt.h>

#include "bpi_utils.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "adc_util.h"

static struct amboot_params params = {};
static int adc_rwflag = -1;

static void get_amboot_params(app_conf_t* config)
{
    params.enable_audio = config->audio_enable;
    params.enable_fastosd = 0;
    params.enable_ldc = (unsigned char)config->enable_ldc;
    params.rotation_mode = config->rotate;

    params.stream0_enable = (unsigned int)config->video0_enable; //enable

    if (1080 == config->video0_height && 1920 == config->video0_width) {
        params.stream0_resolution = 0; //1080p
    }
    else if (720 == config->video0_height && 1280 == config->video0_width) {
        params.stream0_resolution = 1; //720p
    }
    else {
        LOG_ERROR("invalid stream0_resolution\n");
    }

    params.stream0_fmt = (unsigned int)config->video0_fmt; //h264
    params.stream0_fps = config->video0_frame_rate;
    params.stream0_bitrate = (unsigned int)config->video0_recording_bitrate;

    params.stream1_enable = (unsigned int)config->video1_enable;
    if (480 == config->video1_height && 720 == config->video1_width) {
        params.stream1_resolution = 2; //480p
    }
    else {
        LOG_ERROR("invalid stream1_resolution\n");
    }
    params.stream1_fmt = (unsigned int)config->video1_fmt; //h264
    params.stream1_fps = config->video1_frame_rate;
    params.stream1_bitrate = (unsigned int)config->video1_recording_bitrate;

    snprintf(params.fastosd_string, sizeof(params.fastosd_string), "AMBA");
    params.enable_vca = config->vca_enable;
    params.vca_frame_num = (unsigned char)config->vca_frame_num;
}

static int init_param(int argc, char **argv)
{
    int ret = -1;
    const char *short_options = "wrh";
    struct option long_options[] = {
        { "adc_write", 0, 0, 'w' },
        { "adc_read", 0, 0, 'r' },
        {"help", 0, 0, 'h'}
    };
    int ch = 0;
    int option_index = 0;

    while(1) {
        ch = getopt_long(argc, argv, short_options, long_options, &option_index);
        if(ch == -1 || ret > 0) break;
        switch(ch) {
            case 'w':
                adc_rwflag = 1;
                ret = 0;
                break;
            case 'r':
                adc_rwflag = 0;
                ret = 0;
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
        "\n"
        "Usage: bpi_test_adc [option] [arguments]\n"
        "\t-w, ---adc_write\n"
        "\t-r, --adc_read\n"
        "\t-h, --help: show this help content\n"
        );
}

int main(int argc, char **argv)
{
    if(argc < 2 || init_param(argc, argv) != 0) {
        usage();
        return 1;
    }

    if (adc_rwflag) {
        //load to get app_conf
        app_conf_t tmp_app_config;
        if(load_app_conf(&tmp_app_config) < 0){
            LOG_ERROR("load app config failed\n");
            return 1;
        }
        get_amboot_params(&tmp_app_config);
        adc_util_update(&params);
    }
    else {
        adc_util_dump();
    }
}


