/*******************************************************************************
 * am_audio_utility.h
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
#ifndef AM_AUDIO_UTILITY_H_
#define AM_AUDIO_UTILITY_H_

#include "am_audio_define.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* audio_type_to_codec_str(AM_AUDIO_TYPE type);
const char* audio_type_to_str(AM_AUDIO_TYPE type);
const char* smp_fmt_to_str(AM_AUDIO_SAMPLE_FORMAT format);
int         smp_fmt_to_pcm_fmt(AM_AUDIO_SAMPLE_FORMAT format);
int         smp_fmt_to_pulse_fmt(AM_AUDIO_SAMPLE_FORMAT format);
uint32_t    get_sample_size(AM_AUDIO_SAMPLE_FORMAT format);
void        int_to_float(uint8_t *input,
                         float   *output,
                         uint32_t samples_count,
                         AM_AUDIO_SAMPLE_FORMAT from_format);
void        float_to_int(float   *input,
                         uint8_t *output,
                         uint32_t samples_count,
                         AM_AUDIO_SAMPLE_FORMAT to_format);
void        format_convert(uint8_t *input,
                           uint8_t *output,
                           AM_AUDIO_SAMPLE_FORMAT from,
                           AM_AUDIO_SAMPLE_FORMAT to,
                           uint32_t sample_count);
void        gen_silence(void *buf, uint32_t size, AM_AUDIO_SAMPLE_FORMAT fmt);

#ifdef __cplusplus
}
#endif

#endif /* AM_AUDIO_UTILITY_H_ */
