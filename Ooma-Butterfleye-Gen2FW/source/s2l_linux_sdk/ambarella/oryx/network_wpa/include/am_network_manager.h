/*******************************************************************************
 * am_network_manager.h
 *
 * History:
 *   Jul 12, 2016 - [longli] created file
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
#ifndef ORYX_NETWORK_WPA_INCLUDE_AM_NETWORK_MANAGER_H_
#define ORYX_NETWORK_WPA_INCLUDE_AM_NETWORK_MANAGER_H_

#include "am_network_manager_if.h"
#include "am_ifctrl.h"
#include "am_wifi_manager.h"

struct AMNetworkStat
{
    bool wifi_inited = false;
    bool eth_cfg_changed = true;
    bool mode_changed = true;
    bool wpa_need_restart = true;
    bool sta_cfg_changed = true;
    bool ap_cfg_changed = true;
    bool eth_run = false;
    bool sta_run = false;
    bool ap_run = false;
    bool eth_setup = true;
    bool wifi_setup = true;
    std::string eth_curr_ifname;
    std::string sta_curr_ifname;
    std::string ap_curr_ifname;
};

class AMNetworkManger: public AMINetworkManger
{
    friend class AMINetworkManger;
  public:
    bool set_wifi_enable(const bool wifi_enable);
    bool set_sta_enable(const bool sta_enable);
    bool set_ap_enable(const bool ap_enable);
    bool set_mode_cfg(const AMModeCfg &mode_cfg);
    bool set_eth_cfg(const AMEthCfg &eth_cfg);
    bool set_sta_cfg(const AMStaCfg &sta_cfg);
    bool set_ap_cfg(const AMAPCfg &ap_cfg);
    bool network_setup();
    bool save_network_config();

    bool list_ap();
    bool wifi_status(const char *ifname = nullptr);
    bool eth_status(const char *ifname = nullptr);
    bool all_status();

    void get_eth_cfg(AMEthCfg &eth_cfg);
    void get_wifi_mode_cfg(AMModeCfg &mode_cfg);
    void get_sta_cfg(AMStaCfg &sta_cfg);
    void get_ap_cfg(AMAPCfg &ap_cfg);

    virtual ~AMNetworkManger();

  protected:
    static AMNetworkManger *create();

  private:
    AMNetworkManger();
    bool init();
    bool wifi_wpa_setup();
    bool wifi_sta_setup();
    bool wifi_sta_disconnect();
    bool wifi_ap_setup();
    bool wifi_ap_remove();
    bool wifi_handle_pre_connections();
    bool rough_remove_pre_connections();
    void wifi_setup();
    bool ethernet_setup();
    bool ethernet_disconnect();
    void eth_setup();
    bool up_interface();
    bool generate_dhcp_cfg(const std::string &ifname,
                           const in_addr &in_ip,
                           const in_addr &in_mask);
    bool dhcp_get_ip_addr(const std::string &ifname);
    bool ip_to_string(const in_addr &in_ip,
                      char *ip_str,
                      const size_t size);
    void stop_dnsmasq();
    bool reload_driver();

    bool load_config();
    bool save_config();

  private:
    AMNetworkStat m_stat;
    AMIfCtrlPtr m_ifc_ptr;
    AMWifiManager *m_wifi_manager;
    AMEthCfg m_eth;
    AMWifiCfg m_wifi_cfg;
};

#endif /* ORYX_NETWORK_WPA_INCLUDE_AM_NETWORK_MANAGER_H_ */
