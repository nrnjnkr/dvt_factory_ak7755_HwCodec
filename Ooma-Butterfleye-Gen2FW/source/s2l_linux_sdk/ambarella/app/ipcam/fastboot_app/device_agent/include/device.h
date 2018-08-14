/*
 * device.h
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
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "defs.h"

typedef struct {
    char        srv_ip[MAX_IP_LEN];
    int32_t     srv_dev_port;
    char        dev_uid[MAX_UID_LEN];
    int32_t     dev_mode;
    char        dev_category[MAX_CATEGORY_NAME_LEN];
    char        dev_token[MAX_WAKE_TOKEN_LEN];
} param_device_t, *param_device_ptr_t;
#ifdef __cplusplus
extern "C" {
#endif
handle_t device_create(param_device_ptr_t p_param);
void     device_destroy(handle_ptr_t h_ptr);

int32_t  device_init(handle_t h_device);
int32_t  device_deinit(handle_t h_device);

int32_t  device_start(handle_t h_device);
int32_t  device_stop(handle_t h_device);

int32_t  device_state(handle_t h_device);
bool     device_is_running(handle_t h_device);

int32_t  device_msg_post(handle_t h_device, int32_t what);
int32_t  device_msg_request(handle_t h_device, msg_ptr_t p_msg, msg_notify_cb_t notify_cb);
int32_t  device_msg_cancel(handle_t h_device, int32_t msg_id);

void device_set_msg_notify_cb(handle_t h_device, handle_t h_opaque, msg_notify_cb_t notify_cb);
void device_set_device_mode(handle_t h_device,int32_t  dev_mode);

void device_connect_cloud(handle_t h_device);
void device_connect_cloud_fd(handle_t h_device, int32_t fd);
void device_disconnect_cloud(handle_t h_device);
void device_standby(handle_t h_device);
void device_online_streaming(handle_t h_device);

int32_t device_standby_tcp(int32_t fd, const char *uid, const char *category, unsigned char *p_token, int32_t token_len);
int32_t device_standby_udp(int32_t fd, const char *uid, const char *category, unsigned char *p_token, int32_t token_len);

#ifdef __cplusplus
};
#endif

#endif
