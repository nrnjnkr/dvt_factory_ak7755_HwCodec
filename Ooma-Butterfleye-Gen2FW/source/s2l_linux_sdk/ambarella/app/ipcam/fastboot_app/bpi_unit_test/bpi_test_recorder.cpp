/*
 * Bpi_test_recorder.cpp
 *
 *
 * History:
 *       2017/05/17 - [mzchen] created file
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
#include "bpi_utils.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_oryx_config.h"
#include "bpi_oryx_export.h"

static int test_recorder(void)
{
    LOG_PRINT("test_recorder, get_current_time = %u\n",get_current_time());
    app_conf_t app_config;

    if(load_app_conf(&app_config) < 0) {
        LOG_ERROR("load app config failed\n");
        return -1;
    }
    //config oryx settings
    if(!config_oryx_engine(&app_config)) {
        LOG_ERROR("fail to config oryx configuration files.\n");
        return -1;
    }

    OryxRecorderWrapper recorder;
    recorder.set_data_handler(NULL);
    if(!recorder.init_recorder(&app_config)) {
        LOG_ERROR("Failed to init recorder engine.\n");
        return -1;
    }
    recorder.start_recorder();
    sleep(20);
    recorder.stop_recorder();
    return 0;
}

int main(int argc, char **argv)
{
    int ret = 1;
    int log_options = BPI_LOG_ENABLE_DEBUG_LOG;

    do {
        if(argc > 1 ) {
            printf("test_bpi_recorder has none parameter.\n");
            break;
        }
        set_log_options(log_options);

        if(test_recorder()) {
            LOG_ERROR("recorder failed\n");
            break;
        }
        ret = 0;
    } while(0);

    return ret;
}

