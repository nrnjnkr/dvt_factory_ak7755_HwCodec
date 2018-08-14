/**
 * app/ipcam/fastboot_smart3a/adc_util.h
 *
 * Author: Caizhang Lin <czlin@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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

#ifndef   _ADC_UTIL_H
#define   _ADC_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif
// this struct should be identical to that in amboot/include/adc.h
struct amboot_params{ /* (4 * 1) + (5 * 4) + (5 * 4) + 32 + 2 + 18= 96 */
    unsigned char enable_audio;
    unsigned char enable_fastosd;
    unsigned char enable_ldc;
    unsigned char rotation_mode;

    unsigned int stream0_enable;
    unsigned int stream0_resolution;
    unsigned int stream0_fmt;
    unsigned int stream0_fps;
    unsigned int stream0_bitrate;

    unsigned int stream1_enable;
    unsigned int stream1_resolution;
    unsigned int stream1_fmt;
    unsigned int stream1_fps;
    unsigned int stream1_bitrate;

    char fastosd_string[32];
    unsigned char enable_vca;
    unsigned char vca_frame_num;
    unsigned char reserved[18];
}__attribute__((packed));
/* function: update amboot parameters in ADC partition
**
*/
int adc_util_update(struct amboot_params* params);
int adc_util_dump();
#ifdef __cplusplus
}
#endif
#endif
