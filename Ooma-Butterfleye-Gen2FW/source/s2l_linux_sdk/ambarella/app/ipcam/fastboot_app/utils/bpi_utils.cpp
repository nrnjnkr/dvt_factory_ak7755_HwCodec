/*
 * bpi_utils.c
 *
 * History:
 *       2015/08/27 - [Chu Chen] created file
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <regex>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdarg.h>
#include "basetypes.h"
#include "bpi_utils.h"
static int _log_options = 0;

using std::string;
using std::regex;
using std::smatch;

//utility interface
unsigned  int get_current_time(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC,&now);
    unsigned mseconds = now.tv_sec * 1000+ now.tv_nsec/1000000;
    return mseconds;
}

int get_macaddress(const char *itf_name,unsigned char macAddress[6])
{
    struct ifreq ifreq;
    int sock;
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0) {
        return -1;
    }
    strcpy(ifreq.ifr_name,itf_name);
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0) {
        close(sock);
        return -1;
    }
    LOG_DEBUG("mac address [%02x:%02x:%02x:%02x:%02x:%02x]\n",
              (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
              (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
              (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
              (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
              (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
              (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    macAddress[0] = (unsigned char)ifreq.ifr_hwaddr.sa_data[0];
    macAddress[1] = (unsigned char)ifreq.ifr_hwaddr.sa_data[1];
    macAddress[2] = (unsigned char)ifreq.ifr_hwaddr.sa_data[2];
    macAddress[3] = (unsigned char)ifreq.ifr_hwaddr.sa_data[3];
    macAddress[4] = (unsigned char)ifreq.ifr_hwaddr.sa_data[4];
    macAddress[5] = (unsigned char)ifreq.ifr_hwaddr.sa_data[5];
    close(sock);
    return 0;
}

char *get_device_id(void)
{
    //TODO
    static char s_device_id[128];
    unsigned char macAddress[6];
    if(get_macaddress("wlan0",macAddress) < 0) {
        snprintf(s_device_id,sizeof(s_device_id),"TEST_ELEKTRA_UUID");
    } else {
        snprintf(s_device_id,sizeof(s_device_id),"UUID_%02x:%02x:%02x:%02x:%02x:%02x",\
                 macAddress[0],macAddress[1],macAddress[2],macAddress[3],macAddress[4],macAddress[5]);
    }
    return s_device_id;
}

time_t get_time_from_name(const char* filename){
    smatch match;
    struct tm tm;
    const regex time_pattern("(\\d{14,14})");
    string tmp_str(filename);
    if(nullptr == filename){
        return -1;
    }

    if (regex_search(tmp_str, match, time_pattern)) {
        memset(&tm, 0, sizeof(tm));
        tmp_str = match[0].str();
        sscanf(tmp_str.c_str(), "%4d%2d%2d%2d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return mktime(&tm);
    }
    else{
        return -1;
    }
    return -1;
}

int get_id_from_name(const char* filename){
    smatch match;
    const regex stream_pattern("stream\\d");
    string tmp_str(filename);
    if (regex_search(tmp_str, match, stream_pattern)) {
        const regex id_pattern("\\d");
        tmp_str = match[0].str();
        if (regex_search(tmp_str, match, id_pattern)) {
            return atoi(match[0].str().c_str());
        }
    }
    return -1;
}

int get_num_from_tail(const char* filename){
    smatch match;
    const regex stream_pattern("stream\\d_\\d_\\d");
    string tmp_str(filename);
    if (regex_search(tmp_str, match, stream_pattern)) {
        tmp_str = match[0].str();
        unsigned int i = strlen(tmp_str.c_str()) - 1;
        while(i>0 && (tmp_str[i] >= '0' && tmp_str[i] <= '9')){
            i--;
        }
        if((i == 0) || (i == (strlen(tmp_str.c_str()) - 1))) return -1;
        return atoi(tmp_str.c_str()+i+1);
    }else{
        return -1;
    }
}

//for debug, show the elektra_boot start time
void start_time()
{
    time_t now = time(NULL);
    struct tm* p_tm = localtime(&now);
    LOG_PRINT("\t***************************************************************\n");
    LOG_PRINT("\t******elektra_boot start at the time: %d-%02d-%02d %02d:%02d:%02d******\n",
              (1900 + p_tm->tm_year), (1 + p_tm->tm_mon), p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
    LOG_PRINT("\t***************************************************************\n");
    LOG_DEBUG("elektra_boot start: %u\n", get_current_time());
}

static int parse_ip(const char *p_str, int len, char ip[MAX_IP_LEN]) {
    int ret = -1;
    do {
        const char *p = p_str;
        int times = 0;
        while (NULL != (p = strchr(p, '.'))) {
            times++;
            p++;
        }

        if (times != 3) {
            LOG_DEBUG("invalid ip address");
            break;
        }

        memmove(ip, p_str, len);
        ip[len] = '\0';
        ret = 0;
    } while (0);
    return ret;
}

static int parse_port(const char *p_str, int len, int *p_port) {
    if (!p_port) { return 0; }

    int ret = -1;
    do {
        if (len > MAX_PORT_LEN) {
            LOG_ERROR("port len exceeds 16 bytes %d", len);
            break;
        }

        char buf[MAX_PORT_LEN];
        memmove(buf, p_str, len);
        buf[len] = '\0';

        int port = atoi(buf);
        if (port == 0) {
            LOG_ERROR("invalid port");
            break;
        }

        *p_port = port;
        ret = 0;
    } while (0);
    return ret;
}


int param_device_parse_server(const char *p_str, char* ip, int *p_port)
{
    int ret = -1;
    do {
        // skip protocol
        const char *p_proto = "://";
        const char *p = strstr(p_str, p_proto);
        if (p) { p_str = p + strlen(p_proto); }

        int len = 0;
        p = strchr(p_str, ':');
        if (p) {
            // parse ip & port
            len = (int)(p - p_str);
            ret = parse_ip(p_str, len, ip);
            if (ret < 0) {
                LOG_ERROR("invalid ip");
                break;
            }

            len = (int)(strlen(p_str) - len - 1);
            ret = parse_port(p+1, len, p_port);
        } else {
            // if ip, parse ip
            // else parse port
            len = (int)strlen(p_str);
            p = strchr(p_str, '.');
            if (p) {
                ret = parse_ip(p_str, len, ip);
                break;
            } else {
                ret = parse_port(p_str, len, p_port);
                break;
            }
        }
    } while (0);
    return ret;
}


/*---------------------platform_specific---------------------*/
#include <sys/vfs.h>
static int wait_sda(void){
    unsigned int start = get_current_time();
    unsigned int now = start;
    const char *dev_sda = "/dev/sda";
    char sd_card[16] = {0};

    while(1){
        for(int sda_suffix = 1; sda_suffix < 10; sda_suffix++){
            sprintf(sd_card, "%s%d", dev_sda, sda_suffix);
            if(0 == access(sd_card, F_OK)){
                LOG_DEBUG("check %s costs = %dms\n", sd_card, now - start);
                return sda_suffix;
            }
        }
        if(0 == access(dev_sda, F_OK)){
            LOG_DEBUG("check %s costs = %dms\n", dev_sda, now - start);
            return 0;
        }
        now = get_current_time();
        if(now - start > 3000){
            LOG_ERROR("failed to find any sdcard, please check sdcard: %s\n", strerror(errno));
            return -1;
        }

        usleep(100000);
    }
}

static int showSdcardInfo(void)
{
    struct statfs diskInfo;
    if(statfs("/sdcard",&diskInfo) < 0){
        LOG_DEBUG("/sdcard statfs failed, errno = %d\n",errno);
        return -1;
    }
    unsigned long long blocksize = diskInfo.f_bsize;
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;
    LOG_DEBUG("sdcard TOTAL_SIZE == %llu MB\n",totalsize>>20);
    unsigned long long freeDisk = diskInfo.f_bfree*blocksize;
    LOG_DEBUG("sdcard DISK_FREE == %llu MB\n",freeDisk>>20);
    return 0;
}

int mount_sdcard(void){
    system("/sbin/modprobe ehci_hcd");
    system("/sbin/modprobe ehci_ambarella");
    system("echo host > /proc/ambarella/usbphy0");
    system("/sbin/modprobe scsi_mod");
    system("/sbin/modprobe usb_storage");
    system("/sbin/modprobe sd_mod");
    system("/sbin/modprobe fat");
    system("/sbin/modprobe vfat");
    int sda_suffix;
    char m_sd_cmd[64] = {0};

    if((sda_suffix = wait_sda()) < 0){
        return -1;
    }

    if(sda_suffix > 0){
        sprintf(m_sd_cmd, "/bin/mount -t vfat /dev/sda%d /sdcard", sda_suffix);
    }else{
        sprintf(m_sd_cmd, "/bin/mount -t vfat /dev/sda /sdcard");
    }
    system(m_sd_cmd);
    LOG_DEBUG("run_record_mode, sdcard driver installed\n");
    return showSdcardInfo();
}

 void umount_sdcard(void){
    system("/bin/umount /sdcard");
    LOG_DEBUG("run_record_mode, sdcard driver uninstalled\n");
}

void enable_usb_ethernet(void){
   system("/sbin/modprobe ehci-hcd");
   system("echo device > /proc/ambarella/usbphy0");
   system("/sbin/modprobe ambarella_udc");
   system("/sbin/modprobe g_ether");
   system("/sbin/ifconfig usb0 10.1.0.100 netmask 255.255.255.0 up");
}
void install_sdcard_driver(void){
    system("/sbin/modprobe ehci_hcd");
    system("/sbin/modprobe ehci_ambarella");
    system("echo host > /proc/ambarella/usbphy0");
    system("/sbin/modprobe scsi_mod");
    system("/sbin/modprobe usb_storage");
    system("/sbin/modprobe sd_mod");
    system("/sbin/modprobe fat");
    system("/sbin/modprobe vfat");
}

void set_log_options(int log_options)
{
    _log_options = log_options;
    return;
}

void bpi_log(BPI_LOG_TYPE log_type, const char *format, ...)
{
    char log[512]   = {0};
    char fmt[128]   = {0};
    char tag[16] = {0};
    va_list vlist;
    va_start(vlist, format);
    if((BPI_LOG_DEBUG == log_type) && !(_log_options & BPI_LOG_ENABLE_DEBUG_LOG)){
        return;
    }

    switch(log_type){
        case BPI_LOG_ERROR:
            snprintf(tag, sizeof(tag), "%s", "[bpi-error]");
            break;
        case BPI_LOG_WARN:
            snprintf(tag, sizeof(tag), "%s", "[bpi-warning]");
            break;
        case BPI_LOG_DEBUG:
            snprintf(tag, sizeof(tag), "%s", "[bpi-debug]");
            break;
        default:
            snprintf(tag, sizeof(tag), "%s", "[bpi-info]");
            break;
    }

    if((_log_options & BPI_LOG_ENABLE_TIMESTAMP)){
        struct timeval tm;
        gettimeofday(&tm, NULL);
        snprintf(fmt, sizeof(fmt), "%s[%ld.%ld] %s", tag, tm.tv_sec, tm.tv_usec, format);
    }else{
        snprintf(fmt, sizeof(fmt), "%s %s", tag, format);
    }
    vsnprintf(log, sizeof(log), fmt, vlist);
    va_end(vlist);
    fprintf(stderr, "%s", log);
    fflush(stderr);
}
