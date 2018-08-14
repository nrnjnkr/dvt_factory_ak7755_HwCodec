/**********************************************************************
 * Main.cpp
 *
 * Histroy:
 *  2011年03月17日 - [Yupeng Chang] created file
 *
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

#include "Daemonize.h"
#include "IPCamControlServer.h"
#include <pthread.h>

#ifdef DEBUG
IPCamControlServer * ipcamControlServer;
void signal_handler (int sig)
{
  ipcamControlServer->stop_server();
}

int main (int argc, char * argv[])
{
  ipcamControlServer = new IPCamControlServer ();
  signal (SIGTERM, signal_handler);
  signal (SIGINT, signal_handler);
  signal (SIGQUIT, signal_handler);
  ipcamControlServer->start_server(ipcamControlServer);
  delete ipcamControlServer;
  return 0;
}
#else
int main (int argc, char * argv[])
{
  Daemonize            daemonize(argv[0]);
  IPCamControlServer * ipcamControlServer = 0;
  pthread_t thread;
  if (argc > 2) {
    fprintf (stderr,
             "Usage: AmbaIPCamControlServer {[--kill | -k] | [--check | -c] }\n");
    return -1;
  } else if (argc == 2) {
    if ((strcmp(argv[1], "--kill") == 0) ||
        (strcmp(argv[1], "-k") == 0)) {
      if (daemonize.is_daemon_running()) {
        daemonize.kill_daemon ();
      }
      return 0;
    } else if ((strcmp(argv[1], "--check") == 0) ||
               (strcmp(argv[1], "-c") == 0)) {
      if (daemonize.is_daemon_running()) {
#ifdef DEBUG
        fprintf (stdout, "AmbaIPCamControlServer is running on pid %d\n", daemonize.get_pid());
#endif
        return 0;
      }
#ifdef DEBUG
      fprintf (stderr, "AmbaIPCamControlServer is not running.\n");
#endif
      return 1;
    } else {
      fprintf (stderr, "Usage: AmbaIPCamControlServer {[--kill | -k] | [--check | -c] }\n");
      return -1;
    }
  } else {
    Daemonize::DaemonStatus daemon_status;
    daemon_status = daemonize.create_daemon();
    if (daemon_status == Daemonize::ERROR) {
      return -1;
    } else if ((daemon_status == Daemonize::P_ERROR) ||
               (daemon_status == Daemonize::P_RETURN)) {
#ifdef DEBUG
      fprintf (stderr, "Parent returned\n");
#endif
      return (daemon_status == Daemonize::P_ERROR ? -2: 0);
    } else if (daemon_status == Daemonize::C_ERROR) {
#ifdef DEBUG
      fprintf (stderr, "Daemon Create failed!\n");
#endif
      return -2;
    } else if (daemon_status == Daemonize::C_RETURN) {
      /* Process has become to a daemon process,
       * start the real server now
       */
      ipcamControlServer = new IPCamControlServer ();
      pthread_create (&thread,
                      0,
                      IPCamControlServer::start_server,
                      (void *)ipcamControlServer);
    }
  }
  daemonize.keep_running ();
  /* Process exited*/
  ipcamControlServer->stop_server ();
  delete ipcamControlServer;
  return 0;
}
#endif //DEBUG
