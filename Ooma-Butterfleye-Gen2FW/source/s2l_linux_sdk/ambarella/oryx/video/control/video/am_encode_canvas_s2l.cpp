/*******************************************************************************
 * am_encode_canvas_s2l.cpp
 *
 * History:
 *   May 31, 2017 - [Dong Shiming] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_define.h"

#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_encode_config.h"
#include "am_video_utility.h"
#include "am_encode_canvas.h"

AMEncodeCanvas* AMEncodeCanvas::create()
{
  AMEncodeCanvas *result = new AMEncodeCanvas();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMEncodeCanvas::destroy()
{
  delete this;
}

AMEncodeCanvas::AMEncodeCanvas()
{
  DEBUG("AMEncodeCanvas is created!");
}

AMEncodeCanvas::~AMEncodeCanvas()
{
  m_platform      = nullptr;
  m_buffer_config = nullptr;
  DEBUG("AMEncodeCanvas is destroyed!");
}

AM_RESULT AMEncodeCanvas::init()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    } else if ((result = m_platform->load_config()) != AM_RESULT_OK) {
      ERROR("Load config error!");
      break;
    }
    if (!(m_buffer_config = AMBufferConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMBufferConfig!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeCanvas::get_chan_param(AMMultiVinChanParamMap &param)
{
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeCanvas::get_canvas_param(AMMultiVinCanvasParam &param)
{
   return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeCanvas::get_buffer_param(AMBufferParamMap &param)
{
  param = m_buffer_param;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeCanvas::set_buffer_param(const AMBufferParamMap &param)
{
  for (auto &m : param) {
    if (m.second.type.first) {
      m_buffer_param[m.first].type = m.second.type;
      m_buffer_param[m.first].type.first = true;
    }
    if (m.second.input.first) {
      m_buffer_param[m.first].input = m.second.input;
      m_buffer_param[m.first].input.first = true;
    }
    if (m.second.platform_config.input_crop.first) {
      m_buffer_param[m.first].platform_config.input_crop =
          m.second.platform_config.input_crop;
      m_buffer_param[m.first].platform_config.input_crop.first = true;
    }
    if (m.second.platform_config.size.first) {
      m_buffer_param[m.first].platform_config.size =
          m.second.platform_config.size;
      m_buffer_param[m.first].platform_config.size.first = true;
    }
  }
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeCanvas::get_buffer_state(AM_SOURCE_BUFFER_ID id,
                                           AM_SRCBUF_STATE &state)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (m_platform->buffer_state_get(id, state) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to get buffer setup!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeCanvas::get_buffer_format(AMBufferConfigParam &param)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    AMPlatformBufferFormatMap format;
    if (m_platform->buffer_setup_get(format) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to get buffer setup!");
      break;
    }
    param.input.second = format[param.id].input;
    param.input.first = true;
    param.platform_config.size.second = format[param.id].platform_config.size;
    param.platform_config.size.first = true;
    param.type.second = format[param.id].type;
    param.type.first = true;
  } while (0);
  return result;
}

AM_RESULT AMEncodeCanvas::set_buffer_format(const AMBufferConfigParam &param)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformBufferFormat format;
  AM_SRCBUF_STATE state;

  do {
    if (m_platform->buffer_state_get(param.id, state) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to get buffer state!");
      break;
    }

    if (state != AM_SRCBUF_STATE_IDLE) {
      result = AM_RESULT_ERR_BUSY;
      ERROR("Source buffer[%d] is busy, format can't be set!", param.id);
      break;
    }

    format.id = param.id;
    if (m_platform->buffer_format_get(format) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to get buffer format!");
      break;
    }

    if (param.input.first) {
      if (param.input.second.offset.x > 0) {
        format.input.offset.x = param.input.second.offset.x;
      }
      if (param.input.second.offset.y > 0) {
        format.input.offset.y = param.input.second.offset.y;
      }
      if (param.input.second.size.width > 0) {
        format.input.size.width = param.input.second.size.width;
      }
      if (param.input.second.size.height > 0) {
        format.input.size.height = param.input.second.size.height;
      }
    }
    if (param.platform_config.size.first) {
      if (param.platform_config.size.second.width > 0) {
        format.platform_config.size.width =
            param.platform_config.size.second.width;
      }
      if (param.platform_config.size.second.height > 0) {
        format.platform_config.size.height =
            param.platform_config.size.second.height;
      }
    }

    if (m_platform->buffer_format_set(format) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to set buffer format!");
      break;
    }
    AMBufferParamMap param_map { std::make_pair(param.id, param) };
    set_buffer_param(param_map);

  } while (0);
  return result;
}

AM_RESULT AMEncodeCanvas::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_buffer_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_buffer_config is null!");
      break;
    }
    if ((result = m_buffer_config->get_config(m_buffer_param)) != AM_RESULT_OK) {
      ERROR("Failed to get buffer config!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeCanvas::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_buffer_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_buffer_config is null!");
      break;
    }
    if ((result = m_buffer_config->set_config(m_buffer_param)) != AM_RESULT_OK) {
      ERROR("Failed to set buffer config!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeCanvas::setup()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformBufferFormatMap buffer_list;
    //vca mode just enable main buffer
    AM_POWER_MODE mode = AM_POWER_MODE_INVALID;
    if (m_platform->get_power_mode(mode) != AM_RESULT_OK) {
      ERROR("Failed to get power mode!");
      result = AM_RESULT_ERR_INVALID;
      break;
    } else if (mode == AM_POWER_MODE_INVALID){
      ERROR("Invalid power mode");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    for (auto &m : m_buffer_param) {
      AMPlatformBufferFormat buffer_format;
      buffer_format.id = m.first;
      if ((mode == AM_POWER_MODE_VCA) && (m.first != AM_SOURCE_BUFFER_MAIN)) {
        buffer_format.type = AM_SOURCE_BUFFER_TYPE_OFF;
      } else if (mode == AM_POWER_MODE_NORMAL) {
        buffer_format.type = m.second.type.second;
      }
      if (m.second.platform_config.input_crop.second) {
        buffer_format.input = m.second.input.second;
      }
      buffer_format.platform_config.size = m.second.platform_config.size.second;
      buffer_format.platform_config.vca_interval =
          m.second.platform_config.vca_interval.second;
      buffer_format.platform_config.vca_duration =
          m.second.platform_config.vca_duration.second;

      buffer_list[m.first] = buffer_format;
    }
    if ((result = m_platform->buffer_setup_set(buffer_list)) == AM_RESULT_OK) {
      for (auto &m : buffer_list) {
        if (m.second.type == AM_SOURCE_BUFFER_TYPE_OFF) {
          PRINTF("Source Buffer[%d]: type: off", m.first);
          continue;
        }
        if (m.second.input.size.width == 0 || m.second.input.size.height == 0) {
          PRINTF("Source Buffer[%d]: type: %s, size(%dx%d)",
                 m.first,
                 AMVideoTrans::buffer_type_to_str(m.second.type).c_str(),
                 m.second.platform_config.size.width,
                 m.second.platform_config.size.height);
        } else {
          PRINTF("Source Buffer[%d]: type: %s, size(%dx%d), "
                 "input_size(%dx%d), input_offset(%dx%d)",
                 m.first, AMVideoTrans::buffer_type_to_str(m.second.type).c_str(),
                 m.second.platform_config.size.width,
                 m.second.platform_config.size.height,
                 m.second.input.size.width, m.second.input.size.height,
                 m.second.input.offset.x,  m.second.input.offset.y);
        }
        if (m.second.type == AM_SOURCE_BUFFER_TYPE_VCA) {
          PRINTF("Source Buffer[%d]: VCA interval: %d, duration: %d",
                 m.first, m.second.platform_config.vca_interval,
                 m.second.platform_config.vca_duration);
        }
      }
    }
  } while(0);

  return result;
}
