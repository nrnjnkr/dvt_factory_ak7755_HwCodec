/*******************************************************************************
 * am_mp_wifi_stat.cpp
 *
 * History:
 *   Mar 24, 2015 - [longli] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <net/if.h>
#include <ifaddrs.h>
#include "am_mp_server.h"
#include "am_mp_wifi_stat.h"

static tWifiState wifi = {0};


static int setup_wifi() {

    char cmd[CMD_LEN] = {0};

    snprintf(cmd, CMD_LEN, "rmmod bcmdhd");

    system(cmd);

    snprintf(cmd, CMD_LEN, "modprobe bcmdhd firmware_path=/lib/firmware/broadcom/ap6255/fw_bcm43455c0_ag.bin nvram_path=/lib/firmware/broadcom/ap6255/nvram_ap6255_tcpka.txt");

    system(cmd);

    sleep(1);

    snprintf(cmd, CMD_LEN, "ifconfig -a");

    FILE *fp;
    char buff[BUFF_LEN];

    if ((fp = popen(cmd, "r")) != NULL) {

      while ( fgets(buff, BUFF_LEN, fp) != NULL ) {
          if(strstr(buff, "wlan0")) {
              wifi.state  = WIFI_INTF_UP;
              snprintf(cmd, CMD_LEN, "ifconfig wlan0 up");
              system(cmd);
              pclose(fp);
              break;
          }
      }
    }
    return wifi.state;
}

/*
 * This routine try to connect specific AP.
 *
 * Firstly, we need to delete any connections under:
 * /etc/NetworkManager/system-connections
 *
 * Then, we using nmcli command to connect AP.
 *
 * @param ssid: AP's ssid, sent from client.
 * @param password: AP's password, sent from client.
 *
 * @return: 0 if connected, -1 on failure.
 */


static int32_t connect_to_ap(const char *ssid, const char *password, char *msg) {

    fprintf(stderr, "%s wifi state %d\n", __func__, wifi.state);
    if(wifi.state == WIFI_INTF_UP) {
        char cmd[CMD_LEN] = {0};
        char buff[BUFF_LEN] = {0};
        FILE *fp = NULL;
        char ip[100];
        char netmask[100];
        snprintf(cmd, CMD_LEN, "/usr/local/bin/wl_setup.sh sta %s %s", ssid, password);
        char *ptr = NULL;
        if ((fp = popen(cmd, "r")) != NULL) {
            while (fgets(buff, BUFF_LEN, fp) != NULL) {
                ptr = strstr(buff, "addr:");
                if(  ptr != NULL ) {
                    if(buff[strlen(buff) - 1 ] == '\n')
                        buff[strlen(buff) - 1] = '\0';
                    char *ptr = strtok(buff, ":");
                    ptr = strtok(NULL, ":");
                    strcpy(wifi.ip_addr, ptr);
                }
                ptr = strstr(buff, "Bcast:");
                if(  ptr != NULL ) {
                    if(buff[strlen(buff) - 1 ] == '\n')
                        buff[strlen(buff) - 1] = '\0';
                    char *ptr = strtok(buff, ":");
                    ptr = strtok(NULL, ":");
                    strcpy(wifi.netmask, ptr);
                }

                if(strstr(buff, "wl_setup Finished") != NULL ) {
                    if ((ip != NULL) && (netmask != NULL)) {
                        wifi.state = WIFI_AP_CONNECTED;
                        strcpy(wifi.ssid_name, ssid);
                        strcpy(wifi.password, password);
                        strcpy(msg, "Connected successfully");
                        pclose(fp);
                        return MP_OK;
                    } else {
                        strcpy(msg, "Looks like password is wrong");
                        pclose(fp);
                        return MP_ERROR;
                    }
                }
                if (strstr(buff, "Can not find SSID") != NULL) {
                    strcpy(msg, "SSID is not found");
                    pclose(fp);
                    return MP_ERROR;
                }
            }
            pclose(fp);
            strcpy(msg, "Unable to connect to AP");
            return MP_ERROR;
        } else {
            strcpy(msg, "Unable to start the script wl_setup.sh");
            return MP_ERROR;
        }
    } else {
       strcpy(msg, "WiFi interface is not up");
       return MP_ERROR;
    }
}


/*
 *  Scan near by WiFi Access Points (AP's)
 */
static int list_aps() {

    if(wifi.state == WIFI_INTF_NOT_UP ) {

        fprintf(stderr, "WiFi interface is not up\n");
        return 1;
    }

    char cmd[CMD_LEN] = {0};

    snprintf(cmd, CMD_LEN, "wl scan");
    system(cmd);
    sleep(3);
    snprintf(cmd, CMD_LEN, "wl scanresults");
    FILE *fp;
    char buff[BUFF_LEN] = {0};

    if((fp = popen(cmd, "r")) != NULL ) {
        while(fgets(buff, BUFF_LEN, fp) != NULL) {
            if(strstr(buff, "SSID: ") != NULL) {
                fprintf(stderr, "%s\n", buff);
            }
        }
        pclose(fp);
    }
    return 0;
}

/*
 * Get the signal strength of currently connected AP.
 *
 */
static int signal_stregnth(char *strength) {

    if(wifi.state < WIFI_AP_CONNECTED ) {
        fprintf(stderr, "WiFi interface not up or Not connected any AP\n");
        return 1;
    }

    char cmd[CMD_LEN] = {0};
    snprintf(cmd, CMD_LEN, "wl rssi");
    FILE *fp;
    char buff[BUFF_LEN] = {0};
    if((fp = popen(cmd, "r")) != NULL) {
        if ( fgets(buff, BUFF_LEN, fp) != NULL)
            fprintf(stderr, "RSSI : %s\n", buff);
        pclose(fp);
    }
    return 0;
}


/*
 * Check the thorugh put with currently connected AP.
 * Server will start here, client should be started in other
 * pc(Linux or Window).
 *
 */
static int  wifi_throughput_test(char  *ip) {

    if(wifi.state < WIFI_AP_CONNECTED ) {
        fprintf(stderr, "WiFi interface not up or Not connected any AP\n");
        return 1;
    }

    char cmd[CMD_LEN] = {0};

    snprintf(cmd, CMD_LEN, "iperf3 -s &");
    system(cmd);
    strcpy(ip, wifi.ip_addr);
    //fprintf(stderr, "iperf server started at %s %s\n", wifi.ip_addr, wifi.netmask);
    return 0;
}

/*
 * Get the country code has been set to WiFi AP.
 *
 */
static int getcountrcode(char *code) {

    if(wifi.state < WIFI_INTF_UP ) {
        fprintf(stderr, "WiFi interface not up, Please Setup first\n");
        return 1;
    }

    char cmd[CMD_LEN] = {0};
    snprintf(cmd, CMD_LEN, "wl country");

    FILE *fp;

    if((fp = popen(cmd, "r")) != NULL) {
        if(fgets(code, 100, fp) != NULL) {
            //fprintf(stderr, "%s\n", code);
            if(code[strlen(code)-1] == '\n') {
                code[strlen(code) - 1] = '\0';
            }
        }
        pclose(fp);
    }
    return 0;
}

/*
 * Set the country code for the AP.
 *
 */

static int setcountrycode(char *country_name) {

    if(wifi.state < WIFI_INTF_UP ) {
        fprintf(stderr, "WiFi interface not up, Please Setup first\n");
        return 1;
    }

    char cmd[100] = {0};
    snprintf(cmd, 100, "wl country %s", country_name);
    system(cmd);
    //fprintf(stderr, "country code %s is set\n", country_name);
    return 0;
}


/*
 * This routine will serve as a handler to process
 * client's requests related to wifi.
 *
 * @param from: message sent from client.
 * @param to  : message sent to client.
 *
 * @return: MP_OK on success.
 */
am_mp_err_t mptool_wifi_test_handler(am_mp_msg_t *from, am_mp_msg_t *to) {

  to->result.ret = MP_OK;
  tConnectAp ApCreds;
  char code[100];
  char rssi[50];
  char ip[100];

  switch (from->stage) {
      case CMD_SETUP:
        if (setup_wifi() == WIFI_INTF_UP)
            to->result.ret = MP_OK;
        else
            to->result.ret = MP_ERROR;
        break;

      case CMD_CONNECT:
        memcpy(&ApCreds, from->msg, sizeof(am_mp_msg_t));

        to->result.ret = connect_to_ap(ApCreds.ssid, ApCreds.psswd, to->msg);
        break;
      case CMD_SCAN:
         if(!list_aps()) {
             to->result.ret = MP_OK;
         } else {
             to->result.ret = MP_ERROR;
             memcpy(to->msg, "Please setup WiFi interface first!!",
                     strlen("Please setup WiFi interface first!!"));

         }
         break;
      case CMD_GET_COUNTRY_CODE:
         memset(code, 0, 100);
         if(!getcountrcode(code)) {
             to->result.ret = MP_OK;
             memcpy(to->msg, code, strlen(code));
         } else {
             to->result.ret = MP_ERROR;
             memcpy(to->msg, "Please setup WiFi interface first!!",
                     strlen("Please setup WiFi interface first!!"));
         }
         break;
      case CMD_SET_COUNTRY_CODE:
         memset(code, 0, 100);
         memcpy(code, from->msg, strlen(from->msg));
         if(!setcountrycode(code)) {
             to->result.ret  = MP_OK;
         } else {
             to->result.ret = MP_ERROR;
             memcpy(to->msg, "Please setup WiFi interface first!!",
                     strlen("Please setup WiFi interface first!!"));
         }
         break;
      case CMD_SIGNAL_STRENGTH:
         if(!signal_stregnth(rssi)) {
             to->result.ret  = MP_OK;
         } else {
             to->result.ret = MP_ERROR;
             memcpy(to->msg, "AP may not connected!!",
                     strlen("AP may not connected!!"));
         }
         break;
      case CMD_THROUGHPUT_TEST:
         if(!wifi_throughput_test(ip)) {
             to->result.ret = MP_OK;
             memcpy(to->msg, ip, strlen(ip));
         } else {
             to->result.ret = MP_ERROR;
             memcpy(to->msg, "AP may not connected!!",
                     strlen("AP may not connected!!"));

         }
         break;
      default:
         to->result.ret = MP_NOT_SUPPORT;
         break;
  }
  return MP_OK;
}

