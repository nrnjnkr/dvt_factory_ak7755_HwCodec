/*******************************************************************************
 * am_face_detect.h
 *
 * History:
 *   May 8, 2017 - [nzhang] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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
#ifndef ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_DETECT_H_
#define ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_DETECT_H_

#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_face_detect_if.h"
#include "am_face_detect_types.h"
#include "am_vca_plugin_if.h"
#include "opt_sc_algos_interface.h"
#include "am_face_recognition.h"
#include "am_define.h"
#include "am_thread.h"
#include "am_mutex.h"
#include "am_event.h"
#include <condition_variable>

using std::string;

enum FDSTATUS {
  FDSTATUS_FIRST_FD             = 0,
  FDSTATUS_NO_FACE              = 1,
  FDSTATUS_HAVE_FACE            = 2,
};

struct AMYUVINFO
{
    AMRect rect;
    uint32_t pitch;
    AMAddress y_addr;
    AMAddress uv_addr;
};

struct AMFaceDetectParam
{
    uint8_t mtcnn_thread_number;
    uint8_t caffe_face_thread_number;
    bool create_recognition;
    bool enable_recognition;
    uint32_t width;
    uint32_t height;
    uint32_t offset_x;
    uint32_t offset_y;
};

class AMConfig;
class AMFaceDetect: public AMIVCAPlugin, public AMIFaceDetect
{
  public:
    static AMFaceDetect* create();
    virtual void destroy() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual std::string& name() override;
    virtual void* get_interface() override;
    virtual bool data_loop(const AMQueryFrameDesc &desc) override;
    virtual bool get_buffer_config(AMVCABuf &buf_config) override;

  public:
    virtual bool set_callback(void *context,
                              AMFDCallback recv_vca) override;
    virtual bool set_buffer_param(const AMRect rect) override;
    virtual bool enable_recognition(const bool enable) override;
    virtual bool get_recognition_state() const override;
    virtual float get_male_probability(float *eigenvalues) const override;
    float get_face_distance_quadratic(float *data1, float *data2) const override;

  private:
    bool init();
    bool load_config();
    bool frame_capture(const AMQueryFrameDesc &desc);
    bool fd_prepare();
    bool fd_process();
    void pack_fd_result();
    static void thread_recognition_entry(void *arg);
    bool copy_data_to_recog();
    bool feed_data_to_recognition();
    bool exec_callback(AMFDMessage msg);
    AM_DATA_FRAME_TYPE buf_type_str_to_enum (string buf_str);
  private:
    AMFaceDetect(const char *name);
    virtual ~AMFaceDetect();

  protected:
    std::string                         m_name;

  private:
    AMThread *m_recog_thread                    = nullptr;
    AMEvent *m_fr_enable_event                  = nullptr;
    float *m_image_buffer                       = nullptr;
    float *m_image_buffer_recog                 = nullptr;
    void *m_p_opt_ctx                           = nullptr;
    void *m_callback_owner_obj                  = nullptr;
    uint8_t *m_yuv_buf_fd                       = nullptr;
    uint8_t *m_yuv_buf_fr                       = nullptr;
    AMFaceRecogPtr m_face_recog                 = nullptr;
    AMFDCallback m_callback                     = nullptr;
    AMIVideoReaderPtr m_video_reader            = nullptr;
    AMIVideoAddressPtr m_video_address          = nullptr;
    face_rect_t m_face_rect[DMAX_FACE_NUMBER]   = {0};
    AMFDMessage m_msg                           = {0};
    AMFDMessage m_recog_msg                     = {0};
    AMFaceDetectParam m_fd_config               = {0};
    AMYUVINFO m_yuv                             = {0};
    uint32_t m_det_num                          = 0;
    FDSTATUS m_status                           = FDSTATUS_FIRST_FD;
    bool m_gdma_support                         = false;
    std::atomic_bool m_dynamic_enable_recog     = {false};
    std::atomic_bool m_main_always_run          = {false};
    std::atomic_bool m_fd_run                   = {false};
    std::atomic_bool m_fr_processing            = {false};
    std::atomic_bool m_fd_stopping              = {false};
    std::mutex m_recog_mutex;
    std::mutex m_usr_mutex;
    std::mutex m_fd_stop_mutex;
    std::condition_variable m_recog_cond;
    std::condition_variable m_fd_stop_cond;
    AMVCABuf m_buf_config;
    opt_sc_algos_context_t m_algo_ctx;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define FD_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video/vca"))
#else
#define FD_CONF_DIR ((const char*)"/etc/oryx/video/vca")
#endif

#define FD_CONFIG_FILE "face-detect.acs"

#endif /* ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_DETECT_H_ */
