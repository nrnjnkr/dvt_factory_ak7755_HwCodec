#include <stdio.h>
#include <unistd.h>
#include "easy_setup.h"

#include <string.h>
#include "cooee.h"
#include "akiss.h"
#include "neeze.h"
#include "changhong.h"

int debug_enable = 0;
char filename[256] = {0};
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

int cooee_interface(char *file)
{
    int ret;
    int len;
    int ret_ori = -2;
//    uint16 val;

    easy_setup_enable_cooee();
    strcpy(filename,file);
    ret = easy_setup_start();
    if (ret) return -1;

    /* query for result, blocks until cooee comes or times out */
    ret = easy_setup_query();
    ret_ori = ret;
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
//####################
    if ((filename[0] != '\0' ) && (ssid[0] != '\0') && (password[0] != '\0') && (protocol == EASY_SETUP_PROTO_COOEE)) {
        int fd_conf = -1;
        int size = 0;
        char buf[1024];
        do {
            fd_conf = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666);
            if (fd_conf < 0) {
                perror("open");
                break;
            }
            snprintf(buf, sizeof(buf), "ssid=%s\npassword=%s\n", ssid,password);
            len = strlen(buf);
            size = write(fd_conf, buf, len);
            if (size != len) {
                printf("Write error\n");
            }
            close(fd_conf);
            fd_conf= -1;
            printf("Save config [%s] done\n", filename);
        } while (0);
    }

//####################
}
    /* must do this! */
    easy_setup_stop();

    if(ret_ori == -1)
    {
        return -1;
    }
    return 0;
}

