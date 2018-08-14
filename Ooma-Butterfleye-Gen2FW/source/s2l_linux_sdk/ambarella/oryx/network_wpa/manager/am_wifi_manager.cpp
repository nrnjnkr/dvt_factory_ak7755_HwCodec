/*******************************************************************************
 * am_wifi_manager.cpp
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

#include "am_base_include.h"
#include "am_log.h"
#include <poll.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>

#include "wpa_ctrl.h"
#include "am_wifi_manager.h"

#define PATH_LEN 128
#define WPA_SUPPLICANT "wpa_supplicant"
#define WPA_CTRL_DIR "/var/run/wpa_supplicant/"
#define PROC_DIRECTORY "/proc"

AMWPACtrl::AMWPACtrl():
  m_sta_net_id(-1),
  m_ap_net_id(-1),
  ctrl_conn(NULL),
  monitor_conn(NULL)
{
  exit_sockets[0] = -1;
  exit_sockets[1] = -1;
}

AMWPACtrl::~AMWPACtrl()
{
  DEBUG("~AMWPACtrl() called.");
  close_sockets();
}

void AMWPACtrl::close_sockets()
{
  if (ctrl_conn != NULL) {
    wpa_ctrl_close(ctrl_conn);
    ctrl_conn = NULL;
  }

  if (monitor_conn != NULL) {
    wpa_ctrl_close(monitor_conn);
    monitor_conn = NULL;
  }

  if (exit_sockets[0] >= 0) {
    close(exit_sockets[0]);
    exit_sockets[0] = -1;
  }

  if (exit_sockets[1] >= 0) {
    close(exit_sockets[1]);
    exit_sockets[1] = -1;
  }
}

AMWPACtrl* AMWPACtrl::create(const std::string &ifname)
{
  AMWPACtrl *ins = NULL;

  if (!ifname.empty()) {
    ins = new AMWPACtrl();
    if (ins && !ins->init_connection(ifname)) {
      ERROR("AMDhClient::create failed.");
      delete ins;
      ins = NULL;
    }
  }

  return ins;
}

bool AMWPACtrl::init_connection(const std::string &ifname)
{
  char path[PATH_LEN] = {0};
  bool ret = true;

  do {
    if (ifname.empty()) {
      ERROR("ifname is empty!");
      ret = false;
      break;
    }

    if((ifname.length() + 25) >= sizeof(path)) {
      ERROR("ifname(%s) is too long", ifname.c_str());
      ret = false;
      break;
    }

    m_wpa_ifname = ifname;
    snprintf(path, sizeof(path), "%s%s", WPA_CTRL_DIR, ifname.c_str());
    path[sizeof(path) - 1] = '\0';

    ctrl_conn = wpa_ctrl_open(path);
    if (ctrl_conn == NULL) {
      ERROR("\"%s\": %s, wpa_supplicant may be not run?",
            path, strerror(errno));
      ret = false;
      break;
    }
    monitor_conn = wpa_ctrl_open(path);
    if (monitor_conn == NULL) {
      ERROR("Failed to open monitor connection");
      ret = false;
      break;
    }
    if (wpa_ctrl_attach(monitor_conn) != 0) {
      ERROR("Failed to set monitor connection");
      ret = false;
      break;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, exit_sockets) == -1) {
      ERROR("Failed to open control socket");
      ret = false;
      break;
    }
  } while (0);

  if (!ret) {
    if (ctrl_conn) {
      wpa_ctrl_close(ctrl_conn);
      ctrl_conn = NULL;
    }
    if (monitor_conn) {
      wpa_ctrl_close(monitor_conn);
      monitor_conn = NULL;
    }
  }

  return ret;
}

bool AMWPACtrl::is_wpa_alive(std::string &pid_str)
{
  struct dirent* dir_entry = NULL;
  char file_path[PATH_LEN] = {0};
  char line_str[PATH_LEN] = {0};
  DIR *dir_proc = NULL;
  FILE *fp = NULL;
  char *p_str = NULL;
  bool ret = false;

  do {
    pid_str.clear();
    dir_proc = opendir(PROC_DIRECTORY);
    if (dir_proc == NULL) {
      ERROR("Couldn't open the %s directory", PROC_DIRECTORY);
      ret = false;
      break;
    }

    while ((dir_entry = readdir(dir_proc))) {
      if (dir_entry->d_type == DT_DIR) {
        snprintf(file_path, sizeof(file_path) - 1, "%s/%s/cmdline",
                 PROC_DIRECTORY, dir_entry->d_name);
        if (access(file_path, F_OK)) {
          continue;
        }

        fp = fopen(file_path, "rt");
        if (fp) {
          if(fgets(line_str, sizeof(line_str) - 1, fp)) {
            //printf("line_str=%s\n", line_str);
            p_str = strstr(line_str, WPA_SUPPLICANT);
            if (p_str) {
              pid_str = pid_str + PROC_DIRECTORY + "/" + dir_entry->d_name;
              ret = true;
            }
          }
          fclose(fp);
          if (ret) {
            break;
          }
        }
      }
    }
    closedir(dir_proc);
  } while (0);

  return ret;
}

bool AMWPACtrl::send_command(const char *cmd, char *reply,
                             size_t *reply_len)
{
  int32_t retv = 0;
  bool ret = true;

  do {
    if (ctrl_conn == NULL) {
      ERROR("Not connected to wpa_supplicant - \"%s\" command dropped.\n",
            cmd);
      ret = false;
      break;
    }
    *reply_len = (*reply_len) - 1;
    retv = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd),
                            reply, reply_len, NULL);
    if (retv == -2) {
      WARN("'%s' command timed out.\n", cmd);
      /* unblocks the monitor receive socket for termination */
      TEMP_FAILURE_RETRY(write(exit_sockets[0], "T", 1));
      ret = false;
      break;
    } else if (retv < 0 || strncmp(reply, "FAIL", 4) == 0) {
      ret = false;
      break;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
      reply[*reply_len] = '\0';
    }
  } while (0);

  return ret;
}

int32_t AMWPACtrl::ctrl_recv(char *reply, size_t *reply_len)
{
  int32_t res;
  int32_t ctrlfd = -1;
  struct pollfd rfds[2];
  std::string pid_path("");

  do {
    ctrlfd = wpa_ctrl_get_fd(monitor_conn);

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = exit_sockets[1];
    rfds[1].events |= POLLIN;

    res = TEMP_FAILURE_RETRY(poll(rfds, 2, 30000));
    if (res < 0) {
      WARN("Error poll = %d", res);
      break;
    } else if (res == 0) {
      if (pid_path.empty()) {
        /* timed out, check if supplicant is active
         * or not ..
         */
        if (!is_wpa_alive(pid_path)) {
          res = -2;
          break;
        }
      } else {
        if (access(pid_path.c_str(), F_OK)) {
          res = -2;
          break;
        }
      }
    }
  } while (res == 0);

  if (res > 0 && (rfds[0].revents & POLLIN)) {
    res = wpa_ctrl_recv(monitor_conn, reply, reply_len);
    DEBUG("raw:\n%s", reply);
  }

  /* it is not rfds[0], then it must be rfts[1] (i.e. the exit socket)
   * or we timed out. In either case, this call has failed ..
   */
  return res;
}

int32_t AMWPACtrl::wait_for_event(char *buf, size_t buflen)
{
  size_t nread = buflen - 1;
  int32_t result = 0;
  char *match, *match2;

  do {

    if (monitor_conn == NULL) {
      snprintf(buf, buflen - 1, "IFNAME=%s %s - connection closed",
               m_wpa_ifname.c_str(), WPA_EVENT_TERMINATING);
      result = 1;
      break;
    }

    result = ctrl_recv(buf, &nread);

    /* Terminate reception on exit socket */
    if (result == -2) {
      snprintf(buf, buflen - 1, "IFNAME=%s %s - connection closed",
               m_wpa_ifname.c_str(), WPA_EVENT_TERMINATING);
      result = 1;
      break;
    }

    if (result < 0) {
      ERROR("wifi_ctrl_recv failed: %s\n", strerror(errno));
      snprintf(buf, buflen - 1, "IFNAME=%s %s - recv error",
               m_wpa_ifname.c_str(), WPA_EVENT_TERMINATING);
      result = 2;
      break;
    }
    buf[nread] = '\0';
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
      /* Fabricate an event to pass up */
      DEBUG("Received EOF on supplicant socket\n");
      snprintf(buf, buflen - 1, "IFNAME=%s %s - signal 0 received",
               m_wpa_ifname.c_str(), WPA_EVENT_TERMINATING);
      result = 3;
      break;
    }
    /*
     * Events strings are in the format
     *
     *     IFNAME=iface <N>CTRL-EVENT-XXX
     *        or
     *     <N>CTRL-EVENT-XXX
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */
    if (strncmp(buf, "IFNAME=", 7) == 0) {
      match = strchr(buf, ' ');
      if (match != NULL) {
        if (match[1] == '<') {
          match2 = strchr(match + 2, '>');
          if (match2 != NULL) {
            nread -= (match2 - match);
            memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
          }
        }
      } else {
        snprintf(buf, buflen - 1, "CTRL-EVENT-IGNORE ");
        break;
      }
    } else if (buf[0] == '<') {
      match = strchr(buf, '>');
      if (match != NULL) {
        nread -= (match + 1 - buf);
        memmove(buf, match + 1, nread + 1);
        NOTICE("supplicant generated event without interface - %s\n", buf);
      }
    } else {
      /* let the event go as is! */
      NOTICE("supplicant generated event without interface and without message level - %s\n", buf);
    }
    result = 0;
  } while (0);

  return result;
}

int32_t AMWPACtrl::add_network()
{
  char buf[128] = {0};
  size_t len = sizeof(buf);
  int32_t net_id = -1;

  if (!send_command("ADD_NETWORK", buf, &len) || !isdigit(buf[0])) {
    net_id = -1;
  } else {
    net_id = atoi(buf);
    DEBUG("add_network id=%d", net_id);
  }

  return net_id;
}

bool AMWPACtrl::remove_network(const int32_t net_id)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  INFO("remove_network id=%d", net_id);
  res = snprintf(cmd, cmdlen - 1, "%s %d",
                 "REMOVE_NETWORK", net_id);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_ssid(const int32_t net_id, const std::string &ssid)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[256] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  res = snprintf(cmd, cmdlen - 1, "%s %d %s \"%s\"",
                 "SET_NETWORK", net_id,
                 "ssid", ssid.c_str());
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_psk(const int32_t net_id, const std::string &psk)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[128] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  res = snprintf(cmd, cmdlen - 1, "%s %d %s \"%s\"",
                 "SET_NETWORK", net_id,
                 "psk", psk.c_str());
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_channel(const int32_t net_id,
                            const int32_t channel)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;
  int32_t frequency = 2412; /* 2.4G channel 1*/

  if (channel < 1 || channel > 196) {
    WARN("Wifi Channel %d is wrong (1 ~ 196), "
        "using Channel 1 by default.", channel);
  } else {
    if (channel < 14) {
      frequency = channel * 5 + 2407;
    } else {
      frequency = channel * 5 + 5000;
    }
  }

  res = snprintf(cmd, cmdlen - 1, "%s %d %s %d",
                 "SET_NETWORK", net_id,
                 "frequency", frequency);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_scan_ssid(const int32_t net_id,
                              const AM_SCAN_SSID_MODE mode)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  res = snprintf(cmd, cmdlen - 1, "%s %d %s %d",
                 "SET_NETWORK", net_id,
                 "scan_ssid", mode);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_ap_scan(const AM_AP_SCAN_MODE scan_mode)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  INFO("set_ap_scan mode%d",scan_mode);
  res = snprintf(cmd, cmdlen - 1, "%s %d",
                 "AP_SCAN", scan_mode);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::enable_network(const int32_t net_id)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  INFO("enable_network id=%d", net_id);
  res = snprintf(cmd, cmdlen - 1, "%s %d",
                 "ENABLE_NETWORK", net_id);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::disable_network(const int32_t net_id)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  INFO("disable_network id=%d", net_id);
  res = snprintf(cmd, cmdlen - 1, "%s %d",
                 "DISABLE_NETWORK", net_id);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::disconnect()
{
  char buf[128] = {0};
  size_t len = sizeof(buf);
  bool  ret = true;

  //list_network(char *reply, size_t *reply_len)
  if (!send_command("DISCONNECT", buf, &len)) {
    ret = false;
  }

  return ret;
}

bool AMWPACtrl::sta_link_reconnect()
{
  char buf[128] = {0};
  size_t len = sizeof(buf);
  bool  ret = true;

  if (!send_command("RECONNECT", buf, &len)) {
    ret = false;
  }

  return ret;
}

bool AMWPACtrl::select_network(const int32_t net_id)
{
  bool ret = true;
  char buf[128] = {0};
  char cmd[32] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  INFO("select_network id=%d", net_id);
  res = snprintf(cmd, cmdlen, "%s %d",
                 "SELECT_NETWORK", net_id);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }
  return ret;
}

bool AMWPACtrl::list_network(char *reply, size_t *reply_len)
{
  bool  ret = true;

  if (!send_command("LIST_NETWORKS", reply, reply_len)) {
    ret = false;
  }

  return ret;
}

bool AMWPACtrl::sta_link_disconnect()
{
  bool ret = true;

  ret = disconnect();

  return ret;
}

bool AMWPACtrl::scan_results(char *reply, size_t *reply_len)
{
  bool ret = true;
  size_t r_len = *reply_len;
  int32_t times = 0;

  do {
    if (!send_command("SCAN", reply, reply_len)) {
      ret = false;
      break;
    }

    if (strncmp(reply, "OK", 2)) {
      ret = false;
      break;
    }

    memset(reply, 0 , r_len);

    do {
      *reply_len = r_len;
      ++times;
      usleep(500000);
      if (!send_command("SCAN_RESULTS", reply, reply_len)) {
        ret = false;
        break;
      }
    } while (*reply_len < 64 && times < 10);
    DEBUG("times=%d\n%s", times, reply);
  } while (0);

  return ret;
}

bool AMWPACtrl::set_key_mgmt(const int32_t net_id, const std::string &key_mgmt)
{
  bool ret =true;
  char buf[128] = {0};
  char cmd[256] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  res = snprintf(cmd, cmdlen - 1, "%s %d %s %s",
                 "SET_NETWORK", net_id,
                 "key_mgmt", key_mgmt.c_str());
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::set_wpa_mode(const int32_t net_id, const AM_WPA_MODE mode)
{
  bool ret =true;
  char buf[128] = {0};
  char cmd[256] = {0};
  size_t buflen = sizeof(buf);
  int32_t cmdlen = sizeof(cmd);
  int32_t res = 0;

  res = snprintf(cmd, cmdlen - 1, "%s %d %s %d",
                 "SET_NETWORK", net_id,
                 "mode", mode);
  if (res < 0 || res >= (cmdlen - 1)) {
    ERROR("Too long command\n");
    ret = false;
  } else {
    cmd[cmdlen - 1] = '\0';
    if (!send_command(cmd, buf, &buflen)) {
      ret = false;
    }
  }

  return ret;
}

bool AMWPACtrl::ignore_event(const char *buf)
{
  bool ret = false;

  if (!strncmp(buf, "CTRL-EVENT-BSS-ADDED", 20) ||
      !strncmp(buf, "CTRL-EVENT-BSS-REMOVED", 22)) {
    ret = true;
  }

  return ret;
}

int32_t AMWPACtrl::process_event(const char *buf)
{
  int32_t ret = 0;
  char *buf_ptr = (char*)buf;

  do {
    if (ignore_event(buf)) {
      break;
    }

    if (!strncmp(buf, "CTRL-EVENT-CONNECTED", 20)) {
      DEBUG("link connected");
      ret = 1;
      break;
    }

    if (!strncmp(buf, "CTRL-EVENT-NETWORK-NOT-FOUND", 28)) {
      PRINTF("NETWORK-NOT-FOUND");
      ret = 2;
      break;
    }

    if (!strncmp(buf, "CTRL-EVENT-SSID-TEMP-DISABLED", 29)) {
      DEBUG("CTRL-EVENT-SSID-TEMP-DISABLED");
      ret = 4;

      char *ptr = strstr(buf_ptr + 29, "reason=");
      if (ptr) {
        ptr += 7;
        if (!strncmp(ptr, "WRONG_KEY", 9)) {
          PRINTF("wrong password");
          ret = 3;
        } else {
          DEBUG("reason: %s", ptr);
        }
      }

      break;
    }

    if (!strncmp(buf, "CTRL-EVENT-SCAN-FAILED", 22)) {
      DEBUG("Invalid Scan Command");
      ret = 5;
      break;
    }


    DEBUG("EVENT: %s", buf);
  } while (0);

  return ret;
}

bool AMWPACtrl::remove_current_ap_instance()
{
  bool ret = true;
  if (m_ap_net_id > -1) {
    if (!remove_network(m_ap_net_id)) {
      WARN("failed to remove wpa net_id");
      ret = false;
    } else {
      m_ap_net_id = -1;
    }
  }
  return ret;
}

bool AMWPACtrl::remove_current_sta_instance()
{
  bool ret = true;

  if (m_sta_net_id > -1) {
    if (!remove_network(m_sta_net_id)) {
      WARN("failed to remove wpa net_id");
      ret = false;
    } else {
      m_sta_net_id = -1;
    }
  }

  return ret;
}

bool AMWPACtrl::remove_all_network()
{
  bool ret = true;
  char buf[4096] = {0};
  char *ptr = buf;
  size_t len = sizeof(buf);
  int32_t net_id = -1;;

  do {
    if (!list_network(buf, &len)) {
      WARN("Failed to get net_id list");
      ret = false;
      break;
    }

    while ((ptr = strchr(ptr, '\n')) && *(++ptr)) {
      if (isdigit(*ptr)) {
        if (sscanf(ptr, "%d ", &net_id) != EOF) {
          if (remove_network(net_id)) {
            WARN("Failed to remove net_id: %d", net_id);
            ret= false;
          }
        }
      }
    }
  } while (0);

  return ret;
}

bool AMWPACtrl::link_status(char *reply, size_t *reply_len)
{
  bool  ret = true;

  if (!send_command("STATUS", reply, reply_len)) {
    ret = false;
  }

  return ret;
}

bool AMWPACtrl::ap_setup(const std::string &ssid,
                         const std::string &psk,
                         const int32_t channel)
{
  bool ret = true;
  int32_t net_id = -1;
  int32_t retv = 0;
  std::string key_mgmt;
  char buf[4096] = {0};
  size_t len = 0;

  do {
    if (!set_ap_scan(AM_AP_SCAN_MODE_2)) {
      ret = false;
      break;
    }

    net_id = add_network();
    if (net_id < 0) {
      ret = false;
      break;
    }
    m_ap_net_id = net_id;

    if (!set_wpa_mode(net_id, WPA_AP_MODE)) {
      ret = false;
      break;
    }

    if (!set_ssid(net_id, ssid)) {
      ret = false;
      break;
    }
    /* TODO: key_mgmt */
    if (!psk.empty()) {
      key_mgmt = "WPA-PSK";
      if (!set_key_mgmt(net_id, key_mgmt)) {
        ret = false;
        break;
      }
      if (!set_psk(net_id, psk)) {
        ret = false;
        break;
      }
    } else {
      key_mgmt = "NONE";
      if (!set_key_mgmt(net_id, key_mgmt)) {
        ret = false;
        break;
      }
    }

    if (!set_channel(net_id, channel)) {
      ret = false;
      break;
    }

    if (!select_network(net_id)) {
      ret = false;
      break;
    }

    len = sizeof(buf);
    for (;;) {
      if(wait_for_event(buf, len)) {
        break;
      }
      retv = process_event(buf);
      if (retv > 0) {
        if (retv != 1) {
          remove_network(net_id);
          ret = false;
        }
        break;
      }
    }

  } while (0);

  return ret;
}

bool AMWPACtrl::sta_link_connect(const std::string &ssid,
                                 const std::string &psk,
                                 const bool hidden_ssid)
{
  bool ret = true;
  int32_t net_id = -1;
  int32_t retv = 0;
  std::string key_mgmt;
  char buf[4096] = {0};
  size_t len = 0;

  do {
    if (!set_ap_scan(AM_AP_SCAN_MODE_1)) {
      ret = false;
      break;
    }

    net_id = add_network();
    if (net_id < 0) {
      ret = false;
      break;
    }
    m_sta_net_id = net_id;

    if (!set_wpa_mode(net_id, WPA_STATION_MODE)) {
      ret = false;
      break;
    }

    if (!set_ssid(net_id, ssid)) {
      ret = false;
      break;
    }

    if (hidden_ssid) {
      DEBUG("connect to hidden ssid %s", ssid.c_str());
      if (!set_scan_ssid(net_id, AM_SCAN_SSID_MODE_1)) {
        ret = false;
        break;
      }
    } else {
      if (!set_scan_ssid(net_id, AM_SCAN_SSID_MODE_0)) {
        ret = false;
        break;
      }
    }

    /* TODO: key_mgmt */
    if (!psk.empty()) {
      if (!set_psk(net_id, psk)) {
        ret = false;
        break;
      }
    } else {
      key_mgmt = "NONE";
      if (!set_key_mgmt(net_id, key_mgmt)) {
        ret = false;
        break;
      }
    }

    if (!select_network(net_id)) {
      ret = false;
      break;
    }

    len = sizeof(buf);
    for (;;) {
      if(wait_for_event(buf, len)) {
        break;
      }
      retv = process_event(buf);
      if (retv > 0) {
        if (retv != 1) {
          remove_network(net_id);
          ret = false;
        }
        break;
      }
    }

  } while (0);

  return ret;
}

AMWifiManager::AMWifiManager():
    m_mode(AM_NONE),
    m_pre_mode(AM_NONE),
    m_wpa_ctrl(nullptr),
    m_wpa_ifname(""),
    m_wpa_config(""),
    m_con_ctrl(nullptr),
    m_con_ifname(""),
    m_con_config("")
{

}

void AMWifiManager::deinit()
{
  m_mode = AM_NONE;
  delete m_wpa_ctrl;
  m_wpa_ctrl = nullptr;
  m_wpa_ifname = "";
  m_wpa_config = "";
  delete m_con_ctrl;
  m_con_ctrl = nullptr;
  m_con_ifname = "";
  m_con_config = "";
}

AMWifiManager::~AMWifiManager()
{
  DEBUG("~AMWifiManager() called.");
  deinit();
}

bool AMWifiManager::set_mode_cfg(const AM_WIFI_MODE mode,
                                 const std::string &wpa_ifname,
                                 const std::string &wpa_config,
                                 const std::string &con_ifname,
                                 const std::string &con_config)
{
  bool ret = true;

  INFO("wifi mode = %d", mode);
  m_pre_mode = m_mode;
  deinit();
  m_mode = mode;
  m_wpa_ifname = wpa_ifname;
  m_wpa_config = wpa_config;
  m_con_ifname = con_ifname;
  m_con_config = con_config;

  if (mode == AM_STATION_MODE || mode == AM_AP_MODE) {
    if (start_wpa_supplicant()) {
      m_wpa_ctrl = AMWPACtrl::create(wpa_ifname);
      if (!m_wpa_ctrl) {
        ret = false;
      }
    } else {
      ret = false;
    }
  } else if (mode == AM_CONCURRENT_MODE) {
    if (start_wpa_supplicant()) {
      m_wpa_ctrl = AMWPACtrl::create(wpa_ifname);
      m_con_ctrl = AMWPACtrl::create(con_ifname);
      if (!m_wpa_ctrl || !m_con_ctrl) {
        ret = false;
      }
    }
  } else {
    ret = false;
    ERROR("Unknown mode!");
  }

  if (!ret) {
    m_mode = AM_NONE;
  }

  return ret;
}

bool AMWifiManager::is_wpa_supplicant_run()
{
  struct dirent* dir_entry = NULL;
  char file_path[PATH_LEN] = {0};
  char line_str[PATH_LEN] = {0};
  DIR *dir_proc = NULL;
  FILE *fp = NULL;
  char *p_str = NULL;
  bool ret = false;

  do {
    dir_proc = opendir(PROC_DIRECTORY);
    if (dir_proc == NULL) {
      ERROR("Couldn't open the %s directory", PROC_DIRECTORY);
      ret = false;
      break;
    }

    while ((dir_entry = readdir(dir_proc))) {
      if (dir_entry->d_type == DT_DIR) {
        snprintf(file_path, sizeof(file_path) - 1, "%s/%s/cmdline",
                 PROC_DIRECTORY, dir_entry->d_name);
        if (access(file_path, F_OK)) {
          continue;
        }

        fp = fopen(file_path, "rt");
        if (fp) {
          if(fgets(line_str, sizeof(line_str) - 1, fp)) {
            //printf("line_str=%s\n", line_str);
            p_str = strstr(line_str, WPA_SUPPLICANT);
            if (p_str) {
              ret = true;
            }
          }
          fclose(fp);
          if (ret) {
            break;
          }
        }
      }
    }
    closedir(dir_proc);
  } while (0);

  return ret;
}

bool AMWifiManager::set_mode(const AM_WIFI_MODE mode)
{
  bool ret = true;

  m_pre_mode = m_mode;
  m_mode = mode;

  return ret;
}

bool AMWifiManager::start_wpa_supplicant()
{
  bool ret = true;
  std::string cmd_line(WPA_SUPPLICANT);
  std::string pid_str;

  do {
    INFO("call start_wpa_supplicant.");
    if (m_mode == AM_STATION_MODE || m_mode == AM_AP_MODE) {
      if (m_wpa_ifname.empty() || m_wpa_config.empty() ||
          access(m_wpa_config.c_str(), F_OK)) {
        ERROR("ifname or config path is empty, or config doesn't exist.");
        ret = false;
        break;
      }
      cmd_line = cmd_line + " -c " + m_wpa_config + " -i " +
          m_wpa_ifname + " -B 1>/dev/null";
    } else if (m_mode == AM_CONCURRENT_MODE) {
      if (m_wpa_ifname.empty() || m_con_ifname.empty() ||
          m_wpa_config.empty() || access(m_wpa_config.c_str(), F_OK) ||
          m_con_config.empty() || access(m_con_config.c_str(), F_OK)) {
        ERROR("ifname or config path is empty, or config doesn't exist.");
        ret = false;
        break;
      }
      cmd_line = cmd_line + " -c " + m_wpa_config + " -i " + m_wpa_ifname +
          " -N -c " + m_con_config + " -i " + m_con_ifname + " -B 1>/dev/null";
    } else {
      WARN("wpa_supplicant start up mode is invalid!");
      break;
    }

    stop_wpa_supplicant();

    int32_t status = system(cmd_line.c_str());
    if (WEXITSTATUS(status)) {
      WARN("%s :failed\n", cmd_line.c_str());
      ret = false;
      break;
    }

  } while (0);

  return ret;
}

void AMWifiManager::stop_wpa_supplicant()
{
  int32_t status;

  if (is_wpa_supplicant_run()) {
    status = system("killall -2 wpa_supplicant 2>/dev/null ");
    if (WEXITSTATUS(status)) {
      NOTICE("kill -9 wpa_supplicant.");
      system("killall -9 wpa_supplicant 2>/dev/null");
    }
  }
}

bool AMWifiManager::connect_to_ap(const std::string &ssid,
                                  const std::string &psk,
                                  const bool hidden_ssid)
{
  bool ret = true;

  if (m_mode == AM_STATION_MODE || m_mode == AM_CONCURRENT_MODE) {
    if (m_wpa_ctrl) {
      if (!m_wpa_ctrl->sta_link_connect(ssid, psk, hidden_ssid)) {
        ERROR("Fail to connect to ap");
        ret = false;
      }
    } else {
      ERROR("m_wpa_ctrl not init");
      ret = false;
    }
  } else {
    ERROR("Fail to connect to ap: wrong mode!");
    ret = false;
  }

  return ret;
}

bool AMWifiManager::setup_ap(const std::string &ssid,
                             const std::string &psk,
                             const int32_t channel)
{
  bool ret = true;

  if (m_mode == AM_AP_MODE) {
    if (m_wpa_ctrl) {
      if (!m_wpa_ctrl->ap_setup(ssid, psk, channel)) {
        ERROR("Fail to setup AP");
        ret = false;
      }
    } else {
      ERROR("m_wpa_ctrl not init");
      ret = false;
    }
  } else if (m_mode == AM_CONCURRENT_MODE) {
    if (m_con_ctrl) {
      if (!m_con_ctrl->ap_setup(ssid, psk, channel)) {
        ERROR("Fail to setup AP");
        ret = false;
      }
    } else {
      ERROR("m_con_ctrl not init");
      ret = false;
    }
  } else {
    ERROR("Fail to setup AP: wrong mode!");
    ret = false;
  }

  return ret;
}

bool AMWifiManager::disconnect_to_ap()
{
  bool ret = true;

  if (m_pre_mode == AM_STATION_MODE || m_pre_mode == AM_CONCURRENT_MODE) {
    if (m_wpa_ctrl) {
      if (!m_wpa_ctrl->remove_current_sta_instance()) {
        ERROR("Fail to disconnect to ap");
        ret = false;
      }
    } else {
      ERROR("m_wpa_ctrl not init");
      ret = false;
    }
  }

  return ret;
}

bool AMWifiManager::remove_all_network()
{
  bool ret = true;

  if (m_mode == AM_STATION_MODE || m_mode == AM_CONCURRENT_MODE) {
    if (m_wpa_ctrl) {
      if (!m_wpa_ctrl->remove_all_network()) {
        ERROR("Fail to remove all network");
        ret = false;
      }
    } else {
      ERROR("m_wpa_ctrl not init");
      ret = false;
    }
  } else {
    ERROR("Fail to connect to ap: wrong mode!");
    ret = false;
  }
  return ret;
}

bool AMWifiManager::remove_ap()
{
  bool ret = true;

  if (m_pre_mode == AM_AP_MODE) {
    if (m_wpa_ctrl) {
      if (!m_wpa_ctrl->remove_current_ap_instance()) {
        ERROR("Fail to delete AP");
        ret = false;
      }
    } else {
      ERROR("m_wpa_ctrl not init");
      ret = false;
    }
  } else if (m_pre_mode == AM_CONCURRENT_MODE) {
    if (m_con_ctrl) {
      if (!m_con_ctrl->remove_current_ap_instance()) {
        ERROR("Fail to delete AP");
        ret = false;
      }
    } else {
      ERROR("m_con_ctrl not init");
      ret = false;
    }
  }

  return ret;
}

bool AMWifiManager::list_ap(char *reply, size_t *reply_len)
{
  bool ret = true;

  do {
    if (!reply || !reply_len) {
      ERROR("buf is null or buf length is 0");
      ret = false;
      break;
    }

    if (m_wpa_ctrl) {
      if (m_wpa_ctrl->scan_results(reply, reply_len)) {
        DEBUG("%s", reply);
      } else {
        ERROR("Fail to get ap list");
        ret = false;
      }
    } else {
      AMWPACtrl *wpa_ctrl = nullptr;
      struct dirent* dir_entry = NULL;
      DIR *dir_wpa = nullptr;

      if (access(WPA_CTRL_DIR, F_OK)) {
        ERROR("wpa_supplicant may not run");
        ret = false;
        break;
      }

      dir_wpa = opendir(WPA_CTRL_DIR);
      if (dir_wpa == NULL) {
        ERROR("Couldn't open the %s directory", WPA_CTRL_DIR);
        ret = false;
        break;
      }

      while ((dir_entry = readdir(dir_wpa))) {
        if (dir_entry->d_type != DT_DIR) {
          break;
        }
      }
      closedir(dir_wpa);

      if (dir_entry != NULL) {
        wpa_ctrl = AMWPACtrl::create(dir_entry->d_name);
        if (!wpa_ctrl) {
          ERROR("Failed to initialize wpa_ctrl");
          ret = false;
          break;
        }

        if (wpa_ctrl->scan_results(reply, reply_len)) {
          DEBUG("%s", reply);
        } else {
          ERROR("Fail to get ap list");
          ret = false;
        }
        delete wpa_ctrl;
        break;
      } else {
        ERROR("No wpa_supplicant control interface found.");
        ret = false;
        break;
      }
    }
  } while (0);

  return ret;
}

bool AMWifiManager::wifi_status(char *reply, size_t *reply_len)
{
  bool ret = true;
  size_t total_len = *reply_len;

  do {
    if (!reply || !reply_len) {
      ERROR("buf is null or buf length is 0");
      ret = false;
      break;
    }

    if (m_wpa_ctrl) {
      if (m_wpa_ctrl->link_status(reply, reply_len)) {
        DEBUG("reply_len=%d\n%s", *reply_len, reply);
      } else {
        ERROR("Fail to get wifi status");
        ret = false;
      }

      if (m_mode == AM_CONCURRENT_MODE) {
        size_t offset = (*reply_len) + 1;
        size_t buf_len = 0;

        if (offset < total_len) {
          buf_len = total_len - offset;
          reply[offset - 1] = '\n';
          if (m_con_ctrl->link_status(reply + offset, &buf_len)) {
            PRINTF("reply_len=%d\n%s", buf_len, reply);
            *reply_len = offset + buf_len;
          } else {
            ERROR("Fail to get wifi status");
            ret = false;
          }
        } else {
          ERROR("buf is not enough.");
          ret = false;
        }
      }
    } else {
      struct dirent* dir_entry = NULL;
      DIR *dir_wpa = nullptr;
      size_t offset = 0;
      size_t buf_len = 0;

      if (access(WPA_CTRL_DIR, F_OK)) {
        ERROR("wpa_supplicant may not run");
        ret = false;
        break;
      }

      dir_wpa = opendir(WPA_CTRL_DIR);
      if (dir_wpa == NULL) {
        ERROR("Couldn't open the %s directory", WPA_CTRL_DIR);
        ret = false;
        break;
      }

      while ((dir_entry = readdir(dir_wpa))) {
        if (dir_entry->d_type != DT_DIR) {

          offset =  offset + buf_len;
          if (offset >= total_len) {
            ERROR("Buf is full!");
            ret = false;
            break;
          }
          if (offset != 0) {
            reply[offset] = '\n';
            offset += 1;
          }

          buf_len = total_len - offset;
          if (!wifi_status(dir_entry->d_name, reply + offset, &buf_len)) {
            DEBUG("Failed to get wpa_supplicant status from interface %s.",
                  dir_entry->d_name);
            continue;
          }
        }
      }
      closedir(dir_wpa);

    }

  } while (0);

  return ret;
}

bool AMWifiManager::wifi_status(const char *ifname, char *reply, size_t *reply_len)
{
  bool ret = true;
  AMWPACtrl *wpa_ctrl = nullptr;
  char path[PATH_LEN] = {0};

  do {
    if (!ifname || !reply || !reply_len) {
      ERROR("ifname or buf is null or buf length is 0");
      ret = false;
      break;
    }

    snprintf(path, sizeof(path) - 1, "%s%s", WPA_CTRL_DIR, ifname);
    if (access(path, F_OK)) {
      ERROR("No %s control interface under wpa_supplicant", ifname);
      ret = false;
      break;
    }

    wpa_ctrl = AMWPACtrl::create(ifname);
    if (!wpa_ctrl) {
      INFO("Failed to initialize wpa_ctrl");
      ret = false;
      break;
    }

    if (wpa_ctrl->link_status(reply, reply_len)) {
      DEBUG("reply_len=%d\n%s", *reply_len, reply);
    } else {
      ERROR("Fail to get %s wifi status", ifname);
      ret = false;
    }

    delete wpa_ctrl;

  } while (0);

  return ret;
}

AMWifiManager *AMWifiManager::create()
{
  AMWifiManager *ins = NULL;

  ins = new AMWifiManager();

  return ins;
}
