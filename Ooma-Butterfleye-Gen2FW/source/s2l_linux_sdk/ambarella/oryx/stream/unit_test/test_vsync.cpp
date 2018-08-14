/*******************************************************************************
 * test_vsync.cpp
 *
 * History:
 *   2017-1-4- - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_define.h"
#include "am_log.h"
#include <signal.h>
#include <sys/time.h>

#include "am_timer.h"

int exit_flag = 0;
int64_t last_pts = 0;
int32_t vin_sync_fd = -1;
int32_t hw_timer_fd = -1;
static void sigstop(int arg)
{
  PRINTF("catch sigstop, exit.");
  exit_flag = true;
}

static bool callback(void *data)
{
  int32_t pts_fd = *((int32_t*)data);
  uint8_t pts[32] = {0};
  int64_t current_pts = 0;
  do {
  if (read(pts_fd, pts, sizeof(pts)) < 0) {
    ERROR("Failed to read hw timer.");
    break;
  }
  current_pts = strtoull((const char*)pts, nullptr, 10);
  //ERROR("timer : %jd : %jd", current_pts, (current_pts - last_pts));
  printf("%jd\n", (current_pts - last_pts));
  } while(0);
  return false;

}
int main()
{
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  int32_t ret = -1;
  const char *vin_sync = "/proc/ambarella/vin0_idsp";
  const char *hw_timer = "/proc/ambarella/ambarella_hwtimer";
  AMTimer *timer = nullptr;
  char vin_buf[8] = { 0 };
  uint8_t pts[32] = {0};
  int64_t current_pts = 0;
  do {
    vin_sync_fd = open(vin_sync, O_RDONLY);
    if (vin_sync_fd < 0) {
      ERROR("Failed to open vin sync");
      ret = -1;
      break;
    }
    hw_timer_fd = open(hw_timer, O_RDONLY);
    if (hw_timer_fd < 0) {
      ERROR("Failed to open hw timer.");
      ret = -1;
      break;
    }
    std::string timer_name = "timer_test";
    timer = AMTimer::create(timer_name.c_str(), 17, callback, &hw_timer_fd);
    if (!timer) {
      ERROR("Failed to create timer.");
      ret = -1;
      break;
    }
    int32_t read_len = 0;
    while(!exit_flag) {
      if ((read_len = read(vin_sync_fd, vin_buf, sizeof(vin_buf))) < 0) {
        ERROR("Failed to read vin sync");
        ret = -1;
        break;
      }
      timer->start(15);//16ms
      memset(pts, 0, sizeof(pts));
      current_pts = 0;
      if (read(hw_timer_fd, pts, sizeof(pts)) < 0) {
        ERROR("Failed to read hw timer.");
        ret = -1;
        break;
      }
      current_pts = strtoull((const char*)pts, nullptr, 10);
      ERROR("%jd : %jd", current_pts, (current_pts - last_pts));
      last_pts = current_pts;
    }
  } while(0);
  if (timer) {
    timer->destroy();
  }
  if (vin_sync_fd > 0) {
    close(vin_sync_fd);
  }
  if (hw_timer_fd > 0) {
    close(hw_timer_fd);
  }
  return ret;
}



