/*
 * channel.c
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
#include <ev.h>

#include "defs.h"
#include "channel.h"

int32_t channel_io_start(channel_ptr_t p_channel, loop_ptr_t p_loop) {
    ASSERT(p_channel && p_loop && !p_channel->p_loop);
    ev_io_start(p_loop, &p_channel->watcher.io);
    p_channel->p_loop = p_loop;
    return RET_OK;
}

int32_t channel_io_stop(channel_ptr_t p_channel) {
    ASSERT(p_channel);
    if (p_channel->p_loop && p_channel->watcher.io.fd > 0) {
        ev_io_stop(p_channel->p_loop, &p_channel->watcher.io);
    }
    p_channel->p_loop = NULL;
    return RET_OK;
}

void channel_io_destroy(channel_pptr_t pp_channel) {
    ASSERT(pp_channel);
    channel_ptr_t p_channel = *pp_channel;
    ASSERT(p_channel);

    if (p_channel->watcher.io.fd > 0) {
        channel_io_stop(p_channel);
        close(p_channel->watcher.io.fd);
    }

    MEM_FREE_PPTR(pp_channel);
}

int32_t channel_io_update(channel_ptr_t p_channel, int32_t flags) {
    ev_io *watcher = &p_channel->watcher.io;

    if ((watcher->fd > 0) && ((watcher->events&FLAG_IO_READ_WRITE) != flags)) {
        ev_io_stop(p_channel->p_loop, watcher);
        ev_io_set(watcher, watcher->fd, flags);
        ev_io_start(p_channel->p_loop, watcher);
    }
    return RET_OK;
}

int32_t channel_async_start(channel_ptr_t p_channel, loop_ptr_t p_loop) {
    ASSERT(p_channel && p_loop && !p_channel->p_loop);
    ev_async_start(p_loop, &p_channel->watcher.async);
    p_channel->p_loop = p_loop;
    return RET_OK;
}

int32_t channel_async_stop(channel_ptr_t p_channel) {
    if (p_channel->p_loop) {
        ev_async_stop(p_channel->p_loop, &p_channel->watcher.async);
        p_channel->p_loop = NULL;
    }
    return RET_OK;
}

void channel_async_destroy(channel_pptr_t pp_channel) {
    ASSERT(pp_channel);
    channel_ptr_t p_channel = *pp_channel;
    ASSERT(p_channel);

    channel_async_stop(p_channel);

    MEM_FREE_PPTR(pp_channel);
}

int32_t channel_timer_start(channel_ptr_t p_channel, loop_ptr_t p_loop) {
    ASSERT(p_channel && p_loop && !p_channel->p_loop);
    ev_timer_start(p_loop, &p_channel->watcher.timer);
    p_channel->p_loop = p_loop;
    return RET_OK;
}

int32_t channel_timer_stop(channel_ptr_t p_channel) {
    if (p_channel->p_loop) {
        ev_timer_stop(p_channel->p_loop, &p_channel->watcher.timer);
        p_channel->p_loop = NULL;
    }
    return RET_OK;
}

void channel_timer_destroy(channel_pptr_t pp_channel) {
    ASSERT(pp_channel);
    channel_ptr_t p_channel = *pp_channel;
    ASSERT(p_channel);

    channel_timer_stop(p_channel);

    MEM_FREE_PPTR(pp_channel);
}

int32_t channel_periodic_start(channel_ptr_t p_channel, loop_ptr_t p_loop) {
    ASSERT(p_channel && p_loop && !p_channel->p_loop);
    ev_periodic_start(p_loop, &p_channel->watcher.periodic);
    p_channel->p_loop = p_loop;
    return RET_OK;
}

int32_t channel_periodic_stop(channel_ptr_t p_channel) {
    if (p_channel->p_loop) {
        ev_periodic_stop(p_channel->p_loop, &p_channel->watcher.periodic);
        p_channel->p_loop = NULL;
    }
    return RET_OK;
}

void channel_periodic_destroy(channel_pptr_t pp_channel) {
    ASSERT(pp_channel);
    channel_ptr_t p_channel = *pp_channel;
    ASSERT(p_channel);

    channel_periodic_stop(p_channel);

    MEM_FREE_PPTR(pp_channel);
}
