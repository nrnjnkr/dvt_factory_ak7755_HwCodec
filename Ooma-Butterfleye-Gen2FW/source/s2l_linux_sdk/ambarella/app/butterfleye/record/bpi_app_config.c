/*
 * bpi_app_config.c
 *
 * History:
 *       2015/06/07 - [CZ Lin] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents "Software") are protected by intellectual
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "iniparser.h"

#define _DIFF(config, old_config, item) (config->item != old_config->item)

int load_app_conf(app_conf_t *config){
    const char* value = NULL;
    int ret = -1;
    dictionary* ini_dict = NULL;
    do{
        ini_dict = iniparser_load(BPI_CONFIG_PATH);
        if(!ini_dict){
            break;
        }
        //bpi work mode map
        /************************
        *[mode_map]
        *pir_trigger=recording
        *wifi_wakeup_trigger=streaming
        *wifi_reconnect_trigger=wlan_reconnect
        *notify_trigger=notify
        *button_click_trigger=wlan_config
        *button_2s_trigger=xxxx
        *button_5s_trigger=xxxx
        **add more map here**
        *************************/
        AM_BPI_MODE_MAP* mode_map = config->mode_map;
        if ((value = iniparser_getstring(ini_dict, "mode_map:pir_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_PIR_ON].index = MCU_TRIGGER_PIR_ON;
            mode_map[MCU_TRIGGER_PIR_ON].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value= iniparser_getstring(ini_dict, "mode_map:wifi_wakeup_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_WIFI_WAKEUP].index = MCU_TRIGGER_WIFI_WAKEUP;
            mode_map[MCU_TRIGGER_WIFI_WAKEUP].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value= iniparser_getstring(ini_dict, "mode_map:wifi_reconnect_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_WIFI_RECONNECT].index = MCU_TRIGGER_WIFI_RECONNECT;
            mode_map[MCU_TRIGGER_WIFI_RECONNECT].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value= iniparser_getstring(ini_dict, "mode_map:button_click_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_PWR].index = MCU_TRIGGER_PWR;
            mode_map[MCU_TRIGGER_PWR].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value= iniparser_getstring(ini_dict, "mode_map:button_2s_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_PWR_2S].index = MCU_TRIGGER_PWR_2S;
            mode_map[MCU_TRIGGER_PWR_2S].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value= iniparser_getstring(ini_dict, "mode_map:low_power_trigger", NULL))!= NULL) {
            mode_map[MCU_TRIGGER_POWER_EMPTY].index = MCU_TRIGGER_POWER_EMPTY;
            mode_map[MCU_TRIGGER_POWER_EMPTY].bpi_mode = (AM_BPI_MODE)atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:record_control_mode", NULL))!= NULL){
            config->record_control_mode = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:record_duration_after_motion_starts", NULL))!= NULL){
            config->record_duration_after_motion_starts = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:record_duration_after_motion_stops", NULL))!= NULL){
            config->record_duration_after_motion_stops = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:recording_max_duration", NULL))!= NULL){
            config->recording_max_duration = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:streaming_duration", NULL))!= NULL){
            config->streaming_duration = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:thumbnail", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->thumbnail = true;
            }
            else if(!strcmp(value, "false")){
                config->thumbnail = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:enable_ldc", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->enable_ldc = true;
            }
            else if(!strcmp(value, "false")){
                config->enable_ldc = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:osd_label", NULL))!= NULL){
            strcpy(config->osd_label, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:storage_folder", NULL))!= NULL){
            memset(config->storage_folder, '\0', sizeof(config->storage_folder));
            strcpy(config->storage_folder, value);
            if('/' != value[strlen(value)-1]){
                config->storage_folder[strlen(config->storage_folder)] = '/';
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:file_name_prefix", NULL))!= NULL){
            strcpy(config->file_name_prefix, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:smart_avc", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->smart_avc = true;
            }
            else if(!strcmp(value, "false")){
                config->smart_avc = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }



        if((value = iniparser_getstring(ini_dict, "app:rotate", NULL)) != NULL){
            if( !strcmp(value,"0") || !strcmp(value,"1") || !strcmp(value,"2") || !strcmp(value,"3")){
                config->rotate = (BPI_ROTATION_TYPE)(atoi(value));
            }else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "container:file_duration", NULL) )!= NULL){
            config->file_duration = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "container:fmt", NULL))!= NULL){
            if(!strcmp(value, "mp4")){
                config->file_fmt = BPI_FILE_MUXER_MP4;
            }
            else if(!strcmp(value, "ts")){
                config->file_fmt = BPI_FILE_MUXER_TS;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->video0_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->video0_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:fmt", NULL))!= NULL){
            if(!strcmp(value, "h264")){
                config->video0_fmt = BPI_VIDEO_CODEC_H264;
            }
            else if(!strcmp(value, "h265")){
                config->video0_fmt = BPI_VIDEO_CODEC_H265;
            }
            else if(!strcmp(value, "mjpeg")){
                config->video0_fmt = BPI_VIDEO_CODEC_MJPEG;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:resolution", NULL))!= NULL){
            if(!strcmp(value, "720p")){
                config->video0_width = 1280;
                config->video0_height = 720;
            }
            else if(!strcmp(value, "1080p")){
                config->video0_width = 1920;
                config->video0_height = 1080;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:frame_rate", NULL))!= NULL){
            config->video0_frame_rate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:recording_bitrate", NULL))!= NULL){
            config->video0_recording_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video0:streaming_bitrate", NULL))!= NULL){
            config->video0_streaming_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->video1_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->video1_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:fmt", NULL))!= NULL){
            if(!strcmp(value, "h264")){
                config->video1_fmt = BPI_VIDEO_CODEC_H264;
            }
            else if(!strcmp(value, "h265")){
                config->video1_fmt = BPI_VIDEO_CODEC_H265;
            }
            else if(!strcmp(value, "mjpeg")){
                config->video1_fmt = BPI_VIDEO_CODEC_MJPEG;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:resolution", NULL))!= NULL){
            if(!strcmp(value, "480p")){
                config->video1_width = 720;
                config->video1_height = 480;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:frame_rate", NULL))!= NULL){
            config->video1_frame_rate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:recording_bitrate", NULL))!= NULL){
            config->video1_recording_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video1:streaming_bitrate", NULL))!= NULL){
            config->video1_streaming_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "audio:sample_rate", NULL))!= NULL){
            if(!strcmp(value, "48000")){
                config->audio_sample_rate= 48000;
            }
            else if(!strcmp(value, "16000")){
                config->audio_sample_rate = 16000;
            }
            else if(!strcmp(value, "8000")){
                config->audio_sample_rate = 8000;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_DEBUG( "Audio sample rate entry not found, fastboot only. line:%d\n", __LINE__);
        }

        if((value= iniparser_getstring(ini_dict, "audio:enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->audio_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->audio_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "audio:fmt", NULL))!= NULL){
            if(!strcmp(value, "g726")){
                config->audio_fmt = BPI_AUDIO_CODEC_G726;
            }
            else if(!strcmp(value, "aac")){
                config->audio_fmt = BPI_AUDIO_CODEC_AAC;
            }
            else if(!strcmp(value, "g711a")){
                config->audio_fmt = BPI_AUDIO_CODEC_G711A;
            }
            else if(!strcmp(value, "g711mu")){
                config->audio_fmt = BPI_AUDIO_CODEC_G711MU;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:keepalive_interval", NULL))!= NULL){
            config->keepalive_interval = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:dtim_interval", NULL))!= NULL){
            config->dtim_interval = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:wakeup_token", NULL))!= NULL){
            strcpy(config->wakeup_token, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:cloud_server_ip_string", NULL))!= NULL){
            strcpy(config->cloud_server_ip, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_ssid_string", NULL))!= NULL){
            strcpy(config->ap_ssid, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_password_string", NULL))!= NULL){
            strcpy(config->ap_password, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_wpa_string", NULL))!= NULL){
            strcpy(config->ap_wpa, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:enable", NULL)) != NULL) {
            if (!strcmp(value, "true")) {
                config->vca_enable = true;
            }
            else if (!strcmp(value, "false")) {
                config->vca_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        } else {
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:frame_num", NULL)) != NULL) {
            config->vca_frame_num = atoi(value);
            if(config->vca_frame_num > MAX_VCA_FRAME_NUM){
                config->vca_frame_num = MAX_VCA_FRAME_NUM;
                LOG_PRINT("vca_frame_num too large, set to %d\n", MAX_VCA_FRAME_NUM);
            }
        } else {
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:buffer_size", NULL)) != NULL) {
            if(!strcmp(value, "720p")){
                config->vca_width = 1280;
                config->vca_height = 720;
            }
            else if(!strcmp(value, "1080p")){
                config->vca_width = 1920;
                config->vca_height = 1080;
            }
            else if(!strcmp(value, "480p")){
                config->vca_width = 720;
                config->vca_height = 480;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        } else {
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:timeout", NULL)) != NULL) {
            config->vca_timeout = atoi(value);
        } else {
            break;
        }

        if (config->enable_two_ref){
            if (config->enable_ldc){
                LOG_ERROR("cannot enable two ref and LDC dewarp at the same time, ignore LDC settings\n");
                break;
            }
            if (config->rotate == 1 || config->rotate == 3){
                LOG_ERROR("cannot enable two ref and rotate at the same time, ignore rotate settings\n");
                break;
            }
        }
        ret = 0;
    }while(0);
    if(ini_dict) iniparser_freedict(ini_dict);

    return ret;
}
#if 0
int save_app_conf(app_conf_t *config)
{
    char value[64];
    int ret = -1;
    dictionary* ini_dict = NULL;
    char config_overwrite[64];
    do{
        ini_dict = iniparser_load(BPI_CONFIG_PATH);
        if(!ini_dict){
            break;
        }
        /* Todo: Add any config item you want to modify */

        FILE* f_ini = fopen(config_overwrite, "w");
        if(f_ini){
            iniparser_dump_ini(ini_dict, f_ini);
            fclose(f_ini);
        }

        ret = 0;
    }while(0);
    if(ini_dict) iniparser_freedict(ini_dict);
    return ret;
}
#endif

int config_modified(app_conf_t *config, app_conf_t *old_config)
{
    int is_modified =
        _DIFF(config, old_config, enable_ldc) ||
        _DIFF(config, old_config, video0_enable) ||
        _DIFF(config, old_config, video0_frame_rate) ||
        _DIFF(config, old_config, video0_recording_bitrate) ||
        _DIFF(config, old_config, video0_width) ||
        _DIFF(config, old_config, video0_height) ||
        _DIFF(config, old_config, rotate) ||
        _DIFF(config, old_config, audio_enable) ||
        _DIFF(config, old_config, vca_enable) ||
        _DIFF(config, old_config, vca_frame_num);

    return is_modified;
}

#ifdef DAREDEVIL
int load_doorbell_conf(doorbell_conf_t *config){
    const char* value = NULL;
    int ret = -1;
    dictionary* ini_dict = NULL;
    do{
        ini_dict = iniparser_load"/etc/bpi/setting.ini");
        if(!ini_dict){
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:streaming_duration", NULL))!= NULL){
            config->streaming_duration = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:thumbnail", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->thumbnail = true;
            }
            else if(!strcmp(value, "false")){
                config->thumbnail = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        strcpy(config->storage_folder, SDCARD_PATH);

    #if 0
        if((value= iniparser_getstring(ini_dict, "storage_folder", "app"))!= NULL){
            memset(config->storage_folder, '\0', sizeof(config->storage_folder));
            strcpy(config->storage_folder, value);
            if('/' != value[strlen(value)-1]){
                config->storage_folder[strlen(config->storage_folder)] = '/';
            }
        }else{
            strcpy(config->storage_folder, "/sdcard/video/");
            iniparser_set(ini_dict, "storage_folder", "/sdcard/video/", "app");
        }
    #endif

        if((value= iniparser_getstring(ini_dict, "app:smart_avc", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->smart_avc = true;
            }
            else if(!strcmp(value, "false")){
                config->smart_avc = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "app:hdr_enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->hdr_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->hdr_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value = iniparser_getstring(ini_dict, "app:rotate",NULL)) != NULL){
            if( (strcmp(value,"0")==0) || (strcmp(value,"1")==0)\
                    || (strcmp(value,"2")==0) || (strcmp(value,"3")==0)){
                config->rotate = BPI_ROTATION_TYPE(atoi(value));
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "container:file_duration", NULL) )!= NULL){
            config->file_duration = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "container:fmt", NULL))!= NULL){
            if(!strcmp(value, "mp4")){
                config->file_fmt = BPI_FILE_MUXER_MP4;
            }
            else if(!strcmp(value, "ts")){
                config->file_fmt = BPI_FILE_MUXER_TS;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->video_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->video_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:fmt", NULL))!= NULL){
            if(!strcmp(value, "h264")){
                config->video_fmt = BPI_VIDEO_CODEC_H264;
            }
            else if(!strcmp(value, "h265")){
                config->video_fmt = BPI_VIDEO_CODEC_H265;
            }
            else if(!strcmp(value, "mjpeg")){
                config->video_fmt = BPI_VIDEO_CODEC_MJPEG;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:width", NULL))!= NULL){
            config->video_width = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:height", NULL))!= NULL){
            config->video_height = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:frame_rate", NULL))!= NULL){
            config->video_frame_rate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:recording_bitrate", NULL))!= NULL){
            config->video_recording_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "video:streaming_bitrate", NULL))!= NULL){
            config->video_streaming_bitrate = atoi(value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "audio:enable", NULL))!= NULL){
            if(!strcmp(value, "true")){
                config->audio_enable = true;
            }
            else if(!strcmp(value, "false")){
                config->audio_enable = false;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "audio:fmt", NULL))!= NULL){
            if(!strcmp(value, "g726")){
                config->audio_fmt = BPI_AUDIO_CODEC_G726;
            }
            else if(!strcmp(value, "aac")){
                config->audio_fmt = BPI_AUDIO_CODEC_AAC;
            }
            else if(!strcmp(value, "g711a")){
                config->audio_fmt = BPI_AUDIO_CODEC_G711A;
            }
            else if(!strcmp(value, "g711mu")){
                config->audio_fmt = BPI_AUDIO_CODEC_G711MU;
            }
            else{
                LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
                break;
            }
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:cloud_server_ip_string", NULL))!= NULL){
            strcpy(config->cloud_server_ip, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_ssid_string", NULL))!= NULL){
            strcpy(config->ap_ssid, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_password_string", NULL))!= NULL){
            strcpy(config->ap_password, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if((value= iniparser_getstring(ini_dict, "wlan:ap_wpa_string", NULL))!= NULL){
            strcpy(config->ap_wpa, value);
        }else{
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:width", NULL)) != NULL) {
            config->vca_width = atoi(value);
        } else {
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        if ((value = iniparser_getstring(ini_dict, "vca:height", NULL)) != NULL) {
            config->vca_height = atoi(value);
        } else {
            LOG_ERROR("get ini value failed, line:%d\n", __LINE__);
            break;
        }

        ret = 0;
    }while(0);

    if(ini_dict) iniparser_freedict(ini_dict);
    return ret;
}
#endif
#ifdef __cplusplus
};
#endif
