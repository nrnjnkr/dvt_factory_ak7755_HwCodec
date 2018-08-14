/*
 * proc_dev.c
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
#include "proc_dev.h"

static int32_t agent_dev_request_proc(channel_ptr_t p_channel, int32_t what, json_object *p_in_json, char *p_data, int32_t data_len) {
    int32_t ret = RET_OK;
    int32_t cat = MSG_CAT(what);

    (void)ret;

    msg_t msg;
    msg.what = what;

    channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)p_channel;
    if (cat == MSG_CAT_DEVICE) {
        switch (what) {
        case MSG_DEVICE_LOGIN: {
            json_object *p_json_cloud = NULL;
            if (json_object_object_get_ex(p_in_json, KEY_CLOUD, &p_json_cloud) != TRUE) {
                LOGE("get cloud failure: %s", p_data);
                break;
            }

            json_object *p_json_stream_id = NULL;
            if (json_object_object_get_ex(p_in_json, KEY_STREAM_ID, &p_json_stream_id) != TRUE) {
                LOGE("get stream_id failure: %s", p_data);
                break;
            }

            if (json_object_get_boolean(p_json_cloud)) {
                msg.what = MSG_DBG_START_RTSP_INJECTOR;
                msg.para1 = json_object_get_int(p_json_stream_id);
                agent_dev_notify(p_client->p_ctx, &msg, NULL);
            }
        } break;
        case MSG_DEVICE_ALARM:
        case MSG_DEVICE_STANDBY_TCP:
        case MSG_DEVICE_STANDBY_UDP:
            agent_dev_notify(p_client->p_ctx, &msg, NULL);
            break;
        case MSG_DEVICE_SYNC: {
            sprintf(g_tmp_buf, "{\"%s\":%d, \"%s\":%d}\n",
                        KEY_WHAT, MSG_DEVICE_SYNC,
                        KEY_RET, 0);
            agent_dev_cloud_send_sync(p_channel, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
        } break;
        case MSG_FAST_STREAMING: {
            LOGD("======= MSG_FAST_STREAMING 0x%x=======\n", MSG_FAST_STREAMING);
            json_object *p_json_ip = NULL;
            result_t p_result;
            if (json_object_object_get_ex(p_in_json, KEY_IP, &p_json_ip) != TRUE) {
                LOGE("get cloud failure: %s", p_data);
                break;
            }
            strcpy(g_tmp_buf, json_object_get_string(p_json_ip));
            p_result.obj = (void*)g_tmp_buf;
            agent_dev_notify(p_client->p_ctx, &msg, &p_result);
        } break;
        default:
            LOGD("default handler, what:%d", what);
            agent_dev_notify(p_client->p_ctx, &msg, NULL);
            break;
        }
    } else if (cat == MSG_CAT_DBG) {
        agent_dev_notify(p_client->p_ctx, &msg, NULL);
    } else if (cat == MSG_CAT_RESERVED) {
        switch (what) {
        case MSG_RESERVED_SYNC: {
            agent_dev_cloud_send_sync(p_channel, p_data, data_len+1);
        } break;
        case MSG_RESERVED_PLAY_AUDIO:
        case MSG_RESERVED_SHUTDOWN:
            agent_dev_notify(p_client->p_ctx, &msg, NULL);
            break;
        default:
            LOGD("default handler, what:%d", what);
            agent_dev_notify(p_client->p_ctx, &msg, NULL);
            break;
        }
    }
    return ret;
}

int32_t agent_dev_cloud_send_sync(channel_ptr_t p_channel, const char *p_data, int32_t data_len) {
    channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)p_channel;
    if (!p_client) { return RET_ERROR; }

    int32_t ret = (int32_t)write(p_client->watcher.io.fd, p_data, data_len);
    if (ret > 0) {
        if (ret == data_len) { return RET_OK; }

        int32_t left = data_len - ret;
        if (left > p_client->buf_size) {
            LOGD("buf full");
            return RET_ERROR_OUT_BUF_FULL;
        }

        memmove(p_client->w_buf, p_data + ret, left);
        p_client->w_off = left;
    } else {
        if (errno != EINTR && errno != EAGAIN) { return RET_ERROR_WRITE; }

        if (data_len > p_client->buf_size) {
            LOGD("buf full");
            return RET_ERROR_OUT_BUF_FULL;
        }

        memmove(p_client->w_buf, p_data, data_len);
        p_client->w_off = data_len;
    }

    int32_t flags = EV_READ;
    if (p_client->w_off > 0) { flags |= EV_WRITE; }
    channel_io_update((channel_ptr_t)p_client, flags);

    return RET_OK;
}

int32_t agent_dev_cloud_send_async(channel_ptr_t p_channel, const char *p_data, int32_t data_len) {
    channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)p_channel;
    if (!p_client) { return RET_ERROR; }

    if (p_client->w_off + data_len > p_client->buf_size) {
        LOGE("buf full");
        return RET_ERROR_OUT_BUF_FULL;
    }
    memmove(p_client->w_buf + p_client->w_off, p_data, data_len);
    p_client->w_off += data_len;

    msg_t msg;
    msg.what = MSG_SELF_CHANNEL_UPDATE;
    msg.obj = p_client;
    agent_dev_msg_request(p_client->p_ctx, &msg, NULL);
    return RET_OK;
}

int32_t agent_cloud_msg_proc(channel_ptr_t p_channel, char *p_data, int32_t data_len) {
    int32_t ret = RET_OK;

    json_tokener *p_tokener = NULL;
    json_object *p_in_json = NULL;

    do {
        p_tokener = json_tokener_new();
        if (!p_tokener) {
            LOGE("new tokener failure");
            break;
        }

        p_in_json = json_tokener_parse_ex(p_tokener, p_data, data_len);
        if (!p_in_json) {
            LOGE("parse json failure: %s", p_data);
            // TODO: change daredevil wakeup event to json
            if(!strncmp(p_data, "amba_wakeup", 11)){
                channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)p_channel;
                msg_t msg;
                msg.what = MSG_DEVICE_WAKEUP_TCP;
                agent_dev_notify(p_client->p_ctx, &msg, NULL);
            }
            break;
        }

        LOGD("recv: %s", p_data);

        json_object *p_json_what = NULL;
        if (json_object_object_get_ex(p_in_json, KEY_WHAT, &p_json_what) != TRUE) {
            LOGE("get object failure: %s", p_data);
            break;
        }

        int32_t what = json_object_get_int(p_json_what);
        ret = agent_dev_request_proc(p_channel, what, p_in_json, p_data, data_len);
    } while (0);

    if (p_in_json) { json_object_put(p_in_json); }
    if (p_tokener) { json_tokener_free(p_tokener); }

    return ret;
}
