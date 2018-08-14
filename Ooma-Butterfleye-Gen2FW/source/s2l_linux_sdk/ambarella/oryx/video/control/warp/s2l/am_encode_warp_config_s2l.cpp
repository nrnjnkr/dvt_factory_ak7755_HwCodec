/*******************************************************************************
 * am_encode_warp_config_s2l.cpp
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
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_encode_warp_config_s2l.h"
#include "am_configure.h"

AMWarpConfigS2L *AMWarpConfigS2L::m_instance = nullptr;
std::recursive_mutex AMWarpConfigS2L::m_mtx;
#define AUTO_LOCK_WARP(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMWarpConfigS2L::AMWarpConfigS2L() :
    m_ref_cnt(0)
{
}

AMWarpConfigS2L::~AMWarpConfigS2L()
{
  DEBUG("~AMWarpConfigS2L");
}


AM_RESULT AMWarpConfigS2L::load_config()
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = WARP_CONF_DIR;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + WARP_CONFIG_FILE;
    if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig& config = *config_ptr;
    if (AM_LIKELY(config["log_level"].exists())) {
      m_config.log_level = config["log_level"].get<int>(0);
    }
    if (AM_LIKELY(config["enable"].exists())) {
      m_config.enable.first = true;
      m_config.enable.second = config["enable"].get<bool>(false);
      if (m_config.enable.second) {
        if (AM_LIKELY(config["max_radius"].exists())) {
          m_config.max_radius.first = true;
          m_config.max_radius.second = config["max_radius"].get<int>(0);
        }
        if (AM_LIKELY(config["ldc_strength"].exists())) {
          m_config.ldc_strength.first = true;
          m_config.ldc_strength.second = config["ldc_strength"].get<float>(0);
          if (AM_UNLIKELY((m_config.ldc_strength.second > 36.0) ||
                          (m_config.ldc_strength.second < 0.0))) {
            ERROR("Invalid LDC strength: %f!", m_config.ldc_strength.second);
            result = AM_RESULT_ERR_INVALID;
            break;
          }
        }
        if (AM_LIKELY(config["proj_mode"].exists())) {
          int32_t mode = config["proj_mode"].get<int>(0);
          m_config.proj_mode.first = true;
          m_config.proj_mode.second = AM_WARP_PROJECTION_MODE(mode);
        }
        if (AM_LIKELY(config["lens_center_in_max_input"].exists())) {
          m_config.lens_center_in_max_input.first = true;
          m_config.lens_center_in_max_input.second.x =
              config["lens_center_in_max_input"]["x"].get<int>(0);
          m_config.lens_center_in_max_input.second.y =
              config["lens_center_in_max_input"]["y"].get<int>(0);
        }
        if (AM_LIKELY(config["warp_mode"].exists())) {
          int32_t mode = config["warp_mode"].get<int>(0);
          m_config.warp_mode.first = true;
          m_config.warp_mode.second = AM_WARP_MODE(mode);
        }
        if (AM_LIKELY(config["pitch"].exists())) {
          m_config.pitch.first = true;
          m_config.pitch.second = config["pitch"].get<int>(0);
        }
        if (AM_LIKELY(config["yaw"].exists())) {
          m_config.yaw.first = true;
          m_config.yaw.second = config["yaw"].get<int>(0);
        }
        if (AM_LIKELY(config["rotate"].exists())) {
          m_config.rotate.first = true;
          m_config.rotate.second = config["rotate"].get<int>(0);
        }
        if (AM_LIKELY(config["zoom"].exists())) {
          m_config.zoom.first = true;
          m_config.zoom.second.num = config["zoom"]["num"].get<int>(0);
          m_config.zoom.second.denom = config["zoom"]["den"].get<int>(0);
        }
        if (AM_LIKELY(config["pano_hfov_degree"].exists())) {
          m_config.pano_hfov_degree.first = true;
          m_config.pano_hfov_degree.second =
              config["pano_hfov_degree"].get<float>(0.0);
        }
        if (AM_LIKELY(config["ver_panor_degree"].exists())) {
          m_config.ver_panor_degree.first = true;
          m_config.ver_panor_degree.second =
              config["ver_panor_degree"].get<float>(0.0);
        }
        if (AM_LIKELY(config["pan_tilt_flag"].exists())) {
          m_config.pan_tilt_flag.first = true;
          m_config.pan_tilt_flag.second =
              config["pan_tilt_flag"].get<bool>(false);
        }
        if (AM_LIKELY(config["pan"].exists())) {
          m_config.pan.first = true;
          m_config.pan.second = config["pan"].get<float>(0);
        }
        if (AM_LIKELY(config["tilt"].exists())) {
          m_config.tilt.first = true;
          m_config.tilt.second = config["tilt"].get<float>(0);
        }
        if (AM_LIKELY(config["sub_roi_offset"].exists())) {
          m_config.sub_roi_offset.first = true;
          m_config.sub_roi_offset.second.x =
              config["sub_roi_offset"]["x"].get<float>(0);
          m_config.sub_roi_offset.second.y =
              config["sub_roi_offset"]["y"].get<float>(0);
        }
        if (AM_LIKELY(config["no_roi_offset"].exists())) {
          m_config.no_roi_offset.first = true;
          m_config.no_roi_offset.second.x =
              config["no_roi_offset"]["x"].get<int>(0);
          m_config.no_roi_offset.second.y =
              config["no_roi_offset"]["y"].get<int>(0);
        }
        if (AM_LIKELY(config["no_roi_size"].exists())) {
          m_config.no_roi_size.first = true;
          m_config.no_roi_size.second.width =
              config["no_roi_size"]["width"].get<int>(0);
          m_config.no_roi_size.second.height =
              config["no_roi_size"]["height"].get<int>(0);
        }
        if (AM_LIKELY(config["efl_mm"].exists())) {
          m_config.efl_mm.first = true;
          m_config.efl_mm.second = config["efl_mm"].get<float>(0);
        }
        if (AM_LIKELY(config["lut"].exists())) {
          m_config.lut.first = true;
          m_config.lut.second = config["lut"].get<std::string>("");
        }
        if (AM_LIKELY(config["force_zero"].exists())) {
          m_config.force_zero.first = true;
          m_config.force_zero.second =
              config["force_zero"].get<bool>(false);
        }
        if (AM_LIKELY(config["hor_disable"].exists())) {
          m_config.hor_disable.first = true;
          m_config.hor_disable.second = config["hor_disable"].get<bool>(false);
        }
        if (AM_LIKELY(config["ver_disable"].exists())) {
          m_config.ver_disable.first = true;
          m_config.ver_disable.second = config["ver_disable"].get<bool>(false);
        }
        if (AM_LIKELY(config["virtual_optical_pitch"].exists())) {
          m_config.virtual_optical_pitch.first = true;
          m_config.virtual_optical_pitch.second =
              config["virtual_optical_pitch"].get<int>(0);
        }
      }
    }
  } while (0);

  delete config_ptr;
  return result;
}

AM_RESULT AMWarpConfigS2L::save_config()
{
  AUTO_LOCK_WARP(m_mtx);
    AM_RESULT result = AM_RESULT_OK;

    const char *default_dir = WARP_CONF_DIR;
    AMConfig *config_ptr = nullptr;

    do {
      std::string tmp;
      const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
      if (AM_UNLIKELY(!oryx_config_dir)) {
        oryx_config_dir = default_dir;
      }
      tmp = std::string(oryx_config_dir) + "/" + WARP_CONFIG_FILE;
      if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
        ERROR("Failed to create AMConfig: %s", tmp.c_str());
        result = AM_RESULT_ERR_MEM;
        break;
      }
      AMConfig& config = *config_ptr;
      config["enable"] = m_config.enable.second;
      config["max_radius"] =
          m_config.max_radius.second;
      config["ldc_strength"] = m_config.ldc_strength.second;
      config["proj_mode"] = (int)m_config.proj_mode.second;
      config["lens_center_in_max_input"]["x"] =
          m_config.lens_center_in_max_input.second.x;
      config["lens_center_in_max_input"]["y"] =
          m_config.lens_center_in_max_input.second.y;
      config["warp_mode"] = (int)m_config.warp_mode.second;
      config["pitch"] = m_config.pitch.second;
      config["yaw"] = m_config.yaw.second;
      config["rotate"] = m_config.rotate.second;
      config["zoom"]["num"] = m_config.zoom.second.num;
      config["zoom"]["den"] = m_config.zoom.second.denom;
      config["pano_hfov_degree"] =
          m_config.pano_hfov_degree.second;
      config["ver_panor_degree"] =
          m_config.ver_panor_degree.second;
      config["pan_tilt_flag"] = m_config.pan_tilt_flag.second;
      config["pan"] = m_config.pan.second;
      config["tilt"] = m_config.tilt.second;
      config["sub_roi_offset"]["x"] = m_config.sub_roi_offset.second.x;
      config["sub_roi_offset"]["y"] =m_config.sub_roi_offset.second.y;
      config["no_roi_offset"]["x"] = m_config.no_roi_offset.second.x;
      config["no_roi_offset"]["y"] = m_config.no_roi_offset.second.y;
      config["no_roi_size"]["width"] = m_config.no_roi_size.second.width;
      config["no_roi_size"]["height"] = m_config.no_roi_size.second.height;
      config["efl_mm"] = m_config.efl_mm.second;
      config["lut"] = m_config.lut.second;
      config["force_zero"] = m_config.force_zero.second;
      config["hor_disable"] = m_config.hor_disable.second;
      config["ver_disable"] = m_config.ver_disable.second;
      config["virtual_optical_pitch"] =
          m_config.virtual_optical_pitch.second;
      if (AM_UNLIKELY(!config_ptr->save())) {
        ERROR("Failed to save config: %s", tmp.c_str());
        result = AM_RESULT_ERR_IO;
        break;
      }
    } while (0);
    delete config_ptr;
    return result;
}

AM_RESULT AMWarpConfigS2L::get_config(AMWarpConfigS2LParam &config)
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMWarpConfigS2L::set_config(const AMWarpConfigS2LParam &config)
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_config = config;
  } while (0);
  return result;
}

AMWarpConfigS2LPtr AMWarpConfigS2L::get_instance()
{
  AUTO_LOCK_WARP(m_mtx);
  if (!m_instance) {
    m_instance = new AMWarpConfigS2L();
  }
  return m_instance;
}

void AMWarpConfigS2L::inc_ref()
{
  ++ m_ref_cnt;
}

void AMWarpConfigS2L::release()
{
  AUTO_LOCK_WARP(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}
