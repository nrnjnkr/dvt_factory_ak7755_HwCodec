/*******************************************************************************
 * am_record_engine_config.cpp
 *
 * History:
 *   2014-12-30 - [ypchang] created file
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

#include "am_record_engine_config.h"
#include "am_configure.h"

#include <algorithm>

AMRecordEngineConfig::AMRecordEngineConfig() :
  m_config(nullptr),
  m_engine_config(nullptr)
{}

AMRecordEngineConfig::~AMRecordEngineConfig()
{
  delete m_config;
  delete m_engine_config;
}

static std::string __(const std::string& a)
{
  /* Convert '-'(minus sign) in filter name to '_'(underscore) */
  std::string b = a;
  for_each(b.begin(), b.end(), [](char &c){if (c == '-'){c = '_';}});
  return b;
}

EngineConfig* AMRecordEngineConfig::get_config(const std::string& conf_file)
{
  EngineConfig *config = nullptr;
  do {
    bool isOK = true;
    delete m_config;
    if (AM_LIKELY(!m_engine_config)) {
      m_engine_config = new EngineConfig();
    }

    if (AM_UNLIKELY(!m_engine_config)) {
      ERROR("Failed to allocate memory for EngineConfig!");
      break;
    }
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(!m_config)) {
      break;
    }
    AMConfig& engine = *m_config;

    if (AM_LIKELY(!engine["op_timeout"].exists())) {
      ERROR("Invalid playback engine configuration file, "
            "\"op_timeout\" doesn't exist!");
      break;
    }
    if (AM_LIKELY(!engine["filters"].exists())) {
      ERROR("Invalid playback engine configuration file, "
            "\"filters\" doesn't exist!");
      break;
    }
    if (AM_LIKELY(!engine["connection"].exists())) {
      ERROR("Invalid playback engine configuration file, "
            "\"connection\" doesn't exist!");
      break;
    }
    m_engine_config->op_timeout = engine["op_timeout"].get<unsigned>(5);
    m_engine_config->filter_num = engine["filters"].length();

    m_engine_config->filters = new std::string[m_engine_config->filter_num];
    if (AM_UNLIKELY(!m_engine_config->filters)) {
      ERROR("Failed to allocate memory for filters config!");
      break;
    }

    m_engine_config->connections =
        new ConnectionConfig[m_engine_config->filter_num];
    if (AM_UNLIKELY(!m_engine_config->connections)) {
      ERROR("Failed to allocate memory for connections config!");
      break;
    }
    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      m_engine_config->filters[i] = engine["filters"][i].get<std::string>("");
      m_engine_config->connections[i].filter = m_engine_config->filters[i];
    }

    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      ConnectionConfig& connection = m_engine_config->connections[i];
      std::string filter_name = __(connection.filter);
      uint32_t con_len_i =
          engine["connection"][filter_name]["input"].exists() ?
          engine["connection"][filter_name]["input"].length() : 0;
      uint32_t con_len_o =
          engine["connection"][filter_name]["output"].exists() ?
          engine["connection"][filter_name]["output"].length() : 0;

      if (AM_LIKELY(!engine["connection"][filter_name].exists())) {
        ERROR("Connection information is not defined for filter %s",
              filter_name.c_str());
        isOK = false;
        break;
      }
      /* Get Input Information */
      if (AM_LIKELY(con_len_i)) {
        connection.input_number = 0;
        for (uint32_t ii = 0; ii < con_len_i; ++ ii) {
          std::string con_in_filter =
              engine["connection"][filter_name]["input"][ii].
              get<std::string>("");
          if (AM_LIKELY(filter_exists(con_in_filter))) {
            DEBUG("Found %s's input filter %s",
                 filter_name.c_str(),
                 __(con_in_filter).c_str());
            ++ connection.input_number;
          } else {
            NOTICE("Filter %s's input filter %s is not enabled!",
                   filter_name.c_str(),
                   __(con_in_filter).c_str());
          }
        }
        DEBUG("Filter %s's input filter number is %u",
             filter_name.c_str(),
             connection.input_number);
        if (AM_LIKELY(connection.input_number > 0)) {
          connection.input =
              new ConnectionConfig*[connection.input_number];
          if (AM_UNLIKELY(!connection.input)) {
            ERROR("Failed to allocate memory for input information!");
            isOK = false;
            break;
          } else {
            memset(connection.input, 0,
                   sizeof(ConnectionConfig*) * connection.input_number);
          }
        } else {
          DEBUG("Filter %s doesn't have input!",
               connection.filter.c_str());
        }
      } else {
        DEBUG("Filter %s doesn't have input!", connection.filter.c_str());
      }
      /* Get Output Information */
      if (AM_LIKELY(con_len_o)) {
        connection.output_number = 0;
        for (uint32_t con_out = 0; con_out < con_len_o; ++ con_out) {
          std::string con_out_filter =
              engine["connection"][filter_name]["output"][con_out].
              get<std::string>("");
          if (AM_LIKELY(filter_exists(con_out_filter))) {
            DEBUG("Found %s's output filter %s",
                 filter_name.c_str(),
                 __(con_out_filter).c_str());
            ++ connection.output_number;
          } else {
            NOTICE("Filter %s's output filter %s is not enabled!",
                   filter_name.c_str(),
                   __(con_out_filter).c_str());
          }
        }
        DEBUG("Filter %s's output filter number is %u",
             __(connection.filter).c_str(),
             connection.output_number);
        if (AM_LIKELY(connection.output_number > 0)) {
          connection.output =
              new ConnectionConfig*[connection.output_number];
          if (AM_UNLIKELY(!connection.output)) {
            ERROR("Failed to allocate memory for output information!");
            isOK = false;
            break;
          } else {
            memset(connection.output, 0,
                   sizeof(ConnectionConfig*) * connection.output_number);
          }
        } else {
          DEBUG("Filter %s doesn't have output!",
               connection.filter.c_str());
        }
      } else {
        DEBUG("Filter %s doesn't have output!", connection.filter.c_str());
      }

    } /* for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) */

    /* Build connections relations */
    if (AM_LIKELY(isOK)) {
      for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
        ConnectionConfig &connection = m_engine_config->connections[i];
        std::string filter = __(connection.filter);
        uint32_t con_len_i = engine["connection"][filter]["input"].exists() ?
            engine["connection"][filter]["input"].length() : 0;
        uint32_t con_len_o = engine["connection"][filter]["output"].exists() ?
            engine["connection"][filter]["output"].length() : 0;
        uint32_t k = 0;

        /* Find all input filter */
        for (uint32_t j = 0; j < connection.input_number; ++ j) {
          std::string input;
          while (k < con_len_i) {
            input = engine["connection"][filter]["input"][k ++].
                get<std::string>("");
            if (AM_LIKELY(filter_exists(input))) {
              connection.input[j] = find_connection_by_name(input);
              INFO("Filter [%20s] has  input [%20s]",
                    connection.filter.c_str(),
                    connection.input[j]->filter.c_str());
              break;
            }
          }
          if (AM_UNLIKELY(!connection.input[j])) {
            ERROR("Failed to find configuration of input filter %s",
                  input.c_str());
            isOK = false;
            break;
          }
        }
        if (AM_UNLIKELY(!isOK)) {
          break;
        }
        /* Find all output filter */
        k = 0;
        for (uint32_t j = 0; j < connection.output_number; ++ j) {
          std::string output;
          while (k < con_len_o) {
            output = engine["connection"][filter]["output"][k ++].
                get<std::string>("");
            if (AM_LIKELY(filter_exists(output))) {
              connection.output[j] = find_connection_by_name(output);
              INFO("Filter [%20s] has output [%20s]",
                    connection.filter.c_str(),
                    connection.output[j]->filter.c_str());
              break;
            }
          }
          if (AM_UNLIKELY(!connection.output[j])) {
            ERROR("Failed to find configuration of output filter %s",
                  output.c_str());
            isOK = false;
            break;
          }
        }
        if (AM_UNLIKELY(!isOK)) {
          break;
        }
      } /* for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) */
    }
    if (AM_LIKELY(isOK)) {
      config = m_engine_config;
    }
  }while(0);

  return config;
}

inline bool AMRecordEngineConfig::filter_exists(std::string &filter)
{
  bool found = false;
  for (uint32_t i = 0;i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(__(filter) == __(m_engine_config->filters[i]))) {
      found = true;
      break;
    }
  }
  return found;
}

inline ConnectionConfig* AMRecordEngineConfig::find_connection_by_name(
    std::string &filter)
{
  ConnectionConfig *con = nullptr;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(__(filter) == __(m_engine_config->connections[i].filter))) {
      con = &m_engine_config->connections[i];
      break;
    }
  }

  return con;
}
