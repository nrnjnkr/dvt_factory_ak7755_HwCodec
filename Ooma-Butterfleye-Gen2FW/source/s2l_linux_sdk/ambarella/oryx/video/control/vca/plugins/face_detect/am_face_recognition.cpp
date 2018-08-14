/*******************************************************************************
 * am_face_recognition.cpp
 *
 * History:
 *   Jun 1, 2017 - [nzhang] created file
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
#include "am_log.h"
#include "am_face_recognition.h"

AMFaceRecogPtr AMFaceRecog::create(int thread_number, opt_sc_algos_context_t *algo_ctx)
{
  AMFaceRecog *result = new AMFaceRecog();
  if (result && (result->init(thread_number, algo_ctx) != 0)) {
    delete result;
    result = nullptr;
  }
  return AMFaceRecogPtr(result, [](AMFaceRecog *p) {delete p;});
}

AMFaceRecog::AMFaceRecog()
{

}

AMFaceRecog::~AMFaceRecog()
{
  destroy_face_recognition();
}

int AMFaceRecog::init(int thread_number, opt_sc_algos_context_t *algo_ctx)
{
  int ret = 0;
  std::string path_name =  FACE_RECOG_CONFIG_FILE;
  do {
    m_algo_ctx = *algo_ctx;
    m_cf_image_buffer = new float[D_CF_IN_HEIGHT * D_CF_IN_WIDTH * 3];
    if (!m_cf_image_buffer) {
      ret = -1;
      ERROR("Failet to new m_cf_image_buffer.");
      break;
    }

    m_eigenvalues = new float[D_CF_EIGENVALUES_NUM];
    if (!m_eigenvalues) {
      ret = -2;
      ERROR("Failet to new m_eigenvalues.");
      break;
    }

    m_eigenvalues_save = new float[D_CF_EIGENVALUES_NUM];
    if (!m_eigenvalues_save) {
      ret = -3;
      ERROR("Failet to new m_eigenvalues_save.");
      break;
    }

    m_after_transform_buf = new uint8_t[D_CF_IN_HEIGHT * D_CF_IN_WIDTH * 3];
    if (!m_after_transform_buf) {
      ERROR("malloc buffer failed!");
      ret = false;
      break;
    }

    m_opt_caffe_face_ctx = m_algo_ctx.fr_create_context(path_name.c_str(),
                                                        thread_number);
    if (!m_opt_caffe_face_ctx) {
      ERROR("Failed to create caffe face.");
      ret = -4;
      break;
    }
  } while (0);
  return ret;
}

bool AMFaceRecog::do_caffe_face(AMFDYUVINFO face_yuv,
                                AMFDINFO face_info,
                                float *image,
                                std::pair<bool, std::vector<float>> &eigenvalues,
                                uint8_t &w_after_transform,
                                uint8_t &h_after_transform,
                                uint8_t** addr_after_transform)
{
  bool ret = false;
  do {
    if ((face_info.face_rect_w < 57) || (face_info.face_rect_h < 67)) {
      eigenvalues.first = false;
      ret = true;
      break;
    }
    m_moving_points[0] = face_info.face_ldmk[0].x;
    m_moving_points[1] = face_info.face_ldmk[0].y;
    m_moving_points[2] = face_info.face_ldmk[1].x;
    m_moving_points[3] = face_info.face_ldmk[1].y;
    m_moving_points[4] = face_info.face_ldmk[2].x;
    m_moving_points[5] = face_info.face_ldmk[2].y;
    m_moving_points[6] = face_info.face_ldmk[3].x;
    m_moving_points[7] = face_info.face_ldmk[3].y;
    m_moving_points[8] = face_info.face_ldmk[4].x;
    m_moving_points[9] = face_info.face_ldmk[4].y;

    m_algo_ctx.img_tool_setup_similarity_transform_5(&m_similarity_transform,
                                                     m_moving_points,
                                                     m_fixed_points);

    m_algo_ctx.img_tool_similarity_transform(&m_similarity_transform,
                                             image, m_cf_image_buffer,
                                             face_yuv.width, face_yuv.height,
                                             D_CF_IN_WIDTH, D_CF_IN_HEIGHT);

    if (m_algo_ctx.fr_forward(m_opt_caffe_face_ctx,
                              m_cf_image_buffer,
                              m_eigenvalues) < 0) {
      break;
    }
    eigenvalues.first = true;
    eigenvalues.second.assign(m_eigenvalues, m_eigenvalues + D_CF_EIGENVALUES_NUM);

    w_after_transform = D_CF_IN_WIDTH;
    h_after_transform = D_CF_IN_HEIGHT;
    for (uint32_t i = 0; i < D_CF_IN_HEIGHT * D_CF_IN_WIDTH; i++) {
      //red
      *(m_after_transform_buf + 2 + i * 3) =
          (uint8_t)(*(m_cf_image_buffer + D_CF_IN_HEIGHT * D_CF_IN_WIDTH - 1 - i)
              * 128 + 128);
      //green
      *(m_after_transform_buf + 1 + i * 3) =
          (uint8_t)(*(m_cf_image_buffer + D_CF_IN_HEIGHT * D_CF_IN_WIDTH * 2 - 1 - i)
              * 128 + 128);
      //blue
      *(m_after_transform_buf + 0 + i * 3) =
          (uint8_t)(*(m_cf_image_buffer + D_CF_IN_HEIGHT * D_CF_IN_WIDTH * 3 - 1 - i)
              * 128 + 128);
    }
    *addr_after_transform = m_after_transform_buf;
    ret = true;
  } while (0);
  return ret;
}

void AMFaceRecog::destroy_face_recognition()
{
  delete[] m_cf_image_buffer;
  delete[] m_eigenvalues;
  delete[] m_eigenvalues_save;
  delete[] m_after_transform_buf;
  if (m_opt_caffe_face_ctx) {
    m_algo_ctx.fr_destroy_context(m_opt_caffe_face_ctx);
  }
}
