/*******************************************************************************
 * am_dhcpclient.cpp
 *
 * History:
 *   Jun 30, 2016 - [longli] created file
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "am_dhcpmsg.h"
#include "am_dhclient.h"

#define STATE_SELECTING  1
#define STATE_REQUESTING 2

#define TIMEOUT_INITIAL   2000
#define TIMEOUT_MAX      16000

AMDhClient::AMDhClient():
  m_ifname(""),
  m_ifc(nullptr),
  m_verbose(0)
{

}

AMDhClient* AMDhClient::create(const std::string &ifname)
{
  AMDhClient *ins = NULL;

  if (!ifname.empty()) {
    ins = new AMDhClient();
    if (ins && ins->init(ifname)) {
      ERROR("AMDhClient::create failed.");
      delete ins;
      ins = NULL;
    }
  }

  return ins;
}

void AMDhClient::set_verbose(const int32_t verbose)
{
  m_verbose = verbose;
}

int32_t AMDhClient::init(const std::string &ifname)
{
  int32_t ret = 0;

  if (!ifname.empty()) {
    m_ifc = AMIfCtrl::get_instance();
    if (!m_ifc) {
      ret = -1;
    } else {
      m_ifname = ifname;
    }
  } else {
    ret = -1;
  }

  return ret;
}

int32_t AMDhClient::do_dhcp()
{
  int32_t ret = 0;

  do {
    if (m_ifc->set_addr(m_ifname.c_str(), 0)) {
      ERROR("Failed to set ip addr for %s to 0.0.0.0: %s\n",
            m_ifname.c_str(), strerror(errno));
      ret = -1;
      break;
    }

    if (m_ifc->enable(m_ifname.c_str())) {
      ERROR("failed to bring up interface %s: %s\n",
            m_ifname.c_str(), strerror(errno));
      ret = -1;
      break;
    }

    ret = dhcp_init_ifc();

  } while (0);

  return ret;
}

msecs_t AMDhClient::get_msecs()
{
  struct timespec ts;
  msecs_t result = 0;

  if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
    result = 0;
  } else {
    result = (((msecs_t) ts.tv_sec) * ((msecs_t) 1000)) +
        (((msecs_t) ts.tv_nsec) / ((msecs_t) 1000000));
  }

  return result;
}

const char *AMDhClient::ipaddr(in_addr_t addr)
{
  struct in_addr in_addr;

  in_addr.s_addr = addr;

  return inet_ntoa(in_addr);
}

int32_t AMDhClient::open_raw_socket(uint8_t *hwaddr, int32_t if_index)
{
  int32_t s;
  struct sockaddr_ll bindaddr;

  do {
    if((s = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) {
      ERROR("socket(PF_PACKET): %s", strerror(errno));
      s = -1;
      break;
    }

    memset(&bindaddr, 0, sizeof(bindaddr));
    bindaddr.sll_family = AF_PACKET;
    bindaddr.sll_protocol = htons(ETH_P_IP);
    bindaddr.sll_halen = ETH_ALEN;
    memcpy(bindaddr.sll_addr, hwaddr, ETH_ALEN);
    bindaddr.sll_ifindex = if_index;

    if (bind(s, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
      ERROR("Cannot bind raw socket to interface: %s", strerror(errno));
      close(s);
      s = -1;
      break;
    }

  } while (0);

  return s;
}

uint32_t AMDhClient::checksum(void *buffer, uint32_t count, uint32_t startsum)
{
  uint16_t *up = (uint16_t *)buffer;
  uint32_t sum = startsum;
  uint32_t upper16;

  while (count > 1) {
    sum += *up++;
    count -= 2;
  }
  if (count > 0) {
    sum += (uint16_t) *(uint8_t *)up;
  }
  while ((upper16 = (sum >> 16)) != 0) {
    sum = (sum & 0xffff) + upper16;
  }
  return sum;
}

uint32_t AMDhClient::finish_sum(uint32_t sum)
{
  return ~sum & 0xffff;
}

void *AMDhClient::init_dhcp_msg(dhcp_msg *msg, int32_t type,
                                void *hwaddr, uint32_t xid)
{
  uint8_t *x;

  memset(msg, 0, sizeof(dhcp_msg));

  msg->op = OP_BOOTREQUEST;
  msg->htype = HTYPE_ETHER;
  msg->hlen = 6;
  msg->hops = 0;

  msg->flags = htons(FLAGS_BROADCAST);

  msg->xid = xid;

  memcpy(msg->chaddr, hwaddr, 6);

  x = msg->options;

  *x++ = OPT_COOKIE1;
  *x++ = OPT_COOKIE2;
  *x++ = OPT_COOKIE3;
  *x++ = OPT_COOKIE4;

  *x++ = OPT_MESSAGE_TYPE;
  *x++ = 1;
  *x++ = type;

  return x;
}

int32_t AMDhClient::init_dhcp_discover_msg(dhcp_msg *msg, void *hwaddr,
                                           uint32_t xid)
{
  uint8_t *x;

  x = (uint8_t *)init_dhcp_msg(msg, DHCPDISCOVER, hwaddr, xid);

  *x++ = OPT_PARAMETER_LIST;
  *x++ = 4;
  *x++ = OPT_SUBNET_MASK;
  *x++ = OPT_GATEWAY;
  *x++ = OPT_DNS;
  *x++ = OPT_BROADCAST_ADDR;

  *x++ = OPT_END;

  return DHCP_MSG_FIXED_SIZE + (x - msg->options);
}

int32_t AMDhClient::init_dhcp_request_msg(dhcp_msg *msg, void *hwaddr,
                                          uint32_t xid, uint32_t ipaddr,
                                          uint32_t serveraddr)
{
  uint8_t *x;

  x = (uint8_t *)init_dhcp_msg(msg, DHCPREQUEST, hwaddr, xid);

  *x++ = OPT_PARAMETER_LIST;
  *x++ = 4;
  *x++ = OPT_SUBNET_MASK;
  *x++ = OPT_GATEWAY;
  *x++ = OPT_DNS;
  *x++ = OPT_BROADCAST_ADDR;

  *x++ = OPT_REQUESTED_IP;
  *x++ = 4;
  memcpy(x, &ipaddr, 4);
  x +=  4;

  *x++ = OPT_SERVER_ID;
  *x++ = 4;
  memcpy(x, &serveraddr, 4);
  x += 4;

  *x++ = OPT_END;

  return DHCP_MSG_FIXED_SIZE + (x - msg->options);
}

int32_t AMDhClient::send_packet(int32_t s, int32_t if_index,
                                dhcp_msg *msg, int32_t size,
                                uint32_t saddr, uint32_t daddr,
                                uint32_t sport, uint32_t dport)
{
  struct iphdr ip;
  struct udphdr udp;
  struct iovec iov[3];
  uint32_t udpsum;
  uint16_t temp;
  struct msghdr msghdr;
  struct sockaddr_ll destaddr;

  ip.version = IPVERSION;
  ip.ihl = sizeof(ip) >> 2;
  ip.tos = 0;
  ip.tot_len = htons(sizeof(ip) + sizeof(udp) + size);
  ip.id = 0;
  ip.frag_off = 0;
  ip.ttl = IPDEFTTL;
  ip.protocol = IPPROTO_UDP;
  ip.check = 0;
  ip.saddr = saddr;
  ip.daddr = daddr;
  ip.check = finish_sum(checksum(&ip, sizeof(ip), 0));

  udp.source = htons(sport);
  udp.dest = htons(dport);
  udp.len = htons(sizeof(udp) + size);
  udp.check = 0;

  /* Calculate checksum for pseudo header */
  udpsum = checksum(&ip.saddr, sizeof(ip.saddr), 0);
  udpsum = checksum(&ip.daddr, sizeof(ip.daddr), udpsum);
  temp = htons(IPPROTO_UDP);
  udpsum = checksum(&temp, sizeof(temp), udpsum);
  temp = udp.len;
  udpsum = checksum(&temp, sizeof(temp), udpsum);

  /* Add in the checksum for the udp header */
  udpsum = checksum(&udp, sizeof(udp), udpsum);

  /* Add in the checksum for the data */
  udpsum = checksum(msg, size, udpsum);
  udp.check = finish_sum(udpsum);

  iov[0].iov_base = (char *)&ip;
  iov[0].iov_len = sizeof(ip);
  iov[1].iov_base = (char *)&udp;
  iov[1].iov_len = sizeof(udp);
  iov[2].iov_base = (char *)msg;
  iov[2].iov_len = size;
  memset(&destaddr, 0, sizeof(destaddr));
  destaddr.sll_family = AF_PACKET;
  destaddr.sll_protocol = htons(ETH_P_IP);
  destaddr.sll_ifindex = if_index;
  destaddr.sll_halen = ETH_ALEN;
  memcpy(destaddr.sll_addr, "\xff\xff\xff\xff\xff\xff", ETH_ALEN);

  msghdr.msg_name = &destaddr;
  msghdr.msg_namelen = sizeof(destaddr);
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = sizeof(iov) / sizeof(struct iovec);
  msghdr.msg_flags = 0;
  msghdr.msg_control = 0;
  msghdr.msg_controllen = 0;

  return sendmsg(s, &msghdr, 0);
}

int32_t AMDhClient::receive_packet(int32_t s, dhcp_msg *msg)
{
  int32_t nread;
  int32_t is_valid;
  int32_t dhcp_size;
  uint32_t sum;
  uint16_t temp;
  uint32_t saddr, daddr;
  struct dhcp_packet {
      struct iphdr ip;
      struct udphdr udp;
      struct dhcp_msg dhcp;
  } packet;

  do {
    nread = read(s, &packet, sizeof(packet));
    if (nread < 0) {
      dhcp_size = -1;
      break;
    }
    /*
     * The raw packet interface gives us all packets received by the
     * network interface. We need to filter out all packets that are
     * not meant for us.
     */
    is_valid = 0;
    if (nread < (int32_t)(sizeof(struct iphdr) + sizeof(struct udphdr))) {
      if (m_verbose) {
        PRINTF("Packet is too small (%d) to be a UDP datagram", nread);
      }

    } else if (packet.ip.version != IPVERSION
        || packet.ip.ihl != (sizeof(packet.ip) >> 2)) {
      if (m_verbose) {
        PRINTF("Not a valid IP packet");
      }
    } else if (nread < ntohs(packet.ip.tot_len)) {
      if (m_verbose) {
        PRINTF("Packet was truncated (read %d, needed %d)",
               nread, ntohs(packet.ip.tot_len));
      }
    } else if (packet.ip.protocol != IPPROTO_UDP) {
      if (m_verbose) {
        PRINTF("IP protocol (%d) is not UDP", packet.ip.protocol);
      }
    } else if (packet.udp.dest != htons(PORT_BOOTP_CLIENT)) {
      if (m_verbose) {
        PRINTF("UDP dest port (%d) is not DHCP client",
               ntohs(packet.udp.dest));
      }
    } else {
      is_valid = 1;
    }
    if (!is_valid) {
      dhcp_size = -1;
      break;
    }

    /* Seems like it's probably a valid DHCP packet */
    /* validate IP header checksum */
    sum = finish_sum(checksum(&packet.ip, sizeof(packet.ip), 0));
    if (sum != 0) {
      NOTICE("IP header checksum failure (0x%x)", packet.ip.check);
      dhcp_size = -1;
      break;
    }
    /*
     * Validate the UDP checksum.
     * Since we don't need the IP header anymore, we "borrow" it
     * to construct the pseudo header used in the checksum calculation.
     */
    dhcp_size = ntohs(packet.udp.len) - sizeof(packet.udp);
    saddr = packet.ip.saddr;
    daddr = packet.ip.daddr;
    nread = ntohs(packet.ip.tot_len);
    memset(&packet.ip, 0, sizeof(packet.ip));
    packet.ip.saddr = saddr;
    packet.ip.daddr = daddr;
    packet.ip.protocol = IPPROTO_UDP;
    packet.ip.tot_len = packet.udp.len;
    temp = packet.udp.check;
    packet.udp.check = 0;
    sum = finish_sum(checksum(&packet, nread, 0));
    packet.udp.check = temp;
    if (!sum)
      sum = finish_sum(sum);
    if (temp != sum) {
      NOTICE("UDP header checksum failure (0x%x should be 0x%x)",
             sum, temp);
      dhcp_size = -1;
      break;
    }

    memcpy(msg, &packet.dhcp, dhcp_size);
  } while (0);

  return dhcp_size;
}

int32_t AMDhClient::dhcp_init_ifc()
{
  dhcp_msg discover_msg;
  dhcp_msg request_msg;
  dhcp_msg reply;
  dhcp_msg *msg = NULL;
  dhcp_info info;
  int32_t s = 0, r = 0, size = 0;
  int32_t valid_reply;
  uint32_t xid;
  uint8_t hwaddr[6];
  struct pollfd pfd;
  uint32_t state;
  uint32_t timeout;
  int32_t if_index;
  int32_t ret = 1;
  int32_t wait_response;
  int32_t count = 1;

  do {
    xid = (uint32_t)get_msecs();

    if (m_ifc->get_hwaddr(m_ifname.c_str(), hwaddr)) {
      ERROR("cannot obtain interface address");
      ret = -1;
      break;
    }
    if (m_ifc->get_ifindex(m_ifname.c_str(), &if_index)) {
      ERROR("cannot obtain interface index");
      ret = -1;
      break;
    }

    s = open_raw_socket(hwaddr, if_index);
    if (s < 0) {
      ret = -1;
      break;
    }

    timeout = TIMEOUT_INITIAL;
    state = STATE_SELECTING;
    info.type = 0;
    wait_response = 0;

    for (;;) {
      if (wait_response) {
        pfd.fd = s;
        pfd.events = POLLIN;
        pfd.revents = 0;
        r = poll(&pfd, 1, timeout);
      }

      if (r == 0 || !wait_response) {
        if (wait_response) {
          if (m_verbose) {
            PRINTF("TIMEOUT\n");
          }
          if (timeout >= TIMEOUT_MAX) {
            INFO("timed out\n");
            if (info.type == DHCPOFFER) {
              INFO("no acknowledgement from DHCP server\n"
                  "configuring %s with offered parameters\n", m_ifname.c_str());
              ret = dhcp_configure(&info);
              break;
            }
            PRINTF("Dhcp get ip address timed out");
            errno = ETIME;
            ret = -1;
            break;
          }
          if (count > 1) {
            timeout = timeout * 2;
            count = 1;
          } else {
            ++count;
          }
        }

        size = 0;
        msg = NULL;
        switch(state) {
          case STATE_SELECTING:
            msg = &discover_msg;
            size = init_dhcp_discover_msg(msg, hwaddr, xid);
            break;
          case STATE_REQUESTING:
            msg = &request_msg;
            size = init_dhcp_request_msg(msg, hwaddr, xid,
                                         info.ipaddr, info.serveraddr);
            break;
          default:
            r = 0;
        }

        if (size != 0) {
          r = send_message(s, if_index, msg, size);
          if (r < 0) {
            NOTICE("error sending dhcp msg: %s\n", strerror(errno));
          }
          wait_response = 1;
        }
        continue;
      }

      if (r < 0) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
          continue;
        }
        ret = -1;
        ERROR("poll failed: %s", strerror(errno));
        break;
      }

      errno = 0;
      r = receive_packet(s, &reply);
      if (r < 0) {
        if (errno != 0) {
          NOTICE("receive_packet failed (%d): %s", r, strerror(errno));
          if (errno == ENETDOWN || errno == ENXIO) {
            ret = -1;
            break;
          }
        }
        continue;
      }

      if (m_verbose) {
        dump_dhcp_msg(&reply, r);
      }

      decode_dhcp_msg(&reply, r, &info);

      if (state == STATE_SELECTING) {
        valid_reply = is_valid_reply(&discover_msg, &reply, r);
      } else {
        valid_reply = is_valid_reply(&request_msg, &reply, r);
      }

      if (!valid_reply) {
        INFO("invalid reply\n");
        continue;
      }

      if (m_verbose) {
        dump_dhcp_info(&info);
      }

      switch(state) {
        case STATE_SELECTING:
          if (info.type == DHCPOFFER) {
            state = STATE_REQUESTING;
            timeout = TIMEOUT_INITIAL;
            xid++;
            wait_response = 0;
          }
          break;
        case STATE_REQUESTING:
          if (info.type == DHCPACK) {
            INFO("configuring %s\n", m_ifname.c_str());
            ret = dhcp_configure(&info);
            break;
          } else if (info.type == DHCPNAK) {
            PRINTF("configuration request denied\n");
            ret = -1;
            break;
          } else {
            DEBUG("ignoring %s message in state %d\n",
                  dhcp_type_to_name(info.type), state);
          }
          break;
      }
      if (ret < 1) {
        break;
      }
    }

    close(s);
  } while (0);

  return ret;
}

int32_t AMDhClient::dhcp_configure(dhcp_info *info)
{
  int32_t ret = 0;

  if (m_ifc->generate_dns_cfg(info->dns1, info->dns2)) {
    printf("generate /etc/resolv.conf error\n");
    ret = -1;
  } else {
    ret = m_ifc->configure_ifc(m_ifname.c_str(), info->ipaddr,
                               info->mask, info->gateway);
  }

  return ret;
}

const char *AMDhClient::dhcp_type_to_name(uint32_t type)
{
  switch(type) {
    case DHCPDISCOVER: return "discover";
    case DHCPOFFER:    return "offer";
    case DHCPREQUEST:  return "request";
    case DHCPDECLINE:  return "decline";
    case DHCPACK:      return "ack";
    case DHCPNAK:      return "nak";
    case DHCPRELEASE:  return "release";
    case DHCPINFORM:   return "inform";
    default:           return "???";
  }
}

void AMDhClient::dump_dhcp_info(dhcp_info *info)
{
  char addr[20], mask[20], gway[20];

  PRINTF("--- dhcp %s (%d) ---",
         dhcp_type_to_name(info->type), info->type);
  strcpy(addr, ipaddr(info->ipaddr));
  strcpy(mask, ipaddr(info->mask));
  strcpy(gway, ipaddr(info->gateway));
  PRINTF("ip %s mask %s gw %s", addr, mask, gway);
  if (info->dns1) PRINTF("dns1: %s", ipaddr(info->dns1));
  if (info->dns2) PRINTF("dns2: %s", ipaddr(info->dns2));
  PRINTF("server %s, lease %d seconds",
         ipaddr(info->serveraddr), info->lease);
}

int32_t AMDhClient::decode_dhcp_msg(dhcp_msg *msg, int32_t len,
                                    dhcp_info *info)
{
  uint8_t *x;
  uint32_t opt;
  int32_t optlen;
  int32_t ret = 0;

  do {
    memset(info, 0, sizeof(dhcp_info));
    if (len < (DHCP_MSG_FIXED_SIZE + 4)) {
      ret = -1;
      break;
    }

    len -= (DHCP_MSG_FIXED_SIZE + 4);

    if ((msg->options[0] != OPT_COOKIE1)
        || (msg->options[1] != OPT_COOKIE2)
        || (msg->options[2] != OPT_COOKIE3)
        || (msg->options[3] != OPT_COOKIE4)) {
      ret = -1;
      break;
    }

    x = msg->options + 4;

    while (len > 2) {
      opt = *x++;
      if (opt == OPT_PAD) {
        len--;
        continue;
      }
      if (opt == OPT_END) {
        break;
      }
      optlen = *x++;
      len -= 2;
      if (optlen > len) {
        break;
      }
      switch(opt) {
        case OPT_SUBNET_MASK:
          if (optlen >= 4) {
            in_addr_t mask;
            memcpy(&mask, x, 4);
            info->mask = mask;
          }
          break;
        case OPT_GATEWAY:
          if (optlen >= 4) memcpy(&info->gateway, x, 4);
          break;
        case OPT_DNS:
          if (optlen >= 4) memcpy(&info->dns1, x + 0, 4);
          if (optlen >= 8) memcpy(&info->dns2, x + 4, 4);
          break;
        case OPT_LEASE_TIME:
          if (optlen >= 4) {
            memcpy(&info->lease, x, 4);
            info->lease = ntohl(info->lease);
          }
          break;
        case OPT_SERVER_ID:
          if (optlen >= 4) memcpy(&info->serveraddr, x, 4);
          break;
        case OPT_MESSAGE_TYPE:
          info->type = *x;
          break;
        default:
          break;
      }
      x += optlen;
      len -= optlen;
    }

    info->ipaddr = msg->yiaddr;

  } while (0);

  return ret;
}

void AMDhClient::hex2str(char *buf, size_t buf_size,
                         const uint8_t *array, int32_t len)
{
  int i;
  char *cp = buf;
  char *buf_end = buf + buf_size;
  for (i = 0; i < len; i++) {
    cp += snprintf(cp, buf_end - cp, " %02x ", array[i]);
  }
}

void AMDhClient::dump_dhcp_msg(dhcp_msg *msg, int32_t len)
{
    uint8_t *x;
    uint32_t n,c;
    int32_t optsz;
    const char *name;
    char buf[2048];

    PRINTF("===== DHCP message:");
    if (len < DHCP_MSG_FIXED_SIZE) {
      PRINTF("Invalid length %d, should be %d", len, DHCP_MSG_FIXED_SIZE);
        return;
    }

    len -= DHCP_MSG_FIXED_SIZE;

    if (msg->op == OP_BOOTREQUEST)
        name = "BOOTREQUEST";
    else if (msg->op == OP_BOOTREPLY)
        name = "BOOTREPLY";
    else
        name = "????";
    PRINTF("op = %s (%d), htype = %d, hlen = %d, hops = %d",
           name, msg->op, msg->htype, msg->hlen, msg->hops);
    PRINTF("xid = 0x%08x secs = %d, flags = 0x%04x optlen = %d",
           ntohl(msg->xid), ntohs(msg->secs), ntohs(msg->flags), len);
    PRINTF("ciaddr = %s", ipaddr(msg->ciaddr));
    PRINTF("yiaddr = %s", ipaddr(msg->yiaddr));
    PRINTF("siaddr = %s", ipaddr(msg->siaddr));
    PRINTF("giaddr = %s", ipaddr(msg->giaddr));

    c = msg->hlen > 16 ? 16 : msg->hlen;
    hex2str(buf, sizeof(buf), msg->chaddr, c);
    PRINTF("chaddr = {%s}", buf);

    for (n = 0; n < 64; n++) {
      uint8_t x = msg->sname[n];
      if ((x < ' ') || (x > 127)) {
        if (x == 0) break;
        msg->sname[n] = '.';
      }
    }
    msg->sname[63] = 0;

    for (n = 0; n < 128; n++) {
      uint8_t x = msg->file[n];
      if ((x < ' ') || (x > 127)) {
        if (x == 0) break;
        msg->file[n] = '.';
      }
    }
    msg->file[127] = 0;

    PRINTF("sname = '%s'", msg->sname);
    PRINTF("file = '%s'", msg->file);

    if (len < 4) return;
    len -= 4;
    x = msg->options + 4;

    while (len > 2) {
      if (*x == 0) {
        x++;
        len--;
        continue;
      }
      if (*x == OPT_END) {
        break;
      }
      len -= 2;
      optsz = x[1];
      if (optsz > len) break;
      if (x[0] == OPT_DOMAIN_NAME || x[0] == OPT_MESSAGE) {
        if ((unsigned int)optsz < sizeof(buf) - 1) {
          n = optsz;
        } else {
          n = sizeof(buf) - 1;
        }
        memcpy(buf, &x[2], n);
        buf[n] = '\0';
      } else {
        hex2str(buf, sizeof(buf), &x[2], optsz);
      }
      if (x[0] == OPT_MESSAGE_TYPE)
        name = dhcp_type_to_name(x[2]);
      else
        name = NULL;
      PRINTF("op %d len %d {%s} %s",
             x[0], optsz, buf, name == NULL ? "" : name);
      len -= optsz;
      x = x + optsz + 2;
    }
}

int32_t AMDhClient::send_message(int32_t sock, int32_t if_index,
                                 dhcp_msg  *msg, int32_t size)
{
  if (m_verbose) {
    dump_dhcp_msg(msg, size);
  }
  return send_packet(sock, if_index, msg, size, INADDR_ANY, INADDR_BROADCAST,
                     PORT_BOOTP_CLIENT, PORT_BOOTP_SERVER);
}

int32_t AMDhClient::is_valid_reply(dhcp_msg *msg, dhcp_msg *reply, int32_t sz)
{
  int32_t ret = 0;

  do {
    if (sz < DHCP_MSG_FIXED_SIZE) {
      if (m_verbose) {
        PRINTF("Wrong size %d != %d\n", sz, DHCP_MSG_FIXED_SIZE);
      }
      break;
    }

    if (reply->op != OP_BOOTREPLY) {
      if (m_verbose) {
        PRINTF("Wrong Op %d != %d\n", reply->op, OP_BOOTREPLY);
      }
      break;
    }

    if (reply->xid != msg->xid) {
      if (m_verbose) {
        PRINTF("Wrong Xid 0x%x != 0x%x\n",
               ntohl(reply->xid), ntohl(msg->xid));
      }
      break;
    }

    if (reply->htype != msg->htype) {
      if (m_verbose) {
        PRINTF("Wrong Htype %d != %d\n", reply->htype, msg->htype);
      }
      break;
    }

    if (reply->hlen != msg->hlen) {
      if (m_verbose) {
        PRINTF("Wrong Hlen %d != %d\n", reply->hlen, msg->hlen);
      }
      break;
    }

    if (memcmp(msg->chaddr, reply->chaddr, msg->hlen)) {
      if (m_verbose) {
        PRINTF("Wrong chaddr %x != %x\n",
               *(reply->chaddr),*(msg->chaddr));
      }
      break;
    }

    ret = 1;
  } while (0);

  return ret;
}
