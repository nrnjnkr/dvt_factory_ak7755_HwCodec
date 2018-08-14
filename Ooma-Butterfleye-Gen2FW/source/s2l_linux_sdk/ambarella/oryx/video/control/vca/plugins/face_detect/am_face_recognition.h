/*******************************************************************************
 * am_face_recognition.h
 *
 * History:
 *   May 31, 2017 - [nzhang] created file
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
#ifndef ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_RECOGNITION_H_
#define ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_RECOGNITION_H_

#include <memory>
#include "am_face_detect_types.h"
#include "opt_sc_algos_interface.h"

#define DMAX_FACE_NUMBER 16
#define FACE_RECOG_CONFIG_FILE "/sdcard/model.w"

class AMFaceRecog;

typedef std::shared_ptr<AMFaceRecog> AMFaceRecogPtr;

class AMFaceRecog
{
  public:
    static AMFaceRecogPtr create(int thread_number, opt_sc_algos_context_t *algo_ctx);
    bool do_caffe_face(AMFDYUVINFO face_yuv, AMFDINFO face_info, float *image,
                       std::pair<bool, std::vector<float>> &eigenvalues,
                       uint8_t &w_after_transform,
                       uint8_t &h_after_transform,
                       uint8_t** addr_after_transform);

  private:
    int init(int thread_number, opt_sc_algos_context_t *algo_ctx);
    void destroy_face_recognition();

  private:
    AMFaceRecog();
    virtual ~AMFaceRecog();

  private:
    float                           *m_cf_image_buffer             = nullptr;
    float                           *m_eigenvalues                 = nullptr;
    float                           *m_eigenvalues_save            = nullptr;
    void                            *m_opt_caffe_face_ctx          = nullptr;
    uint8_t                         *m_after_transform_buf         = nullptr;
    img_tool_similarity_transform_t  m_similarity_transform        = {0};
    float                            m_moving_points[10]           = {0};
    opt_sc_algos_context_t           m_algo_ctx;
    float                            m_fixed_points[10]            = {
                                                              30.2946, 51.6963,
                                                              65.5318, 51.5014,
                                                              48.0252, 71.7366,
                                                              33.5493, 92.3655,
                                                              62.7299, 92.2041,
                                                                      };
};

#endif /* ORYX_VIDEO_CONTROL_VCA_PLUGINS_FACE_DETECT_AM_FACE_RECOGNITION_H_ */
