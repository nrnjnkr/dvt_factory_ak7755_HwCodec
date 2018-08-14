/*******************************************************************************
 * dhcpclient.h
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
#ifndef ORYX_NETWORK_WPA_INCLUDE_AM_DHCLIENT_H_
#define ORYX_NETWORK_WPA_INCLUDE_AM_DHCLIENT_H_

#include <arpa/inet.h>
#include "am_ifctrl.h"

struct dhcp_msg;
struct dhcp_info;
typedef unsigned long long msecs_t;

class AMDhClient
{
  public:
    static AMDhClient* create(const std::string &ifname);
    int32_t do_dhcp();
    void set_verbose(const int32_t verbose);
    virtual ~AMDhClient() {};

  private:
    int32_t init(const std::string &ifname);
    void *init_dhcp_msg(dhcp_msg *msg, int32_t type, void *hwaddr, uint32_t xid);
    int32_t init_dhcp_discover_msg(dhcp_msg *msg, void *hwaddr, uint32_t xid);
    int32_t init_dhcp_request_msg(dhcp_msg *msg, void *hwaddr, uint32_t xid,
                                  uint32_t ipaddr, uint32_t serveraddr);
    int32_t open_raw_socket(uint8_t *hwaddr, int32_t if_index);
    uint32_t checksum(void *buffer, uint32_t count, uint32_t startsum);
    uint32_t finish_sum(uint32_t sum);
    int32_t send_packet(int32_t s, int32_t if_index, dhcp_msg *msg, int32_t size,
                        in_addr_t saddr, in_addr_t daddr, uint32_t sport, uint32_t dport);
    int32_t receive_packet(int32_t s, dhcp_msg *msg);
    int32_t dhcp_init_ifc();
    int32_t send_message(int32_t sock, int32_t if_index, dhcp_msg  *msg, int32_t size);
    int32_t is_valid_reply(dhcp_msg *msg, dhcp_msg *reply, int32_t sz);
    void hex2str(char *buf, size_t buf_size,
                 const unsigned char *array, int32_t len);
    void dump_dhcp_msg(dhcp_msg *msg, int32_t len);
    void dump_dhcp_info(dhcp_info *info);
    int32_t decode_dhcp_msg(dhcp_msg *msg, int32_t len, dhcp_info *info);
    const char *dhcp_type_to_name(uint32_t type);
    int32_t dhcp_configure(dhcp_info *info);
    const char *ipaddr(in_addr_t addr);
    msecs_t get_msecs(void);
    AMDhClient();

  private:
    std::string m_ifname;
    AMIfCtrlPtr m_ifc;
    int32_t m_verbose;
};


#endif /* ORYX_NETWORK_WPA_INCLUDE_AM_DHCLIENT_H_ */
