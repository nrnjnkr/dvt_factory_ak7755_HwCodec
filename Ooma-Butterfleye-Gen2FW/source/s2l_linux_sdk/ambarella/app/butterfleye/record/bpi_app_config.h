/*
 * bpi_app_config.h
 *
 * History:
 *       2015/06/07 - [CZ Lin] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef __BPI_APP_CONFIG_H_
#define __BPI_APP_CONFIG_H_

#include <stdint.h>
#include "bpi_typedefs.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TOKEN_LEN 127
#define MAX_VCA_FRAME_NUM 30
#define BPI_CONFIG_PATH "/etc/butterfleye/setting.ini"
#define SDCARD_PATH "/sdcard/"

typedef struct _app_conf_t {
    unsigned char record_control_mode;
    bool thumbnail;
    bool enable_ldc;
    char osd_label[128];
    int record_duration_after_motion_stops;
    int record_duration_after_motion_starts;
    int recording_max_duration;
    int streaming_duration;
    char storage_folder[128];
    char file_name_prefix[64];
    bool smart_avc;
    bool enable_two_ref;
    BPI_ROTATION_TYPE rotate;

    int file_duration;
    BPI_FILE_MUXER_TYPE file_fmt;

    bool video0_enable;
    BPI_VIDEO_CODEC_TYPE video0_fmt;
    int video0_width;
    int video0_height;
    int video0_frame_rate;
    int video0_recording_bitrate;
    int video0_streaming_bitrate;

    bool video1_enable;
    BPI_VIDEO_CODEC_TYPE video1_fmt;
    int video1_width;
    int video1_height;
    int video1_frame_rate;
    int video1_recording_bitrate;
    int video1_streaming_bitrate;

    int audio_sample_rate;
    bool audio_enable;
    BPI_AUDIO_CODEC_TYPE audio_fmt;

    //parameters about wlan keepalive
    int keepalive_interval; //seconds;
    int dtim_interval; //mseconds;
    char wakeup_token[MAX_TOKEN_LEN + 1];
    char ap_ssid[128];
    char ap_password[128];
    char ap_wpa[64];
    char cloud_server_ip[64];

    bool vca_enable;
    int vca_width;
    int vca_height;
    uint8_t reserved;
    int vca_frame_num;
    int vca_timeout; // in ms

    AM_BPI_MODE_MAP mode_map[MCU_TRIGGER_END];
} app_conf_t;
int load_app_conf(app_conf_t *config);
int save_app_conf(app_conf_t *config);
int config_modified(app_conf_t *config, app_conf_t *old_config);

#ifdef DAREDEVIL
typedef struct _doorbell_conf_t {
    bool thumbnail;
    int streaming_duration;
    char storage_folder[128];
    bool smart_avc;
    bool hdr_enable;
    BPI_ROTATION_TYPE rotate;

    int file_duration;
    BPI_FILE_MUXER_TYPE file_fmt;

    bool video_enable;
    BPI_VIDEO_CODEC_TYPE video_fmt;
    int video_width;
    int video_height;
    int video_frame_rate;
    int video_recording_bitrate;
    int video_streaming_bitrate;

    bool audio_enable;
    BPI_AUDIO_CODEC_TYPE audio_fmt;

    char ap_ssid[128];
    char ap_password[128];
    char ap_wpa[128];
    char cloud_server_ip[128];
    int vca_width;
    int vca_height;
} doorbell_conf_t;
int load_doorbell_conf(doorbell_conf_t *config);
#endif

#ifdef __cplusplus
}
#endif

#endif//APP_CONFIG_H
