/*******************************************************************************
 * am_media_service_config.cpp
 *
 * History:
 *   2016-12-23 - [ccjing] created file
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

#include "am_configure.h"
#include "am_media_service_config.h"

AMMediaServiceConfig::AMMediaServiceConfig()
{
}

AMMediaServiceConfig::~AMMediaServiceConfig()
{
  delete m_config;
  delete m_config_struct;
}

bool AMMediaServiceConfig::set_config(AMMediaServiceStruct *config)
{
  bool ret = true;
  do{
    if (AM_UNLIKELY(config == nullptr)) {
      ERROR("Input config struct is NULL.");
      ret = false;
      break;
    }
    AMConfig &config_media = *m_config;
    /* playback_instance_num */
    if (AM_LIKELY(config_media["playback_instance_num"].exists())) {
      config_media["playback_instance_num"] = config->playback_instance_num;
    } else {
      NOTICE("\"playback_instance_num\" is not exist.");
    }
    /*auto_create_recording*/
    if (AM_LIKELY(config_media["auto_create_recording"].exists())) {
      config_media["auto_create_recording"] = config->auto_create_recording;
    } else {
      NOTICE("\"auto_create_recording\" is not exist");
    }
    /*auto_start_recording*/
    if (AM_LIKELY(config_media["auto_start_recording"].exists())) {
      config_media["auto_start_recording"] = config->auto_start_recording;
    } else {
      NOTICE("\"auto_start_recording\" is not exist");
    }
    /*auto_create_playback*/
    if (AM_LIKELY(config_media["auto_create_playback"].exists())) {
      config_media["auto_create_playback"] = config->auto_create_playback;
    } else {
      NOTICE("\"auto_create_playback\" is not exist");
    }
    if (AM_UNLIKELY(!config_media.save())) {
      ERROR("Failed to save config information in media service.");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

AMMediaServiceStruct* AMMediaServiceConfig::get_config(
    const std::string &config_file)
{
  AMMediaServiceStruct *config = nullptr;
  do {
    if (AM_LIKELY(nullptr == m_config_struct)) {
      m_config_struct = new AMMediaServiceStruct();
    }
    if (AM_UNLIKELY(!m_config_struct)) {
      ERROR("Failed to create media service config struct.");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(config_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &config_media = *m_config;
      /* playback_instance_num */
      if (AM_LIKELY(config_media["playback_instance_num"].exists())) {
        m_config_struct->playback_instance_num =
            config_media["playback_instance_num"].get<signed>(2);
      } else {
        NOTICE("\"playback_instance_num\" is not specified, use 2 as default!");
        m_config_struct->playback_instance_num = 2;
      }
      /*auto_create_recording*/
      if (AM_LIKELY(config_media["auto_create_recording"].exists())) {
        m_config_struct->auto_create_recording =
            config_media["auto_create_recording"].get<bool>(true);
      } else {
        NOTICE("\"auto_create_recording\" is not specified, use default!");
        m_config_struct->auto_create_recording = true;
      }
      /*auto_start_recording*/
      if (AM_LIKELY(config_media["auto_start_recording"].exists())) {
        m_config_struct->auto_start_recording =
            config_media["auto_start_recording"].get<bool>(true);
      } else {
        NOTICE("\"auto_start_recording\" is not specified, use default!");
        m_config_struct->auto_start_recording = true;
      }
      /*auto_create_playback*/
      if (AM_LIKELY(config_media["auto_create_playback"].exists())) {
        m_config_struct->auto_create_playback =
            config_media["auto_create_playback"].get<bool>(true);
      } else {
        NOTICE("\"auto_create_playback\" is not specified, use default!");
        m_config_struct->auto_create_playback = true;
      }
    } else {
      ERROR("Failed to create AMConfig object in meida service config.");
      break;
    }
    config = m_config_struct;

  } while (0);

  return config;
}
