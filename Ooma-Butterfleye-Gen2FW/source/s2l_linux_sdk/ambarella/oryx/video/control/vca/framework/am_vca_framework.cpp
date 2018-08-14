/*******************************************************************************
 * am_vca_framework.cpp
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
#include "am_vca_framework.h"
#include "am_face_detect_if.h"
#include "am_motion_detect_if.h"
#include "am_hawxeye_if.h"
#include "am_vca_plugin_if.h"

#include "am_base_include.h"
#include "am_configure.h"
#include "am_define.h"
#include "am_log.h"
#include "am_plugin.h"
#include <mutex>
#include <algorithm>

#define AUTO_LOCK_VCA(mtx) std::lock_guard<std::recursive_mutex> lck (mtx)
#define DEFAULT_ORYX_CONFIG_DIR     "/etc/oryx/video/"
#define FEATURE_CONFIG_FILE         "features.acs"

static std::recursive_mutex video_vca_mtx;

#ifdef __cplusplus
extern "C" {
#endif
ORYX_API AMIEncodePlugin* create_encode_plugin(void *data)
{
  return AMVCA::create((AMVin*)data);
}
#ifdef __cplusplus
}
#endif

AMVCAPlugin::AMVCAPlugin()
{

}

AMVCAPlugin::AMVCAPlugin(const AMVCAPlugin &plugin) :
    so(plugin.so),
    plugin(plugin.plugin)
{

}

AMVCAPlugin::~AMVCAPlugin()
{
  AM_DESTROY(plugin);
  AM_DESTROY(so);
}

AMVCA::AMVCA(const char *name) :
    m_name(name),
    m_capture_loop_run(false),
    m_started(false)
{
  m_plugin_map.clear();
}

AMVCA::~AMVCA()
{
}

AM_RESULT AMVCA::init()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!load_vca_feature()) {
      ERROR("Failed to load vca feature.");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

  } while (0);
  return result;
}

AMVCA *AMVCA::create(AMVin *vin)
{
  AMVCA *result = new AMVCA("video-vca");
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMVCA::destroy()
{
  m_capture_loop_run = false;
  for (auto &m : m_plugin_map) {
    m.second->plugin->destroy();
  }
  m_plugin_map.clear();
  delete this;
}

bool AMVCA::start(AM_STREAM_ID id)
{
  AUTO_LOCK_VCA(video_vca_mtx);
  bool result = true;
  do {
    if (AM_UNLIKELY(m_started)) {
      NOTICE("Video vca has already started!");
      break;
    }
    load_all_plugins();
    m_capture_loop_run = true;
    for (auto &m : m_plugin_map) {
      AMIVCAPlugin *plugin = m.second->plugin;
      if (AM_UNLIKELY(!plugin->start())) {
        ERROR("Failed to start vca plugin %s", plugin->name().c_str());
        break;
      } else {
        INFO("VCA plugin %s started successfully!", plugin->name().c_str());
        m_started_plugin_map[plugin->name()] = plugin;
      }
    }

    if (!m_thread) {

      m_thread = AMThread::create("vca_data_cap",
                                  frame_capture_entry,
                                  this);
    }
    if (!m_thread) {
      ERROR("AMVCA::create m_thread failed!");
      result = false;
      break;
    }
    m_started = true;
  } while (0);
  return result;
}

bool AMVCA::stop(AM_STREAM_ID id)
{
  AUTO_LOCK_VCA(video_vca_mtx);
  bool result = true;
  if (m_started) {
    m_capture_loop_run = false;
    for (auto &m : m_plugin_map) {
      AMIVCAPlugin *plugin = m.second->plugin;
      if (AM_UNLIKELY(!plugin->stop())) {
        ERROR("Failed to stop VCA plugin %s", plugin->name().c_str());
      } else {
        INFO("VCA plugin %s stopped successfully!", plugin->name().c_str());
      }
    }

    m_started_plugin_map.clear();
    m_started = false;
    AM_DESTROY(m_thread);
  } else {
    NOTICE("VCA Framework has been stopped already!");
  }
  return result;
}

std::string& AMVCA::name()
{
  return m_name;
}

void* AMVCA::get_interface()
{
  return ((AMIVCA*)this);
}

bool AMVCA::load_vca_feature()
{
  bool result = true;
  std::string tmp;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  do {
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(FEATURE_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = false;
      break;
    }
    AMConfig &config = *config_ptr;
    if (config["video_vca"].exists()) {
      if (config["video_vca"].length()) {
        for (uint32_t i = 0; i < config["video_vca"].length(); ++i) {
          tmp = config["video_vca"][i].get<std::string>("");
          m_vca_feature.push_back(tmp);
        }
      }
    }
  } while (0);
  delete config_ptr;
  return result;
}

void AMVCA::load_all_plugins()
{
  do {
    // load face detect plugin;
    AMVCAPluginMap::iterator fd_iter =
        m_plugin_map.find(VCA_PLUGIN_FD);
    AMVCAPlugin *fdplugin = nullptr;
    std::string fd_so(VCA_PLUGIN_DIR);
    fd_so.append("/").append(VCA_PLUGIN_FD_SO);

    if (fd_iter != m_plugin_map.end()) {
      NOTICE("VCA Plugin \"%s\" is already loaded!",
            fd_iter->second->plugin->name().c_str());
      delete fd_iter->second;
      m_plugin_map.erase(fd_iter);
    }
    auto feature = std::find(m_vca_feature.begin(),
                        m_vca_feature.end(),
                        VCA_PLUGIN_FD);
    if (feature != m_vca_feature.end()) {
      fdplugin = load_plugin(fd_so.c_str());
      if (fdplugin) {
        m_plugin_map[VCA_PLUGIN_FD] = fdplugin;
        NOTICE("VCA face detect plugin %s is loaded!",
               fdplugin->plugin->name().c_str());
      }
    }

    //load motion detect plugin
    AMVCAPluginMap::iterator md_iter =
        m_plugin_map.find(VCA_PLUGIN_MD);
    AMVCAPlugin *mdplugin = nullptr;
    std::string md_so(VCA_PLUGIN_DIR);
    md_so.append("/").append(VCA_PLUGIN_MD_SO);

    if (md_iter != m_plugin_map.end()) {
      NOTICE("VCA Plugin \"%s\" is already loaded!",
             md_iter->second->plugin->name().c_str());
      delete md_iter->second;
      m_plugin_map.erase(md_iter);
    }
    feature = std::find(m_vca_feature.begin(),
                        m_vca_feature.end(),
                        VCA_PLUGIN_MD);
    if (feature != m_vca_feature.end()) {
      mdplugin = load_plugin(md_so.c_str());
      if (mdplugin) {
        m_plugin_map[VCA_PLUGIN_MD] = mdplugin;
        NOTICE("VCA motion detect plugin %s is loaded!",
               mdplugin->plugin->name().c_str());
      }
    }

    //load hawxeye plugin
    AMVCAPluginMap::iterator hxe_iter =
        m_plugin_map.find(VCA_PLUGIN_HAWXEYE);
    AMVCAPlugin *hxeplugin = nullptr;
    std::string hxe_so(VCA_PLUGIN_DIR);
    hxe_so.append("/").append(VCA_PLUGIN_HAWXEYE_SO);
    if (hxe_iter != m_plugin_map.end()) {
      NOTICE("VCA Plugin %s is already loaded!",
             hxe_iter->second->plugin->name().c_str());
      delete hxe_iter->second;
      m_plugin_map.erase(hxe_iter);
    }
    feature = std::find(m_vca_feature.begin(),
                        m_vca_feature.end(),
                        VCA_PLUGIN_HAWXEYE);
    if (feature != m_vca_feature.end()) {
      hxeplugin = load_plugin(hxe_so.c_str());
      if (hxeplugin) {
        m_plugin_map[VCA_PLUGIN_HAWXEYE] = hxeplugin;
        NOTICE("VCA hawxeye plugin %s is loaded!",
               hxeplugin->plugin->name().c_str());
      }
    }
  } while (0);
}

AMVCAPlugin* AMVCA::load_plugin(const char *name)
{
  AMVCAPlugin *vcaplugin = nullptr;
  do {
    if (!name) {
      ERROR("Invalid plugin (null)!");
      break;
    }

    vcaplugin = new AMVCAPlugin();
    if (!vcaplugin) {
      ERROR("Failed to allocate memory for AMVCAPlugin object!");
      break;
    }

    vcaplugin->so = AMPlugin::create(name);
    if (!vcaplugin->so) {
      ERROR("Failed to load plugin: %s", name);
    } else {
      CreateVCAPlugin create_vca_plugin =
          (CreateVCAPlugin)vcaplugin->so->get_symbol(CREATE_VCA_PLUGIN);
      if (!create_vca_plugin) {
        ERROR("Invalid warp plugin: %s", name);
        break;
      }

      vcaplugin->plugin = create_vca_plugin();
      if (!vcaplugin->plugin) {
        ERROR("Failed to create object of vca plugin: %s!", name);
        break;
      }
    }

    if (vcaplugin && (!vcaplugin->so || !vcaplugin->plugin)) {
      delete vcaplugin;
      vcaplugin = nullptr;
    }
  } while (0);
  return vcaplugin;
}

void AMVCA::frame_capture_entry(void *arg)
{
  AMVCA *vca = nullptr;
  do {
    if (!arg) {
      ERROR("Frame capture entry argument is null");
      break;
    }
    vca = (AMVCA*)arg;
    if (AM_RESULT_OK != vca->frame_capture()) {
      ERROR("Failed to do frame capture!");
      break;
    }
  } while (0);
}

AM_RESULT AMVCA::frame_capture()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMIVideoReaderPtr video_reader = AMIVideoReader::get_instance();
    if (!video_reader) {
      ERROR("Failed to get AMVideoReader instance!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    AMIVideoAddressPtr video_address = AMIVideoAddress::get_instance();
    if (!video_address) {
      ERROR("Failed to get AMVideoAddress instance!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    while (m_capture_loop_run) {
      for (auto &m : m_started_plugin_map) {
        bool ret = true;
        AMQueryFrameDesc frame_desc;
        AMVCABuf buf_info;
        uint32_t try_count = 0;
        if (!m.second->get_buffer_config(buf_info)) {
          continue;
        }
        switch (buf_info.buf_type) {
          case AM_DATA_FRAME_TYPE_YUV:
            result = video_reader->query_yuv_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false);
            if ((AM_RESULT_OK != result) && (AM_RESULT_ERR_AGAIN != result)) {
              ERROR("Failed to query frame");
              ret = false;
            } else {
              while (AM_RESULT_ERR_AGAIN ==
                     video_reader->query_yuv_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false)) {
                if (++ try_count >  20) {
                  ERROR("Failed to query frame");
                  try_count = 0;
                  ret = false;
                  break;
                }
              }
            }
            break;
          case AM_DATA_FRAME_TYPE_ME0:
            result = video_reader->query_me0_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false);
            if ((AM_RESULT_OK != result) && (AM_RESULT_ERR_AGAIN != result)) {
              ERROR("Failed to query frame");
              ret = false;
            } else {
              while (AM_RESULT_ERR_AGAIN ==
                     video_reader->query_me0_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false)) {
                if (++ try_count >  20) {
                  ERROR("Failed to query frame");
                  try_count = 0;
                  ret = false;
                  break;
                }
              }
            }
            break;
          case AM_DATA_FRAME_TYPE_ME1:
            result = video_reader->query_me1_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false);
            if ((AM_RESULT_OK != result) && (AM_RESULT_ERR_AGAIN != result)) {
              ERROR("Failed to query frame");
              ret = false;
            } else {
              while (AM_RESULT_ERR_AGAIN ==
                     video_reader->query_me1_frame(frame_desc,
                                                   buf_info.buf_id,
                                                   false)) {
                if (++ try_count >  20) {
                  ERROR("Failed to query frame");
                  try_count = 0;
                  ret = false;
                  break;
                }
              }
            }
            break;
          default:
            ERROR("Not supported buffer type");
            ret = false;
            break;
        }

        if (!ret) {
          continue;
        }

        if (AM_UNLIKELY(!m.second->data_loop(frame_desc))) {
          ERROR("Failed to call data_loop from plugin %s!",
                m.second->name().c_str());
          result = AM_RESULT_ERR_INVALID;
          continue;
        }
      }
    }
  }while(0);

  return result;
}

void* AMVCA::get_plugin_interface(const std::string &plugin_name)
{
  AMVCAPluginMap::iterator iter = m_plugin_map.find(plugin_name);
  return ((iter != m_plugin_map.end()) ?
      (iter->second->plugin->get_interface()) : nullptr);
}
