/*******************************************************************************
 * mp3.h
 *
 * History:
 *   2017年9月5日 - [ypchang] created file
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
#ifndef MP3_H_
#define MP3_H_
#include <stdint.h>

struct MP3Header
{
    uint16_t    sync_8bits: 8;
    uint16_t    protection: 1;
    uint16_t    layer     : 2;
    uint16_t    version_id: 2;
    uint16_t    sync_3bits: 3;

    uint8_t    private_bit: 1;
    uint8_t    padding    : 1;
    uint8_t samplerate_idx: 2;
    uint8_t    bitrate_idx: 4;

    uint8_t       emphasis: 2;
    uint8_t       original: 1;
    uint8_t      copyright: 1;
    uint8_t       mode_ext: 2;
    uint8_t   channel_mode: 2;
    bool is_sync_ok()
    {
      return (0x07FF == ((sync_3bits << 8) | sync_8bits));
    }

    uint32_t bitrate()
    {
      uint8_t index = (bitrate_idx << 4) | (version_id << 2) | layer;
      uint32_t bitrate = 0;
      switch(index) {
        case 0b00011111: /*V1,   L1*/
        case 0b00011110: /*V1,   L2*/
        case 0b00011101: /*V1,   L3*/
        case 0b00010011: /*V2.5, L1*/
        case 0b00011011: /*V2,   L1*/ bitrate = 32000;  break;
        case 0b00010010: /*V2.5, L2*/
        case 0b00011010: /*V2,   L2*/
        case 0b00010001: /*V2.5, L3*/
        case 0b00011001: /*V2,   L3*/ bitrate = 8000;   break;

        case 0b00101111: /*V1,   L1*/ bitrate = 64000;  break;
        case 0b00101110: /*V1,   L2*/ bitrate = 48000;  break;
        case 0b00101101: /*V1,   L3*/ bitrate = 40000;  break;
        case 0b00100011: /*V2.5, L1*/
        case 0b00101011: /*V2,   L1*/ bitrate = 48000;  break;
        case 0b00100010: /*V2.5, L2*/
        case 0b00101010: /*V2,   L2*/
        case 0b00100001: /*V2.5, L3*/
        case 0b00101001: /*V2,   L3*/ bitrate = 16000;  break;

        case 0b00111111: /*V1,   L1*/ bitrate = 96000;  break;
        case 0b00111110: /*V1,   L2*/ bitrate = 56000;  break;
        case 0b00111101: /*V1,   L3*/ bitrate = 48000;  break;
        case 0b00110011: /*V2.5, L1*/
        case 0b00111011: /*V2,   L1*/ bitrate = 56000;  break;
        case 0b00110010: /*V2.5, L2*/
        case 0b00111010: /*V2,   L2*/
        case 0b00110001: /*V2.5, L3*/
        case 0b00111001: /*V2,   L3*/ bitrate = 24000;  break;

        case 0b01001111: /*V1,   L1*/ bitrate = 128000; break;
        case 0b01001110: /*V1,   L2*/ bitrate = 64000;  break;
        case 0b01001101: /*V1,   L3*/ bitrate = 56000;  break;
        case 0b01000011: /*V2.5, L1*/
        case 0b01001011: /*V2,   L1*/ bitrate = 64000;  break;
        case 0b01000010: /*V2.5, L2*/
        case 0b01001010: /*V2,   L2*/
        case 0b01000001: /*V2.5, L3*/
        case 0b01001001: /*V2,   L3*/ bitrate = 32000;  break;

        case 0b01011111: /*V1,   L1*/ bitrate = 160000; break;
        case 0b01011110: /*V1,   L2*/ bitrate = 80000;  break;
        case 0b01011101: /*V1,   L3*/ bitrate = 64000;  break;
        case 0b01010011: /*V2.5, L1*/
        case 0b01011011: /*V2,   L1*/ bitrate = 80000;  break;
        case 0b01010010: /*V2.5, L2*/
        case 0b01011010: /*V2,   L2*/
        case 0b01010001: /*V2.5, L3*/
        case 0b01011001: /*V2,   L3*/ bitrate = 40000;  break;

        case 0b01101111: /*V1,   L1*/ bitrate = 192000; break;
        case 0b01101110: /*V1,   L2*/ bitrate = 96000;  break;
        case 0b01101101: /*V1,   L3*/ bitrate = 80000;  break;
        case 0b01100011: /*V2.5, L1*/
        case 0b01101011: /*V2,   L1*/ bitrate = 96000;  break;
        case 0b01100010: /*V2.5, L2*/
        case 0b01101010: /*V2,   L2*/
        case 0b01100001: /*V2.5, L3*/
        case 0b01101001: /*V2,   L3*/ bitrate = 48000;  break;

        case 0b01111111: /*V1,   L1*/ bitrate = 224000; break;
        case 0b01111110: /*V1,   L2*/ bitrate = 112000; break;
        case 0b01111101: /*V1,   L3*/ bitrate = 96000;  break;
        case 0b01110011: /*V2.5, L1*/
        case 0b01111011: /*V2,   L1*/ bitrate = 112000; break;
        case 0b01110010: /*V2.5, L2*/
        case 0b01111010: /*V2,   L2*/
        case 0b01110001: /*V2.5, L3*/
        case 0b01111001: /*V2,   L3*/ bitrate = 56000;  break;

        case 0b10001111: /*V1,   L1*/ bitrate = 256000; break;
        case 0b10001110: /*V1,   L2*/ bitrate = 128000; break;
        case 0b10001101: /*V1,   L3*/ bitrate = 112000; break;
        case 0b10000011: /*V2.5, L1*/
        case 0b10001011: /*V2,   L1*/ bitrate = 128000; break;
        case 0b10000010: /*V2.5, L2*/
        case 0b10001010: /*V2,   L2*/
        case 0b10000001: /*V2.5, L3*/
        case 0b10001001: /*V2,   L3*/ bitrate = 64000;  break;

        case 0b10011111: /*V1,   L1*/ bitrate = 288000; break;
        case 0b10011110: /*V1,   L2*/ bitrate = 160000; break;
        case 0b10011101: /*V1,   L3*/ bitrate = 128000; break;
        case 0b10010011: /*V2.5, L1*/
        case 0b10011011: /*V2,   L1*/ bitrate = 144000; break;
        case 0b10010010: /*V2.5, L2*/
        case 0b10011010: /*V2,   L2*/
        case 0b10010001: /*V2.5, L3*/
        case 0b10011001: /*V2,   L3*/ bitrate = 80000;  break;

        case 0b10101111: /*V1,   L1*/ bitrate = 320000; break;
        case 0b10101110: /*V1,   L2*/ bitrate = 192000; break;
        case 0b10101101: /*V1,   L3*/ bitrate = 160000; break;
        case 0b10100011: /*V2.5, L1*/
        case 0b10101011: /*V2,   L1*/ bitrate = 160000; break;
        case 0b10100010: /*V2.5, L2*/
        case 0b10101010: /*V2,   L2*/
        case 0b10100001: /*V2.5, L3*/
        case 0b10101001: /*V2,   L3*/ bitrate = 96000;  break;

        case 0b10111111: /*V1,   L1*/ bitrate = 352000; break;
        case 0b10111110: /*V1,   L2*/ bitrate = 224000; break;
        case 0b10111101: /*V1,   L3*/ bitrate = 192000; break;
        case 0b10110011: /*V2.5, L1*/
        case 0b10111011: /*V2,   L1*/ bitrate = 176000; break;
        case 0b10110010: /*V2.5, L2*/
        case 0b10111010: /*V2,   L2*/
        case 0b10110001: /*V2.5, L3*/
        case 0b10111001: /*V2,   L3*/ bitrate = 112000; break;

        case 0b11001111: /*V1,   L1*/ bitrate = 384000; break;
        case 0b11001110: /*V1,   L2*/ bitrate = 256000; break;
        case 0b11001101: /*V1,   L3*/ bitrate = 224000; break;
        case 0b11000011: /*V2.5, L1*/
        case 0b11001011: /*V2,   L1*/ bitrate = 192000; break;
        case 0b11000010: /*V2.5, L2*/
        case 0b11001010: /*V2,   L2*/
        case 0b11000001: /*V2.5, L3*/
        case 0b11001001: /*V2,   L3*/ bitrate = 128000; break;

        case 0b11011111: /*V1,   L1*/ bitrate = 416000; break;
        case 0b11011110: /*V1,   L2*/ bitrate = 320000; break;
        case 0b11011101: /*V1,   L3*/ bitrate = 256000; break;
        case 0b11010011: /*V2.5, L1*/
        case 0b11011011: /*V2,   L1*/ bitrate = 224000; break;
        case 0b11010010: /*V2.5, L2*/
        case 0b11011010: /*V2,   L2*/
        case 0b11010001: /*V2.5, L3*/
        case 0b11011001: /*V2,   L3*/ bitrate = 144000; break;

        case 0b11101111: /*V1,   L1*/ bitrate = 448000; break;
        case 0b11101110: /*V1,   L2*/ bitrate = 384000; break;
        case 0b11101101: /*V1,   L3*/ bitrate = 320000; break;
        case 0b11100011: /*V2.5, L1*/
        case 0b11101011: /*V2,   L1*/ bitrate = 256000; break;
        case 0b11100010: /*V2.5, L2*/
        case 0b11101010: /*V2,   L2*/
        case 0b11100001: /*V2.5, L3*/
        case 0b11101001: /*V2,   L3*/ bitrate = 160000; break;
        default: break;
      }

      return bitrate;
    }

    uint32_t sample_rate()
    {
      uint32_t sample_rate = 0xFFFFFFFF;
      uint8_t index = (samplerate_idx << 2) | version_id;
      switch(index) {
        case 0x03: /* MPEG1   */ sample_rate = 44100; break;
        case 0x02: /* MPEG2   */ sample_rate = 22050; break;
        case 0x00: /* MPEG2.5 */ sample_rate = 11025; break;

        case 0x07: /* MPEG1   */ sample_rate = 48000; break;
        case 0x06: /* MPEG2   */ sample_rate = 24000; break;
        case 0x04: /* MPEG2.5 */ sample_rate = 12000; break;

        case 0x0B: /* MPEG1   */ sample_rate = 32000; break;
        case 0x0A: /* MPEG2   */ sample_rate = 16000; break;
        case 0x08: /* MPEG2.5 */ sample_rate = 8000;  break;
        default: break;
      }

      return sample_rate;
    }

    uint32_t frame_sample_size()
    {
      uint32_t size = 0;
      switch(layer) {
        case 1: /* Layer III and Layer II always have 1152 samples in 1 frame*/
        case 2: size = 1152; break;
                /* Layer I always has 384 samples in 1 frame */
        case 3: size = 384;  break;
        case 0:
        default: break;;
      }

      return size;
    }

    uint32_t frame_bytes()
    {
      uint32_t bytes = 0;
      switch(layer) {
        case 0b01: /* Layer 3 */
        case 0b10: /* Layer 2 */
          bytes = (uint32_t)(144 * bitrate() / sample_rate()) + padding;
          break;
        case 0b11: /* Layer 1 */
          bytes = ((uint32_t)(12 * bitrate() / sample_rate()) + padding) * 4;
          break;
        default: break;
      }

      return bytes;
    }

    uint8_t channel_number()
    {
      uint8_t channel_number = 1;
      switch(channel_mode) {
        case 0:
        case 1:
        case 2: channel_number = 2; break;
        case 3: channel_number = 1; break;
        default: break;
      }
      return channel_number;
    }

    const char* layer_str()
    {
      const char *layer_str = "Unknown";
      switch(layer) {
        case 1: layer_str = "III"; break;
        case 2: layer_str = "II";  break;
        case 3: layer_str = "I";   break;
        default: break;
      }
      return layer_str;
    }

    const char* version_str()
    {
      const char *version = "Unknown";
      switch(version_id) {
        case 3: version = "3";   break;
        case 2: version = "2";   break;
        case 0: version = "2.5"; break;
        default: break;
      }
      return version;
    }
};

#endif /* MP3_H_ */
