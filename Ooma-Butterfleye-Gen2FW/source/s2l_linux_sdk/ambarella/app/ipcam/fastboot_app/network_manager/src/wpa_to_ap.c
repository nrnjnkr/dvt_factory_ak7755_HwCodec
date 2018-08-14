/*
 * wpa_to_ap.c
 *
 * History:
 *       2015/12/24 - [jbxing] created file
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
#include"wpa_to_ap.h"
#define ITEMS 4
#define LEN 128
#define KEY_COUNTS 4
static void handle_error(const char*error_info)
{
    fprintf(stderr,"ERROR! %s\n", error_info);
    fflush(stderr);
}
int parse_cooee_conf(char *ssid, char *password, char *url, char *wpa)
{
    int ret = -1;
    FILE *fp = NULL;
    do{
        fp = fopen(COOEE_CONF_PATH,"r");
        if(NULL == fp)
        {
            perror("ERROR! fopen");
            break;
        }
        char buf[LEN] = {0};
        char keys[KEY_COUNTS][32] = {{"ssid="}, {"password="}, {"url="}, {"wpa="}};
        char *values[KEY_COUNTS]  = {ssid,password,url,wpa};
        char *start_p = NULL;
        char *end_p  = NULL;
        int index = 0;

        for(;index < sizeof(values)/sizeof(values[0]);index++)
        {
            memset(buf, '\0', sizeof(buf));
            if(fgets(buf,sizeof(buf),fp) != NULL)
            {
                if((start_p=strstr(buf,keys[index])) != NULL)
                {
                    start_p += strlen(keys[index]);
                    if((end_p=strchr(start_p,'\n')) != NULL)
                    {
                        strncpy(values[index],start_p,end_p-start_p);
                    }
                    else
                    {
                        handle_error("could not found cooee value end_indicator");
                        break;
                    }
                }
                else
                {
                    handle_error("could not found cooee key");
                    break;
                }
            }
            else
            {
                if(ferror(fp))
                {
                    perror("ERROR! fgets");
                }
                if(feof(fp))
                {
                    char buf[128] = {0};
                    snprintf(buf,sizeof buf,"the %s has ended",COOEE_CONF_PATH);
                    handle_error(buf);
                }
                break;
            }
        }
        if(index >= sizeof(values)/sizeof(values[0]))
        {
            ret = 0;
        }
    }while(0);
    if((NULL != fp) && (0 != fclose(fp)))
    {
        perror("");
        ret = -1;
    }
    return ret;
}
