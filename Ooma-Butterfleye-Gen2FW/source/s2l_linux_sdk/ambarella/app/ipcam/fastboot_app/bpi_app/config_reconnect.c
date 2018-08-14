/*
 * config_reconnect.c
 *
 * History:
 *       2016/05/31 - [Jb xing] created file
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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>

#include "bpi_typedefs.h"
#include "bpi_wlan.h"
#include "bpi_utils.h"
#include "bpi_app_config.h"
#include "device.h"
#include "cloud_comm.h"

#define EXIT_PARAM 1
#define EXIT_RECONNECT 2
#define EXIT_CONFIG 3
#define EXIT_WAIT_WLAN0 4
#define EXIT_KEEPALIVE 5
#define EXIT_LOAD_CONFIG 6
#define EXIT_UNKNOWN 7

static app_conf_t g_app_config;
static const char *short_options = "crh";

static struct option long_options[] =
{
    { "config", no_argument, 0, 'c' },
    { "reconnect", no_argument, 0, 'r' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
};

static const char* prompt[]=
{
     "elektra config mode",
     "elektra reconnect mode",
     "config_reconnect help"
};

static int usage(void)
{
    int i = 0;
    for(;i<(sizeof(long_options)/sizeof(long_options[0]))-1;i++)
    {
        printf("-%c,  ",long_options[i].val);
        printf("--%-20s  ",long_options[i].name);
        printf("\t%s\n",prompt[i]);
    }
    exit(EXIT_UNKNOWN);
}

static int config_mode(void)
{
    LOG_DEBUG("CONFIG MODE \n");
    cooee_wlan_conf_t save_wlan = {0};
    if(config_elektra_boot(&save_wlan))
        return EXIT_CONFIG;
    else
    {
        //save wlan info
        if(save_wlan_config(BPI_CONFIG_PATH,&save_wlan)!=0)
        {
            LOG_ERROR("save wlan infomation failed");
        }
        else
        {
            LOG_DEBUG("save wlan info OK \n");
        }

        if(0==wlan_keepalive(g_app_config.keepalive_interval,g_app_config.dtim_interval,g_app_config.wakeup_token,g_app_config.cloud_server_ip))
        {
            LOG_DEBUG("wlan standby done\n");
            return 0;
        }
        else
        {
            LOG_ERROR("Cloud/AP not connected, failed to switch to standby state\n");
            return EXIT_KEEPALIVE;
        }
    }
}

static int reconnect_mode(void)
{
    LOG_DEBUG("RECONNECT MODE\n");

    if(connect_ap(g_app_config.ap_ssid, g_app_config.ap_password, g_app_config.ap_wpa)==EWlan_Connected)
    {
        LOG_DEBUG("FASTBOOT_MODE_WLAN_UP: OK \n");
        if(0==wlan_keepalive(g_app_config.keepalive_interval,g_app_config.dtim_interval,g_app_config.wakeup_token,g_app_config.cloud_server_ip))
        {
            LOG_DEBUG("wlan standby done\n");
            return   0;
        }
        else
        {
            LOG_ERROR("Cloud/AP not connected, failed to switch to standby state\n");
            return EXIT_KEEPALIVE;
        }
    }
    else
    {
        LOG_ERROR("FASTBOOT_MODE_WLAN_UP: FAIL \n");
        return EXIT_RECONNECT;
    }
}

static int init_param(int argc,char *argv[])
{
    int ret = 0;
    while((ret=getopt_long(argc,argv,short_options,long_options,NULL))!=-1)
    {
        switch(ret)
        {
            case 'c':
                return  config_mode();
            case 'r':
                return reconnect_mode();
            case 'h':
            default:
                usage();
        }
    }
    return 0;
}

int main(int argc,char **argv)
{
    if(argc<2)
    {
        usage();
    }
    memset(&g_app_config,0,sizeof g_app_config);
    if(load_app_conf(&g_app_config) < 0){
        LOG_ERROR("load app config failed\n");
        return -1;
    }

    int ret = init_param(argc,argv);
    system("echo mem > /sys/power/state");
    return ret;
}
