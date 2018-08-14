/*******************************************************************************
 * \file utils.c
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

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "utils.h"

#define ALPHA_TABLE_8B_MAX                    64

void create_alpha(uint8_t *buf, int width, int height)
{
  int i, j;
  uint8_t alpha_table[ALPHA_TABLE_8B_MAX + 1];

  /*
   * we do not really need to initialize this table during every call, but this
   * is just a unit-test utility
   */
  for(i = 0; i <= ALPHA_TABLE_8B_MAX; ++i)
  {
    uint16_t tmp;

    if (0 == i)
    {
      alpha_table[i] = 0;
      continue;
    }

    tmp = 256 / i;
    if (tmp < 256)
    {
      tmp += 1;
    }
    if (256 == tmp)
    {
      tmp = 255;
    }
    alpha_table[i] = (uint8_t)tmp;
  }

  for (i = 0; i < height; ++i)
  {
    for (j = 0; j < width; ++j)
    {
      buf[i * width + j] = alpha_table[buf[i * width + j]];
    }
  }
}

void down_sample(uint8_t *src, uint8_t *dst, int width, int height, int out_pitch, int shift)
{
  const int step = 1 << shift;
  int in_x, in_y, out_x, out_y;

  assert(src);
  assert(dst);

  for (in_y = 0, out_y = 0; in_y < height; in_y += step, out_y += 1)
  {
    for (in_x = 0, out_x = 0; in_x < width; in_x += step, out_x += 1)
    {
      int x, y;
      int end_x = min(width, in_x + step);
      int end_y = min(height, in_y + step);
      int counter = 0;
      int sum = 0;

      for (y = in_y; y < end_y; ++y)
      {
        for (x = in_x; x < end_x; ++x)
        {
          sum += src[y * width + x];
          counter += 1;
        }
      }

      dst[out_y * out_pitch + out_x] = (sum + (counter >> 1)) / counter;
    }
  }
}

void scale_result(uint8_t *buf, int width, int height, int pitch)
{
  int i, j;

  for (j = 0; j < height; ++j)
  {
    for (i = 0; i < width; ++i)
    {
      buf[i] *= 255;
    }

    buf += pitch;
  }
}
