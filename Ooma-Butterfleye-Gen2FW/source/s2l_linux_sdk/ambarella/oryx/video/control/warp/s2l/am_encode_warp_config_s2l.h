/*******************************************************************************
 * am_encode_warp_config_s2l.h
 *
 * History:
 *   2016/1/6 - [smdong] created file
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
 ******************************************************************************/
#ifndef ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_
#define ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_

#include <map>
#include <mutex>
#include <atomic>

#include "am_pointer.h"
#include "am_encode_warp_if.h"

class AMWarpConfigS2L;
typedef AMPointer<AMWarpConfigS2L> AMWarpConfigS2LPtr;

struct AMWarpConfigS2LParam
{
    uint8_t log_level = 0;
    std::pair<bool, bool> enable = {false, false};
    std::pair<bool, uint8_t> channel_id = {false, 0};
    std::pair<bool, int> max_radius = {false, 0};
    std::pair<bool, float> ldc_strength = {false, 0.0};
    std::pair<bool, AM_WARP_PROJECTION_MODE> proj_mode =
    {false, AM_WARP_PROJRECTION_EQUIDISTANT};
    std::pair<bool, AMPoint> lens_center_in_max_input;
    std::pair<bool, AM_WARP_MODE> warp_mode = {false, AM_WARP_MODE_NO_TRANSFORM};
    std::pair<bool, int> pitch = {false, 0};
    std::pair<bool, int> yaw = {false, 0};
    std::pair<bool, int> rotate = {false, 0};
    std::pair<bool, AMFrac> zoom;
    std::pair<bool, float> pano_hfov_degree = {false, 0.0};
    std::pair<bool, float> ver_panor_degree = {false, 0.0};
    std::pair<bool, bool> pan_tilt_flag = {false, false};
    std::pair<bool, float> pan = {false, 0.0};
    std::pair<bool, float> tilt = {false, 0.0};
    std::pair<bool, AMPointF> sub_roi_offset;
    std::pair<bool, AMPoint> no_roi_offset;
    std::pair<bool, AMResolution> no_roi_size;
    std::pair<bool, float> efl_mm = {false, 0.0};
    std::pair<bool, std::string> lut;
    std::pair<bool, bool> force_zero = {false, false};
    std::pair<bool, bool> hor_disable = {false, false};
    std::pair<bool, bool> ver_disable = {false, false};
    std::pair<bool, int> virtual_optical_pitch = {false, 0};
};

class AMWarpConfigS2L
{
    friend AMWarpConfigS2LPtr;
  public:
    static AMWarpConfigS2LPtr get_instance();
    AM_RESULT get_config(AMWarpConfigS2LParam &config);
    AM_RESULT set_config(const AMWarpConfigS2LParam &config);

    AM_RESULT load_config();
    AM_RESULT save_config();

    AMWarpConfigS2L();
    virtual ~AMWarpConfigS2L();
    void inc_ref();
    void release();

  private:
    static AMWarpConfigS2L      *m_instance;
    static std::recursive_mutex  m_mtx;
    AMWarpConfigS2LParam         m_config;
    std::atomic_int              m_ref_cnt;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define WARP_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video"))
#else
#define WARP_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#define WARP_CONFIG_FILE "warp.acs"



#endif /* ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_ */
