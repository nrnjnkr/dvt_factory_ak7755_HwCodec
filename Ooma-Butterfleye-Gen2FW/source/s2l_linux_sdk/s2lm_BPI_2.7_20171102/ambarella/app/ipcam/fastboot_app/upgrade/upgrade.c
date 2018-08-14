/*
 * upgrade.c
 *
 * History:
 *       2015-09-15 - [Jian Liu] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <pthread.h>

#define ENTER_PBA_CMDLINE  "upgrade_partition -S 2 -C"
#define PBA_CMD_LINE \
  ("console=ttyS0 rootfs=ramfs root=/dev/ram rw "\
  "rdinit=/linuxrc")

static int set_upgrade_cmd(){
    char cmd[512];
    snprintf(cmd,sizeof(cmd),"%s \"%s\"",ENTER_PBA_CMDLINE, PBA_CMD_LINE);
    printf("cmd = %s\n",cmd);
    fflush(stdout);
    int status = system(cmd);
    if (WEXITSTATUS(status)) {
        printf("set_upgrade_cmd, failed, cmd [%s]\n",cmd);
        fflush(stdout);
        return -1;
    }
    printf("set_upgrade_cmd, sucess, cmd [%s]\n",cmd);
    fflush(stdout);
    return 0;
}

int main(int argc, char *argv[]){
    if(set_upgrade_cmd() < 0){
        return -1;
    }
    return 0;
}

