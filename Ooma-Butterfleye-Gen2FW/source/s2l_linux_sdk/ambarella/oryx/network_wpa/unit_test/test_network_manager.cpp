/*******************************************************************************
 * test_network_manager.cpp
 *
 * History:
 *   Jan 13, 2017 - [longli] created file
 *
 * Copyright (C) 2017, Ambarella Co, Ltd.
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
#include "am_define.h"
#include "am_log.h"
#include "am_network_manager_if.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

static void usage(int argc, char**argv)
{
  printf("usage:\n");
  printf("%s help        Get help\n", argv[0]);
  printf("%s status      Get all interface status\n", argv[0]);
  printf("%s eth  status [ifname interface]    Get eth  [interface] status\n", argv[0]);
  printf("%s wifi status [ifname interface]    Get wifi [interface] status\n", argv[0]);
  printf("%s wifi list   Get AP list\n", argv[0]);

  printf("\nEthernet setup:\n");
  printf("%s eth [on|off] [ifname interface] [dhcp on|off]"
      " [ip ip[,mask,gw]]\n", argv[0]);

  printf("\nWifi setup:\n");
  printf("%s wifi [on|off] [mode 1|2|3] [mode parameters below]\n", argv[0]);
  printf("  Wifi mode parameters description:\n"
      "  Station mode 1: [sta on|off] [ifname interface] [ssid ap_name]"
      " [open|[passwd password]] [dhcp on|off] [ip ip[,mask,gw]]\n"
      "                [hidden yes|no] [save_cfg]\n");
  printf("  AP mode 2: [ap on|off] [ifname interface] [ssid ap_name]"
      " [open|[passwd password]] [channel channel_id][ip ip[,mask,gw]] \n"
      "           [dhcp_server ip_range[,dhcp_lease_time]] [save_cfg]\n");
  printf("  Concurrent mode 3: combine mode1 and mode2's parameters above\n");

  printf("\nNote: 1. ip mentioned above is to set interface static ip address.\n");
  printf("      2. dhcp_lease_time is in format of \"num[m|h|d]\",lease is "
      "at least 2m. The combination of 'm', 'h' and 'd' is not supported.\n");
  printf("      3. Concurrent mode is only supported on the wifi chip of realtek.\n");
  printf("\ne.g.:\n");
  printf("  test_network_manager_wpa wifi mode 1 sta ifname wlan0 "
      "ssid wifi_name open dhcp on\n");
  printf("  test_network_manager_wpa wifi mode 1 sta ifname wlan0 "
      "ssid wifi_name passwd password dhcp on\n");
  printf("  test_network_manager_wpa wifi mode 1 sta ifname wlan0 "
      "ssid wifi_name passwd password dhcp off "
      "ip 192.168.0.2,255.255.255.0,192.168.0.1\n");
  printf("  test_network_manager_wpa wifi mode 2 ap ifname wlan0 "
      "ssid ap_name passwd password channel 6"
      "ip 192.168.0.1,255.255.255.0,192.168.0.1 "
      "dhcp_server 192.168.0.2,192.168.0.100,1h\n");
  printf("  test_network_manager_wpa wifi mode 2 ap ifname wlan0 "
        "ssid ap_name open ip 192.168.0.1,255.255.255.0,192.168.0.1 "
        "dhcp_server 192.168.0.2,192.168.0.100,1h channel 1\n");
  printf("  test_network_manager_wpa wifi mode 3 sta ifname wlan0 "
        "ssid wifi_name passwd password dhcp on ap ifname wlan1 "
        "ssid ap_name open ip 192.168.0.1,255.255.255.0,192.168.0.1 "
        "dhcp_server 192.168.0.2,192.168.0.100,1h channel 1\n");
}

static AMModeCfg mode_cfg;
static AMStaCfg sta_cfg;
static AMAPCfg ap_cfg;
static AMEthCfg eth_cfg;
static int32_t sta_ifname_index = 0;
static int32_t ap_ifname_index = 0;
static bool wifi_enable = true;
static bool show_help = false;
static bool list_ap = false;
static bool save_cfg = false;
static bool show_wifi_status = false;
static bool show_eth_status = false;
static bool show_all_status = false;

static bool check_ip(const char* ip)
{
  bool ret = true;

  if (!~inet_addr(ip)) {
    ret = false;
  }

  return ret;
}

static bool parse_ip_address(string &ip_string,
                             string &ip,
                             string &mask,
                             string &gw)
{
  bool ret = true;
  size_t s_pos = 0;
  size_t n_pos = string::npos;
  string tmp_ip("");
  string tmp_mask("");
  string tmp_gw("");

  do {
    if (ip_string.empty()) {
      ret = false;
      break;
    }

    n_pos = ip_string.find_first_of(',');

    if (n_pos != string::npos) {
      tmp_ip = ip_string.substr(s_pos, n_pos);
      s_pos = n_pos + 1;
      n_pos = ip_string.find_first_of(',', s_pos);
      if (n_pos != string::npos) {
        tmp_mask = ip_string.substr(s_pos, n_pos - s_pos);
        s_pos = n_pos + 1;
        n_pos = ip_string.find_first_of(',', s_pos);
        if (n_pos != string::npos) {
          tmp_gw = ip_string.substr(s_pos, n_pos - s_pos);
        } else {
          tmp_gw = ip_string.substr(s_pos);
        }
      } else {
        tmp_mask = ip_string.substr(s_pos);
      }
    } else {
      tmp_ip = ip_string;
    }

    if (tmp_ip.empty()) {
      ret = false;
      break;
    } else {
      if (!check_ip(tmp_ip.c_str())) {
        ret = false;
        break;
      }
    }

    if (!tmp_mask.empty() && !check_ip(tmp_mask.c_str())) {
      ret = false;
      break;
    }

    if (!tmp_gw.empty() && !check_ip(tmp_gw.c_str())) {
      ret = false;
      break;
    }
    ip = tmp_ip;
    mask = tmp_mask;
    gw = tmp_gw;
  } while (0);

  return ret;
}

static bool parse_dhcp_server(const string &server_cfg)
{
  bool ret = true;
  size_t s_pos = 0;
  size_t n_pos = string::npos;

  do {
    if (server_cfg.empty()) {
      ret = false;
      break;
    }

    n_pos = server_cfg.find_first_of(',');
    if (n_pos != string::npos) {
      if (n_pos != 0) {
        s_pos = n_pos + 1;
        n_pos = server_cfg.find_first_of(',', s_pos);
        if (n_pos != string::npos) {
          ap_cfg.dhcp_range = server_cfg.substr(0, n_pos);
          s_pos = n_pos + 1;
          n_pos = server_cfg.find_first_of(',', s_pos);
          if (n_pos != string::npos) {
            ap_cfg.lease = server_cfg.substr(s_pos, n_pos - s_pos);
          } else {
            ap_cfg.lease = server_cfg.substr(s_pos);
          }
        } else {
          ap_cfg.dhcp_range = server_cfg.substr(s_pos);
        }
      } else {
        ap_cfg.dhcp_range = "";
        s_pos = n_pos + 1;
        n_pos = server_cfg.find_first_of(',', s_pos);
        if (n_pos != string::npos) {
          s_pos = n_pos + 1;
          n_pos = server_cfg.find_first_of(',', s_pos);
          if (n_pos != string::npos) {
            ap_cfg.lease = server_cfg.substr(s_pos, n_pos - s_pos);
          } else {
            ap_cfg.lease = server_cfg.substr(s_pos);
          }
        }
      }
      DEBUG("dhcp server ip range, lease: %s,%s",
            ap_cfg.dhcp_range.c_str(), ap_cfg.lease.c_str());
    } else {
      ret = false;
      printf("dhcp_server config is invalid.");
      break;
    }
  } while (0);

  return ret;
}

static bool parse_wifi_parameters(int32_t &index,
                                  int32_t argc,
                                  char **argv)
{
  bool ret = true;
  bool first_loop = true;
  int32_t curr_index = 0;

  if (2 == argc) {
    list_ap = true;
  } else {
    while (index < argc) {
      if (first_loop) {
        if (is_str_equal("on", argv[index])) {
          wifi_enable = true;
          ++index;
        } else if (is_str_equal("off", argv[index])) {
          wifi_enable = false;
          ++index;
        }
        first_loop = false;
        continue;
      }

      if (is_str_equal("list", argv[index])) {
        list_ap = true;
        index = argc;
        break;
      } else if (is_str_equal("status", argv[index])) {
        show_wifi_status = true;
        sta_ifname_index = 0;
        if (++index < argc) {
          if (is_str_equal("ifname", argv[index])) {
            if (++index < argc) {
              sta_ifname_index = index;
            } else {
              PRINTF("Error: wifi status ifname argument is missing.");
              ret = false;
            }
          }
        }
        index = argc;
        break;
      } else if (is_str_equal("mode", argv[index])) {
        if (++index < argc && isdigit(argv[index][0])) {
          int32_t mode =  argv[index][0] - '0';
          if (mode > 0 && mode < 4) {
            mode_cfg.mode = (AM_WIFI_MODE)mode;
          } else {
            PRINTF("Invalid Wifi Mode: %d! "
                "(1: Station Mode, 2: AP Mode 3: Concurrent Mode)", mode);
            ret = false;
            break;
          }
        } else {
          PRINTF("Wifi mode argument is missing. Mode must be set! "
              "(1: Station Mode, 2: AP Mode 3: Concurrent Mode)");
          ret = false;
          break;
        }

      } else if (is_str_equal("sta", argv[index])) {
        curr_index = 1;
        if (++index < argc) {
          if (is_str_equal("on", argv[index])) {
            sta_cfg.enable = true;
          } else if (is_str_equal("off", argv[index])) {
            sta_cfg.enable = false;
          } else {
            sta_cfg.enable = true;
            continue;
          }
        }
      } else if (is_str_equal("ap", argv[index])) {
        curr_index = 2;
        if (++index < argc) {
          if (is_str_equal("on", argv[index])) {
            ap_cfg.enable = true;
          } else if (is_str_equal("off", argv[index])) {
            ap_cfg.enable = false;
          } else {
            ap_cfg.enable = true;
            continue;
          }
        }
      } else if (is_str_equal("ifname", argv[index])) {
        if (++index < argc) {
          if (curr_index == 0) {
            ret = false;
            PRINTF("wifi ifname must be set after ap or sta");
            break;
          }
          if (curr_index == 1) {
            sta_ifname_index = index;
          } else {
            ap_ifname_index = index;
          }
        } else {
          PRINTF("Error: wifi ifname argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("ssid", argv[index])) {
        if (++index < argc) {
          if (curr_index == 1) {
            sta_cfg.ssid = argv[index];
            sta_cfg.passwd = "";
          } else if (curr_index == 2){
            ap_cfg.ssid = argv[index];
            sta_cfg.passwd = "";
          } else {
            PRINTF("ssid must be set after ap or sta");
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi ssid argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("passwd", argv[index])) {
        if (++index < argc) {
          if (curr_index == 1) {
            sta_cfg.passwd = argv[index];
          } else if (curr_index == 2){
            ap_cfg.passwd = argv[index];
          } else {
            PRINTF("passwd must be set after ap or sta");
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi passwd argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("dhcp", argv[index])) {
        if (++index < argc) {
          if (curr_index != 1) {
            ret = false;
            PRINTF("Error: dhcp must be set after sta");
            break;
          }
          if (is_str_equal("on", argv[index])) {
            sta_cfg.dhcp_enable = true;
          } else if (is_str_equal("off", argv[index])) {
            sta_cfg.dhcp_enable = false;
          } else {
            PRINTF("Error: Unknown wifi dhcp argument: %s", argv[index]);
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi dhcp argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("hidden", argv[index])) {
        if (++index < argc) {
          if (curr_index != 1) {
            ret = false;
            PRINTF("Error: hidden must be set after sta, ignore");
            break;
          }
          if (is_str_equal("yes", argv[index])) {
            sta_cfg.hidden_ssid = true;
          } else if (is_str_equal("no", argv[index])) {
            sta_cfg.hidden_ssid = false;
          } else {
            PRINTF("Error: Unknown wifi hidden argument: %s", argv[index]);
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi hidden argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("channel", argv[index])) {
        if (++index < argc) {
          if (curr_index != 2) {
            ret = false;
            PRINTF("Error: channel must be set after ap, ignore.");
            break;
          }
          if (isdigit(argv[index][0])) {
            int32_t channel =  argv[index][0] - '0';
            if (channel > 0 && channel < 197) {
              ap_cfg.channel = channel;
            } else {
              PRINTF("Invalid Wifi channel: %d (2.4G: 1 ~ 14, 5G: 15 ~ 196)",
                     channel);
              ret = false;
              break;
            }
          } else {
            PRINTF("Invalid Wifi channel: %s (2.4G: 1 ~ 14, 5G: 15 ~ 196)",
                   argv[index]);
            ret = false;
            break;
          }
        } else {
          PRINTF("Wifi channel argument is missing! "
              "(2.4G: 1 ~ 14, 5G: 15 ~ 196");
          ret = false;
          break;
        }
      } else if (is_str_equal("ip", argv[index])) {
        if (++index < argc) {
          string ip_string(argv[index]);
          if (curr_index == 1) {
            if (!parse_ip_address(ip_string, sta_cfg.ip,
                                  sta_cfg.mask, sta_cfg.gw)) {
              PRINTF("sta ip configure %s is invalid.", ip_string.c_str());
              ret = false;
              break;
            }
          } else if (curr_index == 2){
            if (!parse_ip_address(ip_string, ap_cfg.ip,
                                  ap_cfg.mask, ap_cfg.gw)) {
              PRINTF("ap ip configure %s is invalid.", ip_string.c_str());
              ret = false;
              break;
            }
          } else {
            PRINTF("ip must be set after ap or sta");
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi ip argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("dhcp_server", argv[index])) {
        if (++index < argc) {
          string dhcp_server(argv[index]);
          if (!parse_dhcp_server(dhcp_server)) {
            PRINTF("dhcp_server configure is invalid.");
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi ap dhcp_server argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("open", argv[index])) {
        if (curr_index == 1) {
          sta_cfg.passwd = "";
        } else if (curr_index == 2){
          ap_cfg.passwd = "";
        } else {
          PRINTF("open must be set after ap or sta");
          ret = false;
          break;
        }
      } else if (is_str_equal("save_cfg", argv[index])){
        save_cfg = true;
      } else if (is_str_equal("wifi", argv[index])) {
        first_loop = true;
        //PRINTF("Find duplicate parameter: %s, ignore!", argv[index]);
      } else if (is_str_equal("eth", argv[index])) {
        --index;
        break;
      } else {
        PRINTF("Unknown wifi parameter: %s!", argv[index]);
        show_help = true;
        break;
      }
      ++index;
    }

    if (ret && wifi_enable) {
      if (mode_cfg.mode == AM_STATION_MODE) {
        if (sta_ifname_index > 0) {
          mode_cfg.ifname = argv[sta_ifname_index];
        }
      } else if (mode_cfg.mode == AM_AP_MODE) {
        if (ap_ifname_index > 0) {
          mode_cfg.ifname = argv[ap_ifname_index];
        }
      } else if (mode_cfg.mode == AM_CONCURRENT_MODE) {
        if (sta_ifname_index > 0) {
          mode_cfg.ifname = argv[sta_ifname_index];
        }
        if (ap_ifname_index > 0) {
          mode_cfg.con_if = argv[ap_ifname_index];
        }
      }
    }
  }

  return ret;
}

static bool parse_eth_parameters(int32_t &index,
                                 int32_t argc,
                                 char **argv)
{
  bool ret = true;
  bool first_loop = true;

  if (2 == argc) {
    show_eth_status = true;
  } else {
    while (index < argc) {
      if (first_loop) {
        if (is_str_equal("on", argv[index])) {
          eth_cfg.enable = true;
          ++index;
        } else if (is_str_equal("off", argv[index])) {
          eth_cfg.enable = false;
          ++index;
        } else {
          eth_cfg.enable = true;
        }
        first_loop = false;
        continue;
      }
      if (is_str_equal("status", argv[index])) {
        show_eth_status = true;
        eth_cfg.ifname = "";
        if (++index < argc) {
          if (is_str_equal("ifname", argv[index])) {
            if (++index < argc) {
              eth_cfg.ifname = argv[index];
            } else {
              PRINTF("Error: eth status ifname argument is missing.");
              ret = false;
            }
          }
        }
        index = argc;
        break;
      } else if (is_str_equal("ifname", argv[index])) {
        if (++index < argc) {
          eth_cfg.ifname = argv[index];
        } else {
          PRINTF("Error: eth ifname argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("ip", argv[index])) {
        if (++index < argc) {
          string ip_string(argv[index]);
          if (!parse_ip_address(ip_string, eth_cfg.ip,
                                eth_cfg.mask, eth_cfg.gw)) {
            PRINTF("eth ip configure %s is invalid.", ip_string.c_str());
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: eth ip argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("dhcp", argv[index])) {
        if (++index < argc) {
          if (is_str_equal("on", argv[index])) {
            sta_cfg.dhcp_enable = true;
          } else if (is_str_equal("off", argv[index])) {
            sta_cfg.dhcp_enable = false;
          } else {
            PRINTF("Error: Unknown wifi dhcp argument: %s", argv[index]);
            ret = false;
            break;
          }
        } else {
          PRINTF("Error: wifi dhcp argument is missing.");
          ret = false;
          break;
        }
      } else if (is_str_equal("save_cfg", argv[index])){
        save_cfg = true;
      } else if (is_str_equal("eth", argv[index])) {
        first_loop = true;
        //PRINTF("Find duplicate parameter: %s, ignore!", argv[index]);
      } else if (is_str_equal("wifi", argv[index])) {
        --index;
        break;
      } else {
        PRINTF("Unknown eth parameter: %s!", argv[index]);
        show_help = true;
        break;
      }
      ++index;
    }
  }

  return ret;
}

static bool init_parameters(int32_t argc, char**argv)
{
  bool ret = true;

  if (argc > 1 && !is_str_equal("help", argv[1])) {
    for (int32_t i = 1; i < argc; ++i) {
      if (is_str_equal("wifi", argv[i])) {
        if (!parse_wifi_parameters(i, argc, argv)) {
          ret = false;
          break;
        }
      } else if (is_str_equal("eth", argv[i])) {
        if (!parse_eth_parameters(i, argc, argv)) {
          ret = false;
          break;
        }
      } else if (is_str_equal("status", argv[i])) {
        show_all_status = true;
        break;
      } else if (is_str_equal("save_cfg", argv[i])){
        save_cfg = true;
      } else {
        PRINTF("Unknown parameter: %s!", argv[i]);
        show_help = true;
      }
      if (show_help) {
        break;
      }
    }
  } else {
    show_help = true;
  }

  return ret;
}

static bool check_network_config()
{
  bool ret = false;

  do {
    if (eth_cfg.enable) {
      if (eth_cfg.ifname.empty()) {
        PRINTF("eth ifname is empty.");
        break;
      }
      if (!eth_cfg.dhcp_enable && eth_cfg.ip.empty()) {
        PRINTF("eth cfg: neither dhcp_enable nor static ip is set!");
        break;
      }
    }

    if (wifi_enable) {
      if (mode_cfg.ifname.empty()) {
        PRINTF("sta ifname is empty.");
        break;
      }

      if (mode_cfg.cfg.empty()) {
        PRINTF("sta ifname is empty.");
        break;
      }
      if (mode_cfg.mode == AM_STATION_MODE) {
        if (sta_cfg.enable) {
          if (sta_cfg.ssid.empty()) {
            PRINTF("sta ssid is empty.");
            break;
          }
          if (!sta_cfg.dhcp_enable && sta_cfg.ip.empty()) {
            PRINTF("sta cfg: neither dhcp_enable nor static ip is set!");
            break;
          }
        } else {
          DEBUG("station is disabled.");
        }
      } else if (mode_cfg.mode == AM_AP_MODE) {
        if (ap_cfg.enable && ap_cfg.ssid.empty()) {
          PRINTF("AP ssid is empty.");
          break;
        }
      } else if (mode_cfg.mode == AM_CONCURRENT_MODE) {
        if (mode_cfg.con_if.empty()) {
          PRINTF("sta ifname is empty.");
          break;
        }
        if (sta_cfg.enable) {
          if (sta_cfg.ssid.empty()) {
            PRINTF("sta ssid is empty.");
            break;
          }
          if (!sta_cfg.dhcp_enable && sta_cfg.ip.empty()) {
            PRINTF("sta cfg: neither dhcp_enable nor static ip is set!");
            break;
          }
        }

        if (ap_cfg.enable && ap_cfg.ssid.empty()) {
          PRINTF("AP ssid is empty.");
          break;
        }
      } else {
        PRINTF("");
        break;
      }
    }
    ret = true;
  } while (0);

  return ret;
}

int32_t main(int32_t argc, char**argv)
{
  int32_t ret = 0;
  AMINetworkManger *ins = nullptr;

  do {
    ins = AMINetworkManger::create();
    if (!ins) {
      ret = -1;
      break;
    }

    ins->get_wifi_mode_cfg(mode_cfg);
    ins->get_eth_cfg(eth_cfg);
    ins->get_sta_cfg(sta_cfg);
    ins->get_ap_cfg(ap_cfg);

    if (!init_parameters(argc, argv)) {
      ret = -1;
      break;
    }

    if (show_help) {
      usage(argc, argv);
      break;
    } else if (list_ap) {
      if (!ins->list_ap()) {
        PRINTF("Failed to list AP(s).");
        ret = -1;
        break;
      }
    } else if (show_wifi_status) {
      if (sta_ifname_index) {
        printf("wifi status:\n");
        if (!ins->wifi_status(argv[sta_ifname_index])) {
          PRINTF("Failed to show wifi %s status.", argv[sta_ifname_index]);
          ret = -1;
          break;
        }
      } else {
        printf("all wifi status:\n");
        if (!ins->wifi_status()) {
          PRINTF("Failed to show (all) wifi status.");
          ret = -1;
          break;
        }
      }
    } else if (show_eth_status) {
      if (eth_cfg.ifname.empty()) {
        printf("all eth status:\n");
        if (!ins->eth_status()) {
          PRINTF("Failed to show all eth status.");
          ret = -1;
          break;
        }
      } else {
        printf("eth status:\n");
        if (!ins->eth_status(eth_cfg.ifname.c_str())) {
          PRINTF("Failed to show eth %s status.", eth_cfg.ifname.c_str());
          ret = -1;
          break;
        }
      }
    } else if (show_all_status) {
      printf("network status:\n");
      if (!ins->all_status()) {
        PRINTF("Failed to show all interface status.");
        ret = -1;
        break;
      }
    } else {
      if (!check_network_config()) {
        ret = -1;
        break;
      }

      ins->set_eth_cfg(eth_cfg);
      ins->set_wifi_enable(wifi_enable);
      ins->set_mode_cfg(mode_cfg);
      ins->set_sta_cfg(sta_cfg);
      ins->set_ap_cfg(ap_cfg);

      if (!ins->network_setup()) {
        ret = -1;
        PRINTF("Failed to set network.");
        break;
      }

      if (save_cfg) {
        if (!ins->save_network_config()) {
          ERROR("Failed to save network config.");
          ret = -1;
          break;
        }
      }
    }

  } while (0);

  delete ins;

  return ret;
}
