/*******************************************************************************
 * am_audio_codec_aac_new_config.h
 *
 * History:
 *   2017-04-06 - [ypchang] created file
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
#ifndef AM_AUDIO_CODEC_AAC_NEW_CONFIG_H_
#define AM_AUDIO_CODEC_AAC_NEW_CONFIG_H_

#include "new_aac_audio_enc.h"
#include "new_aac_audio_dec.h"

enum AAC_QUANTIZER_QUALITY
{
  QUANTIZER_QUALITY_LOW     = 0,
  QUANTIZER_QUALITY_HIGH    = 1,
  QUANTIZER_QUALITY_HIGHEST = 2,
};

enum AAC_TRANS_FORMAT
{
  ADTS = 't',
  LOAS = 'l',
  RAW  = 'r',
};

struct AacEnc
{
    uint32_t enc_out_buf_size  = 1636;
    uint32_t bitrate           = 48000;
    uint32_t tns               = 1;
    uint32_t pns               = 0;
    uint32_t crc               = 0;
    uint32_t quantizer_quality = QUANTIZER_QUALITY_HIGHEST;
    AAC_MODE format            = AACPLUS;
    int8_t   fftype            = ADTS;
    uint8_t  perceptual_mode   = AAC_ENCODE_NORMAL;
    AacEnc(){}
};

struct AacDec
{
    uint32_t dec_out_buf_size  = 16384;
    int32_t  downsampled_sbr   = 0;
    int32_t  downmix           = 0;
    AacDec(){}
};

struct AacConfig
{
    AacEnc encode;
    AacDec decode;
};

class AMConfig;
class AMAudioCodecAacNew;
class AMAacConfig
{
    friend class AMAudioCodecAacNew;
  public:
    AMAacConfig(){};
    virtual ~AMAacConfig();
    AacConfig* get_config(const std::string& config);

  private:
    AMConfig  *m_config     = nullptr;
    AacConfig *m_aac_config = nullptr;
};

#endif /* AM_AUDIO_CODEC_AAC_CONFIG_H_ */
