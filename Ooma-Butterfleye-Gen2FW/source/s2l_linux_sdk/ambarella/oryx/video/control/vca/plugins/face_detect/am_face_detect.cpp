/*******************************************************************************
 * am_face_detect.cpp
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
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_configure.h"
#include <signal.h>

#include "am_face_detect.h"

#ifdef __cplusplus
extern "C" {
#endif
ORYX_API AMIVCAPlugin* create_vca_plugin()
{
  return AMFaceDetect::create();
}
#ifdef __cplusplus
}
#endif

AMFaceDetect::AMFaceDetect(const char *name) :
  m_name(name)
{

}

AMFaceDetect::~AMFaceDetect()
{
  m_main_always_run = false;
  if (!m_dynamic_enable_recog) {
    m_fr_enable_event->signal();
  }
  if (!m_fr_processing) {
    std::unique_lock<std::mutex> lk(m_fd_stop_mutex);
    m_fr_processing = true;
    m_recog_cond.notify_all();
  }
  m_face_recog = nullptr;
  AM_DESTROY(m_recog_thread);
  if (m_p_opt_ctx) {
    m_algo_ctx.fd_destroy_context(m_p_opt_ctx);
  }
  destroy_opt_sc_algos_context(&m_algo_ctx);
  delete[] m_image_buffer;
  delete[] m_image_buffer_recog;
  delete[] m_yuv_buf_fd;
  delete[] m_yuv_buf_fr;
}

AMFaceDetect *AMFaceDetect::create()
{
  AMFaceDetect *result = new AMFaceDetect("vca-face-detect");
  if (result && (!result->init())) {
    ERROR("Failed to create an instance of AMFaceDetect");
    delete result;
    result = nullptr;
  }
  return result;
}

void AMFaceDetect::destroy()
{
  stop();
  delete this;
}

bool AMFaceDetect::init()
{
  bool ret = false;
  do {
    if (!load_config()) {
      ERROR("Failed to load face detect config.");
      break;
    }
    if (!(m_video_reader = AMIVideoReader::get_instance())) {
      ERROR("Failed to get AMVideoReader instance!");
      break;
    }
    if (!(m_video_address = AMIVideoAddress::get_instance())) {
      ERROR("Failed to get AMVideoAddress instance!");
      break;
    }
    if (!(m_gdma_support = m_video_reader->is_gdmacpy_support())) {
      PRINTF("IAV Usr Buffer Size didn't set then the gdmacpy is not support. \n");
      PRINTF("Face detection and recognition will run a bit slow only by memcpy.\n");
      PRINTF("Please set IAV_MEM_USR_SIZE on menuconfig");
    }
    if (!fd_prepare()) {
      ERROR("Failed to prepare face detect.");
      break;
    }

    if (!(m_fr_enable_event = AMEvent::create())) {
      ERROR("Failed to create AMEvent");
      break;
    }
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::start()
{
  bool result = true;
  do {
    m_main_always_run = true;
    if (AM_UNLIKELY(m_fd_run)) {
      INFO("VCA face detect has already started!");
      break;
    }
    m_fd_run = true;
  } while (0);
  return result;
}

bool AMFaceDetect::stop()
{
  bool result = true;
  do {
    if (!m_fd_run) {
      break;
    }
    m_fd_stopping = true;
    m_fd_run = false;
    {
      std::unique_lock<std::mutex> lk(m_fd_stop_mutex);
      while (m_fr_processing) {
        m_fd_stop_cond.wait(lk);
      }
    }
    if (!m_fr_processing) {
      m_fr_processing = true;
      m_recog_cond.notify_all();
    }
    m_fd_stopping = false;
  } while (0);
  return result;
}

std::string& AMFaceDetect::name()
{
  return m_name;
}

void* AMFaceDetect::get_interface()
{
  return ((AMIFaceDetect*)this);
}

bool AMFaceDetect::data_loop(const AMQueryFrameDesc &desc)
{
  bool ret = false;
  do {
    if (m_dynamic_enable_recog) {
      if (!m_recog_thread) {
        if (!(m_recog_thread = AMThread::create("VCA.FaceRecog",
                                                thread_recognition_entry,
                                                this))) {
          m_recog_thread = nullptr;
          ERROR("Failed to create face recognition thread, "
                "disable face recognition!");
          break;
        }
      }
      if (!m_image_buffer_recog) {
        m_image_buffer_recog = new float[m_fd_config.width *
                                         m_fd_config.height * 3];
        if (!m_image_buffer_recog) {
          m_image_buffer_recog = nullptr;
          ERROR("Failed to new m_image_buffer_recog.");
          break;
        }
      }
      if ((!m_gdma_support) && (!m_yuv_buf_fr)) {
        m_yuv_buf_fr = new uint8_t[m_fd_config.width * m_fd_config.height * 2];
        if (!m_yuv_buf_fr) {
          m_yuv_buf_fr = nullptr;
          ERROR("Failed to new m_yuv_buf_fr.");
          break;
        }
      }
    }
    if (!frame_capture(desc)) {
      break;
    }
    if (m_fd_run && !fd_process()) {
      break;
    }
    ret = true;
  } while (0);

  return ret;
}

bool AMFaceDetect::get_buffer_config(AMVCABuf &buf_config)
{
  bool ret = false;
  do {
    buf_config = m_buf_config;
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::load_config()
{
  bool ret = false;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = FD_CONF_DIR;
  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + FD_CONFIG_FILE;
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMFaceDetectConfig::Create AMConfig failed!\n");
      break;
    }

    AMConfig &fd_config = *config_ptr;

    if (fd_config["face_recognition"].exists()) {
      std::string tmp;
      tmp = fd_config["face_recognition"].get<std::string>("");
      const char *type = tmp.c_str();
      if (is_str_equal(type, "create_run")) {
        m_fd_config.create_recognition = true;
        m_fd_config.enable_recognition = true;
      } else if (is_str_equal(type, "create_only")) {
        m_fd_config.create_recognition = true;
        m_fd_config.enable_recognition = false;
      } else {
        m_fd_config.create_recognition = false;
        m_fd_config.enable_recognition = false;
      }
    }

    if (fd_config["mtcnn_thread_number"].exists()) {
      m_fd_config.mtcnn_thread_number =
          fd_config["mtcnn_thread_number"].get<uint32_t>(0);
    } else {
      m_fd_config.mtcnn_thread_number = 4;
    }

    if (fd_config["caffe_face_thread_number"].exists()) {
      m_fd_config.caffe_face_thread_number =
          fd_config["caffe_face_thread_number"].get<uint32_t>(0);
    } else {
      m_fd_config.caffe_face_thread_number = 4;
    }

    if (fd_config["source_buffer_id"].exists()) {
      m_buf_config.buf_id =
          (AM_SOURCE_BUFFER_ID)fd_config["source_buffer_id"].get<uint32_t>(0);
    } else {
      m_buf_config.buf_id = AM_SOURCE_BUFFER_2ND;
    }

    if (fd_config["source_buffer_type"].exists()) {
      std::string tmp;
      tmp = fd_config["source_buffer_type"].get<std::string>("");
      m_buf_config.buf_type = buf_type_str_to_enum(tmp);
    } else {
      m_buf_config.buf_type = AM_DATA_FRAME_TYPE_YUV;
    }

    if (fd_config["rect"].exists()) {
      m_fd_config.offset_x = fd_config["rect"][0].get<uint32_t>(0);
      m_fd_config.offset_y = fd_config["rect"][1].get<uint32_t>(0);
      m_fd_config.width = fd_config["rect"][2].get<uint32_t>(0);
      m_fd_config.height = fd_config["rect"][3].get<uint32_t>(0);
    } else {
      m_fd_config.offset_x = 0;
      m_fd_config.offset_y = 0;
      m_fd_config.width = 400;
      m_fd_config.height = 600;
    }

    m_yuv.rect.size.width = m_fd_config.width;
    m_yuv.rect.size.height = m_fd_config.height;
    m_yuv.rect.offset.x = m_fd_config.offset_x;
    m_yuv.rect.offset.y = m_fd_config.offset_y;

    m_msg.capture_window = m_yuv.rect;
    m_dynamic_enable_recog = m_fd_config.enable_recognition;
    ret = true;
  } while (0);
  delete config_ptr;
  return ret;
}

bool AMFaceDetect::frame_capture(const AMQueryFrameDesc &desc)
{
  bool ret = false;
  AMAddress usr_addr;
  AMAddress y_addr;
  AMAddress uv_addr;
  do {
    std::unique_lock<std::mutex> lk(m_usr_mutex);
    if (desc.yuv.format != AM_CHROMA_FORMAT_YUV420) {
      ERROR("Unsupported chroma format for face detect.");
      break;
    }
    if (m_gdma_support) {
      //copy y buffer
      if (AM_RESULT_OK !=
          m_video_reader->gdmacpy_dsp2usr(
              (void*)0,
              (void*)(uintptr_t)(desc.yuv.y_offset +
                  m_yuv.rect.offset.x + m_yuv.rect.offset.y * desc.yuv.pitch),
                  m_fd_config.width,
                  m_fd_config.height,
                  desc.yuv.pitch)) {
        ERROR("Gdmacpy y buffer failed");
        break;
      }
      //copy uv buffer
      if (AM_RESULT_OK !=
          m_video_reader->gdmacpy_dsp2usr(
              (void*)(uintptr_t)(m_fd_config.width * m_fd_config.height),
              (void*)(uintptr_t)(desc.yuv.uv_offset +
                  m_yuv.rect.offset.x + m_yuv.rect.offset.y * desc.yuv.pitch),
                  m_fd_config.width,
                  m_fd_config.height,
                  desc.yuv.pitch)) {
        ERROR("Gdmacpy uv buffer failed");
        break;
      }
      if (AM_RESULT_OK != m_video_address->usr_addr_get(usr_addr)) {
        ERROR("Failed to get usr addr.");
        break;
      }
      m_yuv.y_addr.data = usr_addr.data;
      m_yuv.y_addr.offset = 0;
      m_yuv.uv_addr.data = usr_addr.data + m_fd_config.width * m_fd_config.height;
      m_yuv.uv_addr.offset = 0;
      m_yuv.pitch = m_yuv.rect.size.width;
    } else {
      if (AM_RESULT_OK !=
          m_video_address->yuv_y_addr_get(desc, y_addr)) {
        ERROR("Failed to get y addr.");
        break;
      }
      if (AM_RESULT_OK !=
          m_video_address->yuv_uv_addr_get(desc, uv_addr)) {
        ERROR("Failed to get uv addr.");
        break;
      }
      //copy y buffer
      for (uint32_t i = 0; i < m_fd_config.height; i++) {
        memcpy(m_yuv_buf_fd + i * m_fd_config.width,
               y_addr.data + y_addr.offset +
               m_yuv.rect.offset.x + m_yuv.rect.offset.y * desc.yuv.pitch +
               i * desc.yuv.pitch,
               m_fd_config.width * m_fd_config.height * sizeof(uint8_t));
      }
      //copy uv buffer
      for (uint32_t i = 0; i < m_fd_config.height; i++) {
        memcpy(m_yuv_buf_fd + m_fd_config.width * m_fd_config.height +
               i * m_fd_config.width,
               uv_addr.data + uv_addr.offset +
               m_yuv.rect.offset.x + m_yuv.rect.offset.y * desc.yuv.pitch +
               i * desc.yuv.pitch,
               m_fd_config.width * m_fd_config.height * sizeof(uint8_t));
      }

      m_yuv.y_addr.data = m_yuv_buf_fd;
      m_yuv.y_addr.offset = 0;
      m_yuv.uv_addr.data = m_yuv_buf_fd + m_fd_config.width * m_fd_config.height;
      m_yuv.uv_addr.offset = 0;
      m_yuv.pitch = m_yuv.rect.size.width;
    }
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::fd_prepare()
{
  bool ret = false;
  INFO("source buffer is %dx%d.", m_fd_config.width, m_fd_config.height);
  do {
    if (initialize_opt_sc_algos_context(&m_algo_ctx)) {
      ERROR("Failed to init algos context!");
      break;
    }
    m_image_buffer = new float[m_fd_config.width * m_fd_config.height * 3];
    if (!m_image_buffer) {
      ERROR("Failed to new m_image_buffer.");
      break;
    }
    m_p_opt_ctx = m_algo_ctx.fd_create_context(m_fd_config.mtcnn_thread_number,
                                               m_fd_config.width,
                                               m_fd_config.height,
                                               3, 50, 6, 10);
    if (!m_gdma_support) {
      m_yuv_buf_fd = new uint8_t[m_fd_config.width * m_fd_config.height * 2];
      if (!m_yuv_buf_fd) {
        ERROR("Failed to new m_yuv_buf_fd.");
        m_yuv_buf_fd = nullptr;
        break;
      }
    }
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::fd_process()
{
  bool ret = false;
  do {
    m_msg.capture_window = m_yuv.rect;
    m_algo_ctx.color_conversion_nv12_rgb_3channel_float(
        m_p_opt_ctx,
        m_yuv.rect.size.width, m_yuv.rect.size.height, (int)m_yuv.pitch,
        (unsigned char*)m_yuv.y_addr.data + (unsigned int)m_yuv.y_addr.offset,
        (unsigned char*)m_yuv.uv_addr.data + (unsigned int)m_yuv.uv_addr.offset,
        m_image_buffer);
    m_det_num = m_algo_ctx.fd_detect(m_p_opt_ctx, m_image_buffer, 0.8, 0.8, 0.8,
                                     m_face_rect, DMAX_FACE_NUMBER);
    if (m_det_num) {
      pack_fd_result();
      if (m_fd_config.enable_recognition && m_recog_thread) {
        if (!m_fr_processing) {
          std::unique_lock<std::mutex> lk(m_recog_mutex);
          if (copy_data_to_recog()) {
            memcpy(m_image_buffer_recog, m_image_buffer,
                   m_fd_config.width * m_fd_config.height * 3 * sizeof(float));
            m_fr_processing = true;
            m_recog_cond.notify_all();
          } else {
            ERROR("Failed to copy data to recognition.");
            break;
          }
        }
      }
    } else {
      m_msg = {0};
    }

    switch (m_status) {
      case FDSTATUS_FIRST_FD: {
        if (m_msg.face_num) {
          m_status = FDSTATUS_HAVE_FACE;
        } else {
          m_status = FDSTATUS_NO_FACE;
        }
        if (!exec_callback(m_msg)) {
          WARN("m_callback occur error.");
        }
      } break;

      case FDSTATUS_NO_FACE: {
        if (m_msg.face_num) {
          m_status = FDSTATUS_HAVE_FACE;
          if (!exec_callback(m_msg)) {
            WARN("m_callback occur error.");
          }
        }
      } break;

      case FDSTATUS_HAVE_FACE: {
        if (!m_msg.face_num) {
          m_status = FDSTATUS_NO_FACE;
        }
        if (!exec_callback(m_msg)) {
          WARN("m_callback occur error.");
        }
      } break;
    }
    ret = true;
  } while (0);
  return ret;
}

void AMFaceDetect::pack_fd_result()
{
  m_msg.face_num = m_det_num;
  m_msg.yuv_info.y_addr = m_yuv.y_addr.data + m_yuv.y_addr.offset;
  m_msg.yuv_info.uv_addr = m_yuv.uv_addr.data + m_yuv.uv_addr.offset;
  m_msg.yuv_info.width = m_yuv.rect.size.width;
  m_msg.yuv_info.height = m_yuv.rect.size.height;
  m_msg.yuv_info.pitch = m_yuv.pitch;

  for (uint32_t i = 0; i < m_det_num; i++) {
    m_msg.face_info[i].face_rect_x = (!(m_face_rect[i].x & 1)) ?
        m_face_rect[i].x : m_face_rect[i].x + 1;

    m_msg.face_info[i].face_rect_y = (!(m_face_rect[i].y & 1)) ?
        m_face_rect[i].y : m_face_rect[i].y + 1;

    m_msg.face_info[i].face_rect_w = (!(m_face_rect[i].w & 1)) ?
        m_face_rect[i].w : m_face_rect[i].w + 1;

    m_msg.face_info[i].face_rect_h = (!(m_face_rect[i].h & 1)) ?
        m_face_rect[i].h : m_face_rect[i].h + 1;

    m_msg.face_info[i].face_ldmk[0].y = m_face_rect[i].ldmk[0][0];
    m_msg.face_info[i].face_ldmk[0].x = m_face_rect[i].ldmk[0][1];

    m_msg.face_info[i].face_ldmk[1].y = m_face_rect[i].ldmk[1][0];
    m_msg.face_info[i].face_ldmk[1].x = m_face_rect[i].ldmk[1][1];

    m_msg.face_info[i].face_ldmk[2].y = m_face_rect[i].ldmk[2][0];
    m_msg.face_info[i].face_ldmk[2].x = m_face_rect[i].ldmk[2][1];

    m_msg.face_info[i].face_ldmk[3].y = m_face_rect[i].ldmk[3][0];
    m_msg.face_info[i].face_ldmk[3].x = m_face_rect[i].ldmk[3][1];

    m_msg.face_info[i].face_ldmk[4].y = m_face_rect[i].ldmk[4][0];
    m_msg.face_info[i].face_ldmk[4].x = m_face_rect[i].ldmk[4][1];

    m_msg.face_info[i].y_addr = m_msg.yuv_info.y_addr +
        m_msg.yuv_info.pitch * m_msg.face_info[i].face_rect_y +
        m_msg.face_info[i].face_rect_x;

    m_msg.face_info[i].uv_addr = m_msg.yuv_info.uv_addr +
        m_msg.yuv_info.pitch * (m_msg.face_info[i].face_rect_y / 2) +
        m_msg.face_info[i].face_rect_x;

    m_msg.face_info[i].pitch = m_msg.yuv_info.pitch;
  }
}

void AMFaceDetect::thread_recognition_entry(void *arg)
{
  if (!arg) {
    ERROR("Thread recognition is null.");
    return;
  }
  AMFaceDetect *p = (AMFaceDetect*)arg;
  do {
    if (!(p->m_face_recog =
        AMFaceRecog::create(p->m_fd_config.caffe_face_thread_number, &p->m_algo_ctx))) {
      ERROR("Failed to create m_face_recog, disable face recognition!");
      break;
    }
    if (!p->feed_data_to_recognition()) {
      ERROR("Failed to feed data to face recognition.");
      break;
    }
  } while (0);
}

bool AMFaceDetect::copy_data_to_recog()
{
  bool ret = false;
  do {
    m_recog_msg = m_msg;
    if (!m_gdma_support) {
      memcpy(m_yuv_buf_fr, m_msg.yuv_info.y_addr,
             m_fd_config.width * m_fd_config.height * sizeof(uint8_t));
      memcpy(m_yuv_buf_fr + m_fd_config.width * m_fd_config.height,
             m_msg.yuv_info.uv_addr,
             m_fd_config.width * m_fd_config.height * sizeof(uint8_t));
      m_recog_msg.yuv_info.y_addr = m_yuv_buf_fr;
      m_recog_msg.yuv_info.uv_addr = m_yuv_buf_fr +
          m_fd_config.width * m_fd_config.height;
      for (int i = 0; i < m_recog_msg.face_num; i++) {
        m_recog_msg.face_info[i].y_addr = m_recog_msg.yuv_info.y_addr +
            m_recog_msg.yuv_info.pitch * m_recog_msg.face_info[i].face_rect_y +
            m_recog_msg.face_info[i].face_rect_x;
        m_recog_msg.face_info[i].uv_addr = m_recog_msg.yuv_info.uv_addr +
            m_recog_msg.yuv_info.pitch * (m_recog_msg.face_info[i].face_rect_y / 2) +
            m_recog_msg.face_info[i].face_rect_x;
      }
    } else {
      if (AM_RESULT_OK !=
          m_video_reader->gdmacpy_usr2usr(
              (void*)(uintptr_t)(m_fd_config.width * m_fd_config.height * 2),
              (void*)0,
              m_msg.yuv_info.width,
              (m_msg.yuv_info.height * 2))) {
        ERROR("gdmacpy usr to usr failed.");
        break;
      }
      m_recog_msg.yuv_info.y_addr = m_msg.yuv_info.y_addr +
          m_fd_config.width * m_fd_config.height * 2;
      m_recog_msg.yuv_info.uv_addr = m_msg.yuv_info.uv_addr +
          m_fd_config.width * m_fd_config.height * 2;
      for (int i = 0; i < m_recog_msg.face_num; i++) {
        m_recog_msg.face_info[i].y_addr = m_msg.face_info[i].y_addr +
            m_fd_config.width * m_fd_config.height * 2;
        m_recog_msg.face_info[i].uv_addr = m_msg.face_info[i].uv_addr +
            m_fd_config.width * m_fd_config.height * 2;
      }
    }
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::feed_data_to_recognition()
{
  bool ret = true;
  while (m_main_always_run) {

    if (!m_dynamic_enable_recog) {
      m_fr_enable_event->wait();
    }
    {
      std::unique_lock<std::mutex> lk(m_recog_mutex);
      while (!m_fr_processing) {
        m_recog_cond.wait(lk);
      }
    }
    if (m_fd_run) {
      for (int i = 0; i < m_recog_msg.face_num; i ++) {
        m_face_recog->do_caffe_face(m_recog_msg.yuv_info,
                                    m_recog_msg.face_info[i],
                                    m_image_buffer_recog,
                                    m_recog_msg.face_info[i].eigenvalues,
                                    m_recog_msg.face_info[i].after_transform_w,
                                    m_recog_msg.face_info[i].after_transform_h,
                                    &m_recog_msg.face_info[i].after_transform_addr);
      }
      if (!exec_callback(m_recog_msg)) {
        WARN("m_callback occur error.");
      }
    }
    m_fr_processing = false;
    if (m_fd_stopping) {
      m_fd_stop_cond.notify_all();
    }
  }
  return ret;
}

bool AMFaceDetect::exec_callback(AMFDMessage msg)
{
  std::unique_lock<std::mutex> lk(m_usr_mutex);
  bool ret = false;
  do {
    if (AM_UNLIKELY(!m_callback)) {
      WARN("No callback function is registered!");
      break;
    }
    if (AM_UNLIKELY(m_callback(m_callback_owner_obj, &msg) < 0)) {
      ERROR("Failed to run Face Detect callback function.");
      break;
    }
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::set_callback(void *context,
                                AMFDCallback recv_vca)
{
  std::unique_lock<std::mutex> lk(m_usr_mutex);
  bool ret = true;
  do {
    if (!recv_vca) {
      ERROR("AMFaceDetect::set_fd_callback error.");
      ret = false;
      break;
    }
    m_callback = recv_vca;
    m_callback_owner_obj = context;
  } while (0);
  return ret;
}

AM_DATA_FRAME_TYPE AMFaceDetect::buf_type_str_to_enum(string buf_str)
{
  AM_DATA_FRAME_TYPE buf_type = AM_DATA_FRAME_TYPE_ME1;
  if (AM_LIKELY(!buf_str.empty())) {
    const char *type = buf_str.c_str();
    if (is_str_equal(type, "ME0") || is_str_equal(type, "me0")) {
      buf_type = AM_DATA_FRAME_TYPE_ME0;
    } else if (is_str_equal(type, "ME1") || is_str_equal(type, "me1")) {
      buf_type = AM_DATA_FRAME_TYPE_ME1;
    } else if (is_str_equal(type, "YUV") || is_str_equal(type, "yuv")) {
      buf_type = AM_DATA_FRAME_TYPE_YUV;
    } else {
      WARN("Invalid face detect buffer type, use default type YUV");
      buf_type = AM_DATA_FRAME_TYPE_YUV;
    }
  }
  return buf_type;
}

bool AMFaceDetect::set_buffer_param(const AMRect rect)
{
  bool ret = false;
  do {
    std::unique_lock<std::mutex> lk(m_usr_mutex);
    if ((rect.offset.x > (int32_t)m_fd_config.width) || (rect.offset.y != 0)) {
      ERROR("The offset is invalid.");
      break;
    }
    m_yuv.rect.offset = rect.offset;
    ret = true;
  } while (0);
  return ret;
}

bool AMFaceDetect::enable_recognition(const bool enable)
{
  bool ret = false;
  do {
    std::unique_lock<std::mutex> lk(m_usr_mutex);

    if (!m_dynamic_enable_recog && enable) {
      m_fr_enable_event->signal();
    }
    m_dynamic_enable_recog = enable;
    ret = true;
  } while(0);
  return ret;
}

bool AMFaceDetect::get_recognition_state() const
{
  return m_dynamic_enable_recog;
}

float AMFaceDetect::get_male_probability(float *eigenvalues) const
{
  return m_algo_ctx.fr_male_probability(eigenvalues);
}

float AMFaceDetect::get_face_distance_quadratic(float *data1, float *data2) const
{
  return m_algo_ctx.fr_distance_quadratic(data1, data2);
}
