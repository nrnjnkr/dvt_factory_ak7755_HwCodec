/*
 * pba_upgrade.c
 *
 * History:
 *       2015/09/15 - [Jian Liu] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dec7z_utils.h"

#define ENTER_MAIN_CMDLINE  "upgrade_partition -S 0 -C"
#define SET_UPGRADE_FLAG      "upgrade_partition -P"
#define MAINSYS_CMD_LINE       "console=tty0 root=/dev/mtdblock7 ro rootfstype=squashfs init=/linuxrc noinitrd lpj=5009408"

#define UPGRADE_PACKAGE   "/tmp/sdcard/elektra_upgrade.7z"
#define UPDATE_PATH  "/tmp/update_img/"
#define KERNERL_IMG   "/tmp/update_img/Image"
#define ROOTFS_IMG    "/tmp/update_img/squashfs"
#define DSP_IMG           "/tmp/update_img/dsp.bin"

static int config_partition_mounted = 0;
static int sdcard_mounted = 0;
static int update_kernel = 0;
static int update_system = 0;
static int update_dsp = 0;

static unsigned  get_current_time(void){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC,&now);
    unsigned mseconds = now.tv_sec * 1000+ now.tv_nsec/1000000;
    return mseconds;
}

static int check_prepare_sdcard(void){
    system("/sbin/modprobe ehci_hcd");
    system("/bin/echo host > /proc/ambarella/usbphy0");
    system("/sbin/modprobe scsi_mod");
    system("/sbin/modprobe usb_storage");
    system("/sbin/modprobe sd_mod");
    system("/sbin/modprobe fat");
    system("/sbin/modprobe vfat");
    system("/bin/mknod /dev/sda b 8 0");
    system("/bin/mknod /dev/sda1 b 8 1");
    system("/bin/mkdir -p /tmp/sdcard");

    unsigned start = get_current_time();
    while(1){
        int status = system("/bin/mount -t vfat /dev/sda1 /tmp/sdcard");
        if (WEXITSTATUS(status)) {
            usleep(1000 * 1000);

            unsigned now = get_current_time();
            if(now - start > 5000){
                printf("mount sdcard failed\n");
                fflush(stdout);
                return -1;
            }
            continue;
        }
        break;
    }
    sdcard_mounted = 1;
    printf("mount sdcard successfully\n");
    fflush(stdout);
    return 0;
}
static void umount_sdcard(void){
    if(sdcard_mounted){
        system("/bin/umount /tmp/sdcard");
    }
}

static int prepare_upgrade_package(void){
    if(access(UPGRADE_PACKAGE, F_OK)) {
        printf("upgrade_package does not exist, [%s] expected\n",UPGRADE_PACKAGE);
        fflush(stdout);
        return -1;
    }
    void *dec7z = dec7z_create(UPGRADE_PACKAGE);
    if(!dec7z){
        printf("Failed to create dec7z handle\n");
        fflush(stdout);
        return -1;
    }

    system("/bin/mkdir -p /tmp/update_img/");//UPDATE_PATH
    if(dec7z_dec(dec7z, UPDATE_PATH) < 0){
        printf("Failed to do dec7z\n");
        fflush(stdout);
        dec7z_destroy(dec7z);
        return -1;
    }

    if(dec7z_is_file_exist(dec7z,"Image")){
        printf("kernel will be updated\n");
        fflush(stdout);
        update_kernel = 1;
    }

    if(dec7z_is_file_exist(dec7z,"squashfs")){
        printf("rootfs will be updated\n");
        fflush(stdout);
        update_system = 1;
    }

    if(dec7z_is_file_exist(dec7z,"dsp.bin")){
        printf("dsp will be updated\n");
        fflush(stdout);
        update_dsp = 1;
    }
    dec7z_destroy(dec7z);
    if(update_kernel || update_system || update_dsp){
        return 0;
    }
    printf("invalid upgrade package\n");
    fflush(stdout);
    return -1;
}

static int update_partition_pri(int verbose){
    char cmd[256];
    FILE   *stream = 0;
    int device_index = -1;
    int status;

    stream = popen( "cat /proc/mtd | grep pri | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd failed\n");
        fflush(stdout);
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found PRI partition on /proc/mtd\n");
        fflush(stdout);
        return -1;
    }

    //erase nand flash before re-write
    sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }

    //do upgrate partition
    sprintf(cmd, "upgrade_partition -p -K  /dev/mtd%d %s", device_index, KERNERL_IMG);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }
    printf("Update kernel done\n");
    fflush(stdout);
    return 0;
}

static int update_partition_lnx(int verbose){
    char cmd[256];
    FILE   *stream = 0;
    int device_index = -1;
    int status;

    stream = popen( "cat /proc/mtd | grep lnx | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd failed\n");
        fflush(stdout);
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found LNX partition on /proc/mtd\n");
        fflush(stdout);
        return -1;
    }

    //erase nand flash before re-write
    sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }

    //do upgrate partition
    sprintf(cmd, "upgrade_partition -p -F 1 -W  /dev/mtd%d %s", device_index, ROOTFS_IMG);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }
    printf("Update rootfs done\n");
    fflush(stdout);
    return 0;
}

static int update_partition_dsp(int verbose){
    char cmd[256];
    FILE   *stream = 0;
    int device_index = -1;
    int status;

    stream = popen( "cat /proc/mtd | grep dsp | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd failed\n");
        fflush(stdout);
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found DSP partition on /proc/mtd\n");
        fflush(stdout);
        return -1;
    }

    //erase nand flash before re-write
    sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }

    //do upgrate partition
    sprintf(cmd, "upgrade_partition -p -U  /dev/mtd%d %s", device_index, DSP_IMG);
    if (verbose) {
        printf("CMD: %s\n", cmd);
        fflush(stdout);
    }
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n",cmd);
        fflush(stdout);
        return -1;
    }
    printf("Update dsp done\n");
    fflush(stdout);
    return 0;
}

static int  do_update(int verbose){
    int result = 0;
    do {
        if (update_kernel) {
            printf("Updating kernel...\n");
            fflush(stdout);
            if(update_partition_pri(verbose) < 0){
                result = -1;
                break;
            }
        }
        if (update_dsp) {
            printf("Updating dsp...\n");
            fflush(stdout);
            if(update_partition_dsp(verbose) < 0){
                result = -1;
                break;
            }
        }
        if (update_system) {
            printf("Updating filesystem...\n");
            fflush(stdout);
            if(update_partition_lnx(verbose) < 0){
                result = -1;
                break;
            }
        }
    } while (0);
    return result;
}

static int  set_bois_to_main_sys(){
    char cmd[256] = { 0 };
    int32_t status;

    sprintf(cmd, "%s \"%s\"", ENTER_MAIN_CMDLINE, MAINSYS_CMD_LINE);
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd);
        fflush(stdout);
        return -1;
    }
    return 0;
}

/*
static int create_mtd_device_node(int verbose){
    char cmd[256];

    //mknod /dev/mtd
    int status;
    int device_index;
    for(device_index = 0; device_index < 16; device_index++){
        sprintf(cmd, "mknod /dev/mtd%d c 90 %d",device_index,device_index * 2);
        if (verbose) {
            printf("CMD: %s\n", cmd);
            fflush(stdout);
        }
        status = system(cmd);
        if (WEXITSTATUS(status)) {
            printf("%s :failed\n",cmd);
            fflush(stdout);
        }
    }
    return 0;
}
*/
static int mount_config_partition_add(void){
    FILE   *stream = 0;
    int device_index = -1;
    stream = popen( "cat /proc/mtd | grep add | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("mount_config_partition_add --- Open /proc/mtd failed\n");
        fflush(stdout);
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found ADD partition on /proc/mtd\n");
        fflush(stdout);
        return -1;
    }

    /*
    root@Ambarella$cat /sys/class/ubi/ubi0/dev
    251:0
    root@Ambarella$cat /sys/class/ubi/ubi0_0/dev
    251:1
    root@Ambarella$cat /sys/class/misc/ubi_ctrl/dev
    10:60
    */
    system("mknod /dev/ubi0 c 252 0");
    system("mknod /dev/ubi0_0 c 252 1");
    system("mknod /dev/ubi_ctrl c 10 60");

    char cmd[256] = { 0 };
    int32_t status;
    sprintf(cmd, "ubiattach  /dev/ubi_ctrl -d 0 -m %d",device_index);
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd);
        fflush(stdout);
        return -1;
    }

    sprintf(cmd, "mount -t ubifs ubi0_0  /mnt");
    status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd);
        fflush(stdout);
        return -1;
    }

    config_partition_mounted = 1;
    return 0;
}
static void umount_config_partition_add(void){
    if(config_partition_mounted){
        system("umount  /mnt");
        system("ubidetach  /dev/ubi_ctrl -d 0");
    }
}

static int upgrade_log(char *log){
    FILE *fp = fopen("/mnt/upgrade_log","a");
    if(fp){
        fprintf(fp,"%s",log);
        fclose(fp);
        return 0;
    }
    return -1;
}

int  main(int argc, char *argv[]){
    printf("Begin to upgrade\n");
    fflush(stdout);
    if(mount_config_partition_add() < 0){
        goto to_main_sys;
    }

    char log[256];
    snprintf(log,sizeof(log),"---------------------------PBA_UPGRADE_LOG__%u---------------------------\n",(unsigned)time(NULL));
    upgrade_log(log);

    upgrade_log("upgrade prepare , mount sdcard...\n");
    if(check_prepare_sdcard() < 0){
        upgrade_log("upgrade prepare , mount sdcard....failed\n");
        goto to_main_sys;
    }

    upgrade_log("upgrade unpacking upgrade_package...\n");
    if(prepare_upgrade_package() < 0){
        upgrade_log("upgrade unpacking upgrade_package...failed\n");
        goto to_main_sys;
    }

    upgrade_log("upgrade writing firmware to flash...\n");
    if (do_update(1) < 0) {
        upgrade_log("upgrade writing firmware to flash...failed\n");
        goto to_main_sys;
    }

to_main_sys:
    upgrade_log("upgrade init main_sys...\n");
    if(set_bois_to_main_sys() < 0  ) {
        upgrade_log("upgrade init main_sys...failed\n");
    }else{
        upgrade_log("upgrade done successfully\n");
    }

    umount_sdcard();
    umount_config_partition_add();
    sync();
    sync();
    reboot(RB_AUTOBOOT);
    return 0;
}

