/*
 * channel.h
 *
 * History:
 *       2015/03/10 - [jywang] created file
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
 */
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

typedef struct ev_loop loop_t, *loop_ptr_t;

typedef struct channel channel_t, *channel_ptr_t, **channel_pptr_t;

#define CHANNEL_COMMON                                                          \
    loop_ptr_t      p_loop;                                                     \
    union ev_any_watcher  watcher;                                                    \
    int32_t         (*start)(channel_ptr_t p_channel, loop_ptr_t p_loop);       \
    int32_t         (*stop)(channel_ptr_t p_channel);                           \
    void            (*destroy)(channel_pptr_t pp_channel);

struct channel {
    CHANNEL_COMMON
};

int32_t channel_io_start(channel_ptr_t p_channel, loop_ptr_t p_loop);
int32_t channel_io_stop(channel_ptr_t p_channel);
void    channel_io_destroy(channel_pptr_t pp_channel);
int32_t channel_io_update(channel_ptr_t p_channel, int32_t flags);

int32_t channel_async_start(channel_ptr_t p_channel, loop_ptr_t p_loop);
int32_t channel_async_stop(channel_ptr_t p_channel);
void    channel_async_destroy(channel_pptr_t pp_channel);

int32_t channel_timer_start(channel_ptr_t p_channel, loop_ptr_t p_loop);
int32_t channel_timer_stop(channel_ptr_t p_channel);
void    channel_timer_destroy(channel_pptr_t pp_channel);

int32_t channel_periodic_start(channel_ptr_t p_channel, loop_ptr_t p_loop);
int32_t channel_periodic_stop(channel_ptr_t p_channel);
void    channel_periodic_destroy(channel_pptr_t pp_channel);

#endif
