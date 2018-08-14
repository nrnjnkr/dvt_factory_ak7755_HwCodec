/*******************************************************************************
 * am_wifi_manager.h
 *
 * History:
 *   Jul 4, 2016 - [longli] created file
 *
 * Copyright (C) 2016, Ambarella Co, Ltd.
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
 ******************************************************************************/
#ifndef ORYX_NETWORK_WPA_INCLUDE_AM_WIFI_MANAGER_H_
#define ORYX_NETWORK_WPA_INCLUDE_AM_WIFI_MANAGER_H_

#include "am_network_structs.h"

enum AM_WPA_MODE
{
  WPA_STATION_MODE = 0,
  WPA_IBSS_MODE,
  WPA_AP_MODE
};

enum AM_AP_SCAN_MODE
{
  AM_AP_SCAN_MODE_0 = 0,
  AM_AP_SCAN_MODE_1,
  AM_AP_SCAN_MODE_2
};

enum AM_SCAN_SSID_MODE
{
  AM_SCAN_SSID_MODE_0 = 0,
  AM_SCAN_SSID_MODE_1
};

enum AM_NET_STATUS
{
  AM_NOT_INIT = 0,

  AM_WPA_STARTING,
  AM_WPA_START_DONE,
  AM_LINK_CONNECTING,
  AM_LINK_CONNECTED,
  AM_IP_GETTING,
  AM_CONNECTED,
  AM_DISCONNECTED,
  AM_WPA_STOPPING,
};

class AMWPACtrl
{
  public:
    static AMWPACtrl* create(const std::string &sta_ifname);
    bool sta_link_connect(const std::string &ssid,
                          const std::string &psk,
                          const bool hidden_ssid);
    bool sta_link_disconnect();
    bool sta_link_reconnect();
    bool ap_setup(const std::string &ssid,
                  const std::string &psk,
                  const int32_t channel);
    bool remove_all_network();
    bool scan_results(char *reply, size_t *reply_len);
    bool remove_current_ap_instance();
    bool remove_current_sta_instance();
    bool link_status(char *reply, size_t *reply_len);

    virtual ~AMWPACtrl();

  private:
    bool init_connection(const std::string &ifname);
    int32_t add_network();

    bool remove_network(const int32_t net_id);
    bool set_ssid(const int32_t net_id, const std::string &ssid);
    bool set_psk(const int32_t net_id, const std::string &psk);
    bool set_channel(const int32_t net_id,
                     const int32_t channel);
    bool set_key_mgmt(const int32_t net_id, const std::string &key_mgmt);
    bool set_wpa_mode(const int32_t net_id, const AM_WPA_MODE mode);
    bool set_ap_scan(const AM_AP_SCAN_MODE scan_mode);
    bool set_scan_ssid(const int32_t net_id,
                       const AM_SCAN_SSID_MODE mode);
    bool enable_network(const int32_t net_id);
    bool disable_network(const int32_t net_id);
    bool disconnect();

    bool list_network(char *reply, size_t *reply_len);
    bool select_network(const int32_t net_id);
    bool send_command(const char *cmd, char *reply,
                      size_t *reply_len);
    int32_t ctrl_recv(char *reply, size_t *reply_len);
    int32_t wait_for_event(char *buf, size_t buflen);
    bool ignore_event(const char *buf);
    int32_t process_event(const char *buf);

    bool is_wpa_alive(std::string &pid_str);
    void close_sockets();
    AMWPACtrl();

  private:
    std::string m_wpa_ifname;
    int32_t m_sta_net_id;
    int32_t m_ap_net_id;
    int32_t exit_sockets[2];
    struct wpa_ctrl *ctrl_conn;
    struct wpa_ctrl *monitor_conn;
};

class AMWifiManager
{
  public:
    static AMWifiManager *create();
    bool set_mode_cfg(const AM_WIFI_MODE mode,
                      const std::string &wpa_ifname,
                      const std::string &wpa_config,
                      const std::string &con_ifname,
                      const std::string &con_config);
    bool set_mode(const AM_WIFI_MODE mode);

    bool connect_to_ap(const std::string &ssid,
                       const std::string &psk,
                       const bool hidden_ssid);
    bool disconnect_to_ap();
    bool setup_ap(const std::string &ssid,
                  const std::string &psk,
                  const int32_t channel);
    bool remove_ap();
    bool remove_all_network();
    bool list_ap(char *reply, size_t *reply_len);
    bool wifi_status(char *reply, size_t *reply_len);
    bool wifi_status(const char *ifname, char *reply, size_t *reply_len);
    void stop_wpa_supplicant();

    virtual ~AMWifiManager();

  private:
    bool start_wpa_supplicant();
    bool is_wpa_supplicant_run();
    void deinit();
    AMWifiManager();

  private:
    AM_WIFI_MODE m_mode;
    AM_WIFI_MODE m_pre_mode;
    /* AM_NET_STATUS m_state; */
    AMWPACtrl *m_wpa_ctrl;
    std::string m_wpa_ifname;
    std::string m_wpa_config;
    /* for concurrent AP mode */
    AMWPACtrl *m_con_ctrl;
    std::string m_con_ifname;
    std::string m_con_config;
};


#endif /* ORYX_NETWORK_WPA_INCLUDE_AM_WIFI_MANAGER_H_ */
