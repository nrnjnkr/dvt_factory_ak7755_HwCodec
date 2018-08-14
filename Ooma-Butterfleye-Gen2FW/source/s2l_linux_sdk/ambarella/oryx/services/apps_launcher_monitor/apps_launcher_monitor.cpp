/*******************************************************************************
 * apps_launcher_monitor.cpp
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
#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_signal.h"
#include "am_io.h"
#include "am_define.h"
#include "apps_launcher_monitor.h"

#define AM_IPC_CLOSE_FD(fd) \
  do { \
    if (fd > 0) { \
      close(fd); \
      fd = -1; \
    } \
  } while (0)

AppsLauncherMonitorPtr AppsLauncherMonitor::create()
{
  AppsLauncherMonitor *result = new AppsLauncherMonitor();
  if (result && (result->client_start() != 0)) {
    delete result;
    result = nullptr;
  }
  return AppsLauncherMonitorPtr(result, [](AppsLauncherMonitor *p)
                                  {delete p;});
}

AppsLauncherMonitor::AppsLauncherMonitor()
{

}

AppsLauncherMonitor::~AppsLauncherMonitor()
{
  client_close();
}

int AppsLauncherMonitor::client_start()
{
  int ret = 0;
  do {
    if (!(m_transfer_thread = AMThread::create("transfer_thread",
                                              thread_transfer_entry, this))) {
      ERROR("Create transfer_thread error!");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

void AppsLauncherMonitor::thread_transfer_entry(void *arg)
{
  if(!arg){
    ERROR("Thread transfer argument is null");
    return;
  }
  AppsLauncherMonitor *p = (AppsLauncherMonitor*)arg;
  do {
    p->client_work();
  } while (0);
}

int AppsLauncherMonitor::client_create()
{
  int ret = 0;
  do {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketpair_fd) < 0) {
      ret = -1;
      PERROR("socketpair:");
      break;
    }
    if ((m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      ret = -2;
    }
    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", DEXPORT_PATH);
  } while (0);
  return ret;
}

void AppsLauncherMonitor::client_close()
{
  m_client_run = false;
  if (!m_transfer_run) {
    std::unique_lock<std::mutex> lk(m_transfer_mutex);
    m_transfer_run = true;
    m_transfer_cond.notify_all();
  }
  if (m_socketpair_fd[0] > 0) {
    if (am_write(m_socketpair_fd[0], "quit", 4, 5) != 4) {
      ERROR("Failed to send \"quit\" command to client with fd %d: %s!",
            m_socketpair_fd[0], strerror(errno));
    }
  }
  shutdown(m_sockfd, SHUT_RDWR);
  AM_IPC_CLOSE_FD(m_sockfd);
  AM_IPC_CLOSE_FD(m_epoll_fd);
  AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
  AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
  AM_DESTROY(m_transfer_thread);
}

int AppsLauncherMonitor::transfer_process()
{
  int ret = 0;
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);

  do {
    epoll_event ev = {0}, events[MAX_EVENT_NUM] = {0};
    if ((m_epoll_fd = epoll_create1(0)) < 0) {
      PERROR("epoll_create1");
      ret = -1;
      break;
    }
    ev.data.fd = m_socketpair_fd[1];
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_socketpair_fd[1], &ev) < 0) {
      PERROR("epoll_ctl");
      ERROR("Server process thread is out of control!");
      ret = -2;
      break;
    }

    ev.data.fd = m_sockfd;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_sockfd, &ev) < 0) {
      PERROR("epoll_ctl");
      ret = -3;
      break;
    }

    bool run_flag = true;
    while (run_flag) {
      int nfds = epoll_pwait(m_epoll_fd, events, MAX_EVENT_NUM, -1, &sigmask);
      if ((nfds < 0 && errno == EINTR) || (nfds == 0)) {
        continue;
      } else if (nfds < 0) {
        PERROR("epoll_pwait, quit!");
      }
      for (int i = 0; i < nfds; ++i) {
        if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
          if (events[i].data.fd == m_sockfd) {
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_sockfd, nullptr);
            AM_IPC_CLOSE_FD(m_sockfd);
            INFO("Server is shutdown!");
            run_flag = false;
            m_result = MONITOR_RESULT_STOP_COMPLETION;
            break;
          } else if (events[i].data.fd == m_socketpair_fd[1])  {
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_socketpair_fd[1], nullptr);
            AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
            AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
            ERROR("Client process thread is out of control!");
            break;
          } else {
            ERROR("Unknown event fd: %d", events[i].data.fd);
            break;
          }
        }

        if (events[i].events & EPOLLIN) {
          if (events[i].data.fd == m_socketpair_fd[1]) {
            run_flag = false;
            INFO("Received quit command, exit socket process thread!");
            break;
          } else if (events[i].data.fd == m_sockfd) {
            if (recv(m_sockfd, &m_state, 1, 0) == 1) {
              switch(m_state) {
                case APPS_LAUNCHER_START_WAIT: {
                  m_result = MONITOR_RESULT_START_WAIT;
                  INFO("Receive start_wait from apps_launcher success.");
                  if (m_type == MONITOR_WAIT_AND_START) {
                    m_command = MONITOR_COMMAND_START_SIGNAL;
                  } else if (m_type == MONITOR_WAIT_AND_STOP) {
                    m_command = MONITOR_COMMAND_STOP_SIGNAL;
                  } else {
                    run_flag = false;
                    break;
                  }
                  if (send(m_sockfd, &m_command, 1, 0) == 1) {
                    INFO("Send signal to apps_luancher success.");
                  } else {
                    m_result = MONITOR_RESULT_SEND_CAMMAND_ERROR;
                  }
                } break;

                case APPS_LAUNCHER_STARTING: {
                  m_result = MONITOR_RESULT_STARTING;
                  INFO("Receive starting from apps_launcher success.");
                } break;

                case APPS_LAUNCHER_START_COMPLETION: {
                  m_result = MONITOR_RESULT_START_COMPLETION;
                  INFO("Receive start_completion from apps_launcher success.");
                  if (m_type == MONITOR_WAIT_AND_START) {
                    run_flag = false;
                  } else if (m_type == MONITOR_WAIT_AND_STOP) {
                    m_command = MONITOR_COMMAND_STOP_SIGNAL;
                    if (send(m_sockfd, &m_command, 1, 0) == 1) {
                      INFO("Send stop_signal to apps_luancher success.");
                    } else {
                      m_result = MONITOR_RESULT_SEND_CAMMAND_ERROR;
                    }
                  } else {
                    run_flag = false;
                    break;
                  }
                } break;

                case APPS_LAUNCHER_START_ERROR: {
                  m_result = MONITOR_RESULT_START_ERROR;
                  INFO("Receive start_error from apps_launcher success.");
                  run_flag = false;
                } break;

                case APPS_LAUNCHER_STOPPING: {
                  m_result = MONITOR_RESULT_STOPPING;
                  INFO("Receive stopping from apps_launcher success.");
                } break;

                case APPS_LAUNCHER_STOP_COMPLETION: {
                  m_result = MONITOR_RESULT_STOP_COMPLETION;
                  INFO("Receive stop_completion from apps_launcher success.");
                  run_flag = false;
                } break;

                case APPS_LAUNCHER_CONNECT_FULL: {
                  m_result = MONITOR_RESULT_CONNECTION_FULL;
                  INFO("Apps_launcher`s connection is full.");
                  run_flag = false;
                } break;

                default: {
                  INFO("The receive data is %d.", m_state);
                  run_flag = false;
                } break;
              }
            } else {
              m_result = MONITOR_RESULT_RECEIVE_DATA_ERROR;
              run_flag = false;
              break;
            }
          }
        }
      }
    }
    if (!m_main_run) {
      std::unique_lock<std::mutex> lk(m_main_mutex);
      m_main_run = true;
      m_main_cond.notify_all();
    }
  } while (0);
  return ret;
}

int AppsLauncherMonitor::client_work()
{
  int ret = 0;
  int i = 0;
  do {
    if (m_client_run) {
      {
        std::unique_lock<std::mutex> lk(m_transfer_mutex);
        while (!m_transfer_run) {
          m_transfer_cond.wait(lk);
        }
      }
      if (client_create() < 0) {
        ERROR("The client start error!");
        m_result = MONITOR_RESULT_CREATE_CLIEND_ERROR;
        ret = -1;
        break;
      }
      while (connect(m_sockfd, (sockaddr *)&m_addr, sizeof(sockaddr_un)) < 0
          && m_client_run) {
        if (i == 0) {
          PRINTF("The apps_launcher does not start or has stopped.");
          PRINTF("Please wait or quit.");
          i ++;
        }
        m_result = MONITOR_RESULT_CONNECTION_FAILED;
        usleep(100*1000);
      }
      if (transfer_process() < 0) {
        ERROR("Failed to transfer content.");
        ret = -2;
      }
    }
  } while (0);
  return ret;
}

MONITOR_RESULT AppsLauncherMonitor::wait(MONITOR_WORK_TYPE type)
{
  MONITOR_RESULT ret = MONITOR_RESULT_OK;
  do {
    m_type = type;
    if (!m_transfer_run) {
      std::unique_lock<std::mutex> lk(m_transfer_mutex);
      m_transfer_run = true;
      m_transfer_cond.notify_all();
    }
    m_main_run = false;
    {
      std::unique_lock<std::mutex> lk(m_main_mutex);
      while (!m_main_run) {
        m_main_cond.wait(lk);
      }
    }
    ret = m_result;
  } while(0);
  return ret;
}

void AppsLauncherMonitor::stop_monitor()
{
  m_client_run = false;
  if (am_write(m_socketpair_fd[0], "quit", 4, 5) != 4) {
    ERROR("Failed to send \"quit\" command to client with fd %d: %s!",
          m_socketpair_fd[0], strerror(errno));
  }
}
