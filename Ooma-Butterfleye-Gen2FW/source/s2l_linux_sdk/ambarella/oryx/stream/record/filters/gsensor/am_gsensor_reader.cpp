/*******************************************************************************
 * am_gsensor_reader.cpp
 *
 * History:
 *   2016-11-28 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <errno.h>
#include <sys/timerfd.h>
#include <time.h>
#include "am_gsensor_reader.h"
#include "am_timer.h"
#include "am_io.h"
#include "am_thread.h"

#define gsensor_path "/dev/gsensor"
#define gyro_sensor_path "/dev/gyro"
#define vin_sync_path "/proc/ambarella/vin0_idsp"
#define hw_timer_path "/proc/ambarella/ambarella_hwtimer"
#define CTRL_READ   m_ctrl_pipe[0]
#define CTRL_WRITE  m_ctrl_pipe[1]

bool MetaDataStruct::get_data(uint8_t *buf, uint32_t &len)
{
  bool ret = true;
  do {
    if (len < 20) {
      ERROR("The buf length is too small.");
      ret = false;
      break;
    }
    if (!buf) {
      ERROR("The buf is invalid.");
      ret = false;
      break;
    }
    buf[0]  = (uint8_t)((meta_id >> 8) & 0x00ff);
    buf[1]  = (uint8_t)(meta_id & 0x00ff);
    buf[2]  = (uint8_t)((meta_length >> 8) & 0x00ff);
    buf[3]  = (uint8_t)(meta_length & 0x00ff);
    buf[4]  = version;
    buf[5]  = (uint8_t)gsensor_x;
    buf[6]  = (uint8_t)gsensor_y;
    buf[7]  = (uint8_t)gsensor_z;
    buf[8]  = (uint8_t)((gyro_y >> 8) & 0x00ff);
    buf[9]  = (uint8_t)(((gyro_y) & 0x00f0) | ((gyro_p >> 12) & 0x000f));
    buf[10] = (uint8_t)((gyro_p >> 4) & 0x00ff);
    buf[11] = (uint8_t)((gyro_r >> 8) & 0x00ff);
    buf[12] = (uint8_t)(((gyro_r) &0x00f0) | ((gyro_y_x4 >> 12) & 0x000f));
    buf[13] = (uint8_t)((gyro_y_x4 >> 4) & 0x00ff);
    buf[14] = (uint8_t)((gyro_p_x4 >> 8) & 0x00ff);
    buf[15] = (uint8_t)(((gyro_p_x4) &0x00f0) | ((gyro_r_x4 >> 12) & 0x000f));
    buf[16] = (uint8_t)((gyro_r_x4 >> 4) & 0x00ff);
    buf[17] = reserved_0;
    buf[18] = reserved_1;
    buf[19] = reserved_2;
  } while(0);
  return ret;
}

AMGsensorReader* AMGsensorReader::create(int32_t data_write_fd)
{
  AMGsensorReader *result = nullptr;
  result = new AMGsensorReader();
  if (result && (!result->init(data_write_fd))) {
    ERROR("Failed to init AMGsensorReader");
    delete result;
    result = nullptr;
  }
  return result;
}

int32_t AMGsensorReader::get_vin_sync_fd()
{
  return m_vin_sync_fd;
}

void AMGsensorReader::destroy()
{
  delete this;
}

AMGsensorReader::AMGsensorReader()
{}

AMGsensorReader::~AMGsensorReader()
{
  //stop();
  AM_DESTROY(m_thread);
  if (m_gyro_fd >= 0) {
    close(m_gyro_fd);
  }
  if (m_gsensor_fd >= 0) {
    close(m_gsensor_fd);
  }
  if (CTRL_READ >= 0) {
    close(CTRL_READ);
  }
  if (CTRL_WRITE >= 0) {
    close(CTRL_WRITE);
  }
  if (m_hw_timer_fd >= 0) {
    close(m_hw_timer_fd);
  }
  if (m_timer_fd > 0) {
    close(m_timer_fd);
  }
}

bool AMGsensorReader::init(int32_t data_write_fd)
{
  bool ret = true;
  do {
    if (data_write_fd < 0) {
      ERROR("The data write fd is invalid.");
      ret = false;
      break;
    }
    m_data_write_fd = data_write_fd;
    m_name = "AMGsensorReader";
    m_gyro_fd = open(gyro_sensor_path, O_RDONLY);
    if (m_gyro_fd < 0) {
      ERROR("Failed to open %s : %s", gyro_sensor_path, strerror(errno));
      ret = false;
      break;
    }
    m_gsensor_fd = open(gsensor_path, O_RDONLY);
    if (m_gsensor_fd < 0) {
      ERROR("Failed to open %s : %s", gyro_sensor_path, strerror(errno));
      ret = false;
      break;
    }
    m_vin_sync_fd = open(vin_sync_path, O_RDONLY);
    if (m_vin_sync_fd < 0) {
      ERROR("Failed to open %s : %s", vin_sync_path, strerror(errno));
      ret = false;
      break;
    }
    if (AM_UNLIKELY(pipe(m_ctrl_pipe) < 0)) {
      PERROR("pipe error : ");
      ret = false;
      break;
    }
    m_hw_timer_fd = open(hw_timer_path, O_RDONLY);
    if (m_hw_timer_fd < 0) {
      PERROR("open hardware timer :");
      ret = false;
      break;
    }
    //CLOCK_MONOTONIC_RAW
    m_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (m_timer_fd < 0) {
      PERROR("Create timerfd :");
      ret = false;
      break;
    }
    if (!init_data_buf()) {
      ERROR("Failed to init gsensor data buffer.");
      ret = false;
      break;
    }
    m_thread = AMThread::create(m_name.c_str(), thread_entry, this);
    if (AM_UNLIKELY(!m_thread)) {
      ERROR("Failed to create thread: %s", m_name.c_str());
      ret = false;
      break;
    }
    if (!m_thread->set_priority(99)) {
      WARN("Failed to set thread priority in %s", m_name.c_str());
    }
  } while(0);
  return ret;
}

void AMGsensorReader::thread_entry(void *p)
{
  ((AMGsensorReader*) p)->main_loop();
}

void AMGsensorReader::main_loop()
{
  fd_set real_fd;
  fd_set reset_fd;
  int32_t max_fd = -1;
  m_run = true;
  FD_ZERO(&reset_fd);
  FD_SET(CTRL_READ, &reset_fd);
  FD_SET(m_timer_fd, &reset_fd);
  max_fd = AM_MAX(CTRL_READ, m_timer_fd);
  uint32_t error_count = 0;
  uint8_t read_vin_sync_error_count = 0;
  uint8_t read_vin_4x_error_count = 0;
  while(m_run) {
    char vin_buf[8] = { 0 };
    int32_t vin_read_len = 0;
    if ((vin_read_len = read(m_vin_sync_fd, vin_buf, sizeof(vin_buf))) <= 0) {
      PERROR("read vin sync : ");
      ++ error_count;
      if (error_count >= 3) {
        m_run = false;
        break;
      } else {
        continue;
      }
    }
    if (!read_vin_sync_frame()) {
      ++ read_vin_sync_error_count;
      ERROR("Failed to read vin sync frame in %s.", m_name.c_str());
    } else {
      read_vin_sync_error_count = 0;
    }
    if (read_vin_sync_error_count > 3) {
      m_run = false;
      continue;
    }
    error_count = 0;
    real_fd = reset_fd;
    int sret = select(max_fd + 1, &real_fd, nullptr, nullptr, nullptr);
    if (sret <= 0) {
      ++ error_count;
      PERROR("Select");
      if (error_count >= 3) {
        m_run = false;
        break;
      } else {
        continue;
      }
    }
    error_count = 0;
    if (FD_ISSET(CTRL_READ, &real_fd)) {
      char cmd[1] = {0};
      if (AM_UNLIKELY(read(CTRL_READ, cmd, sizeof(cmd)) <= 0)) {
        PERROR("Read CTRL_READ :");
        continue;
      } else if (cmd[0] == 's') {
        m_run = false;
        continue;
      } else {
        ERROR("Receive unkonwn cmd in %s", m_name.c_str());
      }
    }
    if (FD_ISSET(m_timer_fd, &real_fd)) {
      if (!read_vin_sync_4x_frame()) {
        ++ read_vin_4x_error_count;
        ERROR("Failed to read vin sync 4x frame in %s", m_name.c_str());
      } else {
        read_vin_4x_error_count = 0;
      }
      if (read_vin_4x_error_count > 3) {
        m_run = false;
        continue;
      }
      if (!send_data()) {
        ERROR("Failed to send data in %s", m_name.c_str());
      }
    }
  }
  m_data.type = AM_META_EOS;
  send_data();
}

bool AMGsensorReader::read_vin_sync_frame()
{
  bool ret = true;
  do {
    int32_t gsensor_data[3] = { 0 };
    int32_t gsensor_read_len = 0;
    if ((gsensor_read_len = am_read(m_gsensor_fd, gsensor_data,
                                    sizeof(gsensor_data), 5)) != sizeof(gsensor_data)) {
      ERROR("Failed to read gsensor data in gsensor reader.");
      ret = false;
      break;
    }
    int32_t gyro_data[3] = {0};
    int32_t gyro_read_len = 0;
    if ((gyro_read_len = am_read(m_gyro_fd, gyro_data, sizeof(gyro_data), 5))
        != sizeof(gyro_data)) {
      ERROR("Failed to read gyro data in gyro reader.");
      ret = false;
      break;
    }
    itimerspec time_spec;
    time_spec.it_interval.tv_nsec = 0;
    time_spec.it_interval.tv_sec = 0;
    time_spec.it_value.tv_sec = 0;
    time_spec.it_value.tv_nsec = 16000000;
    if (timerfd_settime(m_timer_fd, 0, &time_spec, nullptr) < 0) {
      ERROR("Failed to set timer in %s", m_name.c_str());
    }
    m_data.pts = get_current_pts();
    m_data.gsensor_x = (int8_t)gsensor_data[0];
    m_data.gsensor_y = (int8_t)gsensor_data[1];
    m_data.gsensor_z = (int8_t)gsensor_data[2];
    m_data.gyro_y = (int16_t)gyro_data[0];
    m_data.gyro_p = (int16_t)gyro_data[1];
    m_data.gyro_r = (int16_t)gyro_data[2];
    m_data.type = AM_META_DATA;
  } while(0);

  return ret;
}

bool AMGsensorReader::read_vin_sync_4x_frame()
{
  bool ret = true;
  do {
    if (m_data.pts <= 0) {
      WARN("Should not read this frame.");
      break;
    }
    int32_t gyro_data[3] = {0};
    int32_t gyro_read_len = 0;
    if ((gyro_read_len = am_read(m_gyro_fd, gyro_data, sizeof(gyro_data), 5))
        != sizeof(gyro_data)) {
      ERROR("Failed to read gyro data in gyro reader.");
      ret = false;
      break;
    }
    m_data.gyro_y_x4 = (int16_t)gyro_data[0];
    m_data.gyro_p_x4 = (int16_t)gyro_data[1];
    m_data.gyro_r_x4 = (int16_t)gyro_data[2];
    m_data.type = AM_META_DATA;
    if (!data_transformation()) {
      ERROR("Data transformation error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMGsensorReader::send_data()
{
  bool ret = true;
  do {
    if (am_write(m_data_write_fd, &m_data, sizeof(m_data), 3) != sizeof(m_data)) {
      ERROR("fd is %d", m_data_write_fd);
      PERROR("Failed to send data :");
      ret = false;
      break;
    }
  } while(0);
  m_data.type = AM_META_NULL;
  return ret;
}

bool AMGsensorReader::init_data_buf()
{
  m_data.meta_id = 0x00fa;
  m_data.meta_length = 0x0014;
  m_data.version = 0x03;
  return true;
}

int64_t AMGsensorReader::get_current_pts()
{
  int64_t curr_pts = 0;
  uint8_t pts[32] = {0};
  if (read(m_hw_timer_fd, pts, sizeof(pts)) < 0) {
    PERROR("Read");
  } else {
    curr_pts = strtoull((const char*)pts, (char**)nullptr, 10);
  }
  return curr_pts;
}

bool AMGsensorReader::stop()
{
  bool ret = true;
  do {
    if (!m_run) {
      break;
    }
    if (m_vin_sync_fd >= 0) {
      close(m_vin_sync_fd);
      m_vin_sync_fd = -1;
    }
    itimerspec time_spec;
    time_spec.it_value.tv_nsec = 0;
    time_spec.it_value.tv_sec = 0;
    time_spec.it_interval.tv_nsec = 0;
    time_spec.it_interval.tv_sec = 0;
    if (timerfd_settime(m_timer_fd, 0, &time_spec, nullptr) < 0) {
      PERROR("Failed to stop timer : ");
      ret = false;
    }
    if (CTRL_WRITE >= 0) {
      if (write(CTRL_WRITE, "s", 1) != 1) {
        ERROR("Failed to send stop cmd to mainloop");
        ret = false;
      }
    }
  } while(0);
  return ret;
}

bool AMGsensorReader::data_transformation()
{
  bool ret = true;
  int16_t gyro_temp_y = m_data.gyro_y;
  int16_t gyro_temp_p = m_data.gyro_p;
  int16_t gyro_temp_r = m_data.gyro_r;
  m_data.gyro_y = -gyro_temp_y;
  m_data.gyro_p = -gyro_temp_r;
  m_data.gyro_r = gyro_temp_p;
  gyro_temp_y = m_data.gyro_y_x4;
  gyro_temp_p = m_data.gyro_p_x4;
  gyro_temp_r = m_data.gyro_r_x4;
  m_data.gyro_y_x4 = -gyro_temp_y;
  m_data.gyro_p_x4 = -gyro_temp_r;
  m_data.gyro_r_x4 = gyro_temp_p;
  int8_t gsensor_temp_x = m_data.gsensor_x;
  int8_t gsensor_temp_y = m_data.gsensor_y;
  int8_t gsensor_temp_z = m_data.gsensor_z;
  m_data.gsensor_x = -gsensor_temp_x;
  m_data.gsensor_y = gsensor_temp_y;
  m_data.gsensor_z = -gsensor_temp_z;
  return ret;
}
