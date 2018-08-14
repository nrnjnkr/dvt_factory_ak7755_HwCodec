/*******************************************************************************
 * modules.cpp
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

#include "modules.h"
#include "am_define.h"

AMOryxModules::AMOryxModules()
{
}

AMOryxModules::~AMOryxModules()
{
#if 0
  m_image = nullptr;
#endif
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
  m_audio = nullptr;
#endif
  m_media = nullptr;
  m_video = nullptr;
  m_video_reader = nullptr;
  m_video_addr = nullptr;
  AM_DESTROY(m_lbr);
  AM_DESTROY(m_md);
  AM_DESTROY(m_audio_capture);
}

AMOryxModules* AMOryxModules::create(ORYX_MODULE_MODE mode)
{
  AMOryxModules* ret = new AMOryxModules();

  if (ret && (ret->init(mode) != AM_RESULT_OK)) {
    delete ret;
    ERROR("create failed!!!!\n");
    ret = nullptr;
  }

  return ret;
}

static void record_callback(AMRecordMsg &msg)
{
  switch(msg.msg) {
    case AM_RECORD_MSG_START_OK:
      NOTICE("Start OK!");
      break;
    case AM_RECORD_MSG_STOP_OK:
      NOTICE("Stop OK!");
      break;
    case AM_RECORD_MSG_ERROR:
      NOTICE("Error Occurred!");
      break;
    case AM_RECORD_MSG_ABORT:
      NOTICE("Recording Aborted!");
      break;
    case AM_RECORD_MSG_EOS:
      NOTICE("Recording Finished!");
      break;
    case AM_RECORD_MSG_TIMEOUT:
      NOTICE("Operation Timeout!");
      break;
    case AM_RECORD_MSG_OVER_FLOW:
    default: break;
  }
}

AM_RESULT AMOryxModules::init(ORYX_MODULE_MODE mode)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    m_mode = mode;
    m_video= AMIVideoCamera::get_instance();
    if (!m_video) {
      ERROR("VideoService get instance failed\n");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (m_mode == ORYX_MODULE_MODE_FAST) {
      std::string interface = "raw";
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
      m_audio = AMIAudioDevice::create("pulse");
      if (!m_audio) {
        ERROR("Create audio instance failed\n");
        ret = AM_RESULT_ERR_MEM;
        break;
      }
      interface = "pulse";
#endif
      m_audio_capture = create_audio_capture(interface,
                                             "oryx_module_capture",
                                             this,
                                             audio_callback);
      if (AM_UNLIKELY(!m_audio_capture)) {
        ERROR("Failed to create audio capture with interface [%s]",
              interface.c_str());
        ret = AM_RESULT_ERR_MEM;
        break;
      }

      m_video_reader = AMIVideoReader::get_instance();
      if (!m_video_reader) {
        ERROR("Get video reader instance failed\n");
        ret = AM_RESULT_ERR_MEM;
        break;
      }

      m_video_addr = AMIVideoAddress::get_instance();
      if (!m_video_addr) {
        ERROR("Get video address instance failed\n");
        ret = AM_RESULT_ERR_MEM;
        break;
      }
    } else {
      if (m_mode == ORYX_MODULE_MODE_NORMAL) {
        m_image = AMIImageQuality::get_instance();
        if (!m_image) {
          ERROR("ImageQuality get instance failed\n");
          ret = AM_RESULT_ERR_MEM;
          break;
        }
      }
      m_media = AMIRecord::create();
      if (!m_media) {
        ERROR("Create media record instance failed\n");
        ret = AM_RESULT_ERR_MEM;
        break;
      }
      if (!(m_media->init())) {
        ERROR("Failed to initialize media record.");
        ret = AM_RESULT_ERR_MEM;
        break;
      }
      m_media->set_msg_callback(record_callback, nullptr);
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::start()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_mode == ORYX_MODULE_MODE_NORMAL) {
      if (m_image && !m_image->start()) {
        ERROR("Start image module failed\n");
        ret = AM_RESULT_ERR_UNKNOWN;
        break;
      }
    }
    if (m_video && (ret = m_video->start()) != AM_RESULT_OK) {
      ERROR("Start video module failed\n");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::stop()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_mode == ORYX_MODULE_MODE_NORMAL) {
      if (m_image && !m_image->stop()) {
        ERROR("Stop image module failed\n");
        ret = AM_RESULT_ERR_UNKNOWN;
        break;
      }
    }
    if (m_video && (ret = m_video->stop()) != AM_RESULT_OK) {
      ERROR("Stop video module failed\n");
      break;
    }

    if ((ret = stop_record_engine()) != AM_RESULT_OK) {
      ERROR("Stop media module failed\n");
      break;
    }

    if ((ret = stop_audio_capture()) != AM_RESULT_OK) {
      ERROR("Stop audio capture module failed\n");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::destory()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((ret = stop()) != AM_RESULT_OK) {
      break;
    }

    delete this;
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::start_record_engine()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_media && !m_media->is_recording() && !m_media->start()) {
      ERROR("Start media record module failed\n");
      ret = AM_RESULT_ERR_UNKNOWN;
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::stop_record_engine()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_media) {
      if (m_media->is_recording() && !m_media->stop()) {
        ERROR("Stop media module failed\n");
        ret = AM_RESULT_ERR_UNKNOWN;
        break;
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::start_file_recording(uint32_t muxer_id)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((ret = start_record_engine()) == AM_RESULT_OK) {
      if (!m_media->start_file_recording(muxer_id)) {
        break;
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::stop_file_recording(uint32_t muxer_id)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_media) {
      if (!m_media->stop_file_recording(muxer_id)) {
        break;
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::start_all_video_plugins()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video) {
      if (!(m_md =
          (AMIMotionDetect*)m_video->get_vca_plugin(VCA_PLUGIN_MD))) {
        WARN("MD plugin is not loaded!\n");
      }
      if (m_md) {
        if (!(m_lbr =
            (AMILBRControl*)m_video->get_video_plugin(VIDEO_PLUGIN_LBR))) {
          WARN("LBR plugin is not loaded!\n");
        }
        if (!(m_md->set_md_callback(this, receive_data_from_md))) {
          ERROR("Set MD callback failed");
          break;
        }
        if (!(m_md->start())) {
          ERROR("Start MD failed!\n");
          break;
        }
        if (m_lbr && !(m_lbr->start())) {
          ERROR("Start LBR failed!\n");
          break;
        }
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::stop_all_video_plugins()
{
  AM_RESULT ret = AM_RESULT_OK;
  if (m_lbr) {
    m_lbr->stop();
  }
  if (m_md) {
    m_md->stop();
  }

  return ret;
}

AM_RESULT AMOryxModules::start_image()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!m_image) {
      if (!(m_image = AMIImageQuality::get_instance())) {
        ERROR("ImageQuality get instance failed\n");
        ret = AM_RESULT_ERR_MEM;
        break;
      }
    }
    if (!(m_image->start())) {
      ERROR("failed start image");
      ret = AM_RESULT_ERR_UNKNOWN;
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::stop_image()
{
  AM_RESULT ret = AM_RESULT_OK;
  if (m_image && !(m_image->start())) {
    ERROR("failed stop image");
    ret = AM_RESULT_ERR_UNKNOWN;
  }

  return ret;
}

int AMOryxModules::receive_data_from_md(void *owner, AMMDMessage *info)
{
  AMOryxModules *self = (AMOryxModules*)owner;
  self->md_data_process(info);
  return 0;
}

void AMOryxModules::md_data_process(AMMDMessage *info)
{
  AMILBRControl *lbr = (AMILBRControl*)m_lbr;
  if (lbr) {
    lbr->process_motion_info((void *)info);
  }
}

AM_RESULT AMOryxModules::goto_iav_current_state()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video) {
#if 0
      //if let start image in a separate process will saving some time
      AM_WORKING_MODE mode;
      if (AM_UNLIKELY((ret = m_video->get_working_mode(mode))
                      != AM_RESULT_OK)) {
        break;
      }
      if (mode == AM_WORKING_MODE_IAV_UNLOADED) {
        if (m_image && !m_image->start()) {
          ERROR("Start image module failed\n");
          ret = AM_RESULT_ERR_UNKNOWN;
          break;
        }
      }
#endif
      ret = m_video->goto_iav_current_mode();
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::query_video_frame(AMQueryFrameDesc &desc,
                                           uint32_t timeout)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video_reader &&
        ((ret = m_video_reader->query_video_frame(desc, timeout)) !=
        AM_RESULT_OK) && (ret != AM_RESULT_ERR_AGAIN)) {
      ERROR("query video frame failed: ret code=%d \n",ret);
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::query_stream_info(AMStreamInfo &info)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video_reader && (ret = m_video_reader->query_stream_info(info))
        != AM_RESULT_OK) {
      ERROR("query video stream info failed \n");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::video_addr_get(const AMQueryFrameDesc &desc, AMAddress &addr)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video_addr && (ret = m_video_addr->video_addr_get(desc, addr))
        != AM_RESULT_OK) {
      ERROR("Failed to get video address!");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::query_yuv_frame(AMQueryFrameDesc &desc,
                                         AM_SOURCE_BUFFER_ID id,
                                         bool latest_snapshot)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video_reader &&
        (ret = m_video_reader->query_yuv_frame(desc, id, latest_snapshot))
        != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::yuv_addr_get(const AMQueryFrameDesc &desc,
                                      AMAddress &y_addr,  AMAddress &uv_addr)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_video_addr) {
      if ((ret = m_video_addr->yuv_y_addr_get(desc, y_addr))
          != AM_RESULT_OK) {
        ERROR("Failed to get y address!");
        break;
      }
      if ((ret = m_video_addr->yuv_uv_addr_get(desc, uv_addr))
          != AM_RESULT_OK) {
        ERROR("Failed to get uv address!");
        break;
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::set_audio_parameters(uint32_t channel,
                                              uint32_t samplerate,
                                              uint32_t chunksize,
                                              AM_AUDIO_SAMPLE_FORMAT sampleformat,
                                              AudioCapCallback callback)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_audio_capture) {
      if (!m_audio_capture->set_channel(channel) ||
          !m_audio_capture->set_sample_rate(samplerate) ||
          !m_audio_capture->set_chunk_bytes(chunksize) ||
          !m_audio_capture->set_sample_format(sampleformat)) {
        ERROR("Set audio parameter failed");
        ret = AM_RESULT_ERR_INVALID;
        break;
      }
      m_user_audio_callback = callback;
    }
  } while (0);

  return ret;
}

AM_RESULT AMOryxModules::start_audio_capture(int32_t  volume)
{
  AM_RESULT ret = AM_RESULT_OK;
  if (m_audio_capture && !m_audio_capture->start(volume)) {
    ERROR("Start audio capture failed");
    ret = AM_RESULT_ERR_UNKNOWN;
  }

  return ret;
}

AM_RESULT AMOryxModules::stop_audio_capture()
{
  AM_RESULT ret = AM_RESULT_OK;
  if (m_audio_capture && !m_audio_capture->stop()) {
    ERROR("Stop audio capture failed!");
    ret = AM_RESULT_ERR_UNKNOWN;
  }

  return ret;
}

void AMOryxModules::audio_process(AudioPacket  *packet)
{
  if (m_user_audio_callback) {
    m_user_audio_callback(packet);
  }
}

void AMOryxModules::audio_callback(AudioCapture *data)
{
  ((AMOryxModules *)data->owner)->audio_process(&data->packet);
}
