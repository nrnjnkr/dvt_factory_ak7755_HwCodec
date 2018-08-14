/*******************************************************************************
 * modules.h
 *
 * History:
 *   Feb 9, 2017 - [Huaiqing Wang] created file
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
 ******************************************************************************/
#ifndef ORYX_APP_MODULES_DEMO_MODULES_H_
#define ORYX_APP_MODULES_DEMO_MODULES_H_

#include <stdint.h>
#include <stddef.h>
#include <string>
#include "config.h"
#include "am_record_if.h"
#include "am_audio_capture_if.h"
#include "am_video_camera_if.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_low_bitrate_control_if.h"
#include "am_motion_detect_if.h"
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
#include "am_audio_device_if.h"
#endif
#include "am_image_quality_if.h"
#include "am_video_utility.h"

enum ORYX_MODULE_MODE
{
  ORYX_MODULE_MODE_NORMAL,
  ORYX_MODULE_MODE_FAST,
  ORYX_MODULE_MODE_FAST_RECORD,
  ORYX_MODULE_MODE_UNKNOWN
};

typedef void (*AudioCapCallback)(AudioPacket *packet);
class AMOryxModules {
  public:
    static AMOryxModules *create(ORYX_MODULE_MODE mode);
    AM_RESULT start();
    AM_RESULT stop();
    AM_RESULT destory();

  public:
    AM_RESULT start_file_recording(uint32_t muxer_id = 0xffffffff);
    AM_RESULT stop_file_recording(uint32_t muxer_id = 0xffffffff);
    AM_RESULT query_video_frame(AMQueryFrameDesc &desc, uint32_t timeout = -1);
    AM_RESULT query_stream_info(AMStreamInfo &info);
    AM_RESULT video_addr_get(const AMQueryFrameDesc &desc, AMAddress &addr);
    AM_RESULT query_yuv_frame(AMQueryFrameDesc &desc, AM_SOURCE_BUFFER_ID id,
                              bool latest_snapshot);
    AM_RESULT yuv_addr_get(const AMQueryFrameDesc &desc,
                           AMAddress &y_addr,  AMAddress &uv_addr);
    AM_RESULT set_audio_parameters(uint32_t channel,
                                   uint32_t samplerate,
                                   uint32_t chunksize,
                                   AM_AUDIO_SAMPLE_FORMAT sampleformat,
                                   AudioCapCallback callback);
    AM_RESULT start_audio_capture(int32_t  volume);
    AM_RESULT stop_audio_capture();
    AM_RESULT goto_iav_current_state();
    AM_RESULT start_all_video_plugins();
    AM_RESULT stop_all_video_plugins();
    AM_RESULT start_image();
    AM_RESULT stop_image();

  private:
    AMOryxModules();
    ~AMOryxModules();
    AM_RESULT init(ORYX_MODULE_MODE mode);
    AM_RESULT start_record_engine();
    AM_RESULT stop_record_engine();
    void      audio_process(AudioPacket  *packet);
    void      md_data_process(AMMDMessage *info);
    static void audio_callback(AudioCapture *data);
    static int receive_data_from_md(void *owner, AMMDMessage *info);

  private:
    ORYX_MODULE_MODE   m_mode                = ORYX_MODULE_MODE_UNKNOWN;
    AMIImageQualityPtr m_image               = nullptr;
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
    AMIAudioDevicePtr  m_audio               = nullptr;
#endif
    AMIRecordPtr       m_media               = nullptr;
    AMIVideoCameraPtr  m_video               = nullptr;
    AMIVideoReaderPtr  m_video_reader        = nullptr;
    AMIVideoAddressPtr m_video_addr          = nullptr;
    AMILBRControl      *m_lbr                = nullptr;
    AMIMotionDetect    *m_md                 = nullptr;
    AMIAudioCapture    *m_audio_capture      = nullptr;
    AudioCapCallback   m_user_audio_callback = nullptr;
};

#endif /* ORYX_APP_MODULES_DEMO_MODULES_H_ */
