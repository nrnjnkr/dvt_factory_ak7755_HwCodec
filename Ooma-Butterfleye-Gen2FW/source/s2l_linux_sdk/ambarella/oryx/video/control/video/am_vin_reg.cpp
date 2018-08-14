/*******************************************************************************
 * am_vin_reg.cpp
 *
 * History:
 *   Jun 13, 2017 - [ypchang] created file
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

#include "am_vin_reg.h"
#include "am_file.h"
#include "am_configure.h"
#include "am_video_utility.h"

#define DEFAULT_ORYX_CONFIG_DIR "/etc/oryx/video/"
#define VIN_REG_CONFIG_FILE     "vin_reg.acs"

AMVinRegister* AMVinRegister::create(AM_VIN_ID id)
{
  AMVinRegister *result = new AMVinRegister(id);
  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->init()))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMVinRegister::destroy()
{
  delete this;
}

AM_RESULT AMVinRegister::apply(AM_VIN_MODE mode, std::string &name)
{
  AM_RESULT ret = AM_RESULT_OK;
  uint32_t reg_num = m_vin_regs_list.size();
  int32_t index = -1;

  for (uint32_t i = 0; i < reg_num; ++ i) {
    if (AM_LIKELY((m_vin_regs_list.at(i).sensor_mode == mode) &&
                  (m_vin_regs_list.at(i).sensor_name == name))) {
      index = i;
      break;
    }
  }

  if (AM_LIKELY(-1 != index)) {
    AMVinRegVector &reg_list = m_vin_regs_list.at(index).reg_list;
    if (AM_LIKELY(reg_list.size() > 0)) {
      INFO("Applying register settings for sensor %s on VIN[%u]",
           name.c_str(),
           m_id);
      m_vin_regs_list.at(index).print();
      ret = m_platform->vin_register_set(m_id, reg_list);
    }
  } else {
    NOTICE("No special register settings are found for sensor %s with mode %s",
           name.c_str(),
           AMVinTrans::mode_enum_to_str(mode).c_str());
  }

  return ret;
}

AMVinRegister::AMVinRegister(AM_VIN_ID id) :
    m_id(id)
{
}

AMVinRegister::~AMVinRegister()
{
  m_platform = nullptr;
}

AM_RESULT AMVinRegister::init()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (AM_UNLIKELY(!(m_platform = AMIPlatform::get_instance()))) {
      ret = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }
    ret = get_vin_reg_list();
  }while(0);

  return ret;
}

AM_RESULT AMVinRegister::get_vin_reg_list()
{
  AM_RESULT ret = AM_RESULT_OK;
  AMConfig *config = nullptr;
  char *conf_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
  std::string conf_file = conf_dir ? std::string(conf_dir) :
                                     std::string(DEFAULT_ORYX_CONFIG_DIR);
  conf_file += std::string("/") + std::string(VIN_REG_CONFIG_FILE);
  do {
    m_vin_regs_list.clear();
    if (AM_LIKELY(!AMFile::exists(conf_file.c_str()))) {
      NOTICE("Register settings for VIN[%u] doesn't exist!", m_id);
      break;
    }
    if (AM_UNLIKELY(!(config = AMConfig::create(conf_file.c_str())))) {
      ERROR("Failed to create AMConfig: %s", conf_file.c_str());
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &conf = *config;
    if (AM_LIKELY(m_id >= conf.length())) {
      NOTICE("No special registers are set for VIN[%d]", m_id);
      break;
    }

    uint32_t config_num = conf[m_id].length();
    for (uint32_t i = 0; i < config_num; ++ i) {
      AMVinRegConfig reg_conf;
      if (AM_LIKELY(conf[m_id][i]["sensor"].exists())) {
        reg_conf.sensor_name =
            conf[m_id][i]["sensor"].get<std::string>("none");
      }
      if (AM_LIKELY(conf[m_id][i]["mode"].exists())) {
        std::string mode =
            conf[m_id][i]["mode"].get<std::string>("auto");
        reg_conf.sensor_mode = AMVinTrans::mode_str_to_enum(mode);
      }
      if (AM_LIKELY(conf[m_id][i]["regs"].exists())) {
        uint32_t reg_num = conf[m_id][i]["regs"].length();
        reg_conf.reg_list.clear();
        for (uint32_t j = 0; j < reg_num; ++ j) {
          AMVinRegValue reg_value;
          reg_value.reg_addr = conf[m_id][i]["regs"][j][0].get<uint32_t>();
          reg_value.reg_data = conf[m_id][i]["regs"][j][1].get<uint32_t>();
          reg_conf.reg_list.push_back(reg_value);
        }
      }
      m_vin_regs_list.push_back(reg_conf);
    }
  }while(0);

  return ret;
}
