/*******************************************************************************
 * am_audio_codec_mp3.cpp
 *
 * History:
 *   Sep 04 2017 - [ypchang] created file
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

#include "am_audio_codec_mp3.h"

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_utility.h"

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecMp3::create(config);
}

AMIAudioCodec* AMAudioCodecMp3::create(const char *config)
{
  AMAudioCodecMp3 *mp3 = new AMAudioCodecMp3();
  if (AM_UNLIKELY(mp3 && !mp3->init(config))) {
    delete mp3;
    mp3 = nullptr;
  }

  return ((AMIAudioCodec*)mp3);
}

void AMAudioCodecMp3::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecMp3::initialize(AM_AUDIO_INFO *srcAudioInfo,
                                 AM_AUDIO_CODEC_MODE mode)
{
  do {
    if (AM_UNLIKELY(!srcAudioInfo)) {
      ERROR("Invalid audio info, finalize!");
      finalize();
      m_is_initialized = false;
      memset(m_src_audio_info, 0, sizeof(*m_src_audio_info));
      break;
    }

    if (AM_LIKELY(m_is_initialized)) {
      NOTICE("Codec %s is already initialized to %s mode, re-initializing!",
             m_name.c_str(),
             codec_mode_string().c_str());
      finalize();
    }

    memcpy(m_src_audio_info, srcAudioInfo, sizeof(*m_src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_ENCODE],
           srcAudioInfo,
           sizeof(*srcAudioInfo));
    memcpy(&m_audio_info[AUDIO_INFO_DECODE],
           srcAudioInfo,
           sizeof(*srcAudioInfo));
    m_mode = mode;
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO &ainfo = m_audio_info[AUDIO_INFO_DECODE];
        memcpy(&m_mp3_header, ainfo.codec_info, sizeof(m_mp3_header));
        ainfo.codec_info    = nullptr;
        ainfo.sample_format = AM_SAMPLE_F32LE;
        ainfo.sample_size   = sizeof(float);
        do {
          int error = mpg123_init();
          if (AM_UNLIKELY(error != MPG123_OK)) {
            ERROR("Failed to initialize MPG123 library: %s",
                  mpg123_plain_strerror(error));
            break;
          }
          m_mpg123_init = true;
          m_handle = mpg123_new(nullptr, &error);
          if (AM_UNLIKELY(!m_handle)) {
            ERROR("Failed to create mpg123 handle: %s",
                  mpg123_plain_strerror(error));
            break;
          }
          error = mpg123_open_feed(m_handle);
          if (AM_UNLIKELY(error != MPG123_OK)) {
            ERROR("Failed to open feed for MPG123 library!");
            break;
          }
          mpg123_param(m_handle, MPG123_ADD_FLAGS, MPG123_NO_RESYNC, 0);
          mpg123_format_none(m_handle);
          error = mpg123_format(m_handle, ainfo.sample_rate,
                                MPG123_MONO | MPG123_STEREO,
                                MPG123_ENC_FLOAT_32);
          if (AM_UNLIKELY(error != MPG123_OK)) {
            ERROR("Failed to set MPG123 decoding format!");
            break;
          }
          m_is_initialized = true;
        }while(0);
      }break;
      default: {
        ERROR("Invalid MP3 codec mode!");
      }break;
    }
  }while(0);

  if (AM_UNLIKELY(!m_is_initialized)) {
    m_mode = AM_AUDIO_CODEC_MODE_NONE;
  }

  NOTICE("Audio codec %s is initialized to mode %s",
         m_name.c_str(),
         codec_mode_string().c_str());

  return m_is_initialized;
}

bool AMAudioCodecMp3::finalize()
{
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {

    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      m_info_updated = false;
      if (AM_LIKELY(m_handle)) {
        mpg123_delete(m_handle);
      }
      if (AM_LIKELY(m_mpg123_init)) {
        mpg123_exit();
        m_mpg123_init = false;
      }
    }break;
    default: break;
  }

  m_is_initialized = false;
  m_mode = AM_AUDIO_CODEC_MODE_NONE;

  return true;
}

AM_AUDIO_INFO* AMAudioCodecMp3::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = nullptr;
  switch (m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }
      break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }
      break;
    default:
      break;
  }

  return info;
}

uint32_t AMAudioCodecMp3::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {

    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      size = m_mp3_header.frame_sample_size() *
             m_mp3_header.channel_number() *
             sizeof(float) *
             (m_audio_info[AUDIO_INFO_DECODE].chunk_size /
              m_mp3_header.frame_bytes());
      m_dec_out_chunk = size;
      INFO("MP3 decoder output chunk size is %u bytes", m_dec_out_chunk);
    }break;
    default:break;
  }

  return size;
}

bool  AMAudioCodecMp3::check_encode_src_parameter(AM_AUDIO_INFO &info)
{
  return false;
}

uint32_t AMAudioCodecMp3::get_encode_required_chunk_size(AM_AUDIO_INFO &info)
{
  return 0;
}

uint32_t AMAudioCodecMp3::encode(uint8_t *input,  uint32_t in_data_size,
                                 uint8_t *output, uint32_t *out_data_size)
{
  return 0;
}

uint32_t AMAudioCodecMp3::decode(uint8_t *input,  uint32_t in_data_size,
                                 uint8_t *output, uint32_t *out_data_size)
{
  uint32_t ret = in_data_size;
  int error = 0;
  *out_data_size = 0;
  do {
    size_t outlen = 0;
    error = mpg123_feed(m_handle, input, in_data_size);
    if (AM_UNLIKELY(error != MPG123_OK)) {
      ERROR("Failed to feed data: %s", mpg123_plain_strerror(error));
      ret = ((uint32_t)-1);
      break;
    }
    do {
      uint32_t buffer_size = m_dec_out_chunk - *out_data_size;
      error = mpg123_replace_buffer(m_handle,
                                    output + (*out_data_size),
                                    buffer_size);
      if (AM_UNLIKELY(error != MPG123_OK)) {
        ERROR("Failed to replace buffer: %s", mpg123_plain_strerror(error));
        ret = ((uint32_t)-1);
        break;
      }
      error = mpg123_decode_frame(m_handle, nullptr, nullptr, &outlen);
      if (AM_UNLIKELY(error != MPG123_OK)) {
        switch(error) {
          case MPG123_NEW_FORMAT: {
            update_audio_info();
          }break;
          case MPG123_NEED_MORE: {
            ret = in_data_size;
          }break;
          case MPG123_NO_SPACE: {
            NOTICE("MPG123 output %u bytes already, space left %u: %s,",
                   *out_data_size,
                   buffer_size,
                   mpg123_plain_strerror(error));
          }break;
          default: {
            ERROR("MPG123 decoding error: %s", mpg123_plain_strerror(error));
          }break;
        }
      } else if (AM_LIKELY(!m_info_updated)) {
        update_audio_info();
      }
      if (AM_LIKELY((outlen > 0) && (outlen <= buffer_size))) {
        *out_data_size += outlen;
      }
    }while((*out_data_size < m_dec_out_chunk) && (error == MPG123_OK));

  }while(0);

  return ret;
}

void AMAudioCodecMp3::update_audio_info()
{
  mpg123_frameinfo info;
  int error = mpg123_info(m_handle, &info);
  if (AM_UNLIKELY(error != MPG123_OK)) {
    ERROR("mpg123_info failed: %s", mpg123_plain_strerror(error));
  } else {
    m_audio_info[AUDIO_INFO_DECODE].sample_rate = info.rate;
    switch(info.mode) {
      case MPG123_M_DUAL:
      case MPG123_M_STEREO:
      case MPG123_M_JOINT:
        m_audio_info[AUDIO_INFO_DECODE].channels = 2; break;
      case MPG123_M_MONO:
        m_audio_info[AUDIO_INFO_DECODE].channels = 1; break;
      default: break;
    }
    m_info_updated = true;
  }
}

AMAudioCodecMp3::AMAudioCodecMp3() :
    inherited(AM_AUDIO_CODEC_MP3, "mp3")
{}

AMAudioCodecMp3::~AMAudioCodecMp3()
{
  finalize();
}

bool AMAudioCodecMp3::init(const char* UNUSED(config))
{
  bool ret = true;
  do {
    ret = inherited::init();
    if (AM_UNLIKELY(!ret)) {
      ERROR("inherited init failed!");
      break;
    }
  } while (0);

  return ret;
}
