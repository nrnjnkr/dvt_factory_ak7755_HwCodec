/*******************************************************************************
 * ifctrl.h
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
#ifndef ORYX_NETWORK_WPA_INCLUDE_AM_IFCTRL_H_
#define ORYX_NETWORK_WPA_INCLUDE_AM_IFCTRL_H_

#include "am_pointer.h"
#include <atomic>
#include <mutex>
#include <sys/cdefs.h>
#include <arpa/inet.h>

typedef uint32_t in_addr_t;
#define IFNAMSIZ 16

struct AMIfBaseInfo
{
    char ifname[IFNAMSIZ];
    char hwaddr[32];
    in_addr_t ip;
    in_addr_t mask;
    int32_t link_connected;
    int32_t if_up;
};

class AMIfCtrl;
typedef AMPointer<AMIfCtrl> AMIfCtrlPtr;

class AMIfCtrl
{
    friend AMIfCtrlPtr;

  public:
    static AMIfCtrlPtr get_instance();
    int32_t get_ifindex(const char *name, int32_t *if_indexp);
    int32_t get_hwaddr(const char *name, void *ptr);

    int32_t enable(const char *ifname);
    int32_t disable(const char *ifname);

    int32_t get_addr(const char *name, in_addr_t *addr);
    int32_t set_addr(const char *name, in_addr_t addr);
    int32_t set_netmask(const char *name, in_addr_t netmask);
    int32_t set_hwaddr(const char *name, const void *ptr);

    int32_t create_default_route(const char *name, in_addr_t addr);
    int32_t remove_default_route(const char *ifname);
    int32_t generate_dns_cfg(in_addr_t dns1, in_addr_t dns2);
    int32_t print_if_info(const char *ifname);
    int32_t print_all_if_info();
    int32_t print_all_eth_info();
    int32_t get_if_info(struct AMIfBaseInfo *info);
    int32_t is_wireless(const char *ifname);

    int32_t configure_ifc(const char *ifname, in_addr_t address,
                          in_addr_t netmask, in_addr_t gateway);

    in_addr_t prefixLengthToIpv4Netmask(int32_t prefix_length);
    int32_t ipv4NetmaskToPrefixLength(in_addr_t mask);

  private:
    AMIfCtrl();
    virtual ~AMIfCtrl();
    void inc_ref();
    void release();
    int32_t init();
    void deinit();
    int32_t init6();
    void deinit6();

    int32_t up(const char *name);
    int32_t down(const char *name);

    const char *ipaddr_to_string(in_addr_t addr);
    int32_t init_ifr(const char *name, struct ifreq *ifr);
    int32_t set_flags(const char *name, unsigned set, unsigned clr);
    void init_sockaddr_in(struct sockaddr *sa, in_addr_t addr);
    int32_t act_on_ipv4_route(int32_t action, const char *ifname, in_addr_t dst,
                              in_addr_t netmask, in_addr_t gw);
    void parse_if_name(char *name, char *p);
    void get_if_list();

  private:
    static AMIfCtrl *m_instance;
    static std::mutex m_mtx;
    int32_t m_ctl_sock;
    int32_t m_ctl_sock6;
    std::atomic_int m_ref_cnt;
};



#endif /* ORYX_NETWORK_WPA_INCLUDE_AM_IFCTRL_H_ */
