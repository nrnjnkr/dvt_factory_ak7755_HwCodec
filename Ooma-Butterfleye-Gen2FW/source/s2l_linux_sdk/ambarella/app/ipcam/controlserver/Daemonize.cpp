/*******************************************************************************
 * Daemonize.cpp
 *
 * History:
 *  2011年03月22日 - [Yupeng Chang] created file
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

char globalPidFile[256];

Daemonize::Daemonize (const char * daemonName)
  : mPid (-1),
    mRunning (true)
{
  if (daemonName) {
    memset (mDaemonName, 0, 256);
    strcpy (mDaemonName, daemonName);
    /* Set identification string for the daemon for PID file */
    daemon_pid_file_ident = daemon_ident_from_argv0(mDaemonName);
    daemon_log_ident = daemon_pid_file_ident;
    daemon_pid_file_proc = pid_file_proc;
    sprintf (globalPidFile, "/var/run/%s.pid", daemon_pid_file_ident);
#ifdef DEBUG
    fprintf (stderr, "pid file name is %s\n", daemon_pid_file_ident);
#endif
    /* Reset signal handler */
    if (daemon_reset_sigs (-1) < 0) {
      perror ("Failed to reset all signal handlers : ");
    }
    /* Unblock signals */
    if (daemon_unblock_sigs (-1) < 0) {
      perror ("Failed to unblock all signals : ");
    }
  }
}

Daemonize::~Daemonize ()
{
  daemon_signal_done ();
  /* If this is child, remove pid file */
  if (mPid == 0) {
    daemon_pid_file_remove ();
  }
#ifdef DEBUG
  fprintf (stderr, "~Daemonize\n");
#endif
}

const char * Daemonize::pid_file_proc()
{
  return globalPidFile;
}

Daemonize::DaemonStatus Daemonize::create_daemon ()
{
  if (is_daemon_running ()) {
    fprintf (stderr, "Daemon is already running on pid %d\n", mPid);
    return ERROR;
  }
  /* Start to initialize daemon */
  if (daemon_retval_init () < 0) {
    perror ("Failed to create pipe.");
    return ERROR;
  }
  if ((mPid = daemon_fork ()) < 0) {
    perror ("daemon_fork error");
    daemon_retval_done ();
    return ERROR;
  } else if (mPid) { /* Parent */
    int ret;
    /* Wait for 10 seconds for the return value from the daemon process */
    if ((ret = daemon_retval_wait (20)) < 0) {
      perror ("Could not receive return value from daemon process");
      return P_ERROR;
    }
    /* Parent Process returns */
    if (ret != 0) {
#ifdef DEBUG
      fprintf (stderr, "Daemon returned with value %d\n", ret);
#endif
      perror ("Daemon returned with error");
    }
#ifdef DEBUG
    fprintf (stderr, "Parent return, Daemon's pid is %d!\n", mPid);
#endif
    return P_RETURN;
  } else { /* Daemon */
    /* Close all FDs */
    if (daemon_close_all (-1) < 0) {
      daemon_log (LOG_ERR, "Daemon: Failed to close all file descriptors");
      daemon_retval_send (1);
      return C_ERROR;
    }
    /* Create PID file */
    if (daemon_pid_file_create () < 0) {
      daemon_log (LOG_ERR, "Daemon: Could not create PID file");
      daemon_retval_send (2);
      return C_ERROR;
    }
    /* Initialize signal handling */
    if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, 0) < 0) {
      daemon_log (LOG_ERR, "Daemon: Could not register signal handlers");
      daemon_retval_send (3);
      return C_ERROR;
    }
    daemon_retval_send (0);
    return C_RETURN;
  }
  return ERROR;
}

bool Daemonize::kill_daemon ()
{
  return (daemon_pid_file_kill_wait(SIGTERM, 5) < 0) ? false : true;
}

bool Daemonize::is_daemon_running ()
{
  return ((mPid = daemon_pid_file_is_running ()) >= 0) ? true : false;
}

void Daemonize::keep_running ()
{
  int fd = daemon_signal_fd ();
  fd_set fdSet;
  while (mRunning) {
    FD_ZERO (&fdSet);
    FD_SET  (fd, &fdSet);
    /* Wait for incoming signal */
    if (select (fd + 1, &fdSet, NULL, NULL, NULL) < 0) {
      if (errno == EINTR) {
        continue;
      } else {
        perror ("Select () :");
        mRunning = false;
        break;
      }
    }

    if (FD_ISSET(fd, &fdSet)) {
      int sigRecv = daemon_signal_next ();
      if (sigRecv <= 0) {
        perror ("Daemon: daemon_signal_next() failed :");
        mRunning = false;
      }
      switch (sigRecv) {
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
          fprintf (stderr, "%s stoped", mDaemonName);
          mRunning = false;
        default: break;
      }
    }
  }
}
