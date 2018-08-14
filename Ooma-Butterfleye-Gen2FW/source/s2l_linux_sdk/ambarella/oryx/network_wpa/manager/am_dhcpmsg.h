/*******************************************************************************
 * am_dhcpmsg.h
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
#ifndef ORYX_NETWORK_NEW_DHCLIENT_AM_DHCPMSG_H_
#define ORYX_NETWORK_NEW_DHCLIENT_AM_DHCPMSG_H_

#define PORT_BOOTP_SERVER 67
#define PORT_BOOTP_CLIENT 68

/* RFC 2131 p 9 */

#define OP_BOOTREQUEST 1
#define OP_BOOTREPLY   2

#define FLAGS_BROADCAST 0x8000

#define HTYPE_ETHER    1

struct dhcp_msg
{
    uint8_t op;           /* BOOTREQUEST / BOOTREPLY    */
    uint8_t htype;        /* hw addr type               */
    uint8_t hlen;         /* hw addr len                */
    uint8_t hops;         /* client set to 0            */

    uint32_t xid;         /* transaction id             */

    uint16_t secs;        /* seconds since start of acq */
    uint16_t flags;

    uint32_t ciaddr;      /* client IP addr             */
    uint32_t yiaddr;      /* your (client) IP addr      */
    uint32_t siaddr;      /* ip addr of next server     */
                          /* (DHCPOFFER and DHCPACK)    */
    uint32_t giaddr;      /* relay agent IP addr        */

    uint8_t chaddr[16];  /* client hw addr             */
    char sname[64];      /* asciiz server hostname     */
    char file[128];      /* asciiz boot file name      */

    uint8_t options[1024];  /* optional parameters        */
};

struct dhcp_info
{
    uint32_t type;

    uint32_t ipaddr;
    uint32_t gateway;
    uint32_t mask;

    uint32_t dns1;
    uint32_t dns2;

    uint32_t serveraddr;
    uint32_t lease;
};

#define DHCP_MSG_FIXED_SIZE 236

/* first four bytes of options are a cookie to indicate that
** the payload are DHCP options as opposed to some other BOOTP
** extension.
*/
#define OPT_COOKIE1          0x63
#define OPT_COOKIE2          0x82
#define OPT_COOKIE3          0x53
#define OPT_COOKIE4          0x63

/* BOOTP/DHCP options - see RFC 2132 */
#define OPT_PAD              0

#define OPT_SUBNET_MASK      1     /* 4 <ipaddr> */
#define OPT_TIME_OFFSET      2     /* 4 <seconds> */
#define OPT_GATEWAY          3     /* 4*n <ipaddr> * n */
#define OPT_DNS              6     /* 4*n <ipaddr> * n */
#define OPT_DOMAIN_NAME      15    /* n <domainnamestring> */
#define OPT_BROADCAST_ADDR   28    /* 4 <ipaddr> */

#define OPT_REQUESTED_IP     50    /* 4 <ipaddr> */
#define OPT_LEASE_TIME       51    /* 4 <seconds> */
#define OPT_MESSAGE_TYPE     53    /* 1 <msgtype> */
#define OPT_SERVER_ID        54    /* 4 <ipaddr> */
#define OPT_PARAMETER_LIST   55    /* n <optcode> * n */
#define OPT_MESSAGE          56    /* n <errorstring> */
#define OPT_CLASS_ID         60    /* n <opaque> */
#define OPT_CLIENT_ID        61    /* n <opaque> */
#define OPT_END              255

/* DHCP message types */
#define DHCPDISCOVER         1
#define DHCPOFFER            2
#define DHCPREQUEST          3
#define DHCPDECLINE          4
#define DHCPACK              5
#define DHCPNAK              6
#define DHCPRELEASE          7
#define DHCPINFORM           8

#endif /* ORYX_NETWORK_NEW_DHCLIENT_AM_DHCPMSG_H_ */
