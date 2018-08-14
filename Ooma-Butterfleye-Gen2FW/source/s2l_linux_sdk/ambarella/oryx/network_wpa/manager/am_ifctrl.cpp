/*******************************************************************************
 * am_ifctrl.cpp
 *
 * History:
 *   Jun 28, 2016 - [longli] created file
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_log.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#include <linux/sockios.h>
#include <linux/wireless.h>

#include "am_ifctrl.h"

#define _PATH_PROCNET_DEV   "/proc/net/dev"

#define INET_ADDRLEN 4
#define INET6_ADDRLEN 16
#define AUTO_LOCK(mtx)  std::lock_guard<std::mutex> lck(mtx)

AMIfCtrl *AMIfCtrl::m_instance = nullptr;
std::mutex AMIfCtrl::m_mtx;

AMIfCtrl::AMIfCtrl():
        m_ctl_sock(-1),
        m_ctl_sock6(-1),
        m_ref_cnt(0)
{

}

AMIfCtrlPtr AMIfCtrl::get_instance()
{
  AUTO_LOCK(m_mtx);
  DEBUG("AUTO_LOCK(m_mtx)::get_instance() is called.");

  if (!m_instance) {
    m_instance = new AMIfCtrl();
    if (m_instance && m_instance->init()) {
      ERROR("AMIfCtrl::get_instance() init failed.");
      delete m_instance;
      m_instance = NULL;
    }
  }

  return m_instance;
}

void AMIfCtrl::inc_ref()
{
  ++m_ref_cnt;
}

void AMIfCtrl::release()
{
  AUTO_LOCK(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMIfCtrl::~AMIfCtrl()
{
  deinit();
}

in_addr_t AMIfCtrl::prefixLengthToIpv4Netmask(int32_t prefix_length)
{
  in_addr_t mask = 0;

  /* C99 (6.5.7): shifts of 32 bits have undefined results */
  if (prefix_length <= 0 || prefix_length > 32) {
    mask = 0;
  } else {
    mask = ~mask << (32 - prefix_length);
    mask = htonl(mask);
  }

  return mask;
}

int32_t AMIfCtrl::ipv4NetmaskToPrefixLength(in_addr_t mask)
{
  int32_t prefixLength = 0;
  uint32_t m = (uint32_t)ntohl(mask);
  while (m & 0x80000000) {
    prefixLength++;
    m = m << 1;
  }
  return prefixLength;
}

const char *AMIfCtrl::ipaddr_to_string(in_addr_t addr)
{
  struct in_addr in_addr;

  in_addr.s_addr = addr;
  return inet_ntoa(in_addr);
}

int32_t AMIfCtrl::init()
{
  int32_t ret = 0;

  if (m_ctl_sock == -1) {
    m_ctl_sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (m_ctl_sock < 0) {
      ERROR("socket() failed: %s", strerror(errno));
    }
  }

  ret = m_ctl_sock < 0 ? -1 : 0;
  DEBUG("init_returning %d", ret);

  return ret;
}

void AMIfCtrl::deinit()
{
  DEBUG("deinit");
  if (m_ctl_sock != -1) {
    (void)close(m_ctl_sock);
    m_ctl_sock = -1;
  }
}

int32_t AMIfCtrl::init6()
{
  if (m_ctl_sock6 == -1) {
    m_ctl_sock6 = socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (m_ctl_sock6 < 0) {
      ERROR("socket() failed: %s", strerror(errno));
    }
  }
  return m_ctl_sock6 < 0 ? -1 : 0;
}

void AMIfCtrl::deinit6()
{
  if (m_ctl_sock6 != -1) {
    (void)close(m_ctl_sock6);
    m_ctl_sock6 = -1;
  }
}

int32_t AMIfCtrl::init_ifr(const char *name, struct ifreq *ifr)
{
  int32_t ret = 0;

  if (name && name[0] != '\0') {
    strncpy(ifr->ifr_name, name, IFNAMSIZ);
    ifr->ifr_name[IFNAMSIZ - 1] = '\0';
  } else {
    ERROR("ifname is null.");
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::get_hwaddr(const char *name, void *ptr)
{
  int32_t ret = 0;
  int32_t result;
  struct ifreq ifr;

  if (!init_ifr(name, &ifr)) {
    result = ioctl(m_ctl_sock, SIOCGIFHWADDR, &ifr);
    if(result < 0) {
      PERROR("Get hwaddr error.");
      ret = -1;
    } else {
      memcpy(ptr, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    }
  } else {
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::get_ifindex(const char *name, int *if_indexp)
{
  int32_t ret = 0;
  int32_t result;
  struct ifreq ifr;

  if (!init_ifr(name, &ifr)) {
    result = ioctl(m_ctl_sock, SIOCGIFINDEX, &ifr);
    if(result < 0) {
      ret = -1;
    } else {
      *if_indexp = ifr.ifr_ifindex;
    }
  } else {
    ret = -1;
  }
  return ret;
}

int32_t AMIfCtrl::set_flags(const char *name, uint32_t set, uint32_t clr)
{
  AUTO_LOCK(m_mtx);
  struct ifreq ifr;
  int32_t ret = 0;

  if (!init_ifr(name, &ifr)) {
    if(ioctl(m_ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
      ret = -1;
    } else {
      ifr.ifr_flags = (ifr.ifr_flags & (~clr)) | set;
      ret = ioctl(m_ctl_sock, SIOCSIFFLAGS, &ifr);
    }} else {
      ret = -1;
    }

  return ret;
}

int32_t AMIfCtrl::up(const char *name)
{
  int32_t ret = set_flags(name, IFF_UP, 0);
  DEBUG("up(%s) = %d", name, ret);
  return ret;
}

int32_t AMIfCtrl::down(const char *name)
{
  int32_t ret = set_flags(name, 0, IFF_UP);
  DEBUG("down(%s) = %d", name, ret);
  return ret;
}

void AMIfCtrl::init_sockaddr_in(struct sockaddr *sa, in_addr_t addr)
{
  struct sockaddr_in *sin = (struct sockaddr_in *) sa;
  sin->sin_family = AF_INET;
  sin->sin_port = 0;
  sin->sin_addr.s_addr = addr;
}

int32_t AMIfCtrl::set_addr(const char *name, in_addr_t addr)
{
  AUTO_LOCK(m_mtx);
  struct ifreq ifr;
  int32_t ret;

  if (!init_ifr(name, &ifr)) {
    init_sockaddr_in(&ifr.ifr_addr, addr);

    ret = ioctl(m_ctl_sock, SIOCSIFADDR, &ifr);
    DEBUG("set_addr(%s, xx) = %d", name, ret);
  } else {
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::set_hwaddr(const char *name, const void *ptr)
{
  AUTO_LOCK(m_mtx);
  struct ifreq ifr;

  if (!init_ifr(name, &ifr)) {
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(&ifr.ifr_hwaddr.sa_data, ptr, ETH_ALEN);
    return ioctl(m_ctl_sock, SIOCSIFHWADDR, &ifr);
  } else {
    return -1;
  }
}

int32_t AMIfCtrl::set_netmask(const char *name, uint32_t mask)
{
  AUTO_LOCK(m_mtx);
  int32_t ret = 0;
  struct ifreq ifr;
  /* TODO - support ipv6 */

  if (!init_ifr(name, &ifr)) {
    init_sockaddr_in(&ifr.ifr_addr, mask);
    ret = ioctl(m_ctl_sock, SIOCSIFNETMASK, &ifr);
  } else {
    return -1;
  }

  return ret;
}

int32_t AMIfCtrl::get_addr(const char *name, in_addr_t *addr)
{
  struct ifreq ifr;
  int32_t ret = 0;

  if (!init_ifr(name, &ifr)) {
    if (addr != NULL) {
      ret = ioctl(m_ctl_sock, SIOCGIFADDR, &ifr);
      if (ret < 0) {
        *addr = 0;
      } else {
        *addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
      }
    }
  } else {
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::is_wireless(const char *ifname)
{
  int32_t ret = 0;

  if (ifname) {
    struct iwreq wifi;
    strncpy(wifi.ifr_name, ifname, sizeof(wifi.ifr_name) - 1);

    if (ioctl(m_ctl_sock, SIOCGIWNAME, &wifi) < 0) {
      ret = 0;
    } else {
      DEBUG("%s is wifi interface.", ifname);
      ret = 1;
    }
  } else {
    WARN("ifname is null.");
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::get_if_info(struct AMIfBaseInfo *info)
{
  int32_t ret = 0;
  struct ifreq ifr;
  in_addr_t addr;

  if (info && !init_ifr(info->ifname, &ifr)) {
    INFO("ifname=%s", info->ifname);
    if(ioctl(m_ctl_sock, SIOCGIFADDR, &ifr) < 0) {
      addr = 0;
    } else {
      addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
    }
    info->ip = addr;

    strncpy(ifr.ifr_name, info->ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if(ioctl(m_ctl_sock, SIOCGIFNETMASK, &ifr) < 0) {
      addr = 0;
    } else {
      addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
    }
    info->mask = addr;

    strncpy(ifr.ifr_name, info->ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if(ioctl(m_ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
      ifr.ifr_flags = 0;
    }
    info->if_up = ifr.ifr_flags & IFF_UP;
    info->link_connected = ifr.ifr_flags & IFF_RUNNING;

    strncpy(ifr.ifr_name, info->ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(m_ctl_sock, SIOCGIFHWADDR, &ifr) < 0) {
      DEBUG("Get hwaddr error.");
      info->hwaddr[0] = '\0';
    } else {
      char buf[10] = {0};
      strncpy(buf, ifr.ifr_hwaddr.sa_data, 8);
      if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
        snprintf(info->hwaddr, sizeof (info->hwaddr) - 1,
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 (buf[0] & 0377), (buf[1] & 0377), (buf[2] & 0377),
                 (buf[3] & 0377), (buf[4] & 0377), (buf[5] & 0377));
        info->hwaddr[sizeof (info->hwaddr) - 1] = '\0';
      } else if (ifr.ifr_hwaddr.sa_family == ARPHRD_LOOPBACK) {
        info->hwaddr[0] = '\0';
      } else {
        DEBUG("hwaddr type: not supported yet, ignore.\n");
      }
    }
  } else {
    DEBUG("info is null or info->ifname is null.");
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::act_on_ipv4_route(int32_t action, const char *ifname, in_addr_t dst,
                                    in_addr_t netmask, in_addr_t gw)
{
  AUTO_LOCK(m_mtx);
  struct rtentry rt;
  int32_t result;
  char *ifn = (char *)ifname;

  memset(&rt, 0, sizeof(rt));

  rt.rt_dst.sa_family = AF_INET;
  rt.rt_dev = ifn;

  init_sockaddr_in(&rt.rt_genmask, netmask);
  init_sockaddr_in(&rt.rt_dst, dst);
  rt.rt_flags = RTF_UP;

  if (~netmask == 0) {
    rt.rt_flags |= RTF_HOST;
  }

  if (gw != 0) {
    rt.rt_flags |= RTF_GATEWAY;
    init_sockaddr_in(&rt.rt_gateway, gw);
  }

  result = ioctl(m_ctl_sock, action, &rt);
  if (result < 0) {
    if (errno == EEXIST) {
      result = 0;
    } else {
      result = -errno;
    }
  }

  return result;
}

/* deprecated - v4 only */
int32_t AMIfCtrl::create_default_route(const char *name, in_addr_t gw)
{
  in_addr_t in_dst, in_gw;

  in_dst = 0;
  in_gw = gw;

  int32_t ret = act_on_ipv4_route(SIOCADDRT, name, in_dst, 0, in_gw);
  DEBUG("create_default_route(%s, %d) = %d", name, gw, ret);

  return ret;
}

/* Needed by code in hidden partner repositories / branches, so don't delete.*/
int32_t AMIfCtrl::enable(const char *ifname)
{
  int32_t result;

  result = up(ifname);

  return result;
}

// Needed by code in hidden partner repositories / branches, so don't delete.
int32_t AMIfCtrl::disable(const char *ifname)
{
  uint32_t addr, count;
  int32_t result;

  result = down(ifname);

  set_addr(ifname, 0);
  for (count=0, addr=1;((addr != 0) && (count < 255)); count++) {
    if (get_addr(ifname, &addr) < 0) {
      break;
    }
    if (addr) {
      set_addr(ifname, 0);
    }
  }

  return result;
}

/*
 * Removes the default route for the named interface.
 */
int32_t AMIfCtrl::remove_default_route(const char *ifname)
{
  AUTO_LOCK(m_mtx);
  struct rtentry rt;
  int32_t result;
  char *ifn = (char *)ifname;

  memset(&rt, 0, sizeof(rt));
  rt.rt_dev = ifn;
  rt.rt_flags = RTF_UP|RTF_GATEWAY;
  init_sockaddr_in(&rt.rt_dst, 0);
  if ((result = ioctl(m_ctl_sock, SIOCDELRT, &rt)) < 0) {
    ERROR("failed to remove default route for %s: %s", ifname, strerror(errno));
  }

  return result;
}

/* ipv4 */
int32_t AMIfCtrl::generate_dns_cfg(in_addr_t dns1, in_addr_t dns2)
{
  int32_t ret = 0;
  FILE *fp = NULL;
  char line_str[32] = {0};

  if ((dns1 || dns2) && (fp = fopen("/etc/resolv.conf", "w+"))) {

    if (dns1) {
      snprintf(line_str, sizeof(line_str) - 1,"nameserver %s",
               ipaddr_to_string(dns1));
      if (fputs(line_str, fp) > 0) {
        fputs("\n", fp);
      } else {
        printf("generate dns1 error\n");
        ret = -1;
      }
    }

    if (dns2) {
      snprintf(line_str, sizeof(line_str) - 1,"nameserver %s",
               ipaddr_to_string(dns2));
      if (fputs(line_str, fp) > 0) {
        fputs("\n", fp);
      } else {
        printf("generate dns1 error\n");
        ret = -1;
      }
    }

    fclose(fp);
  }

  return ret;
}

void AMIfCtrl::parse_if_name(char *name, char *p)
{
  while (isspace(*p))
    p++;
  while (*p) {
    if (isspace(*p)) {
      break;
    }
    if (*p == ':') {  /* could be an alias */
      char *dot = p, *dotname = name;
      *name++ = *p++;
      while (isdigit(*p)) {
        *name++ = *p++;
      }
      if (*p != ':') {  /* it wasn't, backup */
        p = dot;
        name = dotname;
      }
      break;
    }
    *name++ = *p++;
  }
  *name = '\0';
}

int32_t AMIfCtrl::print_if_info(const char *ifname)
{
  int32_t ret = 0;
  struct AMIfBaseInfo eth_info;
  struct in_addr in_addr;

  if (ifname) {
    strncpy(eth_info.ifname, ifname, sizeof(eth_info.ifname) - 1);
    eth_info.ifname[sizeof(eth_info.ifname) - 1] = '\0';

    if (get_if_info(&eth_info)) {
      ret = -1;
    } else {
      printf("interface=%s\n", eth_info.ifname);
      if (eth_info.link_connected == -1) {
        printf("link=unknown\n");
      } else {
        printf("link=%s\n", eth_info.link_connected ? "on" : "off");
      }
      printf("interface_up=%s\n", eth_info.if_up ? "yes" : "no");
      in_addr.s_addr = eth_info.ip;
      printf("ip_address=%s\n", inet_ntoa(in_addr));
      in_addr.s_addr = eth_info.mask;
      printf("mask_address=%s\n", inet_ntoa(in_addr));
      printf("hwaddr=%s\n\n", eth_info.hwaddr);
    }
  } else {
    ERROR("ifname is null.");
    ret = -1;
  }

  return ret;
}

int32_t AMIfCtrl::print_all_if_info()
{
  FILE *fh;
  char buf[512];
  char name[IFNAMSIZ];
  int32_t ret = 0;

  do {
    fh = fopen(_PATH_PROCNET_DEV, "r");
    if (!fh) {
      fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
              _PATH_PROCNET_DEV, strerror(errno));
      ret = -1;
      break;
    }
    fgets(buf, sizeof buf, fh); /* eat line */
    fgets(buf, sizeof buf, fh);

    while (fgets(buf, sizeof buf, fh)) {
      parse_if_name(name, buf);
      DEBUG("interface=%s", name);
      if (print_if_info(name)) {
        INFO("print %s info error.", name);
      }
    }
    if (ferror(fh)) {
      perror(_PATH_PROCNET_DEV);
      ret = -1;
    }
    fclose(fh);
  } while (0);

  return ret;
}

int32_t AMIfCtrl::print_all_eth_info()
{
  FILE *fh;
  char buf[512];
  char name[IFNAMSIZ];
  int32_t ret = 0;

  do {
    fh = fopen(_PATH_PROCNET_DEV, "r");
    if (!fh) {
      fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
              _PATH_PROCNET_DEV, strerror(errno));
      ret = -1;
      break;
    }
    fgets(buf, sizeof buf, fh); /* eat line */
    fgets(buf, sizeof buf, fh);

    while (fgets(buf, sizeof buf, fh)) {
      parse_if_name(name, buf);
      if (!is_wireless(name)) {
        if (print_if_info(name)) {
          INFO("print %s info error.", name);
        }
      }
    }
    if (ferror(fh)) {
      perror(_PATH_PROCNET_DEV);
      ret = -1;
    }
    fclose(fh);
  } while (0);

  return ret;
}

int32_t AMIfCtrl::configure_ifc(const char *ifname,
                                in_addr_t address,
                                in_addr_t netmask,
                                in_addr_t gateway)
{
  int32_t ret = -1;

  do {
    if (up(ifname)) {
      ERROR("failed to up interface %s: %s", ifname, strerror(errno));
      break;
    }
    if (set_addr(ifname, address)) {
      ERROR("failed to set ipaddr %s: %s", ipaddr_to_string(address), strerror(errno));
      break;
    }
    if (set_netmask(ifname, netmask)) {
      ERROR("failed to set mask %s: %s", ipaddr_to_string(netmask), strerror(errno));
      break;
    }
    if (create_default_route(ifname, gateway)) {
      ERROR("failed to set default route %s: %s", ipaddr_to_string(gateway), strerror(errno));
      break;
    }

    ret = 0;
  } while (0);

  return ret;
}
