/*******************************************************************************
 * am_audio_utility.cpp
 *
 * History:
 *   Apr 16, 2017 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_utility.h"

#include <alsa/asoundlib.h>
#include <pulse/pulseaudio.h>
#include <math.h>

const char* audio_type_to_codec_str(AM_AUDIO_TYPE type)
{
  const char *type_str = "";
  switch(type) {
    case AM_AUDIO_LPCM:    type_str = "lpcm";    break;
    case AM_AUDIO_BPCM:    type_str = "bpcm";    break;
    case AM_AUDIO_FPCM:    type_str = "fpcm";    break;
    case AM_AUDIO_G711A:
    case AM_AUDIO_G711U:   type_str = "g711";    break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: type_str = "g726";    break;
    case AM_AUDIO_MP3:     type_str = "mp3";     break;
    case AM_AUDIO_AAC:     type_str = "aac";     break;
    case AM_AUDIO_OPUS:    type_str = "opus";    break;
    case AM_AUDIO_SPEEX:   type_str = "speex";   break;
    case AM_AUDIO_NULL:    type_str = "null";    break;
    default:               type_str = "unknown"; break;
  }

  return type_str;
}

const char* audio_type_to_str(AM_AUDIO_TYPE type)
{
  const char *type_str = "";
  switch(type) {
    case AM_AUDIO_LPCM:    type_str = "Little Endian Signed Int PCM"; break;
    case AM_AUDIO_BPCM:    type_str = "Big Endian Signed Int PCM";    break;
    case AM_AUDIO_FPCM:    type_str = "Floating Point PCM";           break;
    case AM_AUDIO_G711A:   type_str = "G.711 A_law";                  break;
    case AM_AUDIO_G711U:   type_str = "G.711 MU_law";                 break;
    case AM_AUDIO_G726_40: type_str = "G.726 40bits";                 break;
    case AM_AUDIO_G726_32: type_str = "G.726 32bits";                 break;
    case AM_AUDIO_G726_24: type_str = "G.726 24bits";                 break;
    case AM_AUDIO_G726_16: type_str = "G.726 16bits";                 break;
    case AM_AUDIO_MP3:     type_str = "MP3";                          break;
    case AM_AUDIO_AAC:     type_str = "AAC";                          break;
    case AM_AUDIO_OPUS:    type_str = "OPUS";                         break;
    case AM_AUDIO_SPEEX:   type_str = "SPEEX";                        break;
    case AM_AUDIO_NULL:    type_str = "NULL";                         break;
    default:               type_str = "Unknown";                      break;
  }

  return type_str;
}

const char* smp_fmt_to_str(AM_AUDIO_SAMPLE_FORMAT format)
{
  const char *format_str = "";
  switch(format) {
    case AM_SAMPLE_U8:        format_str = "u8";       break;
    case AM_SAMPLE_ALAW :     format_str = "a_law";    break;
    case AM_SAMPLE_ULAW :     format_str = "mu_law";   break;
    case AM_SAMPLE_S16LE :    format_str = "s16le";    break;
    case AM_SAMPLE_S16BE :    format_str = "s16be";    break;
    case AM_SAMPLE_S24LE :    format_str = "s24le";    break;
    case AM_SAMPLE_S24BE :    format_str = "s24be";    break;
    case AM_SAMPLE_S24_32LE : format_str = "s24_32le"; break;
    case AM_SAMPLE_S24_32BE : format_str = "s24_32be"; break;
    case AM_SAMPLE_S32LE :    format_str = "s32le";    break;
    case AM_SAMPLE_S32BE :    format_str = "s32be";    break;
    case AM_SAMPLE_F32LE :    format_str = "f32le";    break;
    case AM_SAMPLE_F32BE :    format_str = "f32be";    break;
    case AM_SAMPLE_INVALID :
    default :                 format_str = "invalid";  break;
  }

  return format_str;
}

int smp_fmt_to_pcm_fmt(AM_AUDIO_SAMPLE_FORMAT fmt)
{
  snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;

  switch(fmt) {
    case AM_SAMPLE_U8: {
      format = SND_PCM_FORMAT_U8;
    }break;
    case AM_SAMPLE_ALAW: {
      format = SND_PCM_FORMAT_A_LAW;
    }break;
    case AM_SAMPLE_ULAW: {
      format = SND_PCM_FORMAT_MU_LAW;
    }break;
    case AM_SAMPLE_S16LE: {
      format = SND_PCM_FORMAT_S16_LE;
    }break;
    case AM_SAMPLE_S16BE: {
      format = SND_PCM_FORMAT_S16_BE;
    }break;
    case AM_SAMPLE_S24LE: {
      format = SND_PCM_FORMAT_S24_3LE;
    }break;
    case AM_SAMPLE_S24BE: {
      format = SND_PCM_FORMAT_S24_3BE;
    }break;
    case AM_SAMPLE_S24_32LE: {
      format = SND_PCM_FORMAT_S24_LE;
    }break;
    case AM_SAMPLE_S24_32BE: {
      format = SND_PCM_FORMAT_S24_BE;
    }break;
    case AM_SAMPLE_S32LE: {
      format = SND_PCM_FORMAT_S32_LE;
    }break;
    case AM_SAMPLE_S32BE: {
      format = SND_PCM_FORMAT_S32_BE;
    }break;
    case AM_SAMPLE_F32LE: {
      format = SND_PCM_FORMAT_FLOAT_LE;
    }break;
    case AM_SAMPLE_F32BE: {
      format = SND_PCM_FORMAT_FLOAT_BE;
    }break;
    default: {
      format = SND_PCM_FORMAT_UNKNOWN;
    }break;
  }

  return ((int)format);
}

int smp_fmt_to_pulse_fmt(AM_AUDIO_SAMPLE_FORMAT fmt)
{
  pa_sample_format_t format = PA_SAMPLE_INVALID;

  switch(fmt) {
    case AM_SAMPLE_U8: {
      format = PA_SAMPLE_U8;
    }break;
    case AM_SAMPLE_ALAW: {
      format = PA_SAMPLE_ALAW;
    }break;
    case AM_SAMPLE_ULAW: {
      format = PA_SAMPLE_ULAW;
    }break;
    case AM_SAMPLE_S16LE: {
      format = PA_SAMPLE_S16LE;
    }break;
    case AM_SAMPLE_S16BE: {
      format = PA_SAMPLE_S16BE;
    }break;
    case AM_SAMPLE_S24LE: {
      format = PA_SAMPLE_S24LE;
    }break;
    case AM_SAMPLE_S24BE: {
      format = PA_SAMPLE_S24BE;
    }break;
    case AM_SAMPLE_S24_32LE:
#if 0
      /* Fixme: sound will be noise if convert to S24_32LE */
    {
      format = PA_SAMPLE_S24_32LE;
    }break;
#endif
    case AM_SAMPLE_S32LE: {
      format = PA_SAMPLE_S32LE;
    }break;
    case AM_SAMPLE_S24_32BE:
#if 0
    {
      /* Fixme: sound will be noise if convert to S24_32BE */
      format = PA_SAMPLE_S24_32BE;
    }break;
#endif
    case AM_SAMPLE_S32BE: {
      format = PA_SAMPLE_S32BE;
    }break;
    case AM_SAMPLE_F32LE: {
      format = PA_SAMPLE_FLOAT32LE;
    }break;
    case AM_SAMPLE_F32BE: {
      format = PA_SAMPLE_FLOAT32BE;
    }break;
    default: {
      format = PA_SAMPLE_INVALID;
    }break;
  }
  return ((int)format);
}

uint32_t get_sample_size(AM_AUDIO_SAMPLE_FORMAT format)
{
  uint32_t size = 0;
  switch(format) {
    case AM_SAMPLE_U8: {
      size = sizeof(uint8_t);
    }break;
    case AM_SAMPLE_ALAW: {
      size = sizeof(uint8_t);
    }break;
    case AM_SAMPLE_ULAW: {
      size = sizeof(uint8_t);
    }break;
    case AM_SAMPLE_S16LE: {
      size = sizeof(uint16_t);
    }break;
    case AM_SAMPLE_S16BE: {
      size = sizeof(uint16_t);
    }break;
    case AM_SAMPLE_S24LE: {
      size = 3;
    }break;
    case AM_SAMPLE_S24BE: {
      size = 3;
    }break;
    case AM_SAMPLE_S24_32LE: {
      size = sizeof(uint32_t);
    }break;
    case AM_SAMPLE_S24_32BE: {
      size = sizeof(uint32_t);
    }break;
    case AM_SAMPLE_S32LE: {
      size = sizeof(uint32_t);
    }break;
    case AM_SAMPLE_S32BE: {
      size = sizeof(uint32_t);
    }break;
    case AM_SAMPLE_F32LE: {
      size = sizeof(float);
    }break;
    case AM_SAMPLE_F32BE: {
      size = sizeof(float);
    }break;
    default: {
      size = 0;
    }break;
  }

  return size;
}

void int_to_float(uint8_t *input,
                  float   *output,
                  uint32_t samples_count,
                  AM_AUDIO_SAMPLE_FORMAT from_format)
{
  if (AM_LIKELY(output)) {
    switch(from_format) {
      case AM_SAMPLE_U8: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          output[i] = ((int)(input[i] - 128)) / 128.0f;
        }
      }break;
      case AM_SAMPLE_S16LE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          int8_t *in = (int8_t*)(input + (i * 2));
          output[i] = ((in[1] << 8) | (uint8_t)in[0]) / 32768.0f;
        }
      }break;
      case AM_SAMPLE_S16BE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          int8_t *in = (int8_t*)(input + (i * 2));
          output[i] = ((in[0] << 8) | (uint8_t)in[1]) / 32768.0f;
        }
      }break;
      case AM_SAMPLE_S24LE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          uint8_t *in = (input + (i * 3));
          output[i] = ((((int8_t)in[2]) << 16) | (in[1] << 8) | in[0]) /
              8388608.0f;
        }
      }break;
      case AM_SAMPLE_S24BE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          uint8_t *in = (input + (i * 3));
          output[i] = ((((int8_t)in[0]) << 16) | (in[1] << 8) | in[2]) /
              8388608.0f;
        }
      }break;
      case AM_SAMPLE_S32LE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          uint8_t *in = (input + (i * 4));
          output[i] = ((((int8_t)in[3]) << 24) | (in[2] << 16) |
                     (in[1] << 8) | in[0]) / 2147483648.0f;
        }
      }break;
      case AM_SAMPLE_S32BE: {
        for (uint32_t i = 0; i < samples_count; ++ i) {
          uint8_t *in = (input + (i * 4));
          output[i] = ((((int8_t)in[0]) << 24) | (in[1] << 16) |
                     (in[2] << 8) | in[3]) / 2147483648.0f;
        }
      }break;
      default: {
        ERROR("Cannot convert audio sample format %s to float!",
              smp_fmt_to_str(from_format));
      }break;
    }
  }
}

void float_to_int(float   *input,
                  uint8_t *output,
                  uint32_t samples_count,
                  AM_AUDIO_SAMPLE_FORMAT to_format)
{
  switch(to_format) {
    case AM_SAMPLE_U8: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        output[i] =
            (uint8_t)lrintf(AM_MAX(-128, \
                                   AM_MIN(input[i] * 128.0f, 127)) + 128);
      }
    }break;
    case AM_SAMPLE_S16LE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 2);
        int16_t sample =
            (int16_t)lrintf(AM_MAX(-32768, \
                                   AM_MIN(input[i] * 32768.0f, 32767)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
      }
    }break;
    case AM_SAMPLE_S16BE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 2);
        int16_t sample =
            (int16_t)lrintf(AM_MAX(-32768, \
                                   AM_MIN(input[i] * 32768.0f, 32767)));
        out[0] = (sample >> 8) & 0xff;
        out[1] = sample & 0xff;
      }
    }break;
    case AM_SAMPLE_S24LE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 3);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-8388608, \
                                   AM_MIN(input[i] * 8388608.0f, 8388607)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = (sample >> 16) & 0xff;
      }
    }break;
    case AM_SAMPLE_S24BE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 3);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-8388608, \
                                   AM_MIN(input[i] * 8388608.0f, 8388607)));
        out[0] = (sample >> 16) & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = sample & 0xff;
      }
    }break;
    case AM_SAMPLE_S32LE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 4);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-2147483648, \
                                   AM_MIN(input[i] * 2147483648.0f, \
                                          2147483647)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = (sample >> 16) & 0xff;
        out[3] = (sample >> 24) & 0xff;
      }
    }break;
    case AM_SAMPLE_S32BE: {
      for (uint32_t i = 0; i < samples_count; ++ i) {
        uint8_t *out = (output + i * 4);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-2147483648, \
                                   AM_MIN(input[i] * 2147483648.0f, \
                                          2147483647)));
        out[0] = (sample >> 24) & 0xff;
        out[1] = (sample >> 16) & 0xff;
        out[2] = (sample >> 8)  & 0xff;
        out[3] = sample & 0xff;
      }
    }break;
    default: {
      ERROR("Cannot convert float to audio sample format %s!",
            smp_fmt_to_str(to_format));
    }break;
  }
}

void format_convert(uint8_t *input,
                    uint8_t *output,
                    AM_AUDIO_SAMPLE_FORMAT from,
                    AM_AUDIO_SAMPLE_FORMAT to,
                    uint32_t sample_count)
{
  uint8_t *out = output;
  for (uint32_t i = 0; i < sample_count; ++ i) {
    float tmp = 0.0f;
    switch(from) {
      case AM_SAMPLE_U8: {
        tmp = ((int)(input[i] - 128)) / 128.0f;
      }break;
      case AM_SAMPLE_S16LE: {
        int8_t *in = (int8_t*)(input + (i * 2));
        tmp = ((in[1] << 8) | (uint8_t)in[0]) / 32768.0f;
      }break;
      case AM_SAMPLE_S16BE: {
        int8_t *in = (int8_t*)(input + (i * 2));
        tmp = ((in[0] << 8) | (uint8_t)in[1]) / 32768.0f;
      }break;
      case AM_SAMPLE_S24LE: {
        uint8_t *in = (input + (i * 3));
        tmp = ((((int8_t)in[2]) << 16) | (in[1] << 8) | in[0]) / 8388608.0f;
      }break;
      case AM_SAMPLE_S24BE: {
        uint8_t *in = (input + (i * 3));
        tmp = ((((int8_t)in[0]) << 16) | (in[1] << 8) | in[2]) / 8388608.0f;
      }break;
      case AM_SAMPLE_S32LE: {
        uint8_t *in = (input + (i * 4));
        tmp = ((((int8_t)in[3]) << 24) | (in[2] << 16) |
                (in[1] << 8) | in[0]) / 2147483648.0f;
      }break;
      case AM_SAMPLE_S32BE: {
        uint8_t *in = (input + (i * 4));
        tmp = ((((int8_t)in[0]) << 24) | (in[1] << 16) |
                (in[2] << 8) | in[3]) / 2147483648.0f;
      }break;
      case AM_SAMPLE_F32LE: {
        tmp = *((float*)(input + i * 4));
      }break;
      default: {
        ERROR("Cannot convert audio sample format %s to float!",
              smp_fmt_to_str(from));
      }break;
    }
    switch(to) {
      case AM_SAMPLE_U8: {
          out[i] = (uint8_t)lrintf(AM_MAX(-128, \
                                          AM_MIN(tmp * 128.0f, 127)) + 128);
      }break;
      case AM_SAMPLE_S16LE: {
        uint8_t *out = (output + i * 2);
        int16_t sample =
            (int16_t)lrintf(AM_MAX(-32768, \
                                   AM_MIN(tmp * 32768.0f, 32767)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
      }break;
      case AM_SAMPLE_S16BE: {
        uint8_t *out = (output + i * 2);
        int16_t sample =
            (int16_t)lrintf(AM_MAX(-32768, \
                                   AM_MIN(tmp * 32768.0f, 32767)));
        out[0] = (sample >> 8) & 0xff;
        out[1] = sample & 0xff;
      }break;
      case AM_SAMPLE_S24LE: {
        uint8_t *out = (output + i * 3);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-8388608, \
                                   AM_MIN(tmp * 8388608.0f, 8388607)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = (sample >> 16) & 0xff;
      }break;
      case AM_SAMPLE_S24BE: {
        uint8_t *out = (output + i * 3);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-8388608, \
                                   AM_MIN(tmp * 8388608.0f, 8388607)));
        out[0] = (sample >> 16) & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = sample & 0xff;
      }break;
      case AM_SAMPLE_S32LE: {
        uint8_t *out = (output + i * 4);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-2147483648, \
                                   AM_MIN(tmp * 2147483648.0f, \
                                          2147483647)));
        out[0] = sample & 0xff;
        out[1] = (sample >> 8) & 0xff;
        out[2] = (sample >> 16) & 0xff;
        out[3] = (sample >> 24) & 0xff;
      }break;
      case AM_SAMPLE_S32BE: {
        uint8_t *out = (output + i * 4);
        int32_t sample =
            (int32_t)lrintf(AM_MAX(-2147483648, \
                                   AM_MIN(tmp * 2147483648.0f, \
                                          2147483647)));
        out[0] = (sample >> 24) & 0xff;
        out[1] = (sample >> 16) & 0xff;
        out[2] = (sample >> 8)  & 0xff;
        out[3] = sample & 0xff;
      }break;
      case AM_SAMPLE_F32LE: {
        *((float*)(output + i * 4)) = tmp;
      }break;
      default: {
        ERROR("Cannot convert float to audio sample format %s!",
              smp_fmt_to_str(to));
      }break;
    }
  }
}

void gen_silence(void *buf, uint32_t size, AM_AUDIO_SAMPLE_FORMAT fmt)
{
  switch(fmt) {
    case AM_SAMPLE_U8:
      memset(buf, 0x80, size);
      break;
    case AM_SAMPLE_F32LE:
    case AM_SAMPLE_F32BE:
      memset(buf, 0x00, size);
      break;
    case AM_SAMPLE_ALAW:
    case AM_SAMPLE_ULAW:
    case AM_SAMPLE_S16LE:
    case AM_SAMPLE_S16BE:
    case AM_SAMPLE_S24LE:
    case AM_SAMPLE_S24BE:
    case AM_SAMPLE_S24_32LE:
    case AM_SAMPLE_S24_32BE:
    case AM_SAMPLE_S32LE:
    case AM_SAMPLE_S32BE:
    default:
      memset(buf, 0xff, size);
      break;
  }
}
