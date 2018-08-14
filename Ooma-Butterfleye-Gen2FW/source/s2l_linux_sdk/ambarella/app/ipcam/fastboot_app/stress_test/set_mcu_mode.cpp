/*
 * set_mcu_mode.cpp
 *
 * History:
 *       2016/09/13 - [j Yi] created file
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
#include "bpi_typedefs.h"
#include "bpi_utils.h"
#include "bpi_mcu_proxy.h"

static int is_number(void *arg)
{
    char *p = (char*)arg;
    //assume *arg is not NULL
    while(*p != '\0'){
        if(*p < '0' || *p > '9')
            return 0;
        p++;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    BPIMcuProxy mcu_proxy;
    MCU_CMD_TYPE mcu_cmd = MCU_CMD_SET_STRESS_TEST_MODE;
    int info_print = 0;
    do{
        if(argc != 2 || !is_number(argv[1])){
            info_print = 1;
            break;
        }

        if(atoi(argv[1]) == 1){
            mcu_cmd = MCU_CMD_SET_STRESS_TEST_MODE;
        }
        else if(atoi(argv[1]) == 0){
            mcu_cmd = MCU_CMD_SET_NORMAL_MODE;
        }
        else{
            info_print = 1;
            break;
        }

        if(!mcu_proxy.init()){
            break;
        }
        mcu_proxy.send_cmd(mcu_cmd);
        LOG_PRINT("set mode done\n");
    }while(0);

    if(info_print){
        LOG_PRINT("set mcu mode: parameters wrong\n");
        LOG_PRINT("Usgae:\n");
        LOG_PRINT("\t[0] normal mode. [1] stress test mode\n");
    }
    return 0;
}