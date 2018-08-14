/*
 * bpi_wlan.h
 *
 * History:
 *       2016/01/18 - [CZ Lin] created file
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
#ifndef ELEKTRA_APP_WLAN_H_
#define ELEKTRA_APP_WLAN_H_


typedef enum {
    EWlan_Connected = 0x11,
    EWlan_Disconnected = 0x22,
} EWlan;

typedef enum {
    ECloud_Connected = 0,
    ECloud_Disconnected,
} ECloud;

typedef enum error_code {
    ECode_Success = 0,
    ECode_Error,
    ECode_WlanUp_Fail,
    ECode_WlanConfig_Fail,
    ECode_RecordingStart_Fail,
} ERROR_CODE;

typedef enum protocol {
    Net_UDP = 0,
    Net_TCP,
} PROTOCOL;

typedef enum {
    INVALID_STATE = -1,
    WIFI_WAKE_NORMAL = 0,
    AP_LOSS_BEACON,
    AP_DISASSOCIATION_OR_AUTHENTICATION,
    TCP_KEEPALIVE_TIMEOUT,

    WIFI_WAKE_ERROR = INVALID_STATE
} WIFI_WAKE_STATE;

typedef struct _cooee_wlan_conf_t{
    char ap_ssid[128];
    char ap_password[128];
    char ap_wpa[128];
    char cloud_server_ip[128];
}cooee_wlan_conf_t;

// keep_alive related
#define MAX_WAKEUP_DATA_LEN  128
typedef struct{
    char* server_addr;
    int server_port;
    int keepalive_interval/*seconds*/;
    int dtim_interval/*mseconds*/;
    int timeout/*seconds*/;
    int verbose;
    unsigned char wake_data[MAX_WAKEUP_DATA_LEN];
    int wake_data_len;
    char dev_category[64];
}keep_alive_param_t;

#ifdef __cplusplus
extern "C" {
#endif

int connect_ap(const char *ssid, const char *passwd, const char *wpa);
int connect_ap_async(const char *ssid, const char *passwd, const char *wpa);

int ap_connect_status();

int wlan_conf_changed();
void clear_wlan_flag();

int connect_server(const char* server_addr, int server_port, int protocol);
int get_fd_to_server();
void close_fd_to_server();

int save_wlan_config(const char *file_path, cooee_wlan_conf_t *sWlanConfig);
int config_elektra_boot(cooee_wlan_conf_t *wlan_write_back);
int udp_keep_alive(keep_alive_param_t  *param);
void wifi_power_save();
void wifi_host_sleep();
void wifi_power_normal();
int tcp_keep_alive(keep_alive_param_t *param, int fd);
char * get_server_ip(const char * file_path);

int init_wifi_param(int is_poll, char const *iface,int iface_len);
int resume_wifi(int* wake_reason, int tcp_fix);
#ifdef __cplusplus
};
#endif

#endif //ELEKTRA_APP_WLAN_H_

