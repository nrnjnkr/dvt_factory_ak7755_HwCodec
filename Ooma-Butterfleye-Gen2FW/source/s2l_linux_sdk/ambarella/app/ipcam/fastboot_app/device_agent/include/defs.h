/*
 * defs.h
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
#ifndef __DEFS_H__
#define __DEFS_H__

// #define NDEBUG

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <errno.h>

#ifndef NDEBUG
#define LOGI(fmt, ...)      fprintf(stdout, "[I] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)      fprintf(stdout, "[D] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)      fprintf(stdout, "[E] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)      fprintf(stdout, "[W] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGV(fmt, ...)      fprintf(stdout, "[V] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ASSERT(x)			assert(x);
#else
#define LOGI(fmt, ...)
#define LOGD(fmt, ...)
#define LOGE(fmt, ...)
#define LOGW(fmt, ...)
#define LOGV(fmt, ...)
#define ASSERT(x)
#endif

#define MAJOR_VERSION       1
#define MINOR_VERSION       1

typedef void    *handle_t, **handle_ptr_t;
typedef void    *void_ptr_t, **void_pptr_t;

void_ptr_t 	mem_allocz(int32_t size);
void		mem_free_ptr(void_ptr_t p);
void		mem_free_pptr(void_pptr_t pp);

#define MEM_ALLOCZ(type)	(type*)mem_allocz(sizeof(type))
#define MEM_FREE_PTR(p)		mem_free_ptr((void_ptr_t)p)
#define MEM_FREE_PPTR(pp)	mem_free_pptr((void_pptr_t)pp)

#define INVALID_FD          -1
#define INVALID_ID          -1

#define MAX_IP_LEN                          64
#define MAX_PORT_LEN                        12
#define MAX_PATH_LEN                        256
#define BUF_SIZE_2048                       2048
#define FLAG_IO_READ_WRITE                  0x03
#define MAX_UID_LEN                         64
#define MAX_WAKE_TOKEN_LEN                  128
#define MAX_CATEGORY_NAME_LEN               64

#define CONF_DEVICE_APP_PORT                7777
#define CONF_CLOUD_APP_PORT                 8000
#define CONF_CLOUD_DEV_PORT                 8888
#define CONF_DEV_WAKEUP_PORT                7877
#define CONF_WDOG_TIME_PERIOD               4                               // unit: sec
#define CONF_DEV_CLIENT_ONLINE_TIMOUT       (16/CONF_WDOG_TIME_PERIOD)      // unit: CONF_WDOG_TIME_PERIOD sec
#define CONF_DEV_CLIENT_STANDBY_TIMOUT      (16/CONF_WDOG_TIME_PERIOD)      // unit: CONF_WDOG_TIME_PERIOD sec
#define CONF_APP_CLIENT_TIMEOUT             (3600/CONF_WDOG_TIME_PERIOD)    // unit: CONF_WDOG_TIME_PERIOD sec
#define CONF_APP_CONNECT_TIMEOUT            4


#define MSG_COMMON          \
    uint32_t    what;       \
    handle_t    obj;        \
    int32_t     para1;      \
    int32_t     para2;

typedef struct {
    MSG_COMMON
} msg_t, *msg_ptr_t, **msg_pptr_t;

#define RESULT_COMMON       \
    int32_t     ret;        \
    handle_t    obj;        \
    int32_t     para1;      \
    int32_t     para2;

typedef struct {
    RESULT_COMMON
} result_t, *result_ptr_t, **result_pptr_t;

typedef void (*msg_notify_cb_t)(handle_t h, msg_ptr_t p_msg, result_ptr_t p_result);
typedef void (*msg_proc_cb_t)(handle_t h, msg_ptr_t p_msg, result_ptr_t p_result);

enum {
    STATE_INVALID,
    STATE_CREATED,
    STATE_INITIALIZED,
    STATE_STARTED,
    STATE_STOPPED,
    STATE_ERROR_INIT,
    STATE_ERROR_START,
};

/* msg */
// |--8--|--8--|-----16-----|
//
// |-reserved-|-catgory-|-detail-|

#define MSG_RESERVED_OFFSET             24
#define MSG_CAT_OFFSET                  16
#define MSG_VALUE_OFFSET                0

#define MSG_RESERVED_MASK               (0xFF<<MSG_RESERVED_OFFSET)
#define MSG_CAT_MASK                    (0xFF<<MSG_CAT_OFFSET)
#define MSG_VALUE_MASK                  (0xFFFF<<MSG_VALUE_OFFSET)

#define MSG_CAT(what)                   ( ((what) & MSG_CAT_MASK) >> MSG_CAT_OFFSET)
#define MSG_VALUE(what)                 ( ((what) & MSG_VALUE_MASK) >> MSG_VALUE_OFFSET)
#define MSG_WHAT(cat, value)            ((cat)<<MSG_CAT_OFFSET | (value)<<MSG_VALUE_OFFSET)
#define MSG_DELIMITER                   '\n'

enum {
    MSG_CAT_RESERVED                = 0x00,
    MSG_CAT_CLOUD                   = 0x01,
    MSG_CAT_DEVICE                  = 0x02,
    MSG_CAT_APP                     = 0x03,
    MSG_CAT_WEB                     = 0x04,

    MSG_CAT_DBG                     = 0xFE,
    MSG_CAT_SELF                    = 0xFF,
};

enum {
    RET_OK                          = 0,
    //

    RET_ERROR                       = -1,
    RET_ERROR_BUSY                  = -2,
    RET_ERROR_INVLAID_STATE         = -3,
    RET_ERROR_READ                  = -4,
    RET_ERROR_WRITE                 = -5,
    RET_ERROR_EOF                   = -6,
    RET_ERROR_IN_BUF_FULL           = -7,
    RET_ERROR_OUT_BUF_FULL          = -8,
    RET_ERROR_CLIENT                = -9,
    RET_ERROR_INVLAID_MSG           = -10,
    RET_ERROR_NOT_FOUND             = -11,
    RET_ERROR_NO_HANDLER            = -12,
    RET_ERROR_MEM_NOT_ENOUGH        = -13,
};

typedef struct ctx ctx_t, *ctx_ptr_t, **ctx_pptr_t;

#define CTX_COMMON                                                          \
    int32_t (*start)(ctx_ptr_t p_ctx);                                      \
    int32_t (*stop)(ctx_ptr_t p_ctx);                                       \
    void    (*destroy)(ctx_pptr_t pp_ctx);

#define CTX_PPTR_DESTROY(ctx_pptr)                                          \
    if (ctx_pptr) (*(ctx_pptr))->destroy(ctx_pptr);


struct ctx {
    CTX_COMMON
};


#define KEY_WHAT                        "what"
#define KEY_RET                         "ret"
#define KEY_ERROR                       "error"
#define KEY_STAT                        "stat"
#define KEY_QUANTITY                    "quantity"
#define KEY_STATE                       "state"
#define KEY_IP                          "ip"
#define KEY_PORT                        "port"
#define KEY_UID                         "uid"
#define KEY_MODE                        "mode"
#define KEY_TOKEN                       "token"
#define KEY_CATEGORY                    "category"

#define KEY_PIR_STAT                    "pir_stat"
#define KEY_BATTERY_STAT                "battery_stat"
#define KEY_BATTERY_QUANTITY            "battery_quantity"
#define KEY_CHARGE_STAT                 "charge_stat"
#define KEY_DC_STAT                     "dc_stat"
#define KEY_CLOUD                       "cloud"
#define KEY_STREAM_ID                   "stream_id"


enum {
    MSG_RESERVED_HOME               = MSG_WHAT(MSG_CAT_RESERVED, 0),
    MSG_RESERVED_SYNC               = MSG_RESERVED_HOME,
    MSG_RESERVED_PLAY_AUDIO,
    MSG_RESERVED_SHUTDOWN,
    MSG_RESERVED_END,

    MSG_DEVICE_HOME                 = MSG_WHAT(MSG_CAT_DEVICE, 0),
    MSG_DEVICE_LOGIN,
    MSG_DEVICE_LOGOUT,
    MSG_DEVICE_BATTERY,
    MSG_DEVICE_CHARGE,
    MSG_DEVICE_DC,
    MSG_DEVICE_PIR,
    MSG_DEVICE_ALARM,
    MSG_DEVICE_SHUTDOWN,
    MSG_DEVICE_STANDBY_TCP,
    MSG_DEVICE_STANDBY_UDP,
    MSG_DEVICE_WAKEUP_TCP,
    MSG_DEVICE_WAKEUP_UDP,
    MSG_DEVICE_STATUS_UPDATE,
    MSG_DEVICE_AGENT_CONNECTED,
    MSG_DEVICE_SYNC,
    MSG_DEVICE_AGENT_CANNOT_CONNECT,
    MSG_FAST_STREAMING              = MSG_WHAT(MSG_CAT_DEVICE, 0x12),
    MSG_DEVICE_END,

    MSG_APP_HOME                    = MSG_WHAT(MSG_CAT_APP, 0),
    MSG_APP_LOGIN,
    MSG_APP_LOGOUT,
    MSG_APP_GET_DEVICE_LIST,
    MSG_APP_GET_DEVICE_DETAIL,
    MSG_APP_GET_EVENT_LIST,
    MSG_APP_GET_EVENT_DETAIL,
    MSG_APP_END,

    MSG_CLOUD_HOME                  = MSG_WHAT(MSG_CAT_CLOUD, 0),
    MSG_CLOUD_END,

    MSG_DBG_HOME                    = MSG_WHAT(MSG_CAT_DBG, 0),
    MSG_DBG_CANNOT_CONNECT_CLOUD,
    MSG_DBG_START_RTSP_INJECTOR,
    MSG_DBG_END,
};

enum {
    DEVICE_STATE_OFFLINE,
    DEVICE_STATE_ONLINE,
    DEVICE_STATE_STANDBY,
};

enum {
    DEVICE_MODE_INVALID,
    DEVICE_MODE_RECORDING,
    DEVICE_MODE_STREAMING,
    DEVICE_MODE_STANDBY_TCP,
    DEVICE_MODE_STANDBY_UDP,
};

#endif
