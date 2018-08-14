/*
 * bpi_wlan.c --
 *
 * History:
 *       2016/01/15 - [CZ Lin] created file
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include "basetypes.h"
#include "iniparser.h"
#include "bpi_utils.h"
#include "audio_play.h"
#include "bpi_wlan.h"
#include "wpa_to_ap.h"
#include "cooee_inter.h"

#define PLAY_AUDIO
#define USE_FIRMWARE_SUPPLICANT
#define COOEE_MAX_TIMES 3
typedef struct _wlan_pipe
{
    const  char *ssid;
    const  char *passwd;
    const  char *wpa;
    int pipe_fd;
}wrap_wlan_pipe;

static int s_wlan_conf_changed = 0;
static pthread_t wlan_thread_id = 0;
static int socket_to_server = -1;
// for communication between connect_ap thread and wlan_status function call.
static int pipe_fd[2] = {-1, -1};

#ifdef USE_FIRMWARE_SUPPLICANT
// use in-firmware wpa supplicant
#ifdef __cplusplus
extern "C" {
#endif

int save_wlan_config(const char *file_path, cooee_wlan_conf_t *sWlanConfig)
{
    if(NULL == sWlanConfig || NULL == file_path)
    {
        LOG_DEBUG("parameter is invalid\n");
        return -1;
    }

    dictionary* ini_dict = iniparser_load(file_path);
    if(!ini_dict){
        return -1;
    }

    if(NULL != iniparser_getstring(ini_dict, "wlan:ap_ssid_string", NULL)){
        iniparser_set(ini_dict, "wlan:ap_ssid_string", sWlanConfig->ap_ssid);
    }

    if(NULL != iniparser_getstring(ini_dict, "wlan:ap_password_string", NULL)){
        iniparser_set(ini_dict, "wlan:ap_password_string", sWlanConfig->ap_password);
    }

    if(NULL != iniparser_getstring(ini_dict, "wlan:ap_wpa_string", NULL)){
        iniparser_set(ini_dict, "wlan:ap_wpa_string", sWlanConfig->ap_wpa);
    }

    if(NULL != iniparser_getstring(ini_dict, "wlan:cloud_server_ip_string", NULL)){
        iniparser_set(ini_dict, "wlan:cloud_server_ip_string", sWlanConfig->cloud_server_ip);
    }

    FILE* f_ini = fopen(file_path, "w");
    if(f_ini) {
        iniparser_dump_ini(ini_dict, f_ini);
        fclose(f_ini);
    }
    iniparser_freedict(ini_dict);
    return 0;
}

char * get_server_ip(const char * file_path)
{
    static char server_ip[32] = {0};
    const char* value = NULL;
    char * ret = NULL;
    dictionary* ini_dict = NULL;
    do{
        if(NULL == file_path)
            ini_dict = iniparser_load("/etc/bpi/setting.ini");
        else
            ini_dict = iniparser_load(file_path);
        if(!ini_dict){
            break;
        }
        memset(server_ip,'\0',sizeof server_ip);
        if((value= iniparser_getstring(ini_dict, "wlan:cloud_server_ip_string", NULL))!= NULL) {
            snprintf(server_ip, sizeof(server_ip),"%s", value);
            server_ip[sizeof(server_ip)-1] = '\0';
        } else {
            break;
        }

        ret = server_ip;
    }while(0);
    if(ini_dict) iniparser_freedict(ini_dict);
    if(NULL == ret)
        LOG_ERROR("read server url failed\n");
    return ret;
}
#ifdef __cplusplus
};
#endif
static int wl_setup(const char *ssid, const char *passwd)
{
    if (NULL == ssid || NULL == passwd) {
        LOG_PRINT(" ssid or password should not be null pointers.\n");
        return EWlan_Disconnected;
    }
    int state = EWlan_Disconnected;
    do {
        char cmd[256] = {0};
        if(passwd[0] == '\0') {
            snprintf(cmd, sizeof(cmd), "/usr/local/bin/wl_setup.sh sta '%s' 2>&1 |grep 'wl_setup' |awk '{print $2}'",ssid);
        } else {
            snprintf(cmd, sizeof(cmd), "/usr/local/bin/wl_setup.sh sta '%s' '%s' 2>&1 |grep 'wl_setup' |awk '{print $2}'", ssid, passwd);
        }

        char connected_ret[] = "Finished";
        LOG_DEBUG("cmd: %s\n", cmd);

        FILE* p_fd = popen((const char*)cmd, "r");
        if (p_fd) {
            char buf[256] = {0};
            while(fgets(buf, sizeof(buf), p_fd)) {
                if (strncmp(buf, connected_ret, strlen(connected_ret)) == 0) {
                    state = EWlan_Connected;
                    break;
                }
            }
            pclose(p_fd);
            p_fd = NULL;
        }
    } while (0);

    return state;
}
#else
// use Linux wpa supplicant
static int wifi_setup(const char *ssid, const char *passwd, const char* ap_wpa)
{
    if (NULL == ssid || NULL == passwd) {
        LOG_PRINT(" ssid or password should not be null pointers.\n");
        return EWlan_Disconnected;
    }
    int state = EWlan_Disconnected;
    do {
        char cmd[256] = {0};
        if (access(WPA_SUPPLICANT_CONF_PATH, 0) == 0) {
            snprintf(cmd, sizeof(cmd), "/usr/local/bin/wifi_setup.sh sta nl80211 2>&1 |grep wifi_setup | awk '{print $2}'");
        } else {
            if(passwd[0] == '\0') {
                snprintf(cmd, sizeof(cmd), "/usr/local/bin/wifi_setup.sh sta nl80211 \'%s\'  2>&1 |grep wifi_setup | awk '{print $2}'", ssid);
            } else if(passwd[0] == '0' && strcmp(ap_wpa,"open")==0 ) {
                snprintf(cmd, sizeof(cmd), "/usr/local/bin/wifi_setup.sh sta nl80211 \'%s\' 0 open 2>&1 |grep wifi_setup | awk '{print $2}'", ssid);
            } else if(ap_wpa[0] == '\0') {
                snprintf(cmd, sizeof(cmd), "/usr/local/bin/wifi_setup.sh sta nl80211 \'%s\'  \'%s\' 2>&1 |grep wifi_setup | awk '{print $2}'", ssid, passwd);
            } else {
                snprintf(cmd, sizeof(cmd), "/usr/local/bin/wifi_setup.sh sta nl80211 \'%s\'  \'%s\'  \'%s\' 2>&1 |grep wifi_setup | awk '{print $2}'", ssid, passwd, ap_wpa);
            }
        }
        char connected_ret[] = "OK<<<";
        LOG_DEBUG("cmd: %s\n", cmd);

        FILE* p_fd = popen((const char*)cmd, "r");
        if (p_fd) {
            char buf[256] = {0};
            if (fgets(buf, sizeof(buf), p_fd)) {
                if (strncmp(buf, connected_ret, strlen(connected_ret)) == 0) {
                    state = EWlan_Connected;
                }
            }
            pclose(p_fd);
            p_fd = NULL;
        }
    } while (0);

    return state;
}
#endif

int wlan_conf_changed(){
    return s_wlan_conf_changed;
}

void clear_wlan_flag(){
    s_wlan_conf_changed = 0;
}

int connect_ap(const char *ssid, const char *passwd, const char *wpa)
{
    if(NULL == ssid || NULL == passwd || NULL == wpa)
    {
        LOG_ERROR("connect ap info memory address invalid\n");
        return -1;
    }
    LOG_PRINT("connect_ap start: %u\n", get_current_time());
#ifdef USE_FIRMWARE_SUPPLICANT
    int state = wl_setup(ssid, passwd);
#else
    int state = wifi_setup(ssid, passwd, wpa);
#endif
    if(0 != init_wifi_param(1, "wlan0", sizeof("wlan0"))){
        LOG_ERROR("wifi init: load_default_param failed!\n");
        return -1;
    }
    LOG_PRINT("connect_ap done: %u, state = %s\n", get_current_time(), (state == EWlan_Disconnected) ? "FAIL":"OK" );
    return state;
}

static void* _wlan_entry(void* p)
{
    if(NULL == p)
        return NULL;
    wrap_wlan_pipe* wlan_pipe = (wrap_wlan_pipe*)p;

    u8 status = (u8)connect_ap(wlan_pipe->ssid, wlan_pipe->passwd, wlan_pipe->wpa);
    if (wlan_pipe->pipe_fd > 0) {
        write(wlan_pipe->pipe_fd, &status, 1);
    }

    //TODO, send msg to mcu to notify wlan status
    pthread_exit("wlan done");
    return NULL;
}

int connect_ap_async(const char *ssid, const char *passwd, const char *wpa)
{
    if (pipe(pipe_fd) < 0) {
        LOG_ERROR("pipe fail\n");
        perror("pipe: ");
        //pipe_fd[0] = pipe_fd[1] = -1;
    }
    wrap_wlan_pipe tmp = {ssid,passwd,wpa,pipe_fd[1]};
    int ret = pthread_create(&wlan_thread_id, NULL, _wlan_entry, &tmp);
    if (ret != 0) {
        LOG_ERROR("pthread_create fail\n");
        return ECode_WlanUp_Fail;
    }
    return ECode_Success;
}

int ap_connect_status()
{
    FILE *p_fd = NULL;
    int ret = EWlan_Disconnected;
    char wpa_state[] = "COMPLETED";
    p_fd = popen("wpa_cli status | grep wpa_state | awk -F '=' '{print $2}' ", "r");
    if(p_fd){
        char buf[16] = {0};
        if(fgets(buf, sizeof(buf), p_fd)){
            if (strncmp(buf, wpa_state, strlen(wpa_state)) == 0){
                ret = EWlan_Connected;
            }
        }
        pclose(p_fd);
        p_fd = NULL;
    }else{
        LOG_ERROR("popen failed.\n");
    }
    return ret;
}

#include "test_tcp_connect_nonblock.h"

int connect_server(const char* server_addr, int server_port, int protocol)
{
    int ret = -1;
    do {
        if (Net_UDP == protocol) {
            socket_to_server = socket(AF_INET, SOCK_DGRAM, 0);
        } else if (Net_TCP == protocol) {
            socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
        }
        if (socket_to_server < 0) {
            LOG_ERROR("socket fail\n");
            perror("socket: ");
            break;
        }

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0x0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = inet_addr(server_addr);
        dest_addr.sin_port = htons(server_port);
        bzero(&(dest_addr.sin_zero), 8);

        int err;
        if ((err = connect_nonb(socket_to_server, &dest_addr, sizeof(dest_addr), 10)) < 0) {
            close(socket_to_server);
            socket_to_server = -1;
            perror("connect: ");
            break;
        }
        LOG_DEBUG("Socket [%d], Host: %s:%d, Connect OK\n", socket_to_server, server_addr, server_port);
        ret = 0;
    } while (0);

    return ret;
}

int get_fd_to_server()
{
    return socket_to_server;
}

void close_fd_to_server()
{
    if (socket_to_server >= 0) {
        close(socket_to_server);
        LOG_DEBUG("Close fd [%d].\n", socket_to_server);
        socket_to_server = -1;
    }
}

static void play_audio(char* path)
{
#ifdef  PLAY_AUDIO
    play_audio_by_aplay(path);
#else
    LOG_DEBUG("DEBUG: audio play diabled. %s\n", path);
#endif

}

static int wait_wlan0()
{
    struct stat buf;
    int count = 0;
    while(stat("/sys/class/net/wlan0/address", &buf) < 0) {
        usleep(10000);
        ++count;
        if(count > 2000) {
            return -1;
        }
    }
    return 0;
}

int config_elektra_boot(cooee_wlan_conf_t *wlan_write_back)
{
    int ret = -1;
    do{
        if(NULL == wlan_write_back)
        {
            break;
        }
        char buf[256] = {0};
        char ssid[64] = {0};
        char psword[32] = {0};
        char ip[64] = {0};
        char wpa[64] =  {0};
        int  cooee_max_times = COOEE_MAX_TIMES;
        char *head = ssid;
        char *tail = ssid;
        int len = 0;
        int offset = 0;

        system("/sbin/ifconfig wlan0 up");
        if(wait_wlan0()) {
            return -1;
        }

        load_audio_playback_driver();
        play_audio("/usr/local/bin/config_mode_s16le.wav");

        while(cooee_max_times-- > 0) {
            if(!cooee_interface(COOEE_CONF_PATH))
            {
                play_audio("/usr/local/bin/cooee_ok_s16le.wav");
                LOG_DEBUG("cooee ok!\n");
                break;
            } else {
                play_audio("/usr/local/bin/cooee_fail_s16le.wav");
                LOG_DEBUG("cooee fail!\n");
            }
        }

        if(cooee_max_times < 0) {
            play_audio("/usr/local/bin/cooee_time_out_s16le.wav");
            LOG_DEBUG("cooee time_out !\n");
            break;
        }

        //  parse  /tmp/config/cooee.conf
        LOG_DEBUG(" ############# Parse result to Connect to Ap :ssid:%s,password:%s,ip:%s,wpa:%s\n",ssid,psword,ip,wpa);
        if(!parse_cooee_conf(ssid,psword,ip,wpa)){
            play_audio("/usr/local/bin/parse_cooee_transmitted_content_ok_s16le.wav");
            LOG_DEBUG("Parse cooee ok\n");
        } else {
            play_audio("/usr/local/bin/parse_cooee_transmitted_content_fail_s16le.wav");
            LOG_DEBUG("parse cooee fail!\n");
            break;
        }
        memset(buf,0,sizeof(buf));
        head = tail = ssid;
        while(*head) {
            if((tail=strchr(head,'\''))!=NULL) {
                len = tail - head;
                strncpy(buf+offset,"\'",1);
                offset++;
                strncpy(buf+offset,head,len);
                offset+=len;
                strncpy(buf+offset,"\'\"\'\"",4);
                offset+=4;
                head = ++tail;
            } else {
                snprintf(buf+offset,sizeof(buf)-offset,"\'%s\'",head);
                break;
            }
        }
        if(strlen(buf) < sizeof(ssid)) {
            memset(ssid,'\0',sizeof(ssid));
            strncpy(ssid,buf,sizeof(ssid));
        } else {
            LOG_ERROR("The ssid too long \n");
            break;
        }

        if(EWlan_Connected == connect_ap(ssid, psword, wpa)) {
            play_audio("/usr/local/bin/connecting_ap_ok_s16le.wav");
        } else {
            play_audio("/usr/local/bin/connecting_ap_fail_s16le.wav");
            break;
        }

        LOG_DEBUG("ssid:%s|psword:%s|pwa:%s|server_ip:%s|\n",ssid,psword,wpa,ip);
        memset(wlan_write_back, '\0', sizeof(cooee_wlan_conf_t));
        strncpy(wlan_write_back->ap_ssid,ssid,strlen(ssid));
        strncpy(wlan_write_back->ap_password,psword,strlen(psword));
        strncpy(wlan_write_back->ap_wpa,wpa,strlen(wpa));
        snprintf(wlan_write_back->cloud_server_ip,sizeof(wlan_write_back->cloud_server_ip),"%s",ip);
        ret = 0;
    } while(0);
    if (ret == 0) {
        play_audio("/usr/local/bin/config_mode_ok_s16le.wav");
        LOG_DEBUG("############## Config Mode Ok !   ############### \n");
    }
    else{
        play_audio("/usr/local/bin/config_mode_fail_s16le.wav");
        LOG_DEBUG("############## Config Mode Fail !   ############### \n");
    }
    system("/bin/rm -rf /tmp/config/cooee.conf");
    unload_audio_playback_driver();
    return  ret;
}

