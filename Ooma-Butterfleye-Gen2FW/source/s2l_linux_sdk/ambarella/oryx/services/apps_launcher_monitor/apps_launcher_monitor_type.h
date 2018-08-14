/*******************************************************************************
 * apps_launcher_monitor_type.h
 *
 * History:
 *   Feb 24, 2017 - [nzhang] created file
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
 ******************************************************************************/
#ifndef ORYX_SERVICES_APPS_LAUNCHER_MONITOR_APPS_LAUNCHER_MONITOR_TYPE_H_
#define ORYX_SERVICES_APPS_LAUNCHER_MONITOR_APPS_LAUNCHER_MONITOR_TYPE_H_

enum APPS_LAUNCHER_STATE {
  APPS_LAUNCHER_IDLE                   = 0,
  APPS_LAUNCHER_START_WAIT             = 1,
  APPS_LAUNCHER_STARTING               = 2,
  APPS_LAUNCHER_START_COMPLETION       = 3,
  APPS_LAUNCHER_START_ERROR            = 4,
  APPS_LAUNCHER_STOPPING               = 5,
  APPS_LAUNCHER_STOP_COMPLETION        = 6,
  APPS_LAUNCHER_STOP_SIGNAL            = 7,
  APPS_LAUNCHER_CONNECT_FULL           = 8,
};

enum MONITOR_COMMAND {
  MONITOR_COMMAND_IDLE                 = 0,
  MONITOR_COMMAND_START_SIGNAL         = 1,
  MONITOR_COMMAND_STOP_SIGNAL          = 2,
};

enum MONITOR_WORK_TYPE {
  MONITOR_IDLE                         = 0,
  MONITOR_WAIT_AND_START               = 1,
  MONITOR_WAIT_AND_STOP                = 2,
};

enum MONITOR_RESULT {
  MONITOR_RESULT_OK                    = 0,
  MONITOR_RESULT_START_WAIT            = 1,
  MONITOR_RESULT_STARTING              = 2,
  MONITOR_RESULT_START_COMPLETION      = 3,
  MONITOR_RESULT_START_ERROR           = 4,
  MONITOR_RESULT_STOPPING              = 5,
  MONITOR_RESULT_STOP_COMPLETION       = 6,
  MONITOR_RESULT_RECEIVE_DATA_ERROR    = 7,
  MONITOR_RESULT_SEND_CAMMAND_ERROR    = 8,
  MONITOR_RESULT_CREATE_CLIEND_ERROR   = 9,
  MONITOR_RESULT_CONNECTION_FULL       = 10,
  MONITOR_RESULT_CONNECTION_FAILED     = 11,
};

enum APPS_LAUNCHER_CMD {
  APPS_LAUNCHER_QUIT                   = 0,
  APPS_LAUNCHER_SEND                   = 1,
};

#endif /* ORYX_SERVICES_APPS_LAUNCHER_MONITOR_APPS_LAUNCHER_MONITOR_TYPE_H_ */
