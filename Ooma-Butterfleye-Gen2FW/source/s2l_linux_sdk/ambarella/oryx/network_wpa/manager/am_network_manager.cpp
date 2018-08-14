/*******************************************************************************
 * am_network_manager.cpp
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
#include "am_base_include.h"
#include "am_log.h"
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "am_configure.h"
#include "am_dhclient.h"
#include "am_network_manager_if.h"
#include "am_network_manager.h"

#define IFC_LEN 32
#define PATH_LEN 128
#define SSID_LEN 64
#define PASSWD_LEN 64
#define CONFIG_PATH "/etc/oryx/network/network_cfg.acs"
#define DHCP_CFG_FILE "/etc/dnsmasq.d/oryx_dhcp.conf"
#define WPA_CFG "/etc/wpa_supplicant/wpa_supplicant.conf"

using namespace std;

AMNetworkManger::AMNetworkManger():
  m_ifc_ptr(nullptr),
  m_wifi_manager(nullptr)
{
}

ORYX_API AMINetworkManger *AMINetworkManger::create()
{
  return ((AMINetworkManger *)AMNetworkManger::create());
}

AMNetworkManger *AMNetworkManger::create()
{
  AMNetworkManger *ins = nullptr;

  ins = new AMNetworkManger();
  if (ins && !ins->init()) {
    ERROR("init AMNetworkManger error.");
    delete ins;
    ins = nullptr;
  }

  return ins;
}

bool AMNetworkManger::init()
{
  bool ret = false;

  do {
    m_wifi_manager = AMWifiManager::create();
    if (!m_wifi_manager) {
      ERROR("create AMWifiManager error");
      break;
    }

    m_ifc_ptr = AMIfCtrl::get_instance();
    if (!m_ifc_ptr) {
      ERROR("AMIfCtrl::get_instance error");
      break;
    }

    if (!load_config()) {
      ERROR("Failed to load config.");
      break;
    }

    ret = true;
  } while (0);

  if (!ret) {
    if (m_wifi_manager) {
      delete m_wifi_manager;
      m_wifi_manager = nullptr;
    }
  }

  return ret;
}

bool AMNetworkManger::set_wifi_enable(const bool wifi_enable)
{
  bool ret = true;

  m_wifi_cfg.enable = wifi_enable;

  return ret;
}

bool AMNetworkManger::set_sta_enable(const bool sta_enable)
{
  bool ret = true;

  if (m_wifi_cfg.sta_cfg.enable != sta_enable) {
    m_wifi_cfg.sta_cfg.enable = sta_enable;
    m_stat.sta_cfg_changed = true;
  }

  return ret;
}

bool AMNetworkManger::set_ap_enable(const bool ap_enable)
{
  bool ret = true;

  if (m_wifi_cfg.ap_cfg.enable != ap_enable) {
    m_wifi_cfg.ap_cfg.enable = ap_enable;
    m_stat.ap_cfg_changed = true;
  }

  return ret;
}

bool AMNetworkManger::set_mode_cfg(const AMModeCfg &mode_cfg)
{
  bool ret = true;
  AM_WIFI_MODE pre_mode = m_wifi_cfg.mode_cfg.mode;

  if ((!mode_cfg.ifname.empty())
      && mode_cfg.ifname != m_wifi_cfg.mode_cfg.ifname) {
    m_wifi_cfg.mode_cfg.ifname = mode_cfg.ifname;
    m_stat.wpa_need_restart = true;
  }

  if ((!mode_cfg.cfg.empty())
      && mode_cfg.cfg != m_wifi_cfg.mode_cfg.cfg) {
    m_wifi_cfg.mode_cfg.cfg = mode_cfg.cfg;
    m_stat.wpa_need_restart = true;
  }

  if ((!mode_cfg.con_if.empty())
      && mode_cfg.con_if != m_wifi_cfg.mode_cfg.con_if) {
    m_wifi_cfg.mode_cfg.cfg = mode_cfg.cfg;
    if (mode_cfg.mode == AM_CONCURRENT_MODE) {
      m_stat.wpa_need_restart = true;
    }
  }

  if (mode_cfg.mode != pre_mode) {
    if (m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE ||
        mode_cfg.mode == AM_CONCURRENT_MODE) {
      m_stat.wpa_need_restart = true;
    }
    m_wifi_cfg.mode_cfg.mode = mode_cfg.mode;
    m_stat.mode_changed = true;
  }

  return ret;
}

bool AMNetworkManger::set_eth_cfg(const AMEthCfg &eth_cfg)
{
  bool ret = true;

  if (m_eth.enable != eth_cfg.enable ||
      m_eth.ifname != eth_cfg.ifname ||
      m_eth.dhcp_enable != eth_cfg.dhcp_enable ||
      m_eth.ip != eth_cfg.ip ||
      m_eth.mask != eth_cfg.mask ||
      m_eth.gw != eth_cfg.gw) {
    DEBUG("Eth cfg changed.");
    m_stat.eth_cfg_changed = true;
    m_eth = eth_cfg;
  }

  return ret;
}

bool AMNetworkManger::set_sta_cfg(const AMStaCfg &sta_cfg)
{
  bool ret = true;

  if (m_wifi_cfg.sta_cfg.enable != sta_cfg.enable ||
      m_wifi_cfg.sta_cfg.dhcp_enable != sta_cfg.dhcp_enable ||
      m_wifi_cfg.sta_cfg.ssid != sta_cfg.ssid ||
      m_wifi_cfg.sta_cfg.passwd != sta_cfg.passwd ||
      m_wifi_cfg.sta_cfg.ip != sta_cfg.ip ||
      m_wifi_cfg.sta_cfg.mask != sta_cfg.mask ||
      m_wifi_cfg.sta_cfg.gw != sta_cfg.gw ||
      m_wifi_cfg.sta_cfg.hidden_ssid != sta_cfg.hidden_ssid) {
    if (!m_wifi_cfg.sta_cfg.hidden_ssid) DEBUG("hidden_ssid is false");
    DEBUG("Station cfg changed.");
    m_stat.sta_cfg_changed = true;
    m_wifi_cfg.sta_cfg = sta_cfg;
  }

  return ret;
}

bool AMNetworkManger::set_ap_cfg(const AMAPCfg &ap_cfg)
{
  bool ret = true;

  if (m_wifi_cfg.ap_cfg.enable != ap_cfg.enable ||
      m_wifi_cfg.ap_cfg.ssid != ap_cfg.ssid ||
      m_wifi_cfg.ap_cfg.passwd != ap_cfg.passwd ||
      m_wifi_cfg.ap_cfg.channel != ap_cfg.channel ||
      m_wifi_cfg.ap_cfg.ip != ap_cfg.ip ||
      m_wifi_cfg.ap_cfg.mask != ap_cfg.mask ||
      m_wifi_cfg.ap_cfg.gw != ap_cfg.gw ||
      m_wifi_cfg.ap_cfg.dhcp_range != ap_cfg.dhcp_range ||
      m_wifi_cfg.ap_cfg.lease != ap_cfg.lease) {
    DEBUG("AP cfg changed.");
    m_stat.ap_cfg_changed = true;
    m_wifi_cfg.ap_cfg = ap_cfg;
  }

  return ret;
}

bool AMNetworkManger::ip_to_string(const in_addr &in_ip,
                                   char *ip_str,
                                   const size_t size)
{
  bool ret = true;
  int32_t num = 0;
  int32_t shift = 0;
  in_addr_t ip_tmp;

  do {
    if (!ip_str || size < 16) {
      ERROR("ip_str is null or buf size is not enough\n");
      ret = false;
      break;
    }

    for (int32_t i = 0; i < 32; i += 8) {
      ip_tmp = (in_ip.s_addr >> i) & 0x000000ff;
      num = snprintf(ip_str + shift, (size - shift), "%d.", ip_tmp);
      shift += num;
    }
    ip_str[shift - 1] = '\0';
  } while (0);

  return ret;
}

bool AMNetworkManger::wifi_wpa_setup()
{
  bool ret = true;

  do {
    if (!m_stat.wpa_need_restart) {
      m_wifi_manager->set_mode(m_wifi_cfg.mode_cfg.mode);
    } else {
      DEBUG("call m_wifi_manager->set_mode_cfg");
      if (!m_wifi_manager->set_mode_cfg(m_wifi_cfg.mode_cfg.mode,
                                        m_wifi_cfg.mode_cfg.ifname,
                                        m_wifi_cfg.mode_cfg.cfg,
                                        m_wifi_cfg.mode_cfg.con_if,
                                        m_wifi_cfg.mode_cfg.cfg)) {
        ret = false;
        break;
      }

      if (m_wifi_cfg.mode_cfg.mode == AM_STATION_MODE) {
        m_stat.sta_curr_ifname = m_wifi_cfg.mode_cfg.ifname;
      } else if (m_wifi_cfg.mode_cfg.mode == AM_AP_MODE){
        m_stat.ap_curr_ifname = m_wifi_cfg.mode_cfg.ifname;
      } else if (m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE) {
        m_stat.sta_curr_ifname = m_wifi_cfg.mode_cfg.ifname;
        m_stat.ap_curr_ifname = m_wifi_cfg.mode_cfg.con_if;
      }
      m_stat.wifi_inited = true;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::up_interface()
{
  bool ret = true;

  if (m_wifi_cfg.mode_cfg.mode == AM_STATION_MODE ||
      m_wifi_cfg.mode_cfg.mode == AM_AP_MODE) {
    if (m_wifi_cfg.mode_cfg.ifname.empty() ||
        m_ifc_ptr->enable(m_wifi_cfg.mode_cfg.ifname.c_str())) {
      ERROR("failed to up interface %s: %s",
            m_wifi_cfg.mode_cfg.ifname.c_str(), strerror(errno));
      ret = false;
    }
  } else if (m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE ||
      m_ifc_ptr->enable(m_wifi_cfg.mode_cfg.ifname.c_str()) ||
      m_ifc_ptr->enable(m_wifi_cfg.mode_cfg.con_if.c_str())) {
    ERROR("failed to up interface %s or %s: %s",
          m_wifi_cfg.mode_cfg.ifname.c_str(),
          m_wifi_cfg.mode_cfg.con_if.c_str(),
          strerror(errno));
    ret = false;
  }

  return ret;
}

bool AMNetworkManger::dhcp_get_ip_addr(const string &ifname)
{
  bool ret = true;
  AMDhClient *dhc = NULL;

  do {
    if (ifname.empty()) {
      ERROR("ifname is NULL");
      ret = false;
      break;
    }

    dhc = AMDhClient::create(ifname);
    if (!dhc) {
      ret = false;
      break;
    }

    if (dhc->do_dhcp()) {
      ret = false;
      break;
    }

  } while (0);

  delete dhc;

  return ret;
}

bool AMNetworkManger::wifi_ap_setup()
{
  string cmdline("dnsmasq --no-daemon &");
  char ip_str[16] = {0};
  in_addr in_ip, net_id;
  in_addr in_mask;
  in_addr in_gw;
  int32_t status = 0;
  bool ret = true;
  string ifname;

  do {
    if (!m_wifi_cfg.ap_cfg.enable) {
      break;
    }

    if (m_stat.wifi_inited) {
      DEBUG("begin to setup wifi AP.");
      if (m_wifi_cfg.ap_cfg.ssid.empty()) {
        ERROR("ssid is empty.");
        ret = false;
        break;
      }

      if (m_wifi_cfg.mode_cfg.mode == AM_AP_MODE) {
        ifname = m_wifi_cfg.mode_cfg.ifname;
      } else {
        ifname = m_wifi_cfg.mode_cfg.con_if;
      }

      if (m_wifi_cfg.ap_cfg.ip.empty() ||
          inet_aton(m_wifi_cfg.ap_cfg.ip.c_str(), &in_ip) == 0) {
        if (m_ifc_ptr->get_addr(ifname.c_str(), &(in_ip.s_addr))) {
          if(!ip_to_string(in_ip, ip_str, sizeof(ip_str))) {
            ret = false;
            break;
          }
          m_wifi_cfg.ap_cfg.ip = ip_str;
        } else {
          ERROR("ip address is empty or invalid.");
          ret = false;
          break;
        }
      }

      if (m_wifi_cfg.ap_cfg.passwd.empty()) {
        m_wifi_cfg.ap_cfg.passwd = "";
      }

      if (m_wifi_cfg.ap_cfg.mask.empty() ||
          inet_aton(m_wifi_cfg.ap_cfg.mask.c_str(), &in_mask) == 0) {
        WARN("netmask address is null or invalid, "
            "use default one \"255.255.255.0\"");
        if (inet_aton("255.255.255.0", &in_mask) == 0) {
          ERROR("inet_aton error.");
          ret = false;
          break;
        }
        m_wifi_cfg.ap_cfg.mask = string("255.255.255.0");
      }

      net_id.s_addr = in_ip.s_addr & in_mask.s_addr;

      if (m_wifi_cfg.ap_cfg.gw.empty() ||
          inet_aton(m_wifi_cfg.ap_cfg.gw.c_str(), &in_gw) == 0) {
        in_gw.s_addr = net_id.s_addr | 0x01000000;

        if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
          ret = false;
          break;
        }
        m_wifi_cfg.ap_cfg.gw = ip_str;
        WARN("gw address is null or invalid, "
            "use a default one\"%s\"", m_wifi_cfg.ap_cfg.gw.c_str());
      } else {
        if ((in_gw.s_addr & in_mask.s_addr) != net_id.s_addr) {
          in_gw.s_addr = net_id.s_addr | 0x01000000;
          if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
            ret = false;
            break;
          }
          m_wifi_cfg.ap_cfg.gw = ip_str;
          WARN("gw address and ip address are not in the same network,"
              "use a default gw\"%s\"", m_wifi_cfg.ap_cfg.gw.c_str());
        }
      }

      if (!generate_dhcp_cfg(ifname, in_ip, in_mask)) {
        ERROR("failed to generate dns dhcp config.");
        ret = false;
        break;
      }

      if (m_ifc_ptr->configure_ifc(ifname.c_str(), in_ip.s_addr,
                                   in_mask.s_addr, in_gw.s_addr)) {
        DEBUG("configure_ifc.");
        ret = false;
        break;
      }

      if (!m_wifi_manager->setup_ap(m_wifi_cfg.ap_cfg.ssid,
                                    m_wifi_cfg.ap_cfg.passwd,
                                    m_wifi_cfg.ap_cfg.channel)) {
        DEBUG("setup_ap.");
        ret = false;
        break;
      }

      status = system(cmdline.c_str());
      if (WEXITSTATUS(status)) {
        ERROR("Failed to start dnsmasq!\n");
        ret = false;
        break;
      }
      m_stat.ap_cfg_changed = false;
      m_stat.ap_run = true;
      DEBUG("Configure %s, done.", ifname.c_str());
    } else {
      ERROR("not inited! Please call set_mode_cfg first.");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::generate_dhcp_cfg(const string &ifname,
                                        const in_addr &in_ip,
                                        const in_addr &in_mask)
{
  bool ret = false;
  bool valid = false;
  in_addr dhcp_start, dhcp_end, net_id;
  FILE *fp = NULL;
  char line_str[64] = {0};
  char ip_start[16] = {0};
  char ip_end[16] = {0};

  do {
    if ((fp = fopen(DHCP_CFG_FILE, "w+"))) {
      net_id.s_addr = in_ip.s_addr & in_mask.s_addr;

      if (!m_wifi_cfg.ap_cfg.dhcp_range.empty()) {
        size_t pos = m_wifi_cfg.ap_cfg.dhcp_range.find_first_of(',');
        if (pos != string::npos && (pos + 1) <
            m_wifi_cfg.ap_cfg.dhcp_range.size()) {
          string start_ip, end_ip;

          start_ip = m_wifi_cfg.ap_cfg.dhcp_range.substr(0, pos);
          end_ip = m_wifi_cfg.ap_cfg.dhcp_range.substr(pos + 1);
          DEBUG("AP dhcp ip range: %s,%s.", start_ip.c_str(), end_ip.c_str());
          if (inet_aton(start_ip.c_str(), &dhcp_start) &&
              inet_aton(end_ip.c_str(), &dhcp_end)) {
            if (((dhcp_start.s_addr & in_mask.s_addr) == net_id.s_addr) &&
                ((dhcp_end.s_addr & in_mask.s_addr) == net_id.s_addr)) {
              if (in_ip.s_addr < dhcp_start.s_addr ||
                  in_ip.s_addr > dhcp_end.s_addr) {
                valid = true;
              }
            }
          }
        }
      }

      if (!valid) {
        WARN("AP dhcp ip range %lu,%lu is invalid, use a default one instead.",
             dhcp_start.s_addr, dhcp_end.s_addr);
        in_addr_t tmp = (in_ip.s_addr >> 24);
        if (tmp < 101) {
          dhcp_start.s_addr = (101 << 24) | net_id.s_addr;
          dhcp_end.s_addr = (200 << 24) | net_id.s_addr;
        } else {
          dhcp_start.s_addr = (1 << 24) | net_id.s_addr;
          dhcp_end.s_addr = (100 << 24) | net_id.s_addr;
        }
        if(!ip_to_string(dhcp_start, ip_start, sizeof(ip_start))) {
          ret = false;
          break;
        }
        if(!ip_to_string(dhcp_end, ip_end, sizeof(ip_end))) {
          ret = false;
          break;
        }
        m_wifi_cfg.ap_cfg.dhcp_range = string(ip_start) + "," + string(ip_end);
        DEBUG("dhcp_range: %s", m_wifi_cfg.ap_cfg.dhcp_range.c_str());
      }

      valid = false;

      if (!m_wifi_cfg.ap_cfg.lease.empty()) {
        int32_t num = 0;
        char ch = 0;
        char tmp_str[8] = {0};

        num = sscanf(m_wifi_cfg.ap_cfg.lease.c_str(),
                     "%d%c%s", &num, &ch, tmp_str);
        if ((num == 1) || (num == 2 &&
            (ch == 'm' || ch == 'h' || ch == 'd'))) {
          valid = true;
        }
      }

      if (!valid) {
        WARN("dhcp lease is empty or invalid, use default dhcp lease: 1h");
        m_wifi_cfg.ap_cfg.lease = "1h";
      }
      if (!ifname.empty()) {
        snprintf(line_str, sizeof(line_str) - 1, "interface=%s\n",
                 ifname.c_str());
        if (fputs(line_str, fp) <= 0) {
          ret = false;
          fclose(fp);
          break;
        }
      }
      snprintf(line_str, sizeof(line_str) - 1, "dhcp-range=%s,%s\n",
               m_wifi_cfg.ap_cfg.dhcp_range.c_str(),
               m_wifi_cfg.ap_cfg.lease.c_str());
      if (fputs(line_str, fp) <= 0) {
        fclose(fp);
        ret = false;
        break;
      }
      fclose(fp);
      ret = true;
    } else {
      ERROR("Failed to open %s", DHCP_CFG_FILE);
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::wifi_ap_remove()
{
  bool ret = true;

  if (m_stat.wifi_inited && m_stat.ap_run) {
    DEBUG("begin to remove wifi AP.");
    if (!m_wifi_manager->remove_ap()) {
      ret = false;
      ERROR("Failed to remove ap");
    }

    stop_dnsmasq();

    if (m_ifc_ptr->set_addr(m_stat.ap_curr_ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n", m_stat.ap_curr_ifname.c_str());
    }
  }
  m_stat.ap_run = false;

  return ret;
}

bool AMNetworkManger::wifi_sta_setup()
{
  bool ret = true;
  in_addr in_ip, net_id;
  in_addr in_mask;
  in_addr in_gw;
  char ip_str[16] = {0};

  do {
    if (!m_wifi_cfg.sta_cfg.enable) {
      break;
    }

    if (m_stat.wifi_inited) {
      DEBUG("begin to setup wifi station connection.");

      if (m_wifi_cfg.sta_cfg.ssid.empty()) {
        ret = false;
        break;
      }

      if (m_wifi_cfg.sta_cfg.passwd.empty()) {
        m_wifi_cfg.sta_cfg.passwd = "";
      }

      if (m_wifi_cfg.sta_cfg.hidden_ssid) DEBUG("hidden ssid.");
      if (!m_wifi_manager->connect_to_ap(m_wifi_cfg.sta_cfg.ssid,
                                         m_wifi_cfg.sta_cfg.passwd,
                                         m_wifi_cfg.sta_cfg.hidden_ssid)) {
        ret = false;
        break;
      }
      if (m_wifi_cfg.sta_cfg.dhcp_enable) {
        /* simple dhcpclient */
        if (!dhcp_get_ip_addr(m_wifi_cfg.mode_cfg.ifname.c_str())) {
          ret = false;
          break;
        }
        m_wifi_cfg.sta_cfg.dhcp_enable = true;
        //TODO: fill ip addr
      } else if (!m_wifi_cfg.sta_cfg.ip.empty()) {
        if (inet_aton(m_wifi_cfg.sta_cfg.ip.c_str(), &in_ip) == 0) {
          ERROR("ip address is invalid.");
          ret = false;
          break;
        }

        if (m_wifi_cfg.sta_cfg.mask.empty() ||
            inet_aton(m_wifi_cfg.sta_cfg.mask.c_str(), &in_mask) == 0) {
          WARN("netmask address is null or invalid, "
              "use default one \"255.255.255.0\"");
          if (inet_aton("255.255.255.0", &in_mask) == 0) {
            ERROR("inet_aton error.");
            ret = false;
            break;
          }
          m_wifi_cfg.sta_cfg.mask = string("255.255.255.0");
        }

        net_id.s_addr = in_ip.s_addr & in_mask.s_addr;

        if (m_wifi_cfg.sta_cfg.gw.empty() ||
            inet_aton(m_wifi_cfg.sta_cfg.gw.c_str(), &in_gw) == 0) {
          in_gw.s_addr = net_id.s_addr | 0x01000000;
          if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
            ret = false;
            break;
          }
          m_wifi_cfg.sta_cfg.gw = ip_str;
          WARN("gw address is null or invalid, "
              "use a default one\"%s\"", ip_str);
        } else {
          if ((in_gw.s_addr & in_mask.s_addr) != net_id.s_addr) {
            in_gw.s_addr = net_id.s_addr | 0x01000000;
            if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
              ret = false;
              break;
            }
            m_wifi_cfg.sta_cfg.gw = ip_str;
            WARN("gw address and ip address are not in the same network,"
                "use a default gw\"%s\"", ip_str);
          }
        }

        if (m_ifc_ptr->configure_ifc(m_wifi_cfg.mode_cfg.ifname.c_str(),
                                     in_ip.s_addr,in_mask.s_addr,
                                     in_gw.s_addr)) {
          ret = false;
          break;
        }
      } else {
        ERROR("neither dhcp_enable nor static ip is set!");
        ret = false;
        break;
      }
      m_stat.sta_cfg_changed = false;
      m_stat.sta_run = true;
      DEBUG("Configure %s, done.", m_wifi_cfg.mode_cfg.ifname.c_str());
    } else {
      ERROR("not inited! Please call set_mode_cfg first.");
      ret = false;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::wifi_sta_disconnect()
{
  bool ret = true;

  if (m_stat.wifi_inited && m_stat.sta_run) {
    DEBUG("begin to destroy wifi station connection");
    if (!m_wifi_manager->disconnect_to_ap()) {
      ret = false;
      ERROR("Failed to disconnect to ap");
    }

    //do dhcp release?
    if (m_ifc_ptr->set_addr(m_stat.sta_curr_ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n", m_stat.ap_curr_ifname.c_str());
    }
  }
  m_stat.sta_run = false;

  return ret;
}

bool AMNetworkManger::ethernet_setup()
{
  bool ret = true;
  in_addr in_ip, net_id;
  in_addr in_mask;
  in_addr in_gw;
  char ip_str[16] = {0};

  do {
    if (!m_eth.enable) {
      break;
    }
    DEBUG("begin to setup eth connection.");
    if (!m_eth.ifname.empty()) {
      if (m_eth.dhcp_enable) {
        /* simple dhcpclient */
        if (!dhcp_get_ip_addr(m_eth.ifname)) {
          ret = false;
          break;
        }
        m_stat.eth_curr_ifname = m_eth.ifname;
        m_stat.eth_run = true;
        m_eth.dhcp_enable = true;
      } else if (!m_eth.ip.empty()) {
        if (inet_aton(m_eth.ip.c_str(), &in_ip) == 0) {
          ERROR("ip address is invalid.");
          ret = false;
          break;
        }

        if (m_eth.mask.empty() ||
            inet_aton(m_eth.mask.c_str(), &in_mask) == 0) {
          WARN("netmask address is null or invalid, "
              "use default one \"255.255.255.0\"");
          if (inet_aton("255.255.255.0", &in_mask) == 0) {
            ERROR("inet_aton error.");
            ret = false;
            break;
          }
          m_eth.mask = string("255.255.255.0");
        }

        net_id.s_addr = in_ip.s_addr & in_mask.s_addr;

        if (m_eth.gw.empty() ||
            inet_aton(m_eth.gw.c_str(), &in_gw) == 0) {
          in_gw.s_addr = net_id.s_addr | 0x01000000;
          if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
            ret = false;
            break;
          }
          m_eth.gw = ip_str;
          WARN("gw address is null or invalid, "
              "use a default one\"%s\"", ip_str);
        } else {
          if ((in_gw.s_addr & in_mask.s_addr) != net_id.s_addr) {
            in_gw.s_addr = net_id.s_addr | 0x01000000;
            if(!ip_to_string(in_gw, ip_str, sizeof(ip_str))) {
              ret = false;
              break;
            }
            m_eth.gw = ip_str;
            WARN("gw address and ip address are not in the same network,"
                "use a default gw\"%s\"", ip_str);
          }
        }

        if (m_ifc_ptr->configure_ifc(m_eth.ifname.c_str(), in_ip.s_addr,
                                     in_mask.s_addr, in_gw.s_addr)) {
          ret = false;
          break;
        }
        m_stat.eth_curr_ifname = m_eth.ifname;
        m_stat.eth_run = true;
        DEBUG("Configure %s, done.", m_eth.ifname.c_str());
      } else {
        ERROR("neither dhcp_enable nor static ip is set!");
        ret = false;
        break;
      }
      m_stat.eth_cfg_changed = false;
    } else {
      ERROR("ifname is NULL!");
      ret = false;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::ethernet_disconnect()
{
  bool ret = true;

  if (m_stat.eth_run) {
    DEBUG("begin to destroy eth connection.");
    if (m_ifc_ptr->set_addr(m_stat.eth_curr_ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n",
            m_stat.eth_curr_ifname.c_str());
    }
    m_stat.eth_run = false;
  } else { /* for test_network_manager_wpa */
    DEBUG("begin to destroy eth connection.");
    if (m_ifc_ptr->set_addr(m_eth.ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n",
            m_eth.ifname.c_str());
    }
  }

  return ret;
}

void AMNetworkManger::eth_setup()
{
  if (!m_eth.enable ||
      m_stat.eth_cfg_changed) {
    if (!ethernet_disconnect()) {
      WARN("Failed to disable eth connection(s).");
    }
  }

  m_stat.eth_setup = true;

  if (m_stat.eth_cfg_changed) {
    if (!ethernet_setup()) {
      ERROR("ethernet_setup fail.");
      m_stat.eth_setup = false;
    }
  }
}

bool AMNetworkManger::rough_remove_pre_connections()
{
  bool ret = true;

  DEBUG("rough_remove_pre_connections.");

  stop_dnsmasq();
  m_wifi_manager->stop_wpa_supplicant();

  if (m_wifi_cfg.mode_cfg.mode == AM_STATION_MODE ||
      m_wifi_cfg.mode_cfg.mode == AM_AP_MODE) {
    if (m_ifc_ptr->set_addr(m_wifi_cfg.mode_cfg.ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n", m_wifi_cfg.mode_cfg.ifname.c_str());
    }
  } else if (m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE) {
    if (m_ifc_ptr->set_addr(m_wifi_cfg.mode_cfg.ifname.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n", m_wifi_cfg.mode_cfg.ifname.c_str());
    }
    if (m_ifc_ptr->set_addr(m_wifi_cfg.mode_cfg.con_if.c_str(), 0)) {
      ret = false;
      ERROR("failed to clear %s ipaddr\n", m_wifi_cfg.mode_cfg.con_if.c_str());
    }
  }

  return ret;
}

bool AMNetworkManger::wifi_handle_pre_connections()
{
  bool ret = true;

  if (m_stat.wifi_inited) {
    DEBUG("m_stat.wifi_inited.");
    if (!m_wifi_cfg.enable ||
        !m_wifi_cfg.ap_cfg.enable ||
        m_stat.mode_changed ||
        m_stat.wpa_need_restart ||
        m_stat.ap_cfg_changed) {
      if (!wifi_ap_remove()) {
        ret = false;
      }
    }

    if (!m_wifi_cfg.enable ||
        !m_wifi_cfg.sta_cfg.enable ||
        m_stat.mode_changed ||
        m_stat.wpa_need_restart ||
        m_stat.sta_cfg_changed) {
      if (!wifi_sta_disconnect()) {
        ret = false;
      }
    }
  } else { /* for test_network_manager_wpa */
    if (!rough_remove_pre_connections()) {
      ret = false;
    }
  }

  return ret;
}

void AMNetworkManger::wifi_setup()
{
  do {
    if (!wifi_handle_pre_connections()) {
      WARN("Failed to disable wifi connection(s).");
    }
    m_stat.wifi_setup = true;
    if (m_wifi_cfg.enable) {
      if (m_stat.mode_changed) {
        if (!reload_driver()) {
          ERROR("reload_driver fail.");
          m_stat.wifi_setup = false;
          break;
        }
      }

      /* up interface */
      if (!up_interface()) {
        m_stat.wifi_setup = false;
        break;
      }

      if (!wifi_wpa_setup()) {
        ERROR("wifi_wpa_setup fail.");
        m_stat.wifi_setup = false;
        break;
      }

      if (m_wifi_cfg.mode_cfg.mode == AM_AP_MODE ||
          m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE) {
        if (m_stat.wpa_need_restart ||
            m_stat.mode_changed ||
            m_stat.ap_cfg_changed) {
          if (!wifi_ap_setup()) {
            ERROR("wifi_ap_setup fail.");
            m_stat.wifi_setup = false;
          }
        }
      }

      if (m_wifi_cfg.mode_cfg.mode == AM_STATION_MODE ||
          m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE) {
        if (m_stat.wpa_need_restart ||
            m_stat.mode_changed||
            m_stat.sta_cfg_changed) {
          if (!wifi_sta_setup()) {
            ERROR("wifi_sta_setup fail.");
            m_stat.wifi_setup = false;
          }
        }
      }

      m_stat.wpa_need_restart = false;
      m_stat.mode_changed = false;
    }
  } while (0);
}

bool AMNetworkManger::network_setup()
{
  bool ret = false;

  do {
    DEBUG("Call network_setup.");
    thread eth_thread(&AMNetworkManger::eth_setup, this);
    thread wifi_thread(&AMNetworkManger::wifi_setup, this);
    eth_thread.join();
    wifi_thread.join();

    if (m_stat.eth_setup && m_stat.wifi_setup) {
      PRINTF("Set network successfully.");
      ret = true;
    }

  } while (0);

  return ret;
}

bool AMNetworkManger::save_network_config()
{
  return save_config();
}

bool AMNetworkManger::list_ap()
{
  bool ret = true;
  char buf[8192] = {0};
  size_t buf_len = sizeof(buf);

  if (m_wifi_manager->list_ap(buf, &buf_len)) {
    printf("%s\n", buf);
  } else {
    ERROR("Failed to get AP list");
    ret = false;
  }

  return ret;
}

bool AMNetworkManger::eth_status(const char *ifname)
{
  bool ret = true;

  if (ifname) {
    if (m_ifc_ptr->print_if_info(ifname)) {
      ret = false;
    }
  } else {
    if (m_ifc_ptr->print_all_eth_info()) {
      ret = false;
    }
  }

  return ret;
}

bool AMNetworkManger::all_status()
{
  bool ret = true;

  if (m_ifc_ptr->print_all_if_info()) {
    ret = false;
  }

  return ret;
}

bool AMNetworkManger::wifi_status(const char *ifname)
{
  bool ret = true;
  char buf[1024] = {0};
  size_t buf_len = sizeof(buf);

  if (ifname) {
    if (m_wifi_manager->wifi_status(ifname, buf, &buf_len)) {
      printf("%s\n", buf);
    } else {
      ret = false;
      ERROR("Failed to get wifi status");
    }
  } else {
    if (m_wifi_manager->wifi_status(buf, &buf_len)) {
      printf("%s\n", buf);
    } else {
      ret = false;
      ERROR("Failed to get wifi status");
    }
  }

  return ret;
}

AMNetworkManger::~AMNetworkManger()
{
  INFO("~AMNetworkManger called.");
  delete m_wifi_manager;
  m_wifi_manager = nullptr;
}

void AMNetworkManger::get_eth_cfg(AMEthCfg &eth_cfg)
{
  eth_cfg = m_eth;
}

void AMNetworkManger::get_wifi_mode_cfg(AMModeCfg &mode_cfg)
{
  mode_cfg = m_wifi_cfg.mode_cfg;
}

void AMNetworkManger::get_sta_cfg(AMStaCfg &sta_cfg)
{
  sta_cfg = m_wifi_cfg.sta_cfg;
}

void AMNetworkManger::get_ap_cfg(AMAPCfg &ap_cfg)
{
  ap_cfg = m_wifi_cfg.ap_cfg;
}

void AMNetworkManger::stop_dnsmasq()
{
  system("killall dnsmasq 2> /dev/null");
}

bool AMNetworkManger::reload_driver()
{
  bool ret = true;
  int32_t status = 0;
  string cmdline("");

  do {
    if (m_wifi_cfg.drv_cfg.reload) {
      if (m_wifi_cfg.drv_cfg.name.empty()) {
        ERROR("wifi driver name is empty");
        ret = false;
        break;
      }
      cmdline = string("rmmod ") + m_wifi_cfg.drv_cfg.name;
      DEBUG("rmmod %s", cmdline.c_str());
      status = system(cmdline.c_str());
      if (WEXITSTATUS(status)) {
        ret = false;
        ERROR("Failed to remove kernel module %s!\n",
              m_wifi_cfg.drv_cfg.name.c_str());
        break;
      }
      cmdline.clear();
      if (m_wifi_cfg.mode_cfg.mode == AM_STATION_MODE) {
        if (m_wifi_cfg.drv_cfg.sta_fw.empty()) {
          cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name;
        } else {
          if (!access(m_wifi_cfg.drv_cfg.sta_fw.c_str(), F_OK)) {
            cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name +
                string(" firmware_path=") + m_wifi_cfg.drv_cfg.sta_fw;
          } else {
            ret = false;
            ERROR("firmware path is invalid: %s",
                  m_wifi_cfg.drv_cfg.sta_fw.c_str());
            break;
          }
        }
      } else if (m_wifi_cfg.mode_cfg.mode == AM_AP_MODE) {
        if (m_wifi_cfg.drv_cfg.ap_fw.empty()) {
          cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name;
        } else {
          if (!access(m_wifi_cfg.drv_cfg.ap_fw.c_str(), F_OK)) {
            cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name +
                string(" firmware_path=") + m_wifi_cfg.drv_cfg.ap_fw;
          } else {
            ret = false;
            ERROR("firmware path is invalid: %s",
                  m_wifi_cfg.drv_cfg.ap_fw.c_str());
            break;
          }
        }
      } else if (m_wifi_cfg.mode_cfg.mode == AM_CONCURRENT_MODE) {
        if (m_wifi_cfg.drv_cfg.con_fw.empty()) {
          cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name;
        } else {
          if (!access(m_wifi_cfg.drv_cfg.con_fw.c_str(), F_OK)) {
            cmdline = string("modprobe ") + m_wifi_cfg.drv_cfg.name +
                string(" firmware_path=") + m_wifi_cfg.drv_cfg.con_fw;
          } else {
            ret = false;
            ERROR("firmware path is invalid: %s",
                  m_wifi_cfg.drv_cfg.con_fw.c_str());
            break;
          }
        }
      } else {
        ret = false;
        ERROR("invalid wpa mode: %d", m_wifi_cfg.mode_cfg.mode);
        break;
      }
      DEBUG("reload_driver: %s", cmdline.c_str());
      status = system(cmdline.c_str());
      if (WEXITSTATUS(status)) {
        ret = false;
        ERROR("Failed to remove kernel module %s!\n",
              m_wifi_cfg.drv_cfg.name.c_str());
        break;
      }
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::load_config()
{
  bool ret = false;
  AMConfig *config_ptr = nullptr;

  do {
    if (!access(CONFIG_PATH, F_OK)) {
      config_ptr = AMConfig::create(CONFIG_PATH);
      if (!config_ptr) {
        ERROR("AMConfig::create error");
        break;
      }
      AMConfig &config = *config_ptr;
      if (config["eth"].exists()) {
        if (config["eth"]["enable"].exists()) {
          m_eth.enable = config["eth"]["enable"].get<bool>(false);
        }
        if (config["eth"]["ifc"].exists()) {
          m_eth.ifname = config["eth"]["ifc"].get<string>("");
        }
        if (config["eth"]["dhcp_enable"].exists()) {
          m_eth.dhcp_enable = config["eth"]["dhcp_enable"].get<bool>(false);
        }
        if (config["eth"]["ip"].exists()) {
          m_eth.ip = config["eth"]["ip"].get<string>("");
        }
        if (config["eth"]["mask"].exists()) {
          m_eth.mask = config["eth"]["mask"].get<string>("");
        }
        if (config["eth"]["gw"].exists()) {
          m_eth.gw = config["eth"]["gw"].get<string>("");
        }
      }

      if (config["wifi"].exists()) {
        if (config["wifi"]["enable"].exists()) {
          m_wifi_cfg.enable = config["wifi"]["enable"].get<bool>(false);
        }
        if (config["wifi"]["wpa"].exists()) {
          if (config["wifi"]["wpa"]["mode"].exists()) {
            m_wifi_cfg.mode_cfg.mode =
                (AM_WIFI_MODE)(config["wifi"]["wpa"]["mode"].get<int>(0));
          }
          if (config["wifi"]["wpa"]["ifname"].exists()) {
            m_wifi_cfg.mode_cfg.ifname =
                config["wifi"]["wpa"]["ifname"].get<string>("");
          }
          if (config["wifi"]["wpa"]["con_if"].exists()) {
            m_wifi_cfg.mode_cfg.con_if =
                config["wifi"]["wpa"]["con_if"].get<string>("");
          }
          if (config["wifi"]["wpa"]["cfg"].exists()) {
            m_wifi_cfg.mode_cfg.cfg =
                config["wifi"]["wpa"]["cfg"].get<string>("");
          }
        }

        if (config["wifi"]["sta"].exists()) {
          if (config["wifi"]["sta"]["enable"].exists()) {
            m_wifi_cfg.sta_cfg.enable =
                config["wifi"]["sta"]["enable"].get<bool>(false);
          }
          if (config["wifi"]["sta"]["ssid"].exists()) {
            m_wifi_cfg.sta_cfg.ssid =
                config["wifi"]["sta"]["ssid"].get<string>("");
          }
          if (config["wifi"]["sta"]["passwd"].exists()) {
            m_wifi_cfg.sta_cfg.passwd =
                config["wifi"]["sta"]["passwd"].get<string>("");
          }
          if (config["wifi"]["sta"]["dhcp_enable"].exists()) {
            m_wifi_cfg.sta_cfg.dhcp_enable =
                config["wifi"]["sta"]["dhcp_enable"].get<bool>(false);
          }
          if (config["wifi"]["sta"]["ip"].exists()) {
            m_wifi_cfg.sta_cfg.ip = config["wifi"]["sta"]["ip"].get<string>("");
          }
          if (config["wifi"]["sta"]["mask"].exists()) {
            m_wifi_cfg.sta_cfg.mask =
                config["wifi"]["sta"]["mask"].get<string>("");
          }
          if (config["wifi"]["sta"]["gw"].exists()) {
            m_wifi_cfg.sta_cfg.gw = config["wifi"]["sta"]["gw"].get<string>("");
          }
          if (config["wifi"]["sta"]["hidden_ssid"].exists())
            m_wifi_cfg.sta_cfg.hidden_ssid =
                config["wifi"]["sta"]["hidden_ssid"].get<bool>(false);
        }

        if (config["wifi"]["ap"].exists()) {
          if (config["wifi"]["ap"]["enable"].exists()) {
            m_wifi_cfg.ap_cfg.enable =
                config["wifi"]["ap"]["enable"].get<bool>(false);
          }
          if (config["wifi"]["ap"]["ssid"].exists()) {
            m_wifi_cfg.ap_cfg.ssid =
                config["wifi"]["ap"]["ssid"].get<string>("");
          }
          if (config["wifi"]["ap"]["passwd"].exists()) {
            m_wifi_cfg.ap_cfg.passwd =
                config["wifi"]["ap"]["passwd"].get<string>("");
          }
          if (config["wifi"]["ap"]["channel"].exists()) {
            m_wifi_cfg.ap_cfg.channel =
                config["wifi"]["ap"]["channel"].get<int>(1);
          }
          if (config["wifi"]["ap"]["hidden"].exists()) {
            m_wifi_cfg.ap_cfg.hidden =
                config["wifi"]["ap"]["hidden"].get<bool>(false);
          }
          if (config["wifi"]["ap"]["ip"].exists()) {
            m_wifi_cfg.ap_cfg.ip =
                config["wifi"]["ap"]["ip"].get<string>("");
          }
          if (config["wifi"]["ap"]["mask"].exists()) {
            m_wifi_cfg.ap_cfg.mask =
                config["wifi"]["ap"]["mask"].get<string>("");
          }
          if (config["wifi"]["ap"]["gw"].exists()) {
            m_wifi_cfg.ap_cfg.gw =
                config["wifi"]["ap"]["gw"].get<string>("");
          }
          if (config["wifi"]["ap"]["dhcp_range"].exists()) {
            m_wifi_cfg.ap_cfg.dhcp_range =
                config["wifi"]["ap"]["dhcp_range"].get<string>("");
          }
          if (config["wifi"]["ap"]["lease"].exists()) {
            m_wifi_cfg.ap_cfg.lease =
                config["wifi"]["ap"]["lease"].get<string>("");
          }
        }
        if (config["wifi"]["drv"].exists()) {
          if (config["wifi"]["drv"]["reload"].exists()) {
            m_wifi_cfg.drv_cfg.reload =
                config["wifi"]["drv"]["reload"].get<bool>(false);
          }
          if (config["wifi"]["drv"]["name"].exists()) {
            m_wifi_cfg.drv_cfg.name =
                config["wifi"]["drv"]["name"].get<string>("");
          }
          if (config["wifi"]["drv"]["ap_fw"].exists()) {
            m_wifi_cfg.drv_cfg.ap_fw =
                config["wifi"]["drv"]["ap_fw"].get<string>("");
          }
          if (config["wifi"]["drv"]["sta_fw"].exists()) {
            m_wifi_cfg.drv_cfg.sta_fw =
                config["wifi"]["drv"]["sta_fw"].get<string>("");
          }
          if (config["wifi"]["drv"]["con_fw"].exists()) {
            m_wifi_cfg.drv_cfg.con_fw =
                config["wifi"]["drv"]["con_fw"].get<string>("");
          }
        }
      }
      delete config_ptr;
      config_ptr = nullptr;
      ret = true;
    } else {
      printf("%s does not exist, ignore!\n", CONFIG_PATH);
      break;
    }
  } while (0);

  return ret;
}

bool AMNetworkManger::save_config()
{
  bool ret = true;
  AMConfig *config_ptr = nullptr;

  do {
    config_ptr = AMConfig::create(CONFIG_PATH);
    if (!config_ptr) {
      ERROR("AMConfig::create error");
      ret = false;
      break;
    }
    AMConfig &config = *config_ptr;

    config["eth"]["enable"] = m_eth.enable;
    config["eth"]["ifc"] = m_eth.ifname;
    config["eth"]["dhcp_enable"] = m_eth.dhcp_enable;
    config["eth"]["ip"] = m_eth.ip;
    config["eth"]["mask"] = m_eth.mask;
    config["eth"]["gw"] = m_eth.gw;

    config["wifi"]["enable"] = m_wifi_cfg.enable;

    config["wifi"]["wpa"]["mode"] = (int)(m_wifi_cfg.mode_cfg.mode);
    config["wifi"]["wpa"]["ifname"] = m_wifi_cfg.mode_cfg.ifname;
    config["wifi"]["wpa"]["con_if"] = m_wifi_cfg.mode_cfg.con_if;
    config["wifi"]["wpa"]["cfg"] = m_wifi_cfg.mode_cfg.cfg;

    config["wifi"]["sta"]["enable"] = m_wifi_cfg.ap_cfg.enable;
    config["wifi"]["sta"]["ssid"] = m_wifi_cfg.sta_cfg.ssid;
    config["wifi"]["sta"]["passwd"] = m_wifi_cfg.sta_cfg.passwd;
    config["wifi"]["sta"]["dhcp_enable"] = m_wifi_cfg.sta_cfg.dhcp_enable;
    config["wifi"]["sta"]["ip"] = m_wifi_cfg.sta_cfg.ip;
    config["wifi"]["sta"]["mask"] = m_wifi_cfg.sta_cfg.mask;
    config["wifi"]["sta"]["gw"] = m_wifi_cfg.sta_cfg.gw;
    config["wifi"]["sta"]["hidden_ssid"] = m_wifi_cfg.sta_cfg.hidden_ssid;

    config["wifi"]["ap"]["enable"] = m_wifi_cfg.ap_cfg.enable;
    config["wifi"]["ap"]["ssid"] = m_wifi_cfg.ap_cfg.ssid;
    config["wifi"]["ap"]["passwd"] = m_wifi_cfg.ap_cfg.passwd;
    config["wifi"]["ap"]["channel"] = m_wifi_cfg.ap_cfg.channel;
    config["wifi"]["ap"]["hidden"] = m_wifi_cfg.ap_cfg.hidden;
    config["wifi"]["ap"]["ip"] = m_wifi_cfg.ap_cfg.ip;
    config["wifi"]["ap"]["mask"] = m_wifi_cfg.ap_cfg.mask;
    config["wifi"]["ap"]["gw"] = m_wifi_cfg.ap_cfg.gw;
    config["wifi"]["ap"]["dhcp_range"] = m_wifi_cfg.ap_cfg.dhcp_range;
    config["wifi"]["ap"]["lease"] = m_wifi_cfg.ap_cfg.lease;

    config["wifi"]["drv"]["reload"] = m_wifi_cfg.drv_cfg.reload;
    config["wifi"]["drv"]["name"] = m_wifi_cfg.drv_cfg.name;
    config["wifi"]["drv"]["ap_fw"] = m_wifi_cfg.drv_cfg.ap_fw;
    config["wifi"]["drv"]["sta_fw"] = m_wifi_cfg.drv_cfg.sta_fw;
    config["wifi"]["drv"]["con_fw"] = m_wifi_cfg.drv_cfg.con_fw;

    if (!config_ptr->save()) {
      ERROR("Failed to save config: %s", CONFIG_PATH);
      ret = false;
    }
    delete config_ptr;
    config_ptr = nullptr;
  } while(0);

  return ret;
}
