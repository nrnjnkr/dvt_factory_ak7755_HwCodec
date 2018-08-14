/*******************************************************************************
 * apps_launcher_monitor.h
 *
 * History:
 *   Jan 19, 2017 - [Ning Zhang] created file
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
#ifndef APPS_LAUNCHER_MONITOR_H_
#define APPS_LAUNCHER_MONITOR_H_

#include <sys/un.h>
#include <memory>
#include <condition_variable>
#include "am_thread.h"
#include "am_mutex.h"

#include "apps_launcher_monitor_type.h"

#define MAX_EVENT_NUM 20
#define DEXPORT_PATH ("/run/oryx/apps_launcher_MONITOR.socket")

class AppsLauncherMonitor;

typedef std::shared_ptr<AppsLauncherMonitor> AppsLauncherMonitorPtr;

class AppsLauncherMonitor
{
  public:
    static AppsLauncherMonitorPtr create();
    MONITOR_RESULT wait(MONITOR_WORK_TYPE type);
    void stop_monitor();

  private:
    int client_start();
    int client_create();
    void client_close();
    static void thread_transfer_entry(void *arg);
    int transfer_process();
    int client_work();

  private:
    AppsLauncherMonitor();
    virtual ~AppsLauncherMonitor();

  private:
    AMThread *m_transfer_thread = nullptr;
    bool m_client_run = true;
    int m_state = 0;
    int m_command = 0;
    int m_sockfd = 0;
    int m_epoll_fd = -1;
    int m_socketpair_fd[2] = {-1, -1};
    sockaddr_un m_addr = {0};
    MONITOR_RESULT m_result = MONITOR_RESULT_OK;
    MONITOR_WORK_TYPE m_type = MONITOR_IDLE;
    std::mutex m_main_mutex;
    std::condition_variable m_main_cond;
    std::atomic_bool m_main_run = {false};
    std::mutex m_transfer_mutex;
    std::condition_variable m_transfer_cond;
    std::atomic_bool m_transfer_run = {false};
};

#endif /* APPS_LAUNCHER_MONITOR_H_ */
