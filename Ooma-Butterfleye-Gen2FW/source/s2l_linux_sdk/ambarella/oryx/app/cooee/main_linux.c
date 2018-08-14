 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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
#include <unistd.h>
#include "easy_setup.h"

#include "proto/cooee.h"
#include "proto/neeze.h"
#include "proto/akiss.h"
#include "proto/changhong.h"

int debug_enable = 0;

void usage() {
    printf("-h: show help message\n");
    printf("-d: show debug message\n");
    printf("-k <v>: set 16-char key for all protocols\n");
    printf("-p <v>: bitmask of protocols to enable\n");
    printf("  0x%04x - cooee\n", 1<<EASY_SETUP_PROTO_COOEE);
    printf("  0x%04x - neeze\n", 1<<EASY_SETUP_PROTO_NEEZE);
    printf("  0x%04x - akiss\n", 1<<EASY_SETUP_PROTO_AKISS);
    printf("  0x%04x - changhong\n", 1<<EASY_SETUP_PROTO_CHANGHONG);
}

int main(int argc, char* argv[])
{
    int ret;
    //int len;
    uint16 val;

    for (;;) {
        int c = getopt(argc, argv, "dhk:p:");
        if (c < 0) {
            break;
        }

        switch (c) {
            case 'd':
                debug_enable = 1;
                break;
            case 'k':
                cooee_set_key(optarg);
                cooee_set_key_qqcon(optarg);
                neeze_set_key(optarg);
                neeze_set_key_qqcon(optarg);
                akiss_set_key(optarg);
                break;
            case 'p':
                sscanf(optarg, "%04x", (int *) &val);
                easy_setup_enable_protocols(val);
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return 0;
        }
    }

    ret = easy_setup_start();
    if (ret) return ret;

    /* query for result, blocks until cooee comes or times out */
    ret = easy_setup_query();
    if (!ret) {
        char ssid[33]; /* ssid of 32-char length, plus trailing '\0' */
        ret = easy_setup_get_ssid(ssid, sizeof(ssid));
        if (!ret) {
            printf("ssid: %s\n", ssid);
        }

        char password[65]; /* password is 64-char length, plus trailing '\0' */
        ret = easy_setup_get_password(password, sizeof(password));
        if (!ret) {
            printf("password: %s\n", password);
        }

        uint8 protocol;
        ret = easy_setup_get_protocol(&protocol);
        if (ret) {
            printf("failed getting protocol.\n");
        } else if (protocol == EASY_SETUP_PROTO_COOEE) {
            char ip[16]; /* ipv4 max length */
            ret = cooee_get_sender_ip(ip, sizeof(ip));
            if (!ret) {
                printf("sender ip: %s\n", ip);
            }

            uint16 port;
            ret = cooee_get_sender_port(&port);
            if (!ret) {
                printf("sender port: %d\n", port);
            }
        } else if (protocol == EASY_SETUP_PROTO_NEEZE) {
            char ip[16]; /* ipv4 max length */
            ret = neeze_get_sender_ip(ip, sizeof(ip));
            if (!ret) {
                printf("sender ip: %s\n", ip);
            }

            uint16 port;
            ret = neeze_get_sender_port(&port);
            if (!ret) {
                printf("sender port: %d\n", port);
            }
        } else if (protocol == EASY_SETUP_PROTO_AKISS) {
            uint8 random;
            ret = akiss_get_random(&random);
            if (!ret) {
                printf("random: 0x%02x\n", random);
            }
        } else if (protocol == EASY_SETUP_PROTO_CHANGHONG) {
            uint8 sec;
            ret = changhong_get_sec_mode(&sec);
            if (!ret) {
                printf("sec mode: 0x%02x\n", sec);
            }
        } else {
           printf("unknown protocol %d\n", protocol);
        }

        /* if easy_setup_get_security() returns -1, try it more times */
        int tries = 3;
        while (tries--) {
            ret = easy_setup_get_security();
            if (ret != -1) break;
        }
        printf("security: ");
        if (ret == WLAN_SECURITY_WPA2) printf("wpa2\n");
        else if (ret == WLAN_SECURITY_WPA) printf("wpa\n");
        else if (ret == WLAN_SECURITY_WEP) printf("wep\n");
        else if (ret == WLAN_SECURITY_NONE) printf("none\n");
        else printf("wpa2");
    }

    /* must do this! */
    easy_setup_stop();

    return 0;
}

