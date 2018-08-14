/**
 * am_video_camera.h
 *
 *  History:
 *    Jul 20, 2015 - [Shupeng Ren] created file
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
 */
#ifndef ORYX_VIDEO_INCLUDE_AM_VIDEO_CAMERA_H_
#define ORYX_VIDEO_INCLUDE_AM_VIDEO_CAMERA_H_
#include "am_video_camera_if.h"

class AMEncodeDevice;
class AMDPTZ;
class AMVideoCamera : public AMIVideoCamera
{
    friend AMIVideoCamera;

  public:
    AM_RESULT start()                                                  override;
    AM_RESULT start_stream(AM_STREAM_ID id)                            override;
    AM_RESULT stop()                                                   override;
    AM_RESULT stop_stream(AM_STREAM_ID id)                             override;

    AM_RESULT idle()                                                   override;
    AM_RESULT preview()                                                override;
    AM_RESULT encode()                                                 override;
    AM_RESULT decode()                                                 override;

    uint32_t get_vsrc_id(uint32_t channel_id)                          override;
    AM_RESULT get_vin_status(AMVinInfo &vin)                           override;
    AM_RESULT get_stream_status(AM_STREAM_ID id,
                                AM_STREAM_STATE &state)                override;

    AM_RESULT get_buffer_state(AM_SOURCE_BUFFER_ID id,
                               AM_SRCBUF_STATE &state)                 override;

    AM_RESULT get_buffer_format(AMBufferConfigParam &param)            override;
    AM_RESULT set_buffer_format(const AMBufferConfigParam &param)      override;
    AM_RESULT save_buffer_config()                                     override;
    AM_RESULT get_canvas_state(uint8_t canvas_id,
                               AM_MULTI_VIN_CANVAS_STATE &state)       override;
    AM_RESULT get_canvas_info(AMMultiVinCanvasParam &canvas)           override;

    uint32_t  get_channel_num()                                        override;
    uint32_t  get_encode_stream_max_num()                              override;
    uint32_t  get_source_buffer_max_num()                              override;
    AM_RESULT get_bitrate(AMBitrate &bitrate)                          override;
    AM_RESULT set_bitrate(const AMBitrate &bitrate)                    override;
    AM_RESULT get_framerate(AMFramerate &rate)                         override;
    AM_RESULT set_framerate(const AMFramerate &rate)                   override;

    AM_RESULT stop_vin()                                               override;

    AM_RESULT halt_vout(AM_VOUT_ID id)                                 override;

    AM_RESULT force_idr(AM_STREAM_ID stream_id)                        override;

    AM_RESULT get_ldc_state(uint8_t channel_id, bool &state)           override;

    void* get_video_plugin(const std::string& plugin_name)             override;
    void* get_vca_plugin(const std::string& plugin_name)               override;

    AM_RESULT load_config_all()                                        override;

  public:
    //For configure files operation
    AM_RESULT get_feature_config(AMFeatureParam &config)               override;
    AM_RESULT set_feature_config(const AMFeatureParam &config)         override;
    AM_RESULT get_vin_config(AMVinParamMap &config)                    override;
    AM_RESULT set_vin_config(const AMVinParamMap &config)              override;
    AM_RESULT get_vout_config(AMVoutParamMap &config)                  override;
    AM_RESULT set_vout_config(const AMVoutParamMap &config)            override;
    AM_RESULT get_buffer_config(AMBufferParamMap &config)              override;
    AM_RESULT set_buffer_config(const AMBufferParamMap &config)        override;
    AM_RESULT get_stream_config(AMStreamParamMap &config)              override;
    AM_RESULT set_stream_config(const AMStreamParamMap &config)        override;

    //For dynamically changing
    AM_RESULT get_mjpeg_info(AMMJpegInfo &mjpeg)                       override;
    AM_RESULT set_mjpeg_info(const AMMJpegInfo &mjpeg)                 override;
    AM_RESULT get_h26x_gop(AMGOP &h264)                                override;
    AM_RESULT set_h26x_gop(const AMGOP &h264)                          override;
    AM_RESULT get_abs_bitrate(AM_STREAM_ID stream_id, bool &enable)    override;
    AM_RESULT set_abs_bitrate(AM_STREAM_ID stream_id, bool enable)     override;
    AM_RESULT get_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)   override;
    AM_RESULT set_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)   override;
    AM_RESULT get_stream_size(AM_STREAM_ID id, AMResolution &res)      override;
    AM_RESULT set_stream_size(AM_STREAM_ID id, AMResolution &res)      override;
    AM_RESULT get_stream_offset(AM_STREAM_ID id, AMOffset &offset)     override;
    AM_RESULT set_stream_offset(AM_STREAM_ID id,
                                const AMOffset &offset)                override;
    AM_RESULT get_stream_source(AM_STREAM_ID id,
                                AM_SOURCE_BUFFER_ID &source)           override;
    AM_RESULT set_stream_source(AM_STREAM_ID id,
                                AM_SOURCE_BUFFER_ID &source)           override;
    AM_RESULT get_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip)    override;
    AM_RESULT set_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip)    override;
    AM_RESULT get_stream_rotate(AM_STREAM_ID id, AM_VIDEO_ROTATE &rot) override;
    AM_RESULT set_stream_rotate(AM_STREAM_ID id, AM_VIDEO_ROTATE &rot) override;
    AM_RESULT get_stream_profile(AM_STREAM_ID id, AM_PROFILE &profile) override;
    AM_RESULT set_stream_profile(AM_STREAM_ID id, AM_PROFILE &profile) override;
    AM_RESULT save_stream_config()                                     override;
    AM_RESULT get_avail_cpu_clks(std::map<int32_t, int32_t> &clks)     override;
    AM_RESULT get_cur_cpu_clk(int32_t &clk)                            override;
    AM_RESULT set_cpu_clk(int32_t index)                               override;
    AM_RESULT get_working_mode(AM_WORKING_MODE &mode)                  override;
    AM_RESULT goto_vca_mode()                                          override;
    AM_RESULT goto_iav_current_mode()                                  override;
    AM_RESULT goto_iav_unloaded_mode()                                 override;

  private:
    static AMVideoCamera* get_instance();
    static AMVideoCamera* create();
    void inc_ref();
    void release();
    AMVideoCamera();
    virtual ~AMVideoCamera();
    AM_RESULT init();

  private:
    static AMVideoCamera   *m_instance;
    static recursive_mutex  m_mtx;

  private:
    AMEncodeDevice         *m_device        = nullptr;
    AMIPlatformPtr          m_platform      = nullptr;
    AMVinConfigPtr          m_vin_config    = nullptr;
    AMVoutConfigPtr         m_vout_config   = nullptr;
#if !defined(CONFIG_AMBARELLA_ORYX_VIDEO_IAV4)
    AMBufferConfigPtr       m_buffer_config = nullptr;
#endif
    AMStreamConfigPtr       m_stream_config = nullptr;
    atomic_int              m_ref_cnt       = {0};
};

#endif /* ORYX_VIDEO_INCLUDE_AM_VIDEO_CAMERA_H_ */
