/*
 * cloud_comm.c
 *
 * History:
 *       2016/06/13 - [CZ LIN] created file
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
 * POSSIBILITY OF SUCH DAMAGE.a
 *
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "device.h"
#include "defs.h"
#include "bpi_typedefs.h"
#include "bpi_utils.h"
#include "bpi_wlan.h"
#include "bpi_app_config.h"
#include "cloud_comm.h"

#define SYSTEM_ALARM_AUDIO_PATH     "/usr/local/bin/prompt.wav"
#define DEVICE_ELEKTRA_II            "elektraII"
#define DEVICE_ELEKTRA_S             "elektraS"
#define DEVICE_ELEKTRA_V             "elektraV"
#define DEVICE_DAREDEVIL             "daredevil"
#define DEVICE_BTFL		     "Ooma BTFL"
#define DEVICE_UNKNOWN               "Unknown"

#ifdef __cplusplus
extern "C" {
#endif
static int  cloud_port = CONF_CLOUD_DEV_PORT;
static int local_port = 8282;
static char fast_streaming_client_ip[MAX_IP_LEN] = {0};
static volatile int s_is_cloud_rtsp = 0;
static volatile int cloud_status = ECloud_Disconnected;
static volatile int dbg_cannot_connect_cloud = 0;
static volatile int s_remote_ctrl_standby = 0;
pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

static void agent_msg_proc_alarm()
{
    const char *path = SYSTEM_ALARM_AUDIO_PATH;
    LOG_DEBUG("play audio: %s", path);
    char cmd[100]   = {0} ;
    sprintf(cmd, "/usr/bin/aplay %s", path);
    system(cmd);
}

static void agent_msg_proc_start_rtsp_injector(handle_t h_agent, int stream_id)
{
    char cmd[256];
    sprintf(cmd, "proxy_injector rtsp://127.0.0.1:%d/stream1 rtsp://%s/%s_%d.sdp &", local_port, get_server_ip(NULL), get_device_id(), stream_id);
    system(cmd);
    s_is_cloud_rtsp = 1;
}

void agent_msg_notify_cb(handle_t h_agent, msg_ptr_t p_msg, result_ptr_t p_result)
{
    switch (p_msg->what) {
        case MSG_DEVICE_AGENT_CONNECTED: {
            LOG_DEBUG("agent_msg_notify_cb: ECloud_Connected\n");
            fflush(stdout);
            cloud_status = ECloud_Connected;
        }
        break;
        case MSG_DEVICE_AGENT_CANNOT_CONNECT: {
            LOG_DEBUG("agent_msg_notify_cb: ECloud_Disconnected\n");
            fflush(stdout);
            cloud_status = ECloud_Disconnected;
        }
        break;
        case MSG_DEVICE_ALARM: {
            agent_msg_proc_alarm();
        }
        break;
        case MSG_RESERVED_SHUTDOWN:
        case MSG_DEVICE_STANDBY_UDP:
        case MSG_DEVICE_STANDBY_TCP: {
            s_remote_ctrl_standby = 1;
        }
        break;
        case MSG_DBG_CANNOT_CONNECT_CLOUD: {
            dbg_cannot_connect_cloud = 1;
        }
        break;
        case MSG_RESERVED_PLAY_AUDIO: {
            agent_msg_proc_alarm();
        }
        break;
        case MSG_FAST_STREAMING: {
            set_fast_streaming_client((char*)p_result->obj);
        } break;
        case MSG_DBG_START_RTSP_INJECTOR: {
            agent_msg_proc_start_rtsp_injector(h_agent, p_msg->para1);
        }
        break;
        default:
            LOG_ERROR("no handler, what:%d", p_msg->what);
            break;
    }
}
void set_device_mode( handle_t h_agent, int mode){
    device_set_device_mode(h_agent, mode);
}

handle_t start_device_agent(int mode)
{
    param_device_t param;
    int ret = 0;
    handle_t h_agent = NULL;
    ret = param_device_parse_server(get_server_ip(NULL), param.srv_ip, NULL);
    if (ret == 0) {
        strcpy(param.dev_uid, get_device_id());
        param.dev_mode = mode;
        param.srv_dev_port = cloud_port;
        memset(param.dev_category,0,sizeof(param.dev_category));
#if defined(ELEKTRA_S)
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_ELEKTRA_S);
#elif defined(ELEKTRA_II)
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_ELEKTRA_II);
#elif defined(ELEKTRA_V)
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_ELEKTRA_V);
#elif defined(DAREDEVIL)
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_DAREDEVIL);
#elif defined(BTFL)
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_BTFL);
#else
        snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_UNKNOWN);
#endif

        h_agent = device_create(&param);
        if (h_agent) {
            ret = device_init(h_agent);
            if (ret < 0) {
                device_deinit(h_agent);
                LOG_ERROR("init device_agent failure\n");
            } else {
                device_set_msg_notify_cb(h_agent, h_agent, agent_msg_notify_cb);
                device_start(h_agent);
                LOG_DEBUG("device_agent start OK\n");
            }
        } else {
            LOG_ERROR("create device_agent failure\n");
        }
    } else {
        LOG_ERROR("param_device_parse_server failure\n");
    }
    return h_agent;
}

/*int notify_server_standby_udp(keep_alive_param_t* param)
{
    int result = 0;
    do {
        int fd_client = connect_server(param->server_addr, param->server_port, Net_UDP);
        if (fd_client < 0) {
            LOG_ERROR("connect to server fail\n");
            result = -1;
            break;
        }

        if (device_standby_udp(fd_client, get_device_id(), param->dev_category, param->wake_data, param->wake_data_len) != RET_OK) {
            LOG_ERROR("notify cloud failure");
        }
        if (fd_client > 0) {
            close(fd_client);
        }
    } while(0);
    return result;
}*/

int notify_server_standby_tcp(keep_alive_param_t* param, int tcp_fd)
{
    int result = 0;
    do {
        // notify cloud that device enter standby
        if (device_standby_tcp(tcp_fd, get_device_id(), param->dev_category, param->wake_data, param->wake_data_len) != RET_OK) {
            LOG_ERROR("notify cloud failure");
        }
    } while(0);
    return result ;
}

int check_standby_flag(){
    return s_remote_ctrl_standby;
}

void clear_cloud_comm_flags(){
     s_remote_ctrl_standby = 0;
}

char* get_fast_streaming_client()
{
    int wait_count = 0;
    int MAX_WAIT_COUNT = 200;
    int ret = -1;
    while(wait_count < MAX_WAIT_COUNT){
        pthread_mutex_lock(&s_mutex);
        int cmp_result = strcmp(fast_streaming_client_ip, "\0");
        pthread_mutex_unlock(&s_mutex);
        if(cmp_result == 0){
            wait_count += 1;
            usleep(10 * 1000);
        }else{
            LOG_PRINT("get_ip, wait_count = %d, get_current_time = %u\n",wait_count,get_current_time());
            ret = 0;
            break;
        }
    }
    if (wait_count >= MAX_WAIT_COUNT){
        LOG_ERROR("Time out, no streaming destination\n");
    }
    return (ret == -1)? NULL:fast_streaming_client_ip;
}

void set_fast_streaming_client(char *dest_ip)
{
    if(strlen(dest_ip) >= sizeof(fast_streaming_client_ip)){
        LOG_ERROR("set streaming ip incorrectly\n");
        return;
    }
    pthread_mutex_lock(&s_mutex);
    strcpy(fast_streaming_client_ip,  dest_ip);
    pthread_mutex_unlock(&s_mutex);
}


void wlan_resume(){
    wifi_power_normal();
    int wake_reason = INVALID_STATE;
    if (get_fd_to_server() < 0){
        return;
    }
    if(0 != resume_wifi(&wake_reason, 1)){
        LOG_ERROR("wifi resume:resume wifi failed!\n");
    }
    close_fd_to_server();
}

int hibernate_wlan_resume(){
    int wake_reason = INVALID_STATE;
    wifi_power_normal();
    if(0 != resume_wifi(&wake_reason, 0)){
        LOG_ERROR("wifi resume:resume wifi failed!\n");
    }
    return wake_reason;
}

int wlan_keepalive(int keepalive_interval, int dtim_interval, const char* wakeup_token, char *server_ip)
{
    keep_alive_param_t param;
    memset(&param,0,sizeof param);
    param.server_addr = server_ip;
    param.server_port = CONF_CLOUD_DEV_PORT;
    param.keepalive_interval =keepalive_interval;//seconds
    param.dtim_interval = dtim_interval;//mseconds
    param.timeout = 10;//seconds
    param.verbose = 0;
    param.wake_data_len = snprintf((char*)param.wake_data,sizeof(param.wake_data),"%s", wakeup_token);
#if defined(ELEKTRA_S)
    snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_ELEKTRA_S);
#elif defined(DAREDEVIL)
    snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_DAREDEVIL);
#elif defined(BTFL)
    snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_BTFL);
#else
    snprintf(param.dev_category,sizeof(param.dev_category),"%s",DEVICE_UNKNOWN);
#endif
    int ret = -1;
    do {
        int fd_client = -1;
        LOG_PRINT("wifi standby TCP starts\n");
        close_fd_to_server();
        connect_server(server_ip, CONF_CLOUD_DEV_PORT, Net_TCP);
        fd_client = get_fd_to_server();
        if (fd_client < 0){
            LOG_ERROR("socket to server not valid. \n");
            ret = ECloud_Disconnected;
            break;
        }
        ret = notify_server_standby_tcp(&param, fd_client);
        if (ret < 0) {
            break;
        }
        ret = tcp_keep_alive(&param, fd_client);
    } while(0);
    wifi_power_save();
    return  ret;
}
#ifdef __cplusplus
};
#endif
