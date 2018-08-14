/*******************************************************************************
 * test_hibernation_air_api.cpp
 *
 * History:
 *   Jan 12, 2017 - [Shupeng Ren] created file
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_api_helper.h"
#include "am_api_video.h"
#include "am_api_media.h"
#include "am_log.h"

int main(int argc, char **argv)
{
  int ret = 0;
  AMAPIHelperPtr api_helper = nullptr;
  do {
    if (!(api_helper = AMAPIHelper::get_instance())) {
      ret = -1;
      ERROR("Failed to get AMAPIHelper instance!");
      break;
    }
    am_service_result_t service_result;
    //Set video but don't change IAV mode
    api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_IAV_CURRENT_MODE,
                      nullptr, 0,
                      &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to set to IAV current mode!");
      break;
    }

    //Start to create record engine
    int32_t enable_aec = 0;
    api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_RECORDING,
                            &enable_aec, sizeof(enable_aec),
                            &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to create recording instance!");
    }
  } while (0);

  return ret;
}
