#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <errno.h>
#include "am_mp_server.h"
#include "am_mp_bluetooth.h"

int BleHci = 0;
char interface_id[200];
int scan = STOP_ADV;

static int startAdvertisement(char *name) {

    char cmd[300] = {0};
    FILE *fp;
    char buff[1000];
    if(!BleHci) {
        //Load the BT firmware
        snprintf(cmd , 300,"brcm_patchram_plus --patchram /lib/firmware/broadcom/ap6255/BCM4345C0.hcd --enable_hci --no2bytes --tosleep 10 /dev/ttyS1 &");

        system(cmd);
        //Firmware loaded

        //It takes time to load the firmware
        sleep(9);

        snprintf(cmd , 300, "hciconfig");
        char *str;
        if((fp = popen(cmd, "r")) != NULL) {
            while(fgets(buff, 1000, fp) != NULL) {
                if(strstr(buff, "hci")) {
                    str = strtok(buff, ":");
                    strcpy(interface_id, str);
                    //fprintf(stderr, "Interface name %s\n", interface);
                    BleHci = 1;
                    break;
                }
            }
            pclose(fp);
        }
    }

    if(scan == START_ADV) {
        strcpy(name, "Scan in process\n");
        return MP_OK;
    }
    char *ble_name = NULL;
    if(interface_id != NULL) { //HCI is up
        memset(cmd, 0 , 300);
        snprintf(cmd , 300, "hciconfig %s up", interface_id);
        sleep(1);
        system(cmd);
        sleep(1);
        memset(cmd, 0 , 300);
        snprintf(cmd , 300, "hciconfig %s name", interface_id);
        if((fp = popen(cmd ,"r")) != NULL) {
            while (fgets(buff, 1000, fp) != NULL) {
                if(strstr(buff, "Name")) {
                    strtok(buff, ":");
                    ble_name = strtok(NULL, ":");
                    //fprintf(stderr, "ble name %s\n", ble_name);
                    break;
                }
            }
            pclose(fp);
        }
    } else {
        strcpy(name, "Unable to find BLE hci interface");
        return MP_ERROR;
    }

    if(ble_name != NULL) {
        strcpy(name, ble_name);
        //Start Advertisement 
        memset(cmd, 0 , 300);
        snprintf(cmd, 300, "hciconfig %s leadv", interface_id);
        sleep(1);
        system(cmd);
        scan = START_ADV;
        return MP_OK;
    } else {
        strcpy(name, "Unable to find ble name");
        return MP_ERROR;
    }
}

static int stopAdv(char *name) {

    char cmd[100] = {0};

    if (BleHci && (scan == START_ADV)) {
        snprintf(cmd, 100, "hciconfig %s noleadv", interface_id);
        system(cmd);
        strcpy(name, "Bluetooth interface should not found in scan now!!\n");
        scan  = STOP_ADV;
        return MP_OK;
    } else  {
        strcpy(name, "Scan not started or interface is not up\n");
        return MP_ERROR;
    }
}

am_mp_err_t mptool_bluetooth_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{

  to_msg->result.ret = MP_OK;
  switch (from_msg->stage) {
    case START_ADV:
        to_msg->result.ret = startAdvertisement(to_msg->msg);
      break;

    case STOP_ADV:
        to_msg->result.ret = stopAdv(to_msg->msg);
      break;
    case 2:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
      }
      break;

    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
