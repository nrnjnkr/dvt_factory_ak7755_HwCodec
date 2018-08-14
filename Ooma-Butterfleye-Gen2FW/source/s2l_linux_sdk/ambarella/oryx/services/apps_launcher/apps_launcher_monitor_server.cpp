/*******************************************************************************
 * apps_launcher_monitor_server.cpp
 *
 * History:
 *   Jan 18, 2017 - [Ning Zhang] created file
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
#include <pthread.h>
#include <sys/epoll.h>
#include <algorithm>
#include <fcntl.h>
#include "am_signal.h"
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_file.h"
#include "am_io.h"
#include "apps_launcher_monitor_server.h"

#define AM_IPC_CLOSE_FD(fd) \
  do { \
    if (fd > 0) { \
      close(fd); \
      fd = -1; \
    } \
  } while (0)

AppsLauncherMonitorServerPtr AppsLauncherMonitorServer::create()
{
  AppsLauncherMonitorServer *result = new AppsLauncherMonitorServer();
  if (result && (result->server_start() != 0)) {
    delete result;
    result = nullptr;
  }
  return AppsLauncherMonitorServerPtr(result, [](AppsLauncherMonitorServer *p)
                                    {delete p;});
}

AppsLauncherMonitorServer::AppsLauncherMonitorServer()
{

}

AppsLauncherMonitorServer::~AppsLauncherMonitorServer()
{
  server_close();
}

int AppsLauncherMonitorServer::server_start()
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

void AppsLauncherMonitorServer::thread_transfer_entry(void *arg)
{
  if(!arg){
    ERROR("Thread transfer argument is null");
    return;
  }
  AppsLauncherMonitorServer *p = (AppsLauncherMonitorServer*)arg;
  do {
    if (p->server_create()) {
      ERROR("Failed to create the server.");
      break;
    }
    p->transfer_process();
  } while (0);
}

int AppsLauncherMonitorServer::server_create()
{
  int ret = 0;
  do {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketpair_fd) < 0) {
      ret = -1;
      PERROR("socketpair:");
      break;
    }
    if ((m_listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      ret = -2;
      PERROR("socket:");
      break;
    }
    if (AMFile::exists(DEXPORT_PATH)) {
      NOTICE("%s already exists! Remove it first!", DEXPORT_PATH);
      if (unlink(DEXPORT_PATH) < 0) {
        PERROR("unlink");
        ret = -3;
        break;
      }
    }
    if (!AMFile::create_path(AMFile::dirname(DEXPORT_PATH).c_str())) {
      ERROR("Create path %s error!", AMFile::dirname(DEXPORT_PATH).c_str());
      ret = -4;
      break;
    }
    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", DEXPORT_PATH);
    if (bind(m_listenfd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ret = -5;
      break;
    }
    if (listen(m_listenfd, MAX_CONNECTION_NUMBER) < 0) {
      ret = -6;
      break;
    }
  } while (0);
  return ret;
}

int AppsLauncherMonitorServer::connect_client()
{
  int ret = 0;
  do {
    if (setnonblock(m_listenfd) < 0) {
      ret = -1;
      ERROR("Failed to set m_listenfd to nonblock.");
      break;
    }
    if ((m_acceptfd = accept(m_listenfd, nullptr, nullptr)) < 0) {
      ret = -2;
      PERROR("accept.");
      break;
    }
  } while(0);
  return ret;
}

void AppsLauncherMonitorServer::transfer_process()
{
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);

  do {
    epoll_event ev = {0}, events[MAX_EVENT_NUM] = {0};
    if ((m_epoll_fd = epoll_create1(0)) < 0) {
      PERROR("epoll_create1");
      break;
    }

    ev.data.fd = m_socketpair_fd[1];
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_socketpair_fd[1], &ev) < 0) {
      PERROR("epoll_ctl");
      ERROR("Server process thread is out of control!");
      break;
    }

    ev.data.fd = m_listenfd;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listenfd, &ev)) {
      PERROR("epoll_ctr error");
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
          close_and_remove_clientfd(events[i].data.fd);
          epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
          AM_IPC_CLOSE_FD(events[i].data.fd);
          continue;
        }

        if (events[i].data.fd == m_listenfd) {
          if (connect_client() < 0) {
            continue;
          }
          m_connected_fds.push_back(m_acceptfd);
          if (m_connected_fds.size() > MAX_CONNECTION_NUMBER) {
            int temp = APPS_LAUNCHER_CONNECT_FULL;
            send(m_acceptfd, &temp, 1, 0);
          }
          send_to_all();
          ev.data.fd = m_acceptfd;
          ev.events = EPOLLIN | EPOLLRDHUP;
          if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_acceptfd, &ev) < 0) {
            PERROR("epoll_ctl add:");
            ERROR("Failed to add %d", m_acceptfd);
            continue;
          }
        } else if (events[i].events & EPOLLIN) {
          if (events[i].data.fd == m_socketpair_fd[1]) {
            char cmd = 0;
            if (am_read(m_socketpair_fd[1], &cmd, 1) < 0) {
              PERROR("read");
              continue;
            }
            if (cmd == APPS_LAUNCHER_QUIT) {
              run_flag = false;
              PRINTF("Received quit command, exit socket process thread!");
              break;
            } else if (cmd == APPS_LAUNCHER_SEND) {
              send_to_all();
              if (m_state == APPS_LAUNCHER_STOP_COMPLETION) {
                if (!m_main_exit) {
                  std::unique_lock<std::mutex> lk(m_main_mutex);
                  m_main_exit = true;
                  m_main_cond.notify_all();
                }
                run_flag = false;
              }
            }
          } else if (events[i].data.fd == m_acceptfd){
            if (recv(m_acceptfd, &m_command, 1, 0) > 0) {
              {
                if (!m_main_run) {
                  std::unique_lock<std::mutex> lk(m_main_mutex);
                  m_main_run = true;
                  m_main_cond.notify_all();
                }
              }
            }
          }
        }
      }
    }
  } while (0);
}

int AppsLauncherMonitorServer::setnonblock(int sock)
{
  int ret = 0;
  int opts;
  opts=fcntl(sock,F_GETFL);
  do {
    if(opts<0)
    {
      PERROR("fcntl(sock,GETFL)");
      ret = -1;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
      PERROR("fcntl(sock,SETFL,opts)");
      ret = -2;
    }
  } while (0);
  return ret;
}

void AppsLauncherMonitorServer::close_and_remove_clientfd(int fd)
{
  auto it = std::find_if(m_connected_fds.begin(), m_connected_fds.end(),
                         [=](int f) -> bool {return (f == fd);});
  if (it != m_connected_fds.end()) {
    m_connected_fds.erase(it);
  }
}

void AppsLauncherMonitorServer::send_to_all()
{
  for (auto &v : m_connected_fds) {
    send(v, &m_state, 1, 0);
  }
}

void AppsLauncherMonitorServer::set_state(APPS_LAUNCHER_STATE state)
{
  do {
    m_state = state;
    if (state < 0) {
      ERROR("Please check the apps_launcher_state");
      break;
    }
    int cmd = APPS_LAUNCHER_SEND;
    if (am_write(m_socketpair_fd[0], &cmd, 1, 5) != 1) {
      INFO("Failed to send \"send\" command to client with fd %d: %s!",
            m_socketpair_fd[0], strerror(errno));
    }

    if (state == APPS_LAUNCHER_STOP_SIGNAL) {
      std::unique_lock<std::mutex> lk(m_main_mutex);
      m_main_run = true;
      m_main_cond.notify_all();
      m_command = MONITOR_COMMAND_STOP_SIGNAL;
    }

  } while(0);
}

MONITOR_COMMAND AppsLauncherMonitorServer::wait_command()
{
  MONITOR_COMMAND ret = MONITOR_COMMAND_IDLE;
  do {
    m_main_run = false;
    {
      std::unique_lock<std::mutex> lk(m_main_mutex);
      while (!m_main_run) {
        m_main_cond.wait(lk);
      }
    }
    ret = m_command;
  } while (0);
  return ret;
}

void AppsLauncherMonitorServer::server_close()
{

  int cmd = APPS_LAUNCHER_QUIT;
  if (am_write(m_socketpair_fd[0], &cmd, 1, 5) != 1) {
    ERROR("Failed to send \"quit\" command to client with fd %d: %s!",
          m_socketpair_fd[0], strerror(errno));
  }

  {
    std::unique_lock<std::mutex> lk(m_main_mutex);
    while (!m_main_exit) {
      m_main_cond.wait(lk);
    }
  }

  if (m_acceptfd > 0) {
    shutdown(m_acceptfd, SHUT_RDWR);
    close(m_acceptfd);
    m_acceptfd = -1;
  }

  if (m_listenfd > 0) {
    shutdown(m_listenfd, SHUT_RDWR);
    close(m_listenfd);
    m_listenfd = -1;
    unlink(DEXPORT_PATH);
  }
  AM_IPC_CLOSE_FD(m_epoll_fd);
  AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
  AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
  AM_DESTROY(m_transfer_thread);
}
