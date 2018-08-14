/*
 * agent_dev.h
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
#ifndef __AGENT_DEV_H__
#define __AGENT_DEV_H__

ctx_ptr_t agent_dev_create(device_ptr_t p_device);

int32_t  agent_dev_msg_post(ctx_ptr_t p_ctx, int32_t msg);
int32_t  agent_dev_msg_request(ctx_ptr_t p_ctx, msg_ptr_t p_msg, msg_notify_cb_t notify_cb);
int32_t  agent_dev_msg_cancel(ctx_ptr_t p_ctx, int32_t msg_id);

void agent_dev_set_msg_notify_cb(ctx_ptr_t p_ctx, handle_t h_opaque, msg_notify_cb_t notify_cb);
void agent_dev_notify(ctx_ptr_t p_ctx, msg_ptr_t p_msg, result_ptr_t p_result);

int32_t agent_dev_standby(int32_t fd, const char *uid, const char *category, int32_t mode, unsigned char *p_token, int32_t token_len);

typedef struct {
    CHANNEL_COMMON

    int32_t             buf_size;
    int32_t             w_off;
    char                w_buf[BUF_SIZE_2048];
    int32_t             r_off;
    char                r_buf[BUF_SIZE_2048];

    ctx_ptr_t           p_ctx;
} channel_agent_dev_t, *channel_agent_dev_ptr_t, **channel_agent_dev_pptr_t;


#endif
