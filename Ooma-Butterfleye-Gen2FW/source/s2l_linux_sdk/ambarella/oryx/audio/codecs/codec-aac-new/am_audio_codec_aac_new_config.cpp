/*******************************************************************************
 * am_audio_codec_aac_config.cpp
 *
 * History:
 *   2014-11-4 - [ypchang] created file
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

#include "am_audio_codec_aac_new_config.h"
#include "am_configure.h"

AMAacConfig::~AMAacConfig()
{
  delete m_config;
  delete m_aac_config;
}

AacConfig* AMAacConfig::get_config(const std::string& conf_file)
{
  AacConfig *config = nullptr;
  do {
    if (AM_LIKELY(nullptr == m_aac_config)) {
      m_aac_config = new AacConfig();
    }
    if (AM_UNLIKELY(!m_aac_config)) {
      ERROR("Failed to create AudioCodecAacConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &aac = *m_config;

      /* Encoder settings */
      if (AM_LIKELY(aac["encode"].exists())) {
        /* encode output size */
        if (AM_LIKELY(aac["encode"]["enc_out_buf_size"].exists())) {
          m_aac_config->encode.enc_out_buf_size =
              aac["encode"]["enc_out_buf_size"].get<unsigned>(1636);
        } else {
          NOTICE("\"enc_out_buf_size\" is not specified, use default!");
          m_aac_config->encode.enc_out_buf_size = 6144 / 8 * 2 + 100;
        }

        /* format */
        if (AM_LIKELY(aac["encode"]["format"].exists())) {
          std::string format = aac["encode"]["format"].get<std::string>("aac");
          if (is_str_equal(format.c_str(), "aac")) {
            m_aac_config->encode.format = AACPLAIN;
          } else if (is_str_equal(format.c_str(), "aacplus")) {
            m_aac_config->encode.format = AACPLUS;
          } else if (is_str_equal(format.c_str(), "aacplusps")) {
            m_aac_config->encode.format = AACPLUS_PS;
          } else {
            NOTICE("Unsupported format: %s, use default!");
            m_aac_config->encode.format = AACPLAIN;
          }
        } else {
          NOTICE("\"format\" is not specified, use default!");
          m_aac_config->encode.format = AACPLUS;
        }

        /* bitrate */
        if (AM_LIKELY(aac["encode"]["bitrate"].exists())) {
          m_aac_config->encode.bitrate =
              aac["encode"]["bitrate"].get<unsigned>(48000);
        } else {
          NOTICE("\"bitrate\" is not specified, use default!");
          m_aac_config->encode.bitrate = 48000;
        }

        /* fftype */
        if (AM_LIKELY(aac["encode"]["fftype"].exists())) {
          std::string fftype = aac["encode"]["fftype"].get<std::string>("t");
          const char *fftype_str = fftype.c_str();
          m_aac_config->encode.fftype = fftype_str[0];
        } else {
          NOTICE("\"fftype\" is not specified, use default!");
          m_aac_config->encode.fftype = 't';
        }

        /* tns */
        if (AM_LIKELY(aac["encode"]["tns"].exists())) {
          m_aac_config->encode.tns = aac["encode"]["tns"].get<unsigned>(1);
        } else {
          NOTICE("\"tns\" is not specified, use default!");
          m_aac_config->encode.tns = 1;
        }

        /* pns */
        if (AM_LIKELY(aac["encode"]["pns"].exists())) {
          m_aac_config->encode.pns = aac["encode"]["pns"].get<unsigned>(1);
        } else {
          NOTICE("\"pns\" is not specified, use default!");
          m_aac_config->encode.pns = 1;
        }

        /* crc */
        if (AM_LIKELY(aac["encode"]["crc"].exists())) {
          m_aac_config->encode.crc = aac["encode"]["crc"].get<unsigned>(1);
        } else {
          NOTICE("\"crc\" is not specified, use default!");
          m_aac_config->encode.crc = 1;
        }

        /* perceptual mode */
        if (AM_LIKELY(aac["encode"]["perceptual_mode"].exists())) {
          std::string quality =
              aac["encode"]["perceptual_mode"].get<std::string>("normal");
          if (is_str_equal(quality.c_str(), "normal")) {
            m_aac_config->encode.perceptual_mode = AAC_ENCODE_NORMAL;
          } else if (is_str_equal(quality.c_str(), "fine")) {
            m_aac_config->encode.perceptual_mode = AAC_ENCODE_FINE;
          } else if (is_str_equal(quality.c_str(), "finest")) {
            m_aac_config->encode.perceptual_mode = AAC_ENCODE_SUPER_FINE;
          } else {
            NOTICE("Invalid perceptual mode setting: %s, "
                   "use \"normal\" as default",
                   quality.c_str());
            m_aac_config->encode.perceptual_mode = AAC_ENCODE_NORMAL;
          }
        } else {
          NOTICE("\"perceptual_mode\" is not specified, use default!");
          m_aac_config->encode.perceptual_mode = AAC_ENCODE_NORMAL;
        }

        /* quantizer quality */
        if (AM_LIKELY(aac["encode"]["quantizer_quality"].exists())) {
          std::string quality =
              aac["encode"]["quantizer_quality"].get<std::string>("high");
          if (is_str_equal(quality.c_str(), "low")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_LOW;
          } else if (is_str_equal(quality.c_str(), "high")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
          } else if (is_str_equal(quality.c_str(), "highest")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGHEST;
          } else {
            NOTICE("Invalid quantizer quality setting: %s, use high as default",
                   quality.c_str());
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
          }
        } else {
          NOTICE("\"quantizer_quality\" is not specified, use default!");
          m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
        }
      } else {
        ERROR("Invalid AAC configuration! Encoder settings not found!");
        break;
      }

      /* Decoder settings */
      if (AM_LIKELY(aac["decode"].exists())) {
        /* decode output buffer size */
        if (AM_LIKELY(aac["decode"]["dec_out_buf_size"].exists())) {
          m_aac_config->decode.dec_out_buf_size =
              aac["decode"]["dec_out_buf_size"].get<unsigned>(16384);
        } else {
          NOTICE("\"dec_buf_size\" is not specified, use default!");
          m_aac_config->decode.dec_out_buf_size = 16384;
        }

        /* downsampled_sbr */
        if (AM_LIKELY(aac["decode"]["downsampled_sbr"].exists())) {
          m_aac_config->decode.downsampled_sbr =
              aac["decode"]["downsampled_sbr"].get<int>(0);
        } else {
          NOTICE("\"downsampled_sbr\" is not specified, use default!");
          m_aac_config->decode.downsampled_sbr = 0;
        }

        /* downmix */
        if (AM_LIKELY(aac["decode"]["downmix"].exists())) {
          m_aac_config->decode.downmix =
              aac["decode"]["downmix"].get<int>(0);
        } else {
          NOTICE("\"downmix\" is not specified, use default!");
          m_aac_config->decode.downmix = 0;
        }
      } else {
        ERROR("Invalid AAC configuration! Decoder settings not found!");
        break;
      }
    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }

    config = m_aac_config;
  }while(0);

  return config;
}

