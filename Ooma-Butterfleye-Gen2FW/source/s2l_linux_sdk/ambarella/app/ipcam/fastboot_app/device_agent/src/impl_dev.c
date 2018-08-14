/*
 * impl_dev.c
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
#include "device.h"
#include "impl_dev.h"
#include "agent_dev.h"

char g_tmp_buf[MAX_DBG_BUF_SIZE+1];
#ifdef __cplusplus
extern "C"{
#endif
handle_t device_create(param_device_ptr_t p_param) {
    device_ptr_t p_device = MEM_ALLOCZ(device_t);
    if (p_device) {
        p_device->state = STATE_CREATED;
        memmove(&p_device->param, p_param, sizeof(*p_param));
    }
    return p_device;
}

void device_destroy(handle_ptr_t h_ptr_device) {
    device_stop(*h_ptr_device);
    device_deinit(*h_ptr_device);
    MEM_FREE_PPTR(h_ptr_device);
}

int32_t device_init(handle_t h_device) {
    int32_t ret = RET_ERROR;

    device_ptr_t p_device = (device_ptr_t)h_device;
    int32_t state = p_device->state;
    if (state == STATE_INITIALIZED) {
        ret = RET_OK;
        LOGW("duplicated");
    } else if (p_device->state == STATE_CREATED) {
        do {
            pthread_mutex_init(&p_device->lock, NULL);

            p_device->p_agent_dev = agent_dev_create(p_device);
            if (!p_device->p_agent_dev) {
                LOGE("create agent_dev failure");
                p_device->state = STATE_ERROR_INIT;
                break;
            }

            p_device->state = STATE_INITIALIZED;
            ret = RET_OK;
        } while (0);
    } else {
        ret = RET_ERROR_INVLAID_STATE;
        LOGE("invalid state: %d", state);
    }
    return ret;
}

int32_t device_deinit(handle_t h_device) {
    int32_t ret = RET_ERROR;

    device_ptr_t p_device = (device_ptr_t)h_device;
    int32_t state = p_device->state;
    if (state == STATE_CREATED) {
        ret = RET_OK;
        LOGW("duplicated");
    } else if (state == STATE_INITIALIZED || state == STATE_STOPPED || state == STATE_ERROR_INIT) {
        if (p_device->p_agent_dev) {
            p_device->p_agent_dev->destroy(&p_device->p_agent_dev);
        }

        pthread_mutex_destroy(&p_device->lock);

        p_device->state = STATE_CREATED;
        ret = RET_OK;
    } else {
        ret = RET_ERROR_INVLAID_STATE;
        LOGE("invalid state: %d", state);
    }
    return ret;
}

int32_t device_start(handle_t h_device) {
    int32_t ret = RET_ERROR;

    device_ptr_t p_device = (device_ptr_t)h_device;
    int32_t state = p_device->state;
    if (state == STATE_STARTED) {
        ret = RET_OK;
        LOGW("duplicated");
    } else if (state == STATE_INITIALIZED || state == STATE_STOPPED) {
        if (p_device->b_busy) {
            LOGW("busy");
            ret = RET_ERROR_BUSY;
        } else {
            pthread_mutex_lock(&p_device->lock);
            p_device->b_busy = true;
            do {
                ctx_ptr_t p_ctx = p_device->p_agent_dev;
                ret = p_ctx->start(p_ctx);
                if (ret != RET_OK) {
                    LOGE("start agent_dev failure");
                    p_device->state = STATE_ERROR_START;
                    break;
                }

                p_device->state = STATE_STARTED;
                ret = RET_OK;
            } while (0);
            p_device->b_busy = false;
            pthread_mutex_unlock(&p_device->lock);
        }
    } else {
        ret = RET_ERROR_INVLAID_STATE;
        LOGE("invalid state: %d", state);
    }
    return ret;
}

int32_t device_stop(handle_t h_device) {
    int32_t ret = RET_ERROR;

    device_ptr_t p_device = (device_ptr_t)h_device;
    int32_t state = p_device->state;
    if (state == STATE_STOPPED) {
        ret = RET_OK;
        LOGW("duplicated");
    } else if (state == STATE_STARTED || state == STATE_ERROR_START) {
        if (p_device->b_busy) {
            LOGW("busy");
            ret = RET_ERROR_BUSY;
        } else {
            pthread_mutex_lock(&p_device->lock);
            p_device->b_busy = true;

            // stop agent_dev
            ctx_ptr_t p_ctx = p_device->p_agent_dev;
            if (p_ctx) { p_ctx->stop(p_ctx); }

            p_device->state = STATE_STOPPED;
            ret = RET_OK;

            p_device->b_busy = false;
            pthread_mutex_unlock(&p_device->lock);
        }
    } else {
        ret = RET_ERROR_INVLAID_STATE;
        LOGE("invalid state: %d", state);
    }
    return ret;
}

int32_t device_state(handle_t h_device) {
    ASSERT(h_device);
    device_ptr_t p_device = (device_ptr_t)h_device;
    return p_device->state;
}

bool device_is_running(handle_t h_device) {
    ASSERT(h_device);
    device_ptr_t p_device = (device_ptr_t)h_device;
    return (p_device->state == STATE_STARTED);
}

void device_set_msg_notify_cb(handle_t h_device, handle_t h_opaque, msg_notify_cb_t notify_cb) {
    device_ptr_t p_device = (device_ptr_t)h_device;
    agent_dev_set_msg_notify_cb(p_device->p_agent_dev, h_opaque, notify_cb);
}

void device_set_device_mode(handle_t h_device, int32_t  dev_mode){
    device_ptr_t p_device = (device_ptr_t)h_device;
    p_device ->param.dev_mode = dev_mode;
}
int32_t  device_msg_post(handle_t h_device, int32_t what) {
    device_ptr_t p_device = (device_ptr_t)h_device;
    return agent_dev_msg_post(p_device->p_agent_dev, what);
}

int32_t device_msg_request(handle_t h_device, msg_ptr_t p_msg, msg_notify_cb_t notify_cb) {
    device_ptr_t p_device = (device_ptr_t)h_device;
    return agent_dev_msg_request(p_device->p_agent_dev, p_msg, notify_cb);
}

int32_t device_msg_cancel(handle_t h_device, int32_t msg_id) {
    device_ptr_t p_device = (device_ptr_t)h_device;
    return agent_dev_msg_cancel(p_device->p_agent_dev, msg_id);
}

void device_connect_cloud(handle_t h_device) {
    device_msg_post(h_device, MSG_SELF_CONNECT_CLOUD);
}

void device_connect_cloud_fd(handle_t h_device, int32_t fd) {
    msg_t msg = {0};
    msg.what = MSG_SELF_CONNECT_CLOUD_FD;
    msg.para1 = fd;
    device_msg_request(h_device, &msg, NULL);
}

void device_disconnect_cloud(handle_t h_device) {
    device_msg_post(h_device, MSG_SELF_DISCONNECT_CLOUD);
}

void device_standby(handle_t h_device) {
    device_msg_post(h_device, MSG_SELF_STANDBY_TCP);
}

void device_online_streaming(handle_t h_device) {
    device_msg_post(h_device, MSG_SELF_STREAMING);
}

int32_t device_standby_tcp(int32_t fd, const char *uid, const char *category, unsigned char *p_token, int32_t token_len) {
    return agent_dev_standby(fd, uid, category, DEVICE_MODE_STANDBY_TCP, p_token, token_len);
}

int32_t device_standby_udp(int32_t fd, const char *uid, const char *category, unsigned char *p_token, int32_t token_len) {
    return agent_dev_standby(fd, uid, category, DEVICE_MODE_STANDBY_UDP, p_token, token_len);
}
#ifdef __cplusplus
};
#endif

static int32_t parse_ip(const char *p_str, int32_t len, char ip[MAX_IP_LEN]) {
    int32_t ret = -1;
    do {
        const char *p = p_str;
        int32_t times = 0;
        while (NULL != (p = strchr(p, '.'))) {
            times++;
            p++;
        }

        if (times != 3) {
            LOGW("invalid ip address");
            break;
        }

        memmove(ip, p_str, len);
        ip[len] = '\0';
        ret = 0;
    } while (0);
    return ret;
}

static int32_t parse_port(const char *p_str, int32_t len, int32_t *p_port) {
    if (!p_port) { return RET_OK; }

    int32_t ret = -1;
    do {
        if (len > MAX_PORT_LEN) {
            LOGE("port len exceeds 16 bytes %d", len);
            break;
        }

        char buf[MAX_PORT_LEN];
        memmove(buf, p_str, len);
        buf[len] = '\0';

        int32_t port = atoi(buf);
        if (port == 0) {
            LOGE("invalid port");
            break;
        }

        *p_port = port;
        ret = 0;
    } while (0);
    return ret;
}

int32_t param_device_parse_server(const char *p_str, char ip[MAX_IP_LEN], int32_t *p_port) {
    int32_t ret = -1;
    do {
        // skip protocol
        const char *p_proto = "://";
        char *p = strstr(p_str, p_proto);
        if (p) { p_str = p + strlen(p_proto); }

        int32_t len = 0;
        p = strchr(p_str, ':');
        if (p) {
            // parse ip & port
            len = (int32_t)(p - p_str);
            ret = parse_ip(p_str, len, ip);
            if (ret < 0) {
                LOGE("invalid ip");
                break;
            }

            len = (int32_t)(strlen(p_str) - len - 1);
            ret = parse_port(p+1, len, p_port);
        } else {
            // if ip, parse ip
            // else parse port
            len = (int32_t)strlen(p_str);
            p = strchr(p_str, '.');
            if (p) {
                ret = parse_ip(p_str, len, ip);
                break;
            } else {
                ret = parse_port(p_str, len, p_port);
                break;
            }
        }
    } while (0);
    return ret;
}
