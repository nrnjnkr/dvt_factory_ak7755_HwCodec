/*
 * impl_dev.h
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
#ifndef __IMPL_DEV_H__
#define __IMPL_DEV_H__

#include <pthread.h>
#include <ev.h>
#include <json-c/json.h>

#include "queue.h"
#include "channel.h"

enum {
    MSG_SELF_HOME                   = MSG_WHAT(MSG_CAT_SELF, 0),
    MSG_SELF_BREAK_LOOP,
    MSG_SELF_CHANNEL_UPDATE,
    MSG_SELF_CONNECT_CLOUD,
    MSG_SELF_CONNECT_CLOUD_FD,
    MSG_SELF_DISCONNECT_CLOUD,
    MSG_SELF_SYNC_CLOUD,
    MSG_SELF_STANDBY_TCP,
    MSG_SELF_STREAMING,
    MSG_SELF_END,
};

typedef struct {
    param_device_t      param;

    int32_t             state;
    pthread_mutex_t     lock;
    bool                b_busy;

    ctx_ptr_t           p_agent_dev;
} device_t, *device_ptr_t, **device_pptr_t;

#define MAX_DBG_BUF_SIZE    512
extern char g_tmp_buf[MAX_DBG_BUF_SIZE+1];

#endif
