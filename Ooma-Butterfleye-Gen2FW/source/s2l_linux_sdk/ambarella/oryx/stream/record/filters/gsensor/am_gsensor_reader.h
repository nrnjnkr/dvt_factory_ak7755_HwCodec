/*******************************************************************************
 * am_gsensor_reader.h
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
#ifndef AM_GSENSOR_READER_H_
#define AM_GSENSOR_READER_H_
#include <string>
#include <atomic>
class AMThread;
enum AM_META_TYPE {
  AM_META_NULL = 0,
  AM_META_DATA,
  AM_META_EOS,
};
struct MetaDataStruct
{
    AM_META_TYPE type        = AM_META_NULL;
    int64_t      pts         = 0;
    uint16_t     meta_id     = 0;
    uint16_t     meta_length = 0;
    uint8_t      version     = 0;
    int8_t       gsensor_x   = 0;
    int8_t       gsensor_y   = 0;
    int8_t       gsensor_z   = 0;
    int16_t      gyro_y      = 0;
    int16_t      gyro_p      = 0;
    int16_t      gyro_r      = 0;
    int16_t      gyro_y_x4   = 0;
    int16_t      gyro_p_x4   = 0;
    int16_t      gyro_r_x4   = 0;
    uint8_t      reserved_0  = 0;
    uint8_t      reserved_1  = 0;
    uint8_t      reserved_2  = 0;
    MetaDataStruct() {}
    ~MetaDataStruct() {}
    bool get_data(uint8_t *buf, uint32_t &len);
};

class AMGsensorReader
{
  public :
    static AMGsensorReader* create(int32_t data_write_fd);
    int32_t get_vin_sync_fd();
    bool stop();
    void destroy();
  private :
    AMGsensorReader();
    virtual ~AMGsensorReader();
    bool init(int32_t data_write_fd);
    bool init_data_buf();
    static void thread_entry(void *p);
    void main_loop();
    bool read_vin_sync_frame();
    bool read_vin_sync_4x_frame();
    int64_t get_current_pts();
    bool send_data();
    bool data_transformation();
  private :
    AMThread        *m_thread        = nullptr;
    int32_t          m_gyro_fd       = -1;
    int32_t          m_gsensor_fd    = -1;
    int32_t          m_vin_sync_fd   = -1;
    int32_t          m_data_write_fd = -1;
    int32_t          m_hw_timer_fd   = -1;
    int32_t          m_timer_fd      = -1;
    int              m_ctrl_pipe[2]  = { -1 };
    std::atomic_bool m_run           = {false};
    MetaDataStruct   m_data;
    std::string      m_name;

};

#endif /* AM_GSENSOR_READER_H_ */
