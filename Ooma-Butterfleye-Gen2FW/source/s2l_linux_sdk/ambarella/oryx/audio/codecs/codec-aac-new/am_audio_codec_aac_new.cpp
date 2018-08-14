/*******************************************************************************
 * am_audio_codec_aac.cpp
 *
 * History:
 *   2014-11-3 - [ypchang] created file
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

#include "am_audio_define.h"
#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_codec_aac_new.h"
#include "am_audio_codec_aac_new_config.h"
#include "am_audio_utility.h"

struct AacStatusMsg
{
    uint32_t    error;
    std::string message;
};

static AacStatusMsg encoder_status[] =
{
  {AAC_ENCODE_OK,                      "OK"},
  {AAC_ENCODE_INVALID_POINTER,         "Invalid pointer"},
  {AAC_ENCODE_FAILED,                  "Encode failed"},
  {AAC_ENCODE_UNSUPPORTED_SAMPLE_RATE, "Unsupported sample rate"},
  {AAC_ENCODE_UNSUPPORTED_CH_CFG,      "Unsupported channel configuration"},
  {AAC_ENCODE_UNSUPPORTED_BIT_RATE,    "Unsupported bit rate"},
  {AAC_ENCODE_UNSUPPORTED_MODE,        "Unsupported Mode"},
  {AAC_ENCODE_TOP_ERR_1,               "Top layer error 1"},
  {AAC_ENCODE_TOP_ERR_2,               "Top layer error 2"},
  {AAC_ENCODE_TOP_ERR_3,               "Top layer error 3"},
  {AAC_ENCODE_TOP_ERR_4,               "Top layer error 4"},
  {AAC_ENCODE_TOP_ERR_5,               "Top layer error 5"},
  {AAC_ENCODE_TOP_ERR_6,               "Top layer error 6"},
  {AAC_ENCODE_TOP_ERR_7,               "Top layer error 7"},
  {AAC_ENCODE_TOP_ERR_8,               "Top layer error 8"},
  {AAC_ENCODE_TOP_ERR_9,               "Top layer error 9"},
  {AAC_ENCODE_TOP_ERR_10,              "Top layer error 10"},
  {AAC_ENCODE_TOP_ERR_11,              "Top layer error 11"},
  {AAC_ENCODE_TOP_ERR_12,              "Top layer error 12"},
  {AAC_ENCODE_TOP_ERR_13,              "Top layer error 13"},
  {AAC_ENCODE_TOP_ERR_14,              "Top layer error 14"},
  {AAC_ENCODE_TOP_ERR_15,              "Top layer error 15"},
  {AAC_ENCODE_TOP_ERR_16,              "Top layer error 16"},
};

static AacStatusMsg decoder_status[] =
{
  {AAC_DECODE_OK, "OK"},
  {AAC_DECODE_UNSUPPORTED_FORMAT, "Unsupported format"},
  {AAC_DECODE_DECODE_FRAME_ERROR, "Decode frame error"},
  {AAC_DECODE_CRC_CHECK_ERROR, "CRC check error"},
  {AAC_DECODE_INVALID_CODE_BOOK, "Invalid code book"},
  {AAC_DECODE_UNSUPPORTED_WINOW_SHAPE, "Unsupported winow shape"},
  {AAC_DECODE_PREDICTION_NOT_SUPPORTED_IN_LC_AAC,
   "Prediction NOT supported in LC_AAC"
  },
  {AAC_DECODE_UNIMPLEMENTED_CCE, "CCE NOT implemented"},
  {AAC_DECODE_UNIMPLEMENTED_GAIN_CONTROL_DATA,
   "Gain control data NOT implemented"
  },
  {AAC_DECODE_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE,
   "EP specific config parse NOT implemented"
  },
  {AAC_DECODE_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE,
   "CELP specific config parse NOT implemented"
  },
  {AAC_DECODE_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE,
   "HVXC specific config parse NOT implemented"
  },
  {AAC_DECODE_OVERWRITE_BITS_IN_INPUT_BUFFER,
   "Over write bits in input buffer"
  },
  {AAC_DECODE_CANNOT_REACH_BUFFER_FULLNESS,
   "Cannot reach buffer fullness"
  },
  {AAC_DECODE_TNS_RANGE_ERROR, "TNS range error"},
};

static const char* aac_enc_strerror(uint32_t err)
{
  uint32_t len = sizeof(encoder_status) / sizeof(AacStatusMsg);
  uint32_t i = 0;
  for (i = 0; i < len; ++ i) {
    if (AM_LIKELY(encoder_status[i].error == err)) {
      break;
    }
  }

  return (i >= len) ? "Unknown Error" : encoder_status[i].message.c_str();
}

static const char* aac_dec_strerror(uint32_t err)
{
  uint32_t len = sizeof(decoder_status) / sizeof(AacStatusMsg);
  uint32_t i = 0;
  for (i = 0; i < len; ++ i) {
    if (AM_LIKELY(decoder_status[i].error == err)) {
      break;
    }
  }

  return (i >= len) ? "Unknown Error" : decoder_status[i].message.c_str();
}

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecAacNew::create(config);
}

AMIAudioCodec* AMAudioCodecAacNew::create(const char *config)
{
  AMAudioCodecAacNew *aac = new AMAudioCodecAacNew();
  if (AM_UNLIKELY(aac && !aac->init(config))) {
    delete aac;
    aac = nullptr;
  }
  return ((AMIAudioCodec*)aac);
}

void AMAudioCodecAacNew::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecAacNew::initialize(AM_AUDIO_INFO *srcAudioInfo,
                                    AM_AUDIO_CODEC_MODE mode)
{
  if (AM_LIKELY(srcAudioInfo && (!m_is_initialized ||
      (m_src_audio_info->channels != srcAudioInfo->channels) ||
      (m_src_audio_info->sample_rate != srcAudioInfo->sample_rate)))) {
    memcpy(m_src_audio_info, srcAudioInfo, sizeof(*m_src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_ENCODE],
           srcAudioInfo, sizeof(*srcAudioInfo));
    memcpy(&m_audio_info[AUDIO_INFO_DECODE],
           srcAudioInfo, sizeof(*srcAudioInfo));
    if (AM_UNLIKELY(m_is_initialized)) {
      NOTICE("Codec %s is already initialized to %s mode, re-initializing!",
             m_name.c_str(),
             codec_mode_string().c_str());
      finalize();
    }
    m_mode = mode;
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        AM_AUDIO_INFO *audioInfo = &m_audio_info[AUDIO_INFO_ENCODE];
        m_is_initialized = false;
        do {
          if (AM_LIKELY(audioInfo->sample_format != AM_SAMPLE_S16LE)) {
            ERROR("Invalid input audio sample type! Only S16LE is supported!");
            break;
          }
          if (AM_LIKELY(!m_enc_conf)) {
            m_enc_conf = new au_aacenc_config_t;
          }
          if (AM_UNLIKELY(!m_enc_conf)) {
            ERROR("Failed to allocate AAC codec encode config structure!");
            break;
          }
          audioInfo->type = AM_AUDIO_AAC;
          memset(m_enc_conf, 0, sizeof(*m_enc_conf));
          m_enc_conf->sample_freq      = audioInfo->sample_rate;
          m_enc_conf->Src_numCh        = audioInfo->channels;
          m_enc_conf->bitRate          = m_aac_config->encode.bitrate;
          m_enc_conf->quantizerQuality = m_aac_config->encode.quantizer_quality;
          m_enc_conf->tns              = m_aac_config->encode.tns;
          m_enc_conf->crc              = m_aac_config->encode.crc;
          m_enc_conf->pns              = m_aac_config->encode.pns;
          m_enc_conf->ffType           = m_aac_config->encode.fftype;
          m_enc_conf->enc_mode         = m_aac_config->encode.format;
          m_enc_conf->perceptual_mode  = m_aac_config->encode.perceptual_mode;
          m_enc_conf->original_copy                  = 0;
          m_enc_conf->copyright_identification_bit   = 0;
          m_enc_conf->copyright_identification_start = 0;
          switch(audioInfo->channels) {
            case 1: {
              m_enc_conf->channelMode = AAC_MONO;
            }break;
            case 2: {
              m_enc_conf->channelMode = AAC_STEREO;
            }break;
            default: {
              m_enc_conf->channelMode = AAC_UNDEFINED_CHANNEL_MODE;
            }break;
          }
          m_enc_conf->sendSbrHeader   = 0;

          if (AM_LIKELY(!m_enc_buffer)) {
            m_enc_buffer_size = aacenc_get_mem_size(m_enc_conf->Src_numCh,
                                                    m_enc_conf->sample_freq,
                                                    m_enc_conf->bitRate,
                                                    m_enc_conf->enc_mode);
            if (AM_UNLIKELY(m_enc_buffer_size < 0)) {
              ERROR("Invalid AAC encoder configuration:"
                    "\n       Sample Rate: %u"
                    "\n    Source Channel: %d"
                    "\n          Bit Rate: %u"
                    "\n Quantizer Quality: %u"
                    "\n               tns: %u"
                    "\n               crc: %u"
                    "\n               pns: %u"
                    "\n  Transport Format: %c"
                    "\n       Encode Mode: %d"
                    "\n   Perceptual Mode: %hhu",
                    m_enc_conf->sample_freq,
                    m_enc_conf->Src_numCh,
                    m_enc_conf->bitRate,
                    m_enc_conf->quantizerQuality,
                    m_enc_conf->tns,
                    m_enc_conf->crc,
                    m_enc_conf->pns,
                    m_enc_conf->ffType,
                    m_enc_conf->enc_mode,
                    m_enc_conf->perceptual_mode);
              break;
            } else {
              m_enc_buffer = new uint8_t[m_enc_buffer_size];
            }
            if (AM_UNLIKELY(!m_enc_buffer)) {
              ERROR("Failed to allocate AAC codec encode buffer!");
              break;
            }
          }
          m_enc_conf->codec_lib_mem_adr = (uint32_t*)m_enc_buffer;
          m_enc_conf->codec_lib_mem_size = m_enc_buffer_size;
          if (AM_UNLIKELY((m_enc_conf->Src_numCh != 2) &&
                          (m_aac_config->encode.format == AACPLUS_PS))) {
            ERROR("AAC Plus PS requires stereo audio input, "
                "but source audio channel number is: %u",
                audioInfo->channels);
            break;
          }
          if (AM_LIKELY(m_aac_config->encode.format == AACPLUS_PS)) {
            /* AACPlus_PS's output channel number is always 1 */
            audioInfo->channels = 1;
          }
          aacenc_setup(m_enc_conf);
          if (AM_UNLIKELY(m_enc_conf->ErrorStatus)) {
            ERROR("aacenc_setup failed: 0x%08x", m_enc_conf->ErrorStatus);
            break;
          }
          aacenc_open(m_enc_conf);
          if (AM_UNLIKELY(m_enc_conf->ErrorStatus)) {
            ERROR("aacenc_open failed: 0x%08x", m_enc_conf->ErrorStatus);
            break;
          }
          m_is_initialized = true;
          INFO("AAC codec is initialized for encoding!");
        }while(0);
        if (AM_UNLIKELY(!m_is_initialized)) {
          delete[] m_enc_buffer;
          delete[] m_enc_conf;
          m_enc_buffer = nullptr;
          m_enc_conf = nullptr;
        } else {
          audio_lib_info_t lib_info = {0};
          report_lib_info_aac_enc(&lib_info);
          INFO("\nAAC Encoder Info:"
               "\n SVN  REV: %s"
               "\n SVN HTTP: %s",
               lib_info.svnrev,
               lib_info.svnhttp);
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO *audioInfo = &m_audio_info[AUDIO_INFO_DECODE];
        m_is_initialized = false;
        do {
          if (AM_LIKELY(!m_dec_out_buffer)) {
            m_dec_out_buffer =
                new int32_t[m_aac_config->decode.dec_out_buf_size];
          }
          if (AM_LIKELY(!m_dec_out_buffer)) {
            ERROR("Failed to allocate AAC decode output buffer!");
            break;
          }

          if (AM_LIKELY(!m_dec_conf)) {
            m_dec_conf = new au_aacdec_config_t;
          }
          if (AM_UNLIKELY(!m_dec_conf)) {
            ERROR("Failed to allocate AAC decode config structure!");
            break;
          }
          memset(m_dec_conf, 0, sizeof(*m_dec_conf));
          m_dec_conf->bDownSample = m_aac_config->decode.downsampled_sbr;
          m_dec_conf->bBitstreamDownMix = m_aac_config->decode.downmix;
          if (AM_LIKELY(audioInfo->codec_info)) {
            if (is_str_equal((const char*)audioInfo->codec_info, "adts")) {
              m_dec_conf->bsFormat = ADTS_BSFORMAT;
            } else if (is_str_equal((const char*)audioInfo->codec_info,
                                    "raw")) {
              m_dec_conf->bsFormat = RAW_BSFORMAT;
              m_dec_conf->sample_freq = audioInfo->sample_rate;
            } else {
              ERROR("Invalid bit stream format! Neither ADTS nor RAW!");
              break;
            }
          } else {
            /* If codec info is not set, use default mode */
            m_dec_conf->bsFormat = ADTS_BSFORMAT;
          }
          audioInfo->sample_format = AM_SAMPLE_S16LE;
          audioInfo->sample_size   = sizeof(int16_t);

          if (AM_LIKELY(!m_dec_buffer)) {
            m_dec_buffer_size = aacdec_get_mem_size(audioInfo->channels);
            INFO("AAC Decoder working memory is %u bytes", m_dec_buffer_size);
            m_dec_buffer = new uint32_t[m_dec_buffer_size];
          }
          if (AM_UNLIKELY(!m_dec_buffer)) {
            ERROR("Failed to allocate AAC codec decode buffer!");
            break;
          }
          m_dec_conf->codec_lib_mem_addr = m_dec_buffer;
          m_dec_conf->codec_lib_mem_size = m_dec_buffer_size;

          aacdec_setup(m_dec_conf);
          if (AM_UNLIKELY(m_dec_conf->ErrorStatus)) {
            ERROR("aacdec_setup failed: %s",
                  aac_dec_strerror(m_dec_conf->ErrorStatus));
            break;
          }
          aacdec_open(m_dec_conf);
          if (AM_UNLIKELY(m_dec_conf->ErrorStatus)) {
            ERROR("aacdec_open failed: %s",
                  aac_dec_strerror(m_dec_conf->ErrorStatus));
            break;
          }
          m_is_initialized = true;
          INFO("AAC codec is initialized for decoding!");

        }while(0);
        if (AM_UNLIKELY(!m_is_initialized)) {
          delete m_dec_conf;
          delete[] m_dec_buffer;
          delete[] m_dec_out_buffer;
          m_dec_conf = nullptr;
          m_dec_buffer = nullptr;
          m_dec_out_buffer = nullptr;
        } else {
          audio_lib_info_t lib_info = {0};
          report_lib_info_aac_dec(&lib_info);
          INFO("\nAAC Decoder Info:"
               "\n SVN  REV: %s"
               "\n SVN HTTP: %s",
               lib_info.svnrev,
               lib_info.svnhttp);
        }
      }break;
      default: break;
    }
  } else if (AM_LIKELY(!srcAudioInfo)) {
    ERROR("Invalid audio info!");
    finalize();
    m_is_initialized = false;
    memset(m_src_audio_info, 0, sizeof(*m_src_audio_info));
  }

  if (AM_UNLIKELY(!m_is_initialized)) {
    m_mode = AM_AUDIO_CODEC_MODE_NONE;
  }
  NOTICE("Audio codec %s is initialized to mode %s",
         m_name.c_str(), codec_mode_string().c_str());

  return m_is_initialized;
}

bool AMAudioCodecAacNew::finalize()
{
  if (AM_LIKELY(m_is_initialized)) {
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE : {
        delete[] m_enc_buffer;
        m_enc_buffer = nullptr;
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE : {
        delete[] m_dec_buffer;
        delete[] m_dec_out_buffer;
        m_dec_buffer = nullptr;
        m_dec_out_buffer = nullptr;
      }break;
      default: break;
    }
    m_is_initialized = false;
    m_mode = AM_AUDIO_CODEC_MODE_NONE;
    INFO("AAC codec is finalized!");
  }

  return !m_is_initialized;
}

AM_AUDIO_INFO* AMAudioCodecAacNew::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = nullptr;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE : {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE : {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }break;
    default: break;
  }
  return info;
}

uint32_t AMAudioCodecAacNew::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE:
      size = m_aac_config->encode.enc_out_buf_size;
      break;
    case AM_AUDIO_CODEC_MODE_DECODE:
      size = m_aac_config->decode.dec_out_buf_size;
      break;
    default: ERROR("AAC audio codec is not initialized!"); break;
  }

  return size;
}

bool AMAudioCodecAacNew::check_encode_src_parameter(AM_AUDIO_INFO &info)
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(info.sample_format != AM_SAMPLE_S16LE)) {
      break;
    }

    if (AM_UNLIKELY((AACPLAIN == m_aac_config->encode.format) &&
                    (info.sample_rate != 8000)  &&
                    (info.sample_rate != 11025) &&
                    (info.sample_rate != 12000) &&
                    (info.sample_rate != 16000) &&
                    (info.sample_rate != 22050) &&
                    (info.sample_rate != 24000) &&
                    (info.sample_rate != 32000) &&
                    (info.sample_rate != 44100) &&
                    (info.sample_rate != 48000))) {
      break;
    }

    if (AM_UNLIKELY(((AACPLUS == m_aac_config->encode.format) ||
                     (AACPLUS_PS == m_aac_config->encode.format)) &&
                    (info.sample_rate != 32000) &&
                    (info.sample_rate != 44100) &&
                    (info.sample_rate != 48000))) {
      break;
    }

    if (AM_UNLIKELY((m_aac_config->encode.format == AACPLUS_PS) &&
                    (info.channels == 1))) {
      break;
    }

    switch(m_aac_config->encode.format) {
      case AACPLAIN: {
        uint32_t max = 0;
        uint32_t min = 0;
        switch(info.sample_rate) {
          case 48000:
          case 44100:
          case 32000:
          case 24000:
          case 22050:
          case 16000: {
            min = info.channels * 16000;
            max = info.channels * info.sample_rate * 6144 / 1024;
          }break;
          case 12000:
          case 11025: {
            min = info.channels * 8000;
            max = info.channels * 40000;
          }break;
          case 8000: {
            min = info.channels * 8000;
            max = info.channels * 12000;
          }break;
          default: {
            min = 16000;
            max = 160000;
          }break;
        }
        if (m_aac_config->encode.bitrate > max) {
          WARN("Bit rate exceeds AAC maximum bit rate(%u), "
               "reset to %u", max, max);
          m_aac_config->encode.bitrate = max;
        } else if (m_aac_config->encode.bitrate < min) {
          WARN("Bit rate is less than AAC minimum bit rate(%u) "
               "reset to %u", min, min);
          m_aac_config->encode.bitrate = min;
        }
      }break;
      case AACPLUS: {
        uint32_t min = info.channels * 14000;
        uint32_t max = info.channels * 64000;
        if (m_aac_config->encode.bitrate > max) {
          WARN("Bit rate exceeds AAC Plus maximum bit rate(%u) "
               "reset to %u", max, max);
          m_aac_config->encode.bitrate = max;
        } else if (m_aac_config->encode.bitrate < min) {
          WARN("Bit rate is less than AAC Plus minimum bit rate(%u) "
               "reset to %u");
          m_aac_config->encode.bitrate = min;
        }
      }break;
      case AACPLUS_PS: {
        uint32_t min = 14000;
        uint32_t max = 64000;
        if (m_aac_config->encode.bitrate > max) {
          WARN("Bit rate exceeds AAC Plus PS maximum bit rate(%u) "
               "reset to %u", max, max);
          m_aac_config->encode.bitrate = max;
        } else if (m_aac_config->encode.bitrate < min) {
          WARN("Bit rate is less than AAC Plus PS minimum bit rate"
               "(%u) reset to %u", min, min);
          m_aac_config->encode.bitrate = min;
        }
      }break;
      default: break;
    }
    ret = true;
  } while(0);
  if (AM_UNLIKELY(!ret)) {
    WARN("\nWrong source audio parameters:"
         "\n Sample Format: %s"
         "\n   Sample Rate: %u"
         "\n      Channels: %u"
         "\nRequired source audio parameters:"
         "\n Sample Format: s16le"
         "\n   Sample Rate: %s"
         "\n      Channels: %s",
         smp_fmt_to_str(AM_AUDIO_SAMPLE_FORMAT(info.sample_format)),
         info.sample_rate,
         info.channels,
         (m_aac_config->encode.format == AACPLAIN) ?
         "8000|11025|12000|16000|22050|24000|32000|44100|48000" :
         "32000|44100|48000",
         (m_aac_config->encode.format == AACPLUS_PS) ? "2" : "1 | 2");
  }

  return ret;
}

uint32_t AMAudioCodecAacNew::get_encode_required_chunk_size(AM_AUDIO_INFO &info)
{
  uint32_t chunk_size = 0;
  switch(m_aac_config->encode.format) {
    case AACPLAIN: {
      chunk_size = 1024 * info.channels * sizeof(int16_t);
    }break;
    case AACPLUS:
    case AACPLUS_PS: {
      chunk_size = 2048 * info.channels * sizeof(int16_t);
    }break;
    case AACPLUS_SPEECH:
    case AACPLUS_SPEECH_PS:
    default: chunk_size = 0; break;
  }

  return chunk_size;
}

uint32_t AMAudioCodecAacNew::encode(uint8_t *input,  uint32_t in_data_size,
                                    uint8_t *output, uint32_t *out_data_size)
{
  *out_data_size = 0;
  if (AM_LIKELY(m_enc_conf)) {
    m_enc_conf->enc_rptr = (int32_t*)input;
    m_enc_conf->enc_wptr = output;
    aacenc_encode(m_enc_conf);
    if (AM_UNLIKELY(m_enc_conf->ErrorStatus)) {
      ERROR("AAC encoding error: %s, encode mode: %d!",
            aac_enc_strerror(m_enc_conf->ErrorStatus),
            m_enc_conf->enc_mode);
    } else {
      *out_data_size = (uint32_t)((m_enc_conf->nBitsInRawDataBlock + 7) >> 3);
    }
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return *out_data_size;
}

uint32_t AMAudioCodecAacNew::decode(uint8_t *input,  uint32_t in_data_size,
                                    uint8_t *output, uint32_t *out_data_size)
{
  uint32_t ret = 0;
  if (AM_LIKELY(m_dec_conf)) {
    m_dec_conf->dec_rptr = input;
    m_dec_conf->dec_wptr = m_dec_out_buffer;
    m_dec_conf->consumedByte = 0;
    *out_data_size = 0;
    aacdec_decode(m_dec_conf);

    if (AM_UNLIKELY(m_dec_conf->ErrorStatus)) {
      ERROR("AAC decoding error: %s, consumed %u bytes!",
            aac_dec_strerror(m_dec_conf->ErrorStatus),
            m_dec_conf->consumedByte);
      /*
      ERROR("AAC decoding error: %s, consumed %u bytes!",
            aac_dec_strerror((uint32_t)m_dec_conf->ErrorStatus),
            m_dec_conf->consumedByte);
      */
      /* Skip broken data */
      m_dec_conf->consumedByte = (m_dec_conf->consumedByte == 0) ?
          in_data_size : m_dec_conf->consumedByte;
    }
    if (AM_LIKELY(m_dec_conf->has_dec_out)) {
      *out_data_size = m_dec_conf->frameSize *
          m_dec_conf->outNumCh * sizeof(int16_t);
      memcpy(output, m_dec_conf->dec_wptr, *out_data_size);
      /* This is the sample rate of decoded PCM audio data */
      m_audio_info[AUDIO_INFO_DECODE].sample_rate = m_dec_conf->sample_freq;
      m_audio_info[AUDIO_INFO_DECODE].channels    = m_dec_conf->outNumCh;
    }
    ret = m_dec_conf->consumedByte;
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return ret;
}

AMAudioCodecAacNew::AMAudioCodecAacNew() :
    inherited(AM_AUDIO_CODEC_AAC, "aac")
{}

AMAudioCodecAacNew::~AMAudioCodecAacNew()
{
  finalize();
  delete   m_config;
  delete   m_enc_conf;
  delete   m_dec_conf;
  delete[] m_enc_buffer;
  delete[] m_dec_buffer;
  delete[] m_dec_out_buffer;
}

bool AMAudioCodecAacNew::init(const char *config)
{
  bool ret = true;
  std::string conf_file(config);

  do {
    ret = inherited::init();
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    m_config = new AMAacConfig();
    if (AM_UNLIKELY(!m_config)) {
      ret = false;
      ERROR("Failed to create AAC codec config module!");
      break;
    }
    m_aac_config = m_config->get_config(conf_file);
    if (AM_UNLIKELY(!m_aac_config)) {
      ERROR("Failed to get AAC codec config!");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}
