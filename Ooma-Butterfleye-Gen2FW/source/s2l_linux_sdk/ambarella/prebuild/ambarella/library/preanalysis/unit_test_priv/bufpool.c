/*******************************************************************************
 * \file bufpool.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "preanalysis.h"
#include "bufpool.h"

void *create_bufpool(amba_preanalysis_buf_info_t *info, int size)
{
  int i;
  bufpool_t *pool = NULL;

  assert(info);

  if (32 < size)
  {
    fprintf(stderr, "%s(): current implementation does not supports pool size %d > 32\n", __func__, size);
    return (void *)pool;
  }

  pool = (bufpool_t *)malloc(sizeof(bufpool_t));
  if (NULL == pool)
  {
    fprintf(stderr, "%s(): error calling malloc()\n", __func__);
    return NULL;
  }

  pool->base = (uint8_t *)malloc(info->pitch * info->height * size);
  if (NULL == pool->base)
  {
    fprintf(stderr, "%s(): error calling malloc()\n", __func__);
    free(pool);
    return NULL;
  }

  pool->width = info->width;
  pool->height = info->height;
  pool->pitch = info->pitch;
  pool->in_use_pattern = 0;
  pool->total_num = size;
  pool->used_num = 0;

  for (i = 0; i < size; ++i)
  {
    pool->offset[i] = &pool->base[pool->pitch * pool->height * i];
  }

  return (void *)pool;
}

int request_buf(amba_preanalysis_buf_t *buf)
{
  bufpool_t *pool = NULL;
  int index;

  assert(buf);

  pool = (bufpool_t *)buf->pool;
  assert(pool);

  if (pool->total_num == pool->used_num)
  {
    fprintf(stderr, "%s(): buffer pool full\n", __func__);
    return -1;
  }

  for (index = 0; index < pool->total_num; ++index)
  {
    if (0 == (pool->in_use_pattern & (1 << index)))
    {
      break;
    }
  }

  if (index == pool->total_num)
  {
    fprintf(stderr, "%s(): cannot find free buffer 0x%08x\n", __func__, pool->in_use_pattern);
    return -1;
  }

  pool->in_use_pattern |= 1 << index;
  pool->used_num += 1;
  buf->base = pool->offset[index];
  //fprintf(stdout, "%s(): buffer[%d] %p is allocated, in_use_pattern=0x%08x, used_num=%d\n", __func__, index, buf->base, pool->in_use_pattern, pool->used_num);

  return 0;
}

int release_buf(amba_preanalysis_buf_t *buf)
{
  bufpool_t *pool = NULL;
  int index;

  assert(buf);

  pool = (bufpool_t *)buf->pool;
  assert(pool);

  for (index = 0; index < pool->total_num; ++index)
  {
    if (buf->base == pool->offset[index])
    {
      break;
    }
  }

  if (index == pool->total_num)
  {
    fprintf(stderr, "%s(): buffer %p does not belong to buffer pool %p\n", __func__, buf->base, buf->pool);
    return -1;
  }

  if (0 == (pool->in_use_pattern & (1 << index)) || !pool->used_num)
  {
    fprintf(stderr, "%s(): buffer[%d] %p is unused, in_use_pattern=0x%08x, used_num=%d\n", __func__, index, buf->base, pool->in_use_pattern, pool->used_num);
    return -1;
  }

  pool->in_use_pattern &= ~(1 << index);
  pool->used_num -= 1;
  //fprintf(stdout, "%s(): buffer[%d] %p is freed, in_use_pattern=0x%08x, used_num=%d\n", __func__, index, buf->base, pool->in_use_pattern, pool->used_num);


  return 0;
}

int get_buf_info(void *buf_pool, amba_preanalysis_buf_info_t *info)
{
  bufpool_t *pool = (bufpool_t *)buf_pool;

  assert(buf_pool && info);

  info->width = pool->width;
  info->pitch = pool->pitch;
  info->height = pool->height;

  return 0;
}

void destroy_bufpool(void *buf_pool)
{
  bufpool_t *pool = (bufpool_t *)buf_pool;

  assert(pool);

  if (pool->used_num || pool->in_use_pattern)
  {
    fprintf(stderr, "%s(): there are still buffers in use, num=%d, pattern=0x%08x\n", __func__, pool->used_num, pool->in_use_pattern);
  }

  free(pool->base);
  free(pool);
}
