/*******************************************************************************
 * am_audio_codec_opus_config.cpp
 *
 * History:
 *   2014-11-10 - [ccjing] created file
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_codec_opus_config.h"
#include "am_configure.h"

AMAudioCodecOpusConfig::AMAudioCodecOpusConfig() :
    m_config(nullptr),
    m_opus_config(nullptr)
{
}

AMAudioCodecOpusConfig::~AMAudioCodecOpusConfig()
{
  delete m_config;
  delete m_opus_config;
}

AudioCodecOpusConfig* AMAudioCodecOpusConfig::get_config(const std::string& conf_file)
{
  AudioCodecOpusConfig *config = nullptr;
  do {
    if (AM_LIKELY(nullptr == m_opus_config)) {
      m_opus_config = new AudioCodecOpusConfig();
    }
    if (AM_UNLIKELY(!m_opus_config)) {
      ERROR("Failed to create AudioCodecOpusConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &opus = *m_config;

      /*Encoder settings */
      if (AM_LIKELY(opus["encode"].exists())) {
        /* encode opus_complexity */
        if (AM_LIKELY(opus["encode"]["opus_complexity"].exists())) {
          m_opus_config->encode.opus_complexity =
              opus["encode"]["opus_complexity"].get<unsigned>(1);
        } else {
          NOTICE("\"opus_complexity\" is not specified, use default!");
          m_opus_config->encode.opus_complexity = 1;
        }
        /* opus_avg_bitrate*/
        if (AM_LIKELY(opus["encode"]["opus_avg_bitrate"].exists())) {
          m_opus_config->encode.opus_avg_bitrate =
              opus["encode"]["opus_avg_bitrate"].get<unsigned>(48000);
        } else {
          NOTICE("\"opus_avg_btirate\" is not specified, use default!");
          m_opus_config->encode.opus_avg_bitrate = 48000;
        }
        /*enc_output_size*/
        if (AM_LIKELY(opus["encode"]["enc_output_size"].exists())) {
          m_opus_config->encode.enc_output_size =
              opus["encode"]["enc_output_size"].get<unsigned>(4096);
        } else {
          NOTICE("\"enc_output_size\" is not specified, use default!");
          m_opus_config->encode.enc_output_size = 4096;
        }
        /*enc_frame_time_length*/
        if (AM_LIKELY(opus["encode"]["enc_frame_time_length"].exists())) {
          m_opus_config->encode.enc_frame_time_length =
              opus["encode"]["enc_frame_time_length"].get<unsigned>(40);
          if (AM_UNLIKELY(m_opus_config->encode.enc_frame_time_length > 120)) {
            WARN("Opus maximum frame length should be less then 120ms, "
                 "current value is %u, reset to 40ms!",
                 m_opus_config->encode.enc_frame_time_length);
            m_opus_config->encode.enc_frame_time_length = 40;
          }
        } else {
          NOTICE("\"enc_frame_time_length\" is not specified, use default!");
          m_opus_config->encode.enc_frame_time_length = 20;
        }

        if (AM_LIKELY(opus["encode"]["repacketize"].exists())) {
          m_opus_config->encode.repacketize =
              opus["encode"]["repacketize"].get<bool>(true);
        } else {
          NOTICE("\"repacketize\" is not specified, use default!");
          m_opus_config->encode.repacketize = true;
        }
      } else {
        ERROR("Invalid opus configuration! Encoder setting not found!");
        break;
      }

      /*Decoder settings */
      if (AM_LIKELY(opus["decode"].exists())) {
        /*dec_output_max_time_length setting*/
        if (AM_LIKELY(opus["decode"]["dec_output_max_time_length"].exists())) {
          m_opus_config->decode.dec_output_max_time_length =
              opus["decode"]["dec_output_max_time_length"].get<unsigned>(120);
        } else {
          NOTICE("\"dec_output_max_time_length\" is not specified, "
                 "use default!");
          m_opus_config->decode.dec_output_max_time_length = 120;
        }
        /*dec_output_sample_rate setting*/
        if (AM_LIKELY(opus["decode"]["dec_output_sample_rate"].exists())) {
          m_opus_config->decode.dec_output_sample_rate =
              opus["decode"]["dec_output_sample_rate"].get<unsigned>(48000);
        } else {
          NOTICE("\"dec_output_sample_rate\" is not specified, use default!");
          m_opus_config->decode.dec_output_sample_rate = 48000;
        }
        /* dec_output_channel setting */
        if (AM_LIKELY(opus["decode"]["dec_output_channel"].exists())) {
          m_opus_config->decode.dec_output_channel =
              opus["decode"]["dec_output_channel"].get<unsigned>(2);
        } else {
          NOTICE("\"dec_output_channel\" is not specified, use default!");
          m_opus_config->decode.dec_output_channel = 2;
        }
      } else {
        ERROR("Invalid opus configuration! Decoder setting not found!");
      }
    }
    config = m_opus_config;
  } while (0);

  return config;
}
