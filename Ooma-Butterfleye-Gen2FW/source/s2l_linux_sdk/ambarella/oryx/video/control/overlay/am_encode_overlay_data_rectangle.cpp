/*******************************************************************************
 * am_encode_overlay_data_rectangle.cpp
 *
 * History:
 *   May 15, 2017 - [hqwang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_encode_overlay_area.h"
#include "am_encode_overlay_data_rectangle.h"

static const AMOverlayCLUT color[AM_OVERLAY_COLOR_NUM] =
{                         { 128, 128, 235, 255 },  /*white*/
                          { 128, 128, 12, 255 },   /*black*/
                          { 240, 90, 82, 255 },    /*red*/
                          { 110, 240, 41, 255 },   /*blue*/
                          { 34, 54, 145, 255 },    /*green*/
                          { 146, 16, 210, 255 },   /*yellow*/
                          { 16, 166, 170, 255 },   /*cyan*/
                          { 222, 202, 107, 255 },  /*magenta*/
};

AMOverlayData* AMOverlayRectData::create(AMOverlayArea *area,
                                         AMOverlayAreaData *data)
{
  AMOverlayRectData *result = new AMOverlayRectData(area);
  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayRectData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayRectData::AMOverlayRectData(AMOverlayArea *area) :
    AMOverlayLineData(area)
{
}

AMOverlayRectData::~AMOverlayRectData()
{
}

AM_RESULT AMOverlayRectData::add(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    index_set_map tmp;
    uint8_t hollow_index = 0;
    uint8_t solid_index = 0;
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }

    AMRect &rect = data->rect;
    if ((result=check_block_rect_param(rect.size.width, rect.size.height,
                                       rect.offset.x, rect.offset.y))
        != AM_RESULT_OK) {
      break;
    }
    if (!check_hollow_rtg_parameter(data)) {
      ERROR("Check hollow rtg parameter error.");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!check_solid_rtg_parameter(data)) {
      ERROR("Check solid rtg parameter error.");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    int32_t buf_size = rect.size.width * rect.size.height;
    if (!m_buffer) {
      m_buffer = (uint8_t *) new uint8_t[buf_size];
      if (!m_buffer) {
        ERROR("Can't allot memory[%u].", buf_size);
        break;
      }
    }
    memset(m_buffer, CLUT_ENTRY_BACKGROUND, buf_size);
    AMOverlayRectangle &rtg = data->rtg;
    if (rtg.h_thickness <= 0) {
      rtg.h_thickness = 1;
    }
    if (rtg.h_color.id != AM_OVERLAY_COLOR_CUSTOM) {
      memcpy(&(rtg.h_color.color),
             &color[rtg.h_color.id], sizeof(AMOverlayCLUT));
    }
    if (!rtg.h_rect.empty()) {
      if ((result = adjust_clut_and_data(rtg.h_color.color, &hollow_index,
                                         1, 0, tmp))
          != AM_RESULT_OK) {
        break;
      }
    }

    for (auto &m : rtg.h_rect) {
      if ((result = draw_hollow_rect(m, hollow_index, rtg.h_thickness, rect.size))
          != AM_RESULT_OK) {
        break;
      }
    }
    if (result != AM_RESULT_OK) {
      break;
    }

    if (rtg.s_thickness <= 0) {
      rtg.s_thickness = 1;
    }
    if (rtg.s_color.id != AM_OVERLAY_COLOR_CUSTOM) {
      memcpy(&(rtg.s_color.color),
             &color[rtg.s_color.id], sizeof(AMOverlayCLUT));
    }
    if (!rtg.s_rect.empty()) {
      if ((result = adjust_clut_and_data(rtg.s_color.color, &solid_index,
                                         1, 0, tmp))
          != AM_RESULT_OK) {
        break;
      }
    }
    for (auto &m : rtg.s_rect) {
      if ((result = draw_solid_rect(m, solid_index, rtg.s_thickness, rect.size))
          != AM_RESULT_OK) {
        break;
      }
    }
    if (result != AM_RESULT_OK) {
      break;
    }
    AMRect data_rect;
    data_rect.size = data->rect.size;
    data_rect.offset = {0};
    if ((result = m_area->update_drv_data(m_buffer, data->rect.size, data_rect,
                                          data->rect.offset, true))
        != AM_RESULT_OK) {
      break;
    }

    m_param = *data;
  } while (0);

  return result;
}

AM_RESULT AMOverlayRectData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (memcmp(&data->rtg, &m_param.rtg, sizeof(data->rtg)) == 0) {
      break;
    }
    if (data->type != m_param.type) {
      DEBUG("Don't change data type with update");
      data->type = m_param.type;
    }
    if (memcmp(&(data->rect), &(m_param.rect), sizeof(AMRect)) != 0) {
      DEBUG("Don't change area data block size and offset with update!\n");
      data->rect = m_param.rect;
    }

    if ((result = add(data)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayRectData::blank()
{
  return AMOverlayLineData::blank();
}


AM_RESULT AMOverlayRectData::draw_hollow_rect(AMPointPair &pp, uint8_t color,
                                              int32_t tn, AMResolution &size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
      /*
       * p1----p2
       *  \    \
       *  \    \
       * p4----p3
       */
      AMPoint p1, p2, p3, p4;
      p1 = pp.p1;
      p3 = pp.p2;
      p2.x = p3.x;
      p2.y = p1.y;
      p4.x = p1.x;
      p4.y = p3.y;

      if ((result = check_point_param(p1, size)) != AM_RESULT_OK ||
          (result = check_point_param(p2, size)) != AM_RESULT_OK ||
          (result = check_point_param(p3, size)) != AM_RESULT_OK ||
          (result = check_point_param(p4, size)) != AM_RESULT_OK) {
        break;
      }
      draw_straight_line(p1, p2, color, tn, size);
      draw_straight_line(p2, p3, color, tn, size);
      draw_straight_line(p3, p4, color, tn, size);
      draw_straight_line(p4, p1, color, tn, size);
  } while (0);
  return result;
}

AM_RESULT AMOverlayRectData::draw_solid_rect(AMPoint &p, uint8_t color,
                                             int32_t tn, AMResolution &size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
      /*
       *     2*tn
       * p1--------
       *  \        \
       *  \    p   \ 2*tn
       *  \        \
       *   --------p2
       */
    AMPoint p1, p2;
    p1.x = p.x - tn;
    p1.y = p.y - tn;
    p2.x = p.x + tn;
    p2.y = p.y + tn;
    if ((result = check_point_param(p1, size)) != AM_RESULT_OK ||
        (result = check_point_param(p2, size)) != AM_RESULT_OK) {
      break;
    }
    int32_t len = 2 * tn;
    uint32_t start = 0;
    for (int32_t h = 0; h < len; ++h) {
      start = (p1.y + h) * size.width + p1.x;
      memset(m_buffer+start, color, len);
    }
  } while (0);
  return result;
}

void AMOverlayRectData::draw_straight_line(AMPoint &p1, AMPoint &p2,
                                           uint8_t color, int32_t thickness,
                                           const AMResolution &size)
{
  do {
    uint32_t location = 0;
    int32_t xl, xr, yl, yh;
    int32_t dx, dy;

    if (p1.x > p2.x) {
      xl = p2.x;
      xr = p1.x;
    } else {
      xl = p1.x;
      xr = p2.x;
    }
    if (p1.y > p2.y) {
      yl = p2.y;
      yh = p1.y;
    } else {
      yl = p1.y;
      yh = p2.y;
    }

    if (p1.x == p2.x) {
      for (int32_t tn = 0; tn < thickness; ++ tn) {
        if (p1.x + tn >= size.width) {
          break;
        }
        for (int32_t y = yl; y < yh + thickness; ++ y) {
          location = y * size.width + p1.x + tn;
          m_buffer[location] = color;
        }
      }
      break;
    }

    if (p1.y == p2.y) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        if (p1.y + tn >= size.height) {
          break;
        }
        for (int32_t x = xl; x <= xr; ++ x) {
          location = (p1.y + tn) * size.width + x;
          m_buffer[location] = color;
        }
      }
      break;
    }

    dx = 2 * (p2.x - p1.x);
    dy = 2 * (p2.y - p1.y);
    for (int32_t x = xl; x <= xr; ++ x) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        int32_t y = (p1.y * dx - p1.x * dy + x * dy + dx / 2) / dx + tn;
        if (y >= size.height) {
          break;
        }
        location = y * size.width + x;
        m_buffer[location] = color;
      }
    }

    for (int32_t y = yl; y <= yh; ++ y) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        int32_t x = (p1.x * dy - p1.y * dx + y * dx + dy / 2) / dy + tn;
        if (x >= size.width) {
          break;
        }
        location = y * size.width + x;
        m_buffer[location] = color;
      }
    }
  } while (0);
}

bool AMOverlayRectData::check_hollow_rtg_parameter(AMOverlayAreaData *data)
{
  bool ret = true;
  do {
    int32_t thickness = data->rtg.h_thickness;
    AMRect &rect = data->rect;
    if (data->rtg.h_rect.size() > 0) {
      for (auto &m : data->rtg.h_rect) {
        if ((m.p1.x + thickness) > rect.size.width) {
          ERROR("p1.x(%d) + thickness(%d) > width(%d)",
                m.p1.x, thickness, rect.size.width);
          ret = false;
          break;
        }
        if ((m.p1.y + thickness) > rect.size.height) {
          ERROR("p1.y(%d) + thickness(%d) > height(%d)",
                m.p1.y, thickness, rect.size.height);
          ret = false;
          break;
        }
        if ((m.p2.x + thickness) > rect.size.width) {
          ERROR("p2.x(%d) + thickness(%d) > width(%d)",
                m.p2.x, thickness, rect.size.width);
          ret = false;
          break;
        }
        if ((m.p2.y + thickness) > rect.size.height) {
          ERROR("p2.y(%d) + thickness(%d) > height(%d)",
                m.p2.y, thickness, rect.size.height);
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMOverlayRectData::check_solid_rtg_parameter(AMOverlayAreaData *data)
{
  bool ret = true;
  do {
    int32_t thickness = data->rtg.s_thickness;
    AMRect &rect = data->rect;
    if (data->rtg.s_rect.size() > 0) {
      for (auto &m : data->rtg.s_rect) {
        if ((m.x + thickness) > rect.size.width) {
          ERROR("p.x(%d) + thickness(%d) > width(%d)",
                m.x, thickness, rect.size.width);
          ret = false;
          break;
        }
        if ((m.x - thickness) < 0) {
          ERROR("p.x(%d) - thickness(%d) < 0", m.x, thickness);
          ret = false;
          break;
        }
        if ((m.y + thickness) > rect.size.height) {
          ERROR("p.y(%d) + thickness(%d) > height(%d)",
                m.y, thickness, rect.size.height);
          ret = false;
          break;
        }
        if ((m.y - thickness) < 0) {
          ERROR("p.y(%d) - thickness(%d) < 0", m.y, thickness);
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
  } while(0);
  return ret;
}

