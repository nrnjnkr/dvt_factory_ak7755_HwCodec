/*******************************************************************************
 * bpi_oryx_config.cpp
 *
 * History:
 *   2017-07-17 - [CZ Lin]      created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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
 ******************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include <string>
#include "bpi_utils.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_oryx_config.h"
#include "am_configure.h"
#include "am_define.h"
#include "am_file.h"
#include "am_record_msg.h"
#include "am_low_bitrate_control_if.h"
#include "am_motion_detect_if.h"
#include "am_encode_overlay_if.h"

using std::string;
using std::shared_ptr;

#ifdef ELEKTRA_V
#define  ADV_MODE_RESOURCE_LIMIT  "/etc/oryx/video/hdr_line_interleaved_mode_resource_limit.acs"
#else
#define  ADV_MODE_RESOURCE_LIMIT  "/etc/oryx/video/adv_hdr_mode_resource_limit.acs"
#endif

// Simplify and unify the config process later
void write_sole_config(const char *filename, void *content, size_t length)
{
    int fd = -1;
    size_t write_size = 0;
    int fd_flags = O_WRONLY | O_TRUNC;
    do {
        if (access(filename, F_OK) != 0) {
            fd_flags |= O_CREAT;
        }
        fd = open(filename, fd_flags, 0400);
        if (fd < 0) {
            LOG_ERROR("open file %s failed: %s\n", filename, strerror(errno));
            break;
        }
        while (write_size < length) {
            size_t _size = write(fd, content, length);
            if (_size < 0 && errno == EINTR) {
                continue;
            }
            if (_size < 0) {
                break;
            }
            write_size += _size;
        }
        if (write_size != length) {
            LOG_ERROR("write file %s incorrect: %s\n", filename,
                      strerror(errno));
        }
    } while (0);

    if (fd) {
        close(fd);
    }
}

void read_sole_config(const char *filename, void *content, size_t length)
{
    int fd = -1;
    size_t read_size = 0;
    do {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            LOG_ERROR("open file %s failed: %s\n", filename, strerror(errno));
            break;
        }
        while (read_size < length) {
            size_t _size = read(fd, content, length);
            if (_size < 0 && errno == EINTR) {
                continue;
            }
            if (_size < 0) {
                break;
            }
            read_size += _size;
        }
        if (read_size != length) {
            LOG_ERROR("write file %s incorrect: %s\n", filename,
                      strerror(errno));
        }
    } while (0);

    if (fd) {
        close(fd);
    }
}

static bool config_oryx_image_iq(){
    shared_ptr<AMConfig> p_iq;
    p_iq.reset(AMConfig::create("/etc/oryx/image/iq_config.acs"));
    if(!p_iq){
        LOG_ERROR("load iq_config.acs failed\n");
        return false;
    }
    AMConfig &iq = *p_iq;
    if(iq["ae"].exists()){
        iq["ae"]["slow_shutter_mode"] = 0;
        iq["ae"]["sensor_shutter_max"] = 30;
        iq["ae"]["sensor_gain_max"] = 42; //42 dB
        iq["ae"]["ae_metering_mode"] = 2; //average metering
        iq["ae"]["day_night_mode"] = -1;
    }
    if(p_iq){
        (*p_iq).save();
    }
    return true;
}
bool config_oryx_engine(app_conf_t *p_user_conf){
    shared_ptr<AMConfig> p_record_engine;
    shared_ptr<AMConfig> p_filter_file_muxer;
    shared_ptr<AMConfig> p_muxer;
    shared_ptr<AMConfig> p_motion_detect;
    shared_ptr<AMConfig> p_lbr;
    shared_ptr<AMConfig> p_features;
    shared_ptr<AMConfig> p_hdr_resource_limit;
    shared_ptr<AMConfig> p_audio_source;
    shared_ptr<AMConfig> p_filter_player;
    shared_ptr<AMConfig> p_audio_g711_8k;
    shared_ptr<AMConfig> p_audio_aac_48k;

    if (!config_oryx_image_iq()){
        LOG_ERROR("config_oryx_image_iq failed\n");
        return false;
    }
    p_record_engine.reset(AMConfig::create("/etc/oryx/stream/engine/record-engine.acs"));
    if(!p_record_engine){
        LOG_ERROR("load record-engine.acs failed\n");
        return false;
    }

    p_filter_file_muxer.reset(AMConfig::create("/etc/oryx/stream/filter/filter-file-muxer-0.acs"));
    if(!p_filter_file_muxer){
       LOG_ERROR("load %s failed\n", "/etc/oryx/stream/filter/filter-file-muxer-0.acs");
       return false;
    }

    if(p_user_conf->file_fmt == BPI_FILE_MUXER_MP4){
        p_muxer.reset(AMConfig::create("/etc/oryx/stream/muxer/muxer-mp4-0.acs"));
        if(!p_muxer){
            LOG_ERROR("load %s failed\n", "/etc/oryx/stream/muxer/muxer-mp4-0.acs");
            return false;
        }
    }else if(p_user_conf->file_fmt == BPI_FILE_MUXER_TS){
        p_muxer.reset(AMConfig::create("/etc/oryx/stream/muxer/muxer-ts-0.acs"));
        if(!p_muxer){
            LOG_ERROR("load %s failed\n", "/etc/oryx/stream/muxer/muxer-ts-0.acs");
            return false;
        }
    }else{
        LOG_ERROR("file format %d not supported\n", p_user_conf->file_fmt);
        return false;
    }

    p_motion_detect.reset(AMConfig::create("/etc/oryx/video/vca/motion-detect.acs"));
    if(!p_motion_detect){
        LOG_ERROR("load %s failed\n", "/etc/oryx/video/vca/motion-detect.acs");
        return false;
    }
    p_lbr.reset(AMConfig::create("/etc/oryx/video/lbr.acs"));
    if(!p_lbr){
        LOG_ERROR("load %s failed\n", "/etc/oryx/video/lbr.acs");
        return false;
    }

    p_features.reset(AMConfig::create("/etc/oryx/video/features.acs"));
    if(!p_features){
        LOG_ERROR("load %s failed\n", "/etc/oryx/video/features.acs");
        return false;
    }
    p_hdr_resource_limit.reset(AMConfig::create(ADV_MODE_RESOURCE_LIMIT));
    if(!p_hdr_resource_limit){
        LOG_ERROR("load %s failed\n", ADV_MODE_RESOURCE_LIMIT);
        return false;
    }

    if (p_user_conf->audio_enable) {
        if (48000 == p_user_conf->audio_sample_rate) {
            p_audio_source.reset(AMConfig::create("/etc/oryx/stream/filter/filter-audio-source-48k.acs"));
            if(!p_audio_source){
                LOG_ERROR("load %s failed\n", "/etc/oryx/stream/filter/filter-audio-source-48k.acs");
                return false;
            }

            p_audio_aac_48k.reset(AMConfig::create("/etc/oryx/stream/codec/codec-aac-48k.acs"));
            if(!p_audio_aac_48k){
                LOG_ERROR("load %s failed\n", "/etc/oryx/stream/codec/codec-aac-48k.acs");
                return false;
            }
        } else if (16000 == p_user_conf->audio_sample_rate) {
            p_audio_source.reset(AMConfig::create("/etc/oryx/stream/filter/filter-audio-source-16k.acs"));
            if(!p_audio_source){
                LOG_ERROR("load %s failed\n", "/etc/oryx/stream/filter/filter-audio-source-16k.acs");
                return false;
            }
        } else if (8000 == p_user_conf->audio_sample_rate) {
            p_audio_source.reset(AMConfig::create("/etc/oryx/stream/filter/filter-audio-source-8k.acs"));
            if(!p_audio_source){
                LOG_ERROR("load %s failed\n", "/etc/oryx/stream/filter/filter-audio-source-8k.acs");
                return false;
            }

            p_audio_g711_8k.reset(AMConfig::create("/etc/oryx/stream/codec/codec-g711-8k.acs"));
            if(!p_audio_g711_8k){
                LOG_ERROR("load %s failed\n", "/etc/oryx/stream/codec/codec-g711-8k.acs");
                return false;
            }
        } else {
            LOG_ERROR("audio sample rate =%d not supported\n", p_user_conf->audio_sample_rate);
        }
    }

    p_filter_player.reset(AMConfig::create("/etc/oryx/stream/filter/filter-player.acs"));
    if(!p_filter_player){
        LOG_ERROR("load %s failed\n", "/etc/oryx/stream/filter/filter-player.acs");
        return false;
    }

    AMConfig &record_engine = *p_record_engine;
    AMConfig &muxer = *p_muxer;
    AMConfig &file_muxer_config = *p_filter_file_muxer;
    AMConfig &md = *p_motion_detect;
    AMConfig &lbr = *p_lbr;
    AMConfig &features = *p_features;
    AMConfig &hdr_resource_limit = *p_hdr_resource_limit;
    AMConfig &audio_source = *p_audio_source;
    AMConfig &filter_player = *p_filter_player;
    AMConfig &audio_g711_8k = *p_audio_g711_8k;
    AMConfig &audio_aac_48k = *p_audio_aac_48k;
    int file_muxer_bitmap;

    if(record_engine["filters"].exists()) record_engine["filters"].remove();
    if(record_engine["connection"].exists()) record_engine["connection"].remove();
    record_engine["filters"][0]=string("file-muxer-0");
    record_engine["filters"][1]=string("video-source");
    record_engine["connection"]["video_source"]["output"][0] = string("file-muxer-0");
    record_engine["connection"]["file_muxer_0"]["input"][0] = string("video_source");

    if(file_muxer_config["media_type"].exists()) file_muxer_config["media_type"].remove();
    if(p_user_conf->file_fmt == BPI_FILE_MUXER_MP4){
        file_muxer_config["media_type"][0] = string("mp4-0");
    }else if(p_user_conf->file_fmt == BPI_FILE_MUXER_TS){
        file_muxer_config["media_type"][0] = string("ts-0");
    }
    file_muxer_config["persistent_buf_duration"] = 8; // It needs to save more audio data in fastboot
    //write muxer bitmap to nand
    file_muxer_bitmap |= 0x01 << muxer["muxer_id"].get<unsigned int>(0);
    write_sole_config("/tmp/config/etc/bpi/.file_muxer_bitmap",
                    (void*)&file_muxer_bitmap, sizeof(file_muxer_bitmap));
    muxer["file_name_prefix"] = string(p_user_conf->file_name_prefix);
    muxer["file_location"] = string(p_user_conf->storage_folder);
    muxer["file_location_auto_parse"] = false;

    features["mode"] = string("mode4");
    features["overlay"] = string("enable");
    features["dptz"] = string("disable");
    hdr_resource_limit["dsp_partition_possible"]["sub_buf_prev_C_yuv"] = true;
    md["md_source_buffer_id"] = 2; // preview B ME1, 1/4 width of prevB, 1/4 height of prevB
    md["roi_config"][0]["right"] = 319;  // this should change accordingly if size of prevB buffer changes
    md["roi_config"][0]["bottom"] = 179;
    if (p_user_conf->smart_avc) {
        lbr["StreamConfig"][0]["AutoBitrateTarget"] = false;
        lbr["StreamConfig"][0]["BitrateCeiling"] = 186;
        lbr["StreamConfig"][0]["EnableLBR"] = true;
        features["video_vca"][0] = string("vca-motion-detect");
        features["bitrate_ctrl"] = string("lbr");
        hdr_resource_limit["dsp_partition_possible"]["sub_buf_prev_B_me"] = true; //md use Dsp sub buf4
    } else {
        features["video_vca"][0] = string("");
        features["bitrate_ctrl"] = string("none");
        hdr_resource_limit["dsp_partition_possible"]["sub_buf_prev_B_me"] = false;
    }

    if (p_user_conf->audio_enable){
        filter_player["audio"]["interface"] = string("alsa");
        filter_player["audio"]["buffer_delay_ms"] = 100;
        audio_source["packet_pool_size"] = 128; // Packet poll needs more size than default value 64 in fastboot mode
        if (48000 == p_user_conf->audio_sample_rate){
            record_engine["filters"][2]=string("audio-source-48k");
            record_engine["connection"]["audio_source_48k"]["output"][0] = string("file-muxer-0");
            record_engine["connection"]["file_muxer_0"]["input"][1] = string("audio_source_48k");
            muxer["audio_type"] = string("aac");
            muxer["audio_sample_rate"] = 48000;
            file_muxer_config["audio_type"][0] = string("aac-48k");
            audio_source["interface"] = string("raw");
            if(audio_source["audio_type"].exists()) {audio_source["audio_type"].remove();}
            audio_source["audio_type"][0] = string("aac");
            audio_aac_48k["encode"]["format"] = string("aac");
        }else if (16000 == p_user_conf->audio_sample_rate){
            record_engine["filters"][2]=string("audio-source-16k");
            record_engine["connection"]["audio_source_16k"]["output"][0] = string("file-muxer-0");
            record_engine["connection"]["file_muxer_0"]["input"][1] = string("audio_source_16k");
            muxer["audio_type"] = string("aac");
            muxer["audio_sample_rate"] = 16000;
            file_muxer_config["audio_type"][0] = string("aac-16k");
            audio_source["interface"] = string("raw");
            if(audio_source["audio_type"].exists()) {audio_source["audio_type"].remove();}
            audio_source["audio_type"][0] = string("aac");
        }else if (8000 == p_user_conf->audio_sample_rate){
            record_engine["filters"][2]=string("audio-source-8k");
            record_engine["connection"]["audio_source_8k"]["output"][0] = string("file-muxer-0");
            record_engine["connection"]["file_muxer_0"]["input"][1] = string("audio_source_8k");
            muxer["audio_type"] = string("g726_32");
            muxer["audio_sample_rate"] = 8000;
            file_muxer_config["audio_type"][0] = string("g726_32-8k");
            audio_source["interface"] = string("raw");
            audio_g711_8k["encode_frame_time_length"] = 40;
        }else{
            LOG_ERROR("audio sample rate =%d not supported\n", p_user_conf->audio_sample_rate);
        }
    }

    if(p_record_engine){
        (*p_record_engine).save();
    }
    if(p_filter_file_muxer){
        (*p_filter_file_muxer).save();
    }
    if(p_muxer){
        (*p_muxer).save();
    }
    if(p_features){
        (*p_features).save();
    }
    if(p_lbr){
        (*p_lbr).save();
    }
    if(p_motion_detect){
        (*p_motion_detect).save();
    }
    if(p_hdr_resource_limit){
        (*p_hdr_resource_limit).save();
    }
    if(p_audio_source){
        (*p_audio_source).save();
    }
    if(p_filter_player){
        (*p_filter_player).save();
    }
    if(p_audio_g711_8k){
        (*p_audio_g711_8k).save();
    }
    if(p_audio_aac_48k){
        (*p_audio_aac_48k).save();
    }
    return true;
}

OryxVideoModule* OryxVideoModule::get_instance()
{
    static OryxVideoModule instance;
    return &instance;
}

OryxVideoModule::OryxVideoModule():
    m_image(nullptr),
    m_video_camera(nullptr),
    m_lbr(nullptr),
    m_md(nullptr),
    m_ol(nullptr),
    m_stream0_overlay_area0(nullptr),
    m_area0_data0(nullptr),
    m_am_vca(nullptr)
{
}

OryxVideoModule::~OryxVideoModule()
{
}

bool OryxVideoModule::start_camera()
{
    m_video_camera = AMIVideoCamera::get_instance();
    if (!m_video_camera) {
        LOG_ERROR("Failed to get AMIVideoCamera instance!");
        return false;
    }
    if (m_video_camera->start() != AM_RESULT_OK) { //for fastboot, encoding is always on
        LOG_ERROR("Start encoding failed\n");
        return false;
    }
    return true;
}

bool OryxVideoModule::stop_camera()
{
    if (m_video_camera && m_video_camera->stop() != AM_RESULT_OK) {
        LOG_ERROR("Stop encoding failed!\n");
        return false;
    }
    return true;
}

bool OryxVideoModule::start_linux_aaa(bool enable)
{
    if (!enable) {
        return true;
    }
    m_image = AMIImageQuality::get_instance();
    if (!m_image) {
        LOG_ERROR("failed to get ImageQuality instance\n");
        return false;
    }
    if (!m_image->start()) {
        LOG_ERROR("Start image module failed\n");
        return false;
    }

    return true;
}

bool OryxVideoModule::start_overlay(const char *label)
{
    int stream0_area0_id = -1;
    bool ret = false;
    string osd_label(label);

    do {
        if (!m_video_camera)
            break;
        if (osd_label.empty()){
            LOG_DEBUG("No overlay label assigned.\n");
            ret = true;
            break;
        }
        m_ol = (AMIEncodeOverlay*) m_video_camera->get_video_plugin(
        VIDEO_PLUGIN_OVERLAY);
        if (!m_ol) {
            LOG_ERROR("overlay plugin is not loaded!\n");
            break;
        }

        osd_label = osd_label.append(" ", 1);
        printf("osd_label: %s\n", osd_label.c_str());

        m_stream0_overlay_area0 = new AMOverlayAreaAttr;
        if (NULL == m_stream0_overlay_area0)
            {
            LOG_ERROR("malloc memory for AMOverlayAreaAttr failed! \n");
            break;
        }
        m_area0_data0 = new AMOverlayAreaData;
        if (NULL == m_area0_data0)
            {
            LOG_ERROR("malloc memory for AMOverlayAreaData failed! \n");
            break;
        }

        m_stream0_overlay_area0->enable = 1;
        m_stream0_overlay_area0->rotate = 0;
        m_stream0_overlay_area0->rect.offset.x = 0;
        m_stream0_overlay_area0->rect.offset.y = 0;
        m_stream0_overlay_area0->rect.size.width = 800;
        m_stream0_overlay_area0->rect.size.height = 100;
        m_stream0_overlay_area0->bg_color = {0,0,0,0}; //test color:full transparent

        m_area0_data0->rect.offset.x = 0;
        m_area0_data0->rect.offset.y = 0;
        m_area0_data0->rect.size.width = 750;
        m_area0_data0->rect.size.height = 90;
        m_area0_data0->type = AM_OVERLAY_DATA_TYPE_TIME;
        m_area0_data0->time.text.font.width = 30; //font size
        m_area0_data0->time.text.font.height = 30;
        m_area0_data0->time.text.font_color.id = 0; //font color:white
        m_area0_data0->time.pre_str = osd_label; //time prefix
        m_area0_data0->time.suf_str = "";

        LOG_PRINT("init overlay ok\n");

        if ((stream0_area0_id = m_ol->init_area(AM_STREAM_ID(0),
                                                *m_stream0_overlay_area0)) < 0)
            {
            LOG_ERROR("init_area failed\n");
            break;
        }
        if (m_ol->add_data_to_area(AM_STREAM_ID(0), stream0_area0_id,
                                   *m_area0_data0) < 0)
            {
            LOG_ERROR("add_data_to_area failed\n");
            break;
        }

        ret = true;
        LOG_PRINT("add overlay ok\n");
    } while (0);

    return ret;
}

bool OryxVideoModule::start_smart_avc(bool enable)
{
    bool ret = false;
    AMGOP h264_gop;
    h264_gop.stream_id = AM_STREAM_ID_0;
    h264_gop.N = 90;
    h264_gop.idr_interval = 1;

    do {
        if (!m_video_camera){
            break;
        }
        if (!enable){
            ret = true;
            break;
        }

        if (m_video_camera->set_h26x_gop(h264_gop)){
            LOG_ERROR("modify h264 GOP to %u failed\n", h264_gop.N);
            break;
        }
        m_am_vca = (AMIVCA*)m_video_camera->get_video_plugin(VIDEO_PLUGIN_VCA);
        if (!m_am_vca) {
            LOG_ERROR("AM VCA plugin is not loaded!\n");
            break;
        }
        if (!m_am_vca->start()) {
            LOG_ERROR("Start AM VCA loop failed!\n");
            break;
        }
        m_md = (AMIMotionDetect*) m_am_vca->get_plugin_interface(VCA_PLUGIN_MD);
        if (!m_md) {
            LOG_ERROR("motion detect plugin is not loaded!\n");
            break;
        }
        m_lbr = (AMILBRControl*) m_video_camera->get_video_plugin(
            VIDEO_PLUGIN_LBR);
        if (!m_lbr) {
            LOG_ERROR("LBR plugin is not loaded!\n");
            break;
        }

        if (m_md->set_md_callback(this, receive_data_from_md) == 0) {
            LOG_ERROR("Set motion detect callback failed\n");
            break;
        }
        if (!m_md->start()) {
            LOG_ERROR("Start motion detect failed!\n");
            break;
        }
        if (!m_lbr->start()) {
            LOG_ERROR("Start LBR failed!\n");
            break;
        }
        LOG_PRINT("Low bitrate control start.\n");
        ret = true;
    }while(0);

    return ret;
}

bool OryxVideoModule::set_bitrate(int target_bitrate)
{
    AMBitrate brate;
    brate.stream_id = AM_STREAM_ID(AM_STREAM_ID_0);
    if (m_video_camera->get_bitrate(brate)) {
        LOG_ERROR("fail to get bitrate\n");
        return false;
    }
    brate.target_bitrate = target_bitrate;
    if (m_video_camera->set_bitrate(brate)) {
        LOG_ERROR("fail to set bitrate\n");
        return false;
    }
    LOG_PRINT("set bitrate to %d\n", brate.target_bitrate);

    return true;
}

bool OryxVideoModule::stop_linux_aaa()
{
    if (m_image && !m_image->stop()) {
        LOG_ERROR("Stop image module failed\n");
        return false;
    }
    return true;
}

bool OryxVideoModule::stop_overlay()
{
    if(m_ol)
    {
        if(m_stream0_overlay_area0)
        {
            delete m_stream0_overlay_area0;
            m_stream0_overlay_area0 = NULL;
        }
        if(m_area0_data0)
        {
            delete m_area0_data0;
            m_area0_data0 = NULL;
        }
    }
    return true;
}

bool OryxVideoModule::stop_smart_avc()
{
    /* no need to destroy() m_lbr and m_md addidionally */
    if (m_lbr && !m_lbr->stop()) {
        LOG_ERROR("Stop LBR plugin failed!\n");
        return false;
    }
    if (m_md && !m_md->stop()) {
        LOG_ERROR("Stop motion detect plugin failed!\n");
        return false;
    }
    return true;
}

int OryxVideoModule::receive_data_from_md(void *owner, AMMDMessage *info)
{
    OryxVideoModule *self = (OryxVideoModule*) owner;
    self->m_lbr->process_motion_info(info);
    return 0;
}