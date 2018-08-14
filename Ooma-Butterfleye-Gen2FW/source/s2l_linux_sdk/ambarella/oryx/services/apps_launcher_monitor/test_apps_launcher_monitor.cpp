/*******************************************************************************
 * test_apps_launcher_monitor.cpp
 *
 * History:
 *   Jan 17, 2017 - [Ning Zhang] created file
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
#include <getopt.h>
#include "am_log.h"
#include "am_signal.h"

#include "apps_launcher_monitor.h"

MONITOR_WORK_TYPE command_type = MONITOR_IDLE;
AppsLauncherMonitorPtr apps_launcher_monitor = nullptr;
static const char *short_options = "rs";

static void usage()
{
  printf("test_apps_launcher_monitor usage:\n");
  printf("  -r : wait and start apps_launcher\n");
  printf("  -s : wait and stop apps_launcher\n");
}

static int init_param(int argc, char **argv)
{
  int ch;
  while ((ch = getopt(argc, argv, short_options)) != -1) {
    switch (ch) {
      case 'r' :
        command_type = MONITOR_WAIT_AND_START;
        break;
      case 's' :
        command_type = MONITOR_WAIT_AND_STOP;
        break;
      default:
        break;
    }
  }
  if (!command_type) {
    return -1;
  }
  return 0;
}

static void sigstop(int arg)
{
  apps_launcher_monitor->stop_monitor();
  INFO("Apps_Launcher receives signal %d", arg);
}

int main(int argc, char *argv[])
{
  int ret = 0;
  MONITOR_RESULT result = MONITOR_RESULT_OK;
  apps_launcher_monitor = AppsLauncherMonitor::create();
  do {
    if (argc < 2) {
      usage();
      ret = -1;
      break;
    }
    if (init_param(argc, argv) < 0) {
      ERROR("Failed to init_param!");
      ret = -2;
      break;
    }

    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    if (command_type == MONITOR_WAIT_AND_START) {
      result = apps_launcher_monitor->wait(MONITOR_WAIT_AND_START);
    } else if (command_type == MONITOR_WAIT_AND_STOP) {
      result = apps_launcher_monitor->wait(MONITOR_WAIT_AND_STOP);
    }

    if (result == MONITOR_RESULT_START_COMPLETION) {
      PRINTF("Apps_launcher has started successfully.");
    } else if (result == MONITOR_RESULT_STOP_COMPLETION) {
      PRINTF("Apps_launcher has ended successfully.");
    } else if (result == MONITOR_RESULT_START_ERROR) {
      PRINTF("Apps_launcher failed to start!");
    } else if (result == MONITOR_RESULT_CONNECTION_FULL) {
      PRINTF("Apps_launcher`s connection is full.");
    }
  } while (0);
  apps_launcher_monitor = nullptr;
  return ret;
}
