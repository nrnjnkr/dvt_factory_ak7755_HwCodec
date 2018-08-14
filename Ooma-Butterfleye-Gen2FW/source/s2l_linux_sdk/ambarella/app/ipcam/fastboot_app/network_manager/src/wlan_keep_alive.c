/*
 * wlan_keep_alive.c
 *
 * History:
 *       2015/04/21 - [Chu Chen] created file
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
 */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <poll.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <pthread.h>
#include <basetypes.h>

#include <netinet/ip.h>	/* Internet Protocol 		*/
#include <netinet/tcp.h>	/* Transmission Control Protocol	*/
#include <pcap.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "brcm_ioc.h"
#include "bpi_utils.h"
#include "bpi_wlan.h"

#define DEVICE_INTERFACE "wlan0"
#define UDP_NETWORK_FILTER_FORMAT   "udp dst port %d"
#define TCP_NETWORK_FILTER_FORMAT   "tcp dst port %d"
#define DEFAULT_MSG		            '*'
#define DEFAULT_MSG_HEX	0x2a // char is DEFAULT_MSG
#define HAS_PAYLOAD

//#define PCAP_ERRBUF_SIZE             256
#define PCAP_CAPTURE_MAX_LEN    2048
#define PCAP_DATA_MAX_LEN          32

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    KEEPALIVE_TCP = 0,
    KEEPALIVE_UDP,
} KEEPALIVE_MODE;

static int _verbose = 0;
static int _interval = 0;
static int _timeout = 10;
static int _mode = KEEPALIVE_TCP;
static int _dtim_interval = 600; //mseconds
static unsigned char _wake_data[MAX_WAKEUP_DATA_LEN];
static int _wake_data_len = 0;
static pcap_t* _p_pcap = NULL;

#define DATA_MAXSIZE         (32)
#define PROC_WIFI_STATE    "/proc/ambarella/wifi_pm_state"
typedef struct net_socket_s {
    int enable_pattern;
    int enable_host_sleep;
    int is_poll;
    tcpka_conn_sess_info_t tcp_info_old;
    char iface[IFNAMSIZ];
    wifi_chip_t wifi_chip_id;
    char recv_msg[DATA_MAXSIZE];

} net_socket_t;

static net_socket_t net_socket;

//////////////////////////////////////////////////////////////
//
//copy from wowl.c
#define PROP_VALUE_MAX	(16)
#define WLC_GET_VERSION	(1)
#define WLC_GET_REVINFO	(98)

#define WLC_IOCTL_MAXLEN	(8192)
#define WLC_GET_VAR		(262)
#define WLC_SET_VAR		(263)
#define WLC_GET_BCNPRD	(75)
#define WLC_GET_DTIMPRD	(77)

#define IPV4_ADDR_LEN		(4)
static void _process_packet(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char * packet)
{
    int i = 0;
    int fd = (int)(*arg);
    char dst_mac[PCAP_DATA_MAX_LEN];

    struct ip *iphdr = NULL;		/* IPv4 Header */

    static int count = 0;
    LOG_DEBUG("Packet Count: %d\n", ++count);

    if (_verbose) {
        LOG_PRINT("Received Packet Size: %d\n", pkthdr->len);
        LOG_PRINT("Payload:\n");
        for(i = 0; i < pkthdr->len; ++i) {
            LOG_PRINT(" %02x", packet[i]);
            if( (i + 1) % 16 == 0 ) {
                LOG_PRINT("\n");
            }
        }
        LOG_PRINT("\n\n");
    }

    iphdr = (struct ip *)(packet + 14);
    if (_mode == KEEPALIVE_TCP) {
        struct tcphdr *tcphdr = NULL;	/* TCP Header  */
        unsigned int *tsval;			/* Time Stamp (optional) */
        unsigned int *tsvalR;			/* Time Stamp Reply (optional) */
        tcpka_conn_t tcpka;

        tcphdr = (struct tcphdr *)(packet + 14 + 20);
        tsval = (unsigned int *)(packet + 58);
        tsvalR = tsval + 1;

        if (tcphdr->psh && (pkthdr->len == 67) && (packet[66] == (u_char) DEFAULT_MSG)) {
            memset(dst_mac, 0, sizeof(dst_mac));
            sprintf(dst_mac, "%02X:%02X:%02X:%02X:%02X:%02X", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);

            if (_verbose) {
                LOG_PRINT("	FLags: PSH [%d]\n", tcphdr->psh);
                LOG_PRINT("	DST MAC: %s\n", dst_mac);
                LOG_PRINT("	DST IP: %s\n", inet_ntoa(iphdr->ip_dst));
                LOG_PRINT("	SRC IP: %s\n", inet_ntoa(iphdr->ip_src));
                LOG_PRINT("	SRC PORT: %d\n", ntohs(tcphdr->th_sport));
                LOG_PRINT("	DST PORT: %d\n", ntohs(tcphdr->th_dport));
                LOG_PRINT("	ID: %d\n", ntohs(iphdr->ip_id));
                LOG_PRINT("	SEQ: %u\n", ntohl(tcphdr->th_seq));
                LOG_PRINT("	ACK: %u\n", ntohl(tcphdr->th_ack));
                LOG_PRINT("	Win: %d\n", ntohs(tcphdr->th_win));
                LOG_PRINT("	TS val: %u\n", ntohl(*tsval));
                LOG_PRINT("	TS valR: %u\n", ntohl(*tsvalR));
            }

            /* Fill WiFi FW to KeepAlive and Wakeup Pattern, Session ID: 1 */
            tcpka.sess_id = 1;
            tcpka.dst_mac = (struct ether_addr)* ether_aton(dst_mac);

            memcpy(&tcpka.src_ip, &iphdr->ip_src.s_addr, IPV4_ADDR_LEN);
            memcpy(&tcpka.dst_ip, &iphdr->ip_dst.s_addr, IPV4_ADDR_LEN);

            tcpka.ipid = ntohs(iphdr->ip_id);
            tcpka.srcport = ntohs(tcphdr->th_sport);
            tcpka.dstport = ntohs(tcphdr->th_dport);
            tcpka.seq = ntohl(tcphdr->th_seq);
            tcpka.ack = ntohl(tcphdr->th_ack);
            tcpka.tcpwin = ntohs(tcphdr->th_win);
            tcpka.tsval = ntohl(*tsval);
            tcpka.tsecr = ntohl(*tsvalR);
#ifdef HAS_PAYLOAD
            tcpka.len = pkthdr->len - 66;
            tcpka.ka_payload_len = pkthdr->len - 66; // 66 is the lenght size of IP/TCP head
            tcpka.ka_payload[0] = DEFAULT_MSG_HEX;
#endif
            net_socket.tcp_info_old.tcpka_sess_ipid = tcpka.ipid;
            net_socket.tcp_info_old.tcpka_sess_seq = tcpka.seq;
            net_socket.tcp_info_old.tcpka_sess_ack = tcpka.ack;
            LOG_DEBUG("Suspend TCP info: ipid[%u], seq[%u], ack[%u]\n",
                      net_socket.tcp_info_old.tcpka_sess_ipid,
                      net_socket.tcp_info_old.tcpka_sess_seq,
                      net_socket.tcp_info_old.tcpka_sess_ack);

            //calc mask and pattern, todo
            int offset = 66;
            char tcp_wowl_mask[128];
            char tcp_wowl_pattern[1024];
            snprintf(tcp_wowl_mask, sizeof(tcp_wowl_mask),"0x");
            snprintf(tcp_wowl_pattern, sizeof(tcp_wowl_pattern),"0x");
            if(_wake_data_len) {
                int i;
                char _data_mask[MAX_WAKEUP_DATA_LEN];
                char *ptr_data_mask =_data_mask;
                for(i = 0; i < _wake_data_len /8; i++) {
                    *ptr_data_mask++ = 'f';
                    *ptr_data_mask++ = 'f';
                }
                if(_wake_data_len & 0x07) {
                    int j,last = _wake_data_len & 0x07;
                    unsigned char last_mask = 0;
                    for(j = 0; j < last; j++) {
                        last_mask |= (1 << j);
                    }
                    char m[4];
                    sprintf(m,"%02X",last_mask);
                    *ptr_data_mask++ = m[0];
                    *ptr_data_mask++ = m[1];
                }
                *ptr_data_mask++ = '\0';
                char _data_pattern[MAX_WAKEUP_DATA_LEN * 2 + 1];
                for(i = 0; i < _wake_data_len; i++) {
                    sprintf(&_data_pattern[ 2 * i],"%02X",_wake_data[i]);
                }
                int mask_len = strlen(tcp_wowl_mask);
                snprintf(&tcp_wowl_mask[mask_len],sizeof(tcp_wowl_mask) - mask_len,"%s",_data_mask);
                int pattern_len = strlen(tcp_wowl_pattern);
                snprintf(&tcp_wowl_pattern[pattern_len], sizeof(tcp_wowl_pattern) - pattern_len, "%s",_data_pattern);
            }

            wl_tcpka_conn_add(&tcpka);
            wl_tcpka_conn_enable(1, 1, _interval, 1, 8);

            wl_wowl_pattern(offset,tcp_wowl_mask,tcp_wowl_pattern);
            wl_wowl(0x00016);
            wl_wowl_activate(1);

            LOG_DEBUG("Set TCP KeepAlive Done\n");
            char msg = 's';
            if (fd > 0) write(fd, &msg, 1);
        }
    }

    return;
}

static int _break_pcap_entry(int fd, int timeout)
{
    if (fd < 0) {
        return -1;
    }

    fd_set readfds;
    fd_set fds;
    FD_ZERO(&fds);
    FD_ZERO(&readfds);
    FD_SET(fd, &fds);

    struct timeval time;
    time.tv_sec = timeout;
    time.tv_usec = 0;

    u8 msg = 0;
    int ret = -1;
    while (1) {
        readfds = fds;
        ret = select(fd + 1, &readfds, NULL, NULL, &time);
        if (ret > 0) {
            read(fd, &msg, 1);
            ret = 0;
            break;
        } else if (ret < 0) {
            int err = errno;
            if (err == EINTR || err == EAGAIN) {
                continue;
            }
            LOG_ERROR("select fail, errno %d\n", err);
            perror("select: ");
            ret = -2;
            break;
        } else {
            //timeout
            LOG_ERROR("_break_pcap_entry timeout\n");
            ret = -1;
            break;
        }
    }

    if (_p_pcap) {
        pcap_breakloop(_p_pcap);
    }

    return ret;
}

static void* _pcap_loop_entry(void* arg)
{
    int* pipe_fd = (int*)arg;

    do {
        if (!pipe_fd) {
            LOG_ERROR("_pcap_entry invalid pipe fd\n");
            break;
        }

        if (pcap_loop(_p_pcap, -1, _process_packet, (u_char*)&pipe_fd[1]) == -1) {
            LOG_ERROR("pcap_loop fail, %s\n", pcap_geterr(_p_pcap));
            char msg = 's';
            if (pipe_fd[1] > 0) write(pipe_fd[1], &msg, 1);
            break;
        }
    } while (0);

    LOG_DEBUG("_pcap_entry done\n");

    pthread_exit(NULL);
    return NULL;
}
//////////////////////////////////////////////////////////////////////////////
void wifi_power_save()
{
    wl_set_dtim_interval(_dtim_interval);
    int suspend_pm = 1;
    wl_set_get_pm_mode(&suspend_pm, 1);
    if (net_socket.enable_host_sleep) {
        int host_sleep = 1;
        wl_set_get_host_sleep(&host_sleep, 1);
    }
}

void wifi_host_sleep()
{
      int host_sleep = 1;
      wl_set_get_host_sleep(&host_sleep, 1);
}

void wifi_power_normal()
{
    if (net_socket.enable_host_sleep) {
        int sleep = 0;
        wl_set_get_host_sleep(&sleep, 1);
    }
    wl_set_bcn_li_dtim(0);
    int pm = 2;
    wl_set_get_pm_mode(&pm, 1);
}
int tcp_keep_alive(keep_alive_param_t  *param, int fd_client)
{
    char errbuf[PCAP_ERRBUF_SIZE] = {0};
    pthread_t pcap_loop_thread_id;
    int ret = -1;
    int pipe_fd[2] = {-1, -1};
    int success = 0;
    do {
        _interval = param->keepalive_interval;
        _verbose = param->verbose;
        _timeout = param->timeout;
        _mode = KEEPALIVE_TCP;
        _dtim_interval = param->dtim_interval;
        _wake_data_len = param->wake_data_len;
        memcpy(_wake_data,param->wake_data,_wake_data_len);

        if (!param->server_addr) {
            LOG_ERROR("Invalid argument\n");
            success = -1;
            break;
        }

        _p_pcap = pcap_open_live(DEVICE_INTERFACE, PCAP_CAPTURE_MAX_LEN, 1, 512, errbuf);
        if (!_p_pcap) {
            LOG_ERROR("pcap_open_live fail %s\n",errbuf);
            success = -1;
            break;
        }

        char net_filter[64] = {0};
        struct bpf_program filter;
        snprintf(net_filter, sizeof(net_filter), TCP_NETWORK_FILTER_FORMAT, param->server_port);
        if(pcap_compile(_p_pcap, &filter, net_filter, 1, 0)==-1) {
            LOG_ERROR("pcap_compile failed\n");
            success = -1;
            break;
        }
        if(pcap_setfilter(_p_pcap, &filter)==-1) {
            LOG_ERROR("pcap_setfilter failed\n");
            success = -1;
            break;
        }

        if (pipe(pipe_fd) < 0) {
            success = -1;
            LOG_ERROR("pipe fail\n");
            perror("pipe: ");
            break;
        }

        ret = pthread_create(&pcap_loop_thread_id, NULL, _pcap_loop_entry, (void*)pipe_fd);
        if (ret != 0) {
            LOG_DEBUG("_pcap_loop_entry pthread_create fail\n");
            success = -1;
            break;
        }

        usleep(10*1000);
        int len = writen(fd_client, (void*)"*", 1);
        if (len != 1) {
            LOG_ERROR("Send Msg fail\n");
            success = -1;
            pcap_breakloop(_p_pcap);
            break;
        }
        LOG_DEBUG("Send Length[%d] OK\n", len);
        if (_break_pcap_entry(pipe_fd[0], _timeout) < 0) {
            LOG_ERROR("_timeout_read_pipe fail\n");
            success = -1;
            break;
        }
    } while (0);

    if (ret == 0) {
        pthread_join(pcap_loop_thread_id, NULL);
    }

    if (_p_pcap) {
        pcap_close(_p_pcap);
        _p_pcap = NULL;
    }

    if (pipe_fd[0] > 0) {
        close(pipe_fd[0]);
    }
    if (pipe_fd[1] > 0) {
        close(pipe_fd[1]);
    }
    return success;
}

typedef enum {
    CPU_SUSPEND = 0,
    CPU_NORMAL = 1,
    CPU_UNKNOWN = 2,
} cpu_state;

static int wifi_enter_normal(tcpka_conn_sess_info_t *p_tcp_info, wl_wowl_wakeind_t *wakeind)
{
    wl_tcpka_conn_enable(1, 0, 0, 0, 0);
    wl_tcpka_conn_sess_info(1, p_tcp_info);
    wl_tcpka_conn_del(1);

    if (net_socket.enable_pattern) {
        wl_wowl_wakeind(wakeind);
        wl_wowl_wakeind_clear();
        wl_wowl_clear();
        wl_wowl_pattern_clr();
    }

    LOG_DEBUG("Resume done: ipid[%u], seq_num[%u], ack_num[%u].\n",
              p_tcp_info->tcpka_sess_ipid,
              p_tcp_info->tcpka_sess_seq,
              p_tcp_info->tcpka_sess_ack);
    return 0;
}

static void poll_wait_system_state(void)
{
#define POLL_TIMEOUT (10000) // 10s

    int ret = -1;
    int state_fd = -1;
    cpu_state curr_state = CPU_UNKNOWN;
    struct pollfd fds;

    state_fd = open(PROC_WIFI_STATE, O_RDWR);
    if (state_fd < 0) {
        perror("open" PROC_WIFI_STATE );
        return;
    }
    fds.fd = state_fd;
    fds.events = POLLIN;

    do {
        ret = poll(&fds, 1, POLL_TIMEOUT);
        if(ret == 0) {
            LOG_DEBUG("Poll Time out\n");
        } else {
            ret = read(state_fd, &curr_state, sizeof(curr_state));
            if (ret < 0) {
                perror("read\n");
            } else {
                LOG_DEBUG("System current state [%d]\n", curr_state);
                break;
            }
        }
    } while (0);

    close(state_fd);
    state_fd = -1;
}
// the following is not required if we do not want to reuse the keepalive socket
static int fix_tcp_info(int sock_fd, tcpka_conn_sess_info_t tcp_info)
{
    int ret = -1;

    if (sock_fd < 0) {
        LOG_ERROR("Socket fd [%d] is invalid\n", sock_fd);
        return -1;
    }

    LOG_DEBUG("Modify TCP info start\n");
    ret = ioctl(sock_fd, SET_TCP_FIX, &tcp_info);
    if (ret < 0) {
        perror("SET_TCP_FIX");
    } else {
        LOG_DEBUG("Modify TCP info done\n");
    }

    return ret;
}

int init_wifi_param(int is_poll, char const *iface,int iface_len)
{
    if((is_poll != 1 && is_poll != 0) || NULL == iface || iface_len < 1)
        return -1;
    net_socket.enable_pattern = 1;
    net_socket.enable_host_sleep = 1;
    net_socket.is_poll = is_poll;
    memset(net_socket.iface, 0, sizeof(net_socket.iface));
    strncpy(net_socket.iface,iface,iface_len);
    brcm_ioc_wowl_init(net_socket.iface, WIFI_BCM43438, 0);
    return 0;
}

int resume_wifi(int* wake_reason, int tcp_fix)
{
    int ret = 0;
    /* Resume back */
    if (net_socket.is_poll) {
        poll_wait_system_state();
    } else {
        /*  Wait system state change from respend to resume */
        sleep(1);
    }
    wl_wowl_wakeind_t wakeind= {0};
    tcpka_conn_sess_info_t tcp_info= {0};
    if (wifi_enter_normal(&tcp_info, &wakeind) < 0) {
        ret = -1;
    } else {
        if ((wakeind.ucode_wakeind & WL_WOWL_BCN) == WL_WOWL_BCN) {
            *wake_reason = AP_LOSS_BEACON;
        } else if((wakeind.ucode_wakeind & WL_WOWL_DIS) == WL_WOWL_DIS) {
            *wake_reason = AP_DISASSOCIATION_OR_AUTHENTICATION;
        }else if ((wakeind.ucode_wakeind & WL_WOWL_TCPKEEP_TIME) == WL_WOWL_TCPKEEP_TIME){
            *wake_reason = TCP_KEEPALIVE_TIMEOUT;
        }else{
            *wake_reason = WIFI_WAKE_NORMAL;
        }
        if (tcp_fix) {
            if ((tcp_info.tcpka_sess_ipid > (net_socket.tcp_info_old.tcpka_sess_ipid + 1)) ||
                (tcp_info.tcpka_sess_seq > (net_socket.tcp_info_old.tcpka_sess_seq + 1)) ||
                (tcp_info.tcpka_sess_ack > (net_socket.tcp_info_old.tcpka_sess_ack + 1))) {
                fix_tcp_info(get_fd_to_server(), tcp_info);
            }
        }
    }

    return ret;
}

#ifdef __cplusplus
};
#endif

