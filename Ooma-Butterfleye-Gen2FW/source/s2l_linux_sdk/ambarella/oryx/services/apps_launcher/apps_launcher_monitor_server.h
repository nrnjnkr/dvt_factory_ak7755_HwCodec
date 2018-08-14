/*******************************************************************************
 * apps_launcher_monitor_server.h
 *
 * History:
 *   Jan 18, 2017 - [Niang Zhang] created file
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
#ifndef APPS_LAUNCHER_MONITOR_SERVER_H_
#define APPS_LAUNCHER_MONITOR_SERVER_H_

#include <sys/un.h>
#include <memory>
#include <vector>
#include <condition_variable>
#include "am_thread.h"
#include "am_mutex.h"

#include "apps_launcher_monitor_type.h"

#define MAX_CONNECTION_NUMBER 20
#define MAX_EVENT_NUM 20
#define DEXPORT_PATH ("/run/oryx/apps_launcher_MONITOR.socket")

class AppsLauncherMonitorServer;

typedef std::shared_ptr<AppsLauncherMonitorServer> AppsLauncherMonitorServerPtr;

class AppsLauncherMonitorServer
{
  public:
    static AppsLauncherMonitorServerPtr create();
    void set_state(APPS_LAUNCHER_STATE state);
    MONITOR_COMMAND wait_command();

  private:
    int server_start();
    int server_create();
    void server_close();
    static void thread_transfer_entry(void *arg);
    int connect_client();
    void transfer_process();
    void send_to_all();
    int setnonblock(int sock);
    void close_and_remove_clientfd(int fd);

  private:
    AppsLauncherMonitorServer();
    virtual ~AppsLauncherMonitorServer();

  private:
    APPS_LAUNCHER_STATE m_state = APPS_LAUNCHER_START_WAIT;
    MONITOR_COMMAND m_command = MONITOR_COMMAND_IDLE;
    AMThread *m_transfer_thread = nullptr;
    int m_listenfd = -1;
    int m_acceptfd = -1;
    int m_epoll_fd = -1;
    sockaddr_un m_addr = {0};
    int m_socketpair_fd[2] = {-1, -1};
    std::mutex m_main_mutex;
    std::condition_variable m_main_cond;
    std::atomic_bool m_main_run = {true};
    std::atomic_bool m_main_exit = {false};
    std::vector<int> m_connected_fds;
};

#endif /* AM_APPS_LAUNCHER_MONITOR_SERVER_H_ */
