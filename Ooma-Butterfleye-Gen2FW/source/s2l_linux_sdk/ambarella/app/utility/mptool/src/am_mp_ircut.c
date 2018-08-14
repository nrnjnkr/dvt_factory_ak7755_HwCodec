/*******************************************************************************
 * am_mp_ircut.cpp
 *
 * History:
 *   Mar 19, 2015 - [longli] created file
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


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_ircut.h"
#include "devices.h"
#include "arch_s2l/img_adv_struct_arch.h"
#include "arch_s2l/img_api_adv_arch.h"
#include "img_dev.h"
#include <pthread.h>

#define IRCUT_MAX_GPIO_NUM 5
#define LUX_3X 400
#define LUX_8X 1000

#define IRCUT_DAY_MODE 0
#define IRCUT_NIGHT_MODE 1

enum {
  IRCUT_GPIO_STATE_SET = 0,
  IRCUT_START_RTSP     = 1,
  IRCUT_STOP_RTSP      = 2,
  IRCUT_RESULT_SAVE    = 3
};

int32_t set_ir_brightness(int32_t pwm_ch_id, int32_t value);

static int32_t gpio_export(const int32_t gpio_id, const int32_t gpio_export)
{
  int32_t ret = 0;
  int32_t fd = -1;
  int32_t access_ret = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  if (gpio_id > -1) {
    sprintf(buf, "/sys/class/gpio/gpio%d", gpio_id);
    access_ret = access(buf, F_OK);
    if ((access_ret && gpio_export) || (!access_ret && !gpio_export)) {
      sprintf(buf, "/sys/class/gpio/%s", gpio_export ? "export" : "unexport");
      if ((fd = open(buf, O_WRONLY)) < 0) {
        ret = -1;
        perror("open gpio file fail");
      } else {
        sprintf(vbuf, "%d", gpio_id);
        int32_t len = (int32_t)strlen(vbuf);
        if (len != write(fd, vbuf, len)) {
          ret = -1;
          perror("write gpio value fail.");
        }

        close(fd);
      }
    }
  } else {
    printf("Invalid gpio id: %d", gpio_id);
    ret = -1;
  }

  return ret;
}

static int32_t set_gpio_direction_out(const int32_t gpio_id,
                                      const int32_t gpio_out)
{
  int32_t ret = 0;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
  if ((fd = open(buf, O_WRONLY)) < 0) {
    ret = -1;
    perror("Fail to open gpio");
  } else {
    int32_t len = 0;
    if (gpio_out) {
      sprintf(vbuf, "%s","out");
      len = 3;
    } else {
      sprintf(vbuf, "%s","in");
      len = 2;
    }

    if (len != write(fd, vbuf, len)) {
      ret = -1;
      perror("Fail to write gpio value");
    }
    close(fd);
  }

  return ret;
}

static int32_t set_gpio_value(const int32_t gpio_id, const int32_t led_on)
{
  int32_t ret = 0;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
  fd = open(buf, O_RDWR);
  if (fd < 0) {
    perror("Open gpio value failed");
    ret = -1;
  } else {
    sprintf(vbuf, "%s",(led_on ? "1":"0"));
    if (write(fd, vbuf, 1) != 1) {
      perror("Write gpio value failed");
      ret = -1;
    }
    close(fd);
  }

  return ret;
}

static int32_t set_gpio_state(const int32_t gpio_id, const int32_t led_on)
{
  int32_t ret = 0;

  do {
    if (gpio_export(gpio_id, 1)) {
      ret = -1;
      break;
    }

    if (set_gpio_direction_out(gpio_id, 1)) {
      ret = -1;
      break;
    }

    if (set_gpio_value(gpio_id, led_on)) {
      ret = -1;
      break;
    }

  } while (0);

  return ret;
}

int mode = 0; //Day mode default;

static void disable_night_mode() {

    mode = IRCUT_DAY_MODE;
    set_ir_brightness(0, 0x00);
    img_set_bw_mode(0);
    set_gpio_state(26, 0);
    fprintf(stderr, "Setting the day mode: Done\n");

}

static void enable_night_mode() {

    mode = IRCUT_NIGHT_MODE;
    set_gpio_state(26, 1);
    set_ir_brightness(0, 98);
    img_set_bw_mode(1);
    fprintf(stderr, "Setting the night mode: Done\n");

}

int stop_night = -1;
pthread_t tlow_light_detect;
void* detect_ALS(void *p) {
    unsigned int values;
    //Flag to control this thread
    fprintf(stderr, "ALS thread started\n");
    stop_night = 1;
    while(stop_night) {
        if(!ir_led_get_adc_value(&values)) {
            if(values <= LUX_3X && (mode != IRCUT_NIGHT_MODE)) {
                enable_night_mode();
            } else if(values >= LUX_8X && (mode != IRCUT_DAY_MODE))
                disable_night_mode();
        } else
            fprintf(stderr, "Unable know the Light sensor values %d\n",
                    values);
        sleep(1);
    }
    disable_night_mode();
    fprintf(stderr, "ALS thread exited\n");
    return NULL;
}

//ircut: 0->day, 1->night
am_mp_err_t mptool_ircut_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  to_msg->result.ret = MP_OK;
  switch (from_msg->stage) {
    case IRCUT_GPIO_STATE_SET: {
            pthread_create(&tlow_light_detect, NULL, detect_ALS, NULL);
        } break;

    case IRCUT_START_RTSP: {
      int32_t status = 0;
      /* Start rtsp server*/
      status = system("/usr/local/bin/test_image -i0 &");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "ImageServerDaemon");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      //status = system("/usr/local/bin/test_encode -i0 "
        //  "-X --bmaxsize 720p --bsize 720p -A -h720p -J --btype off");
      /*status = system("/usr/local/bin/test_encode -A -s");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "test_encode");
        to_msg->result.ret = MP_ERROR;
        break;
      } */

      status = system("/usr/local/bin/rtsp_server &");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "rtsp_server");
        to_msg->result.ret = MP_ERROR;
        break;
      }
      sleep(1);

      status = system("/usr/local/bin/test_encode -A -e");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "test_encode -e");
        to_msg->result.ret = MP_ERROR;
        break;
      }
    } break;
    case IRCUT_STOP_RTSP: {
      int32_t status = 0;
      /*stop rtsp server*/
      status = system("/usr/local/bin/test_encode -A -s");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "test_encode -s");
        to_msg->result.ret = MP_ERROR;
        break;
      }
      system("killall test_image");
      system("kill -9 `pidof rtsp_server`");
    } break;
    case IRCUT_RESULT_SAVE: {
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret,
                           SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
      }
    } break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
