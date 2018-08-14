/*******************************************************************************
 * frame_buffer.cpp
 *
 * History:
 *   Nov 30, 2015 - [Huqing Wang] created file
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

#include <mutex>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "am_base_include.h"

#include "frame_buffer.h"

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);
AMFrameBuffer * AMFrameBuffer::m_instance = nullptr;
AMFrameBuffer::AMFrameBuffer() :
    m_buf(nullptr),
    m_fd(-1),
    m_buf_len(0)
{
  memset(&m_vinfo, 0, sizeof(m_vinfo));
  memset(&m_finfo, 0, sizeof(m_finfo));
}

AMFrameBuffer::~AMFrameBuffer()
{
  DEBUG("~AMFrameBuffer");
  close_fb();
}

AMFrameBufferPtr AMFrameBuffer::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMFrameBuffer();
    if (m_instance) {
      if (m_instance->init_fb() != AM_RESULT_OK) {
        delete m_instance;
        m_instance = nullptr;
      }
    }
  }
  return m_instance;
}

void AMFrameBuffer::release()
{
  DECLARE_MUTEX;
  if ((m_ref_counter) > 0 && (-- m_ref_counter == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMFrameBuffer::inc_ref()
{
  ++ m_ref_counter;
}

AM_RESULT AMFrameBuffer::draw_clear()
{
  return blank_fb();
}

void AMFrameBuffer::check_point(point_desc *p)
{
  if (p->x >= m_vinfo.xres) {
    p->x = m_vinfo.xres - 1;
  }

  if (p->y >= m_vinfo.yres) {
    p->y = m_vinfo.yres - 1;
  }
}

AM_RESULT AMFrameBuffer::draw_dot(point_desc *p, uint32_t size, uint32_t color)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!p || !m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    int32_t stride = 1;
    uint32_t location = 0;

    uint8_t *buf = m_buf;
    for (uint32_t n = 0; n < size; ++ n) {
      point_desc start, end;
      start.x = (p[n].x - stride > 0) ? (p[n].x - stride) : 0;
      end.x = p[n].x + stride;
      start.y = (p[n].y - stride > 0) ? (p[n].y - stride) : 0;
      end.y = p[n].y + stride;
      check_point(&start);
      check_point(&end);

      for (uint32_t i = start.x; i <= end.x; ++ i) {
        for (uint32_t j = start.y; j <= end.y; ++ j) {
          location = (j + m_vinfo.yoffset) * m_finfo.line_length
            + (i + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8);
          if (m_vinfo.bits_per_pixel == 8) {
            buf[location] = uint8_t(color);
          } else if (m_vinfo.bits_per_pixel == 16) {
            *((uint16_t *) (buf + location)) = uint16_t(color);
          } else if (m_vinfo.bits_per_pixel == 32) {
            *((uint32_t *) (buf + location)) = color;
          }
        }
      }
    }

    ret = render_frame();
  } while (0);

  return ret;
}

AM_RESULT AMFrameBuffer::draw_line_without_render(point_desc &p1,
                                                  point_desc &p2,
                                                  uint32_t color)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    //    DEBUG("draw line use two point!");
    if (!m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_PERM;
      break;
    }
    uint32_t location = 0;

    uint8_t *buf = m_buf;

    check_point(&p1);
    check_point(&p2);

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
      for (int32_t y = yl; y <= yh; ++ y) {
        location = (y + m_vinfo.yoffset) * m_finfo.line_length
          + (p1.x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8);
        if (m_vinfo.bits_per_pixel == 8) {
          buf[location] = uint8_t(color);
        } else if (m_vinfo.bits_per_pixel == 16) {
          *((uint16_t *) (buf + location)) = uint16_t(color);
        } else if (m_vinfo.bits_per_pixel == 32) {
          *((uint32_t *) (buf + location)) = color;
        }
      }
      break;
    }

    if (p1.y == p2.y) {
      for (int32_t x = xl; x <= xr; ++ x) {
        location = (p1.y + m_vinfo.yoffset) * m_finfo.line_length
          + (x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8);
        if (m_vinfo.bits_per_pixel == 8) {
          buf[location] = uint8_t(color);
        } else if (m_vinfo.bits_per_pixel == 16) {
          *((uint16_t *) (buf + location)) = uint16_t(color);
        } else if (m_vinfo.bits_per_pixel == 32) {
          *((uint32_t *) (buf + location)) = color;
        }
      }
      break;
    }

    dx = 2 * (p2.x - p1.x);
    dy = 2 * (p2.y - p1.y);

    for (int32_t x = xl; x <= xr; ++ x) {
      int32_t y = (p1.y * dx - p1.x * dy + x * dy + dx / 2) / dx;
      location = (y + m_vinfo.yoffset) * m_finfo.line_length
        + (x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8);
      if (m_vinfo.bits_per_pixel == 8) {
        buf[location] = uint8_t(color);
      } else if (m_vinfo.bits_per_pixel == 16) {
        *((uint16_t *) (buf + location)) = uint16_t(color);
      } else if (m_vinfo.bits_per_pixel == 32) {
        *((uint32_t *) (buf + location)) = color;
      }
    }

    for (int32_t y = yl; y <= yh; ++ y) {
      int32_t x = (p1.x * dy - p1.y * dx + y * dx + dy / 2) / dy;
      location = (y + m_vinfo.yoffset) * m_finfo.line_length
        + (x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8);
      if (m_vinfo.bits_per_pixel == 8) {
        buf[location] = uint8_t(color);
      } else if (m_vinfo.bits_per_pixel == 16) {
        *((uint16_t *) (buf + location)) = uint16_t(color);
      } else if (m_vinfo.bits_per_pixel == 32) {
        *((uint32_t *) (buf + location)) = color;
      }
    }
  } while (0);

  return ret;
}

AM_RESULT AMFrameBuffer::draw_line(point_desc p1, point_desc p2, uint32_t color)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((ret = draw_line_without_render(p1, p2, color)) != AM_RESULT_OK) {
      break;
    }
    if ((ret = render_frame()) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMFrameBuffer::draw_polyline(point_desc *p,
                                       uint32_t size,
                                       uint32_t color)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
//    DEBUG("point number = %d", size);
    if (!m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_PERM;
      break;
    }
    if (size <= 1) {
      ERROR("invalid parameter!!!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    point_desc *start = nullptr;
    point_desc *end = nullptr;
    for (uint32_t i = 0; i < size - 1; ++ i) {
      start = &p[i];
      end = &p[i + 1];
      if ((ret = draw_line_without_render(*start, *end, color))
          != AM_RESULT_OK) {
        break;
      }
    }

    ret = render_frame();
  } while (0);

  return ret;
}

/*equation: x = k*y + b,
 *osd->offset.x <= x <= osd->offset.x+osd->size.width or
 *osd->offset.y <= y <= osd->offset.x+osd->size.height*/
AM_RESULT AMFrameBuffer::draw_straight_line(const vout_osd_desc *osd)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!osd || !m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    uint32_t i;
    point_desc start, end;

    for (i = osd->offset[0].y; i < osd->offset[0].y + osd->size.height; ++ i) {
      int32_t x_start = int32_t(osd->k * i + osd->b);
      if ((x_start < int32_t(m_vinfo.xres)) && (x_start >= 0)) {
        start.y = i;
        start.x = uint32_t(x_start);
        break;
      }
    }
    if (i >= (osd->offset[0].y + osd->size.height)) {
      ERROR("no valid start point for the line!!!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }

    for (i = osd->offset[0].y + osd->size.height; i > osd->offset[0].y; -- i) {
      int32_t x_end = int32_t(osd->k * i + osd->b);
      if ((x_end < int32_t(m_vinfo.xres)) && (x_end >= 0)) {
        end.y = i;
        end.x = uint32_t(x_end);
        break;
      }
    }
    if (i <= osd->offset[0].y) {
      ERROR("no valid end point for the line!!!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
//    DEBUG("start.x=%d, end.x=%d, start.y=%d, end.y=%d\n",
//           start.x, end.x, start.y, end.y);
    if ((ret = draw_line_without_render(start, end, osd->color))
        != AM_RESULT_OK) {
      break;
    }

    ret = render_frame();
  } while (0);

  return ret;
}

/*equation: x = k * y2 + b,
 *osd->offset.x <= x <= osd->offset.x+osd->size.width,
 *osd->offset.y <= y <= osd->offset.x+osd->size.height*/
AM_RESULT AMFrameBuffer::draw_curve(const vout_osd_desc *osd)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!osd || !m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    uint32_t i;
    uint32_t stride = 1;
    point_desc start, end;

    for (i = osd->offset[0].y; i < osd->offset[0].y + osd->size.height;) {
      int32_t x_start = int32_t(osd->k * i * i + osd->b);
      if ((x_start < int32_t(m_vinfo.xres)) && (x_start >= 0)) {
        start.y = i;
        start.x = uint32_t(x_start);
      } else {
        ++ i;
        continue;
      }

      i += stride;
      int32_t x_end = int32_t(osd->k * i * i + osd->b);
      if (x_end < 0) {
        break;
      } else if ((x_end < int32_t(m_vinfo.xres)) && (x_end >= 0)) {
        end.y = i;
        end.x = uint32_t(x_end);
      } else {
        continue;
      }
      if ((ret = draw_line_without_render(start, end, osd->color))
          != AM_RESULT_OK) {
        break;
      }
    }
    ret = render_frame();
  } while (0);

  return ret;
}

AM_RESULT AMFrameBuffer::draw_rectangle(point_desc p1,
                                             point_desc p2,
                                             uint32_t color)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!m_buf) {
      ERROR("NULL pointer!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }

    point_desc new_p1, new_p2, new_p3, new_p4;
    //new_p1 and new_p4 is the real start point and end point
    new_p1.x = (p2.x >= p1.x) ? p1.x : p2.x;
    new_p4.x = (new_p1.x == p1.x) ? p2.x : p1.x;
    new_p1.y = (p2.y >= p1.y) ? p1.y : p2.y;
    new_p4.y = (new_p1.y == p1.y) ? p2.y : p1.y;

    new_p2.x = new_p4.x;
    new_p2.y = new_p1.y;
    //draw the top hor line
    if ((ret = draw_line_without_render(new_p1, new_p2, color))
        != AM_RESULT_OK) {
      break;
    }
    new_p3.x = new_p1.x;
    new_p3.y = new_p4.y;
    //draw the left ver line
    if ((ret = draw_line_without_render(new_p1, new_p3, color))
        != AM_RESULT_OK) {
      break;
    }
    //draw the bottom hor line
    if ((ret = draw_line_without_render(new_p3, new_p4, color))
        != AM_RESULT_OK) {
      break;
    }
    //draw the right ver line
    if ((ret = draw_line_without_render(new_p2, new_p4, color))
        != AM_RESULT_OK) {
      break;
    }

    ret = render_frame();
  } while (0);

  return ret;

}

AM_RESULT AMFrameBuffer::load_bmp(const std::string &filename)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {

    uint32_t img_w, img_h, img_pitch, screensize;
    int16_t bit_count;
    FILE *file = nullptr;
    void *buffer = nullptr;
    //fb_var_screeninfo vinfo;
    //fb_fix_screeninfo finfo;

    if (!(file = fopen(filename.c_str(), "r"))) {
      ERROR("Open file error");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    buffer = init_bmp_info(file, img_w, img_h, img_pitch, bit_count);

    if (!m_fd) {
      ERROR("Frame buffer cann't open!");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    screensize = m_vinfo.yres * m_finfo.line_length;

    if (!(m_buf = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE,
                              MAP_SHARED, m_fd, 0))) {
      perror("Faild in mmap");
    }

    uint32_t x, y, x_start, y_start, location;
    x_start = 0;
    y_start = 0;
    for (y = y_start; y < (y_start + img_h); y++) {
      for (x = x_start; x <(x_start + img_w); x++) {
        location = (x + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel / 8) +
            (y + m_vinfo.yoffset) * m_finfo.line_length;
        if (m_vinfo.bits_per_pixel == 8) {
          WARN("Temporary un-available");
        }
        if (m_vinfo.bits_per_pixel == 16) {
          //*((unsigned short *)(fbp + location )) = buffer[w*y +x];
          uint16_t color_bit;
          switch (bit_count) {
            case BMP_BIT_8:
              break;
            case BMP_BIT_16:
              *((unsigned short int *)(m_buf + location)) = ((unsigned short int *)buffer)[img_w*y +x];
              break;
            case BMP_BIT_24: {

              BmpFormatRGB *img_trans = (struct BmpFormatRGB *) buffer;
              color_bit = ((unsigned short int)(img_trans[img_w*y + x].r & 0xf8)) << 8 |
                  ((unsigned short int)(img_trans[img_w*y + x].g & 0xfc)) << 3 |
                  ((unsigned short int)(img_trans[img_w*y + x].b & 0xf8)) >> 3;
              *((unsigned short int *)(m_buf + location)) = color_bit;
              break;
            }
            default:
              ERROR("Unknow bmp color bit type: %d", bit_count);
              break;
          }
        }
      }
    }
    sleep(2);
    ioctl(m_fd, FBIOPAN_DISPLAY, &m_vinfo);
    ioctl(m_fd, FBIOBLANK, FB_BLANK_NORMAL);
    free(buffer);

  } while(0);
  return ret;
}

AM_RESULT AMFrameBuffer::init_fb()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    m_fd = open("/dev/fb0", O_RDWR);
    if (m_fd < 0) {
      ret = AM_RESULT_ERR_IO;
      break;
    }

    if (ioctl(m_fd, FBIOGET_FSCREENINFO, &m_finfo) < 0) {
      perror("FBIOGET_FSCREENINFO");
      ret = AM_RESULT_ERR_IO;
      break;
    }

    if (ioctl(m_fd, FBIOGET_VSCREENINFO, &m_vinfo) < 0) {
      perror("FBIOGET_VSCREENINFO");
      ret = AM_RESULT_ERR_IO;
      break;
    }
#ifdef DELETE_OVERLAY
    //change to 8bit color mode
    m_vinfo.bits_per_pixel = 8;
    memset(&m_vinfo.red, 0, sizeof(m_vinfo.red));
    memset(&m_vinfo.green, 0, sizeof(m_vinfo.green));
    memset(&m_vinfo.blue, 0, sizeof(m_vinfo.blue));
    memset(&m_vinfo.transp, 0, sizeof(m_vinfo.transp));
    if (ioctl(m_fd, FBIOPUT_VSCREENINFO, &m_vinfo) < 0) {
      perror("FBIOPUT_VSCREENINFO");
      ret = AM_RESULT_ERR_IO;
      break;
    }

    uint16_t *buf = new uint16_t[256 * 4];
    if (!buf) {
      ret = AM_RESULT_ERR_MEM;
      break;
    }
    memset(buf, 0, 256 * 4);
    uint16_t *r = buf;
    uint16_t *g = buf + 256;
    uint16_t *b = buf + 512;
    uint16_t *a = buf + 768;

    r[FB_COLOR_TRANSPARENT] = 0;
    g[FB_COLOR_TRANSPARENT] = 0;
    b[FB_COLOR_TRANSPARENT] = 0;
    a[FB_COLOR_TRANSPARENT] = 0;

    r[FB_COLOR_SEMI_TRANSPARENT] = 0;
    g[FB_COLOR_SEMI_TRANSPARENT] = 0;
    b[FB_COLOR_SEMI_TRANSPARENT] = 0;
    a[FB_COLOR_SEMI_TRANSPARENT] = 128;

    r[FB_COLOR_RED] = 255;
    g[FB_COLOR_RED] = 0;
    b[FB_COLOR_RED] = 0;
    a[FB_COLOR_RED] = 255;

    r[FB_COLOR_GREEN] = 0;
    g[FB_COLOR_GREEN] = 255;
    b[FB_COLOR_GREEN] = 0;
    a[FB_COLOR_GREEN] = 255;

    r[FB_COLOR_BLUE] = 0;
    g[FB_COLOR_BLUE] = 0;
    b[FB_COLOR_BLUE] = 255;
    a[FB_COLOR_BLUE] = 255;

    r[FB_COLOR_YELLOW] = 255;
    g[FB_COLOR_YELLOW] = 255;
    b[FB_COLOR_YELLOW] = 0;
    a[FB_COLOR_YELLOW] = 255;

    r[FB_COLOR_WHITE] = 255;
    g[FB_COLOR_WHITE] = 255;
    b[FB_COLOR_WHITE] = 255;
    a[FB_COLOR_WHITE] = 255;

    r[FB_COLOR_BLACK] = 0;
    g[FB_COLOR_BLACK] = 0;
    b[FB_COLOR_BLACK] = 0;
    a[FB_COLOR_BLACK] = 255;

    fb_cmap cmap = { 0 };
    cmap.start = 0;
    cmap.len = 256;
    cmap.red = r;
    cmap.green = g;
    cmap.blue = b;
    cmap.transp = a;

    if (ioctl(m_fd, FBIOPUTCMAP, &cmap) < 0) {
      ret = AM_RESULT_ERR_IO;
      perror("FBIOPUTCMAP");
      delete[] buf;
      break;
    }
    delete[] buf;

    m_buf_len = m_finfo.smem_len; //m_vinfo.yres * m_finfo.line_length;
    DEBUG("mmap length = %d", m_buf_len);
    m_buf = (uint8_t *) mmap(0, m_buf_len,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, m_fd, 0);
    if (!m_buf) {
      perror("mmap");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if ((ret=blank_fb()) != AM_RESULT_OK) {
      break;
    }
#endif
  } while (0);

  if ((ret != AM_RESULT_OK) && (m_fd > 0)) {
    close(m_fd);
  }
  return ret;
}

AM_RESULT AMFrameBuffer::render_frame()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_fd < 0) {
      ERROR("frame buffer handle is null");
      ret = AM_RESULT_ERR_PERM;
      break;
    }

    if (ioctl(m_fd, FBIOPAN_DISPLAY, &m_vinfo) < 0) {
      ret = AM_RESULT_ERR_IO;
      perror("FBIOPAN_DISPLAY");
      break;
    }
  } while (0);

  return ret;
}

AM_RESULT AMFrameBuffer::blank_fb()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_fd < 0) {
      ERROR("frame buffer handle is null");
      ret = AM_RESULT_ERR_PERM;
      break;
    }

    if (ioctl(m_fd, FBIOBLANK, FB_BLANK_NORMAL) < 0) {
      ret = AM_RESULT_ERR_IO;
      perror("FBIOBLANK");
      break;
    }

  } while (0);

  return ret;
}

void AMFrameBuffer::close_fb()
{
  if (m_fd > 0) {
    close(m_fd);
    m_fd = -1;
  }
  if (m_buf) {
    munmap(m_buf, m_buf_len);
    m_buf = nullptr;
  }
}

void* AMFrameBuffer::obtain_image_pixels(FILE *file, uint32_t &w, uint32_t &h, uint32_t &pitch,
                                           void *buf, int16_t type)
{
  uint32_t i = 0;
  void *tmp_buf = NULL;
  void *out_buf = NULL;
  switch(type) {
    case BMP_BIT_16:
      tmp_buf = malloc(pitch*h);
      out_buf = malloc(w*h*sizeof(unsigned short int));
      memcpy(tmp_buf, buf, pitch*h);
      for (; i < h; ++ i) {
        memcpy((char*)out_buf + (i * w *2), (char*)tmp_buf + ((h - 1) - i) * (pitch), w*2);
      }
      break;
    case BMP_BIT_24:
      tmp_buf = malloc(pitch*h);
      out_buf = malloc(w*h*sizeof(struct BmpFormatRGB));
      memcpy(tmp_buf, buf, pitch*h);
      for (; i < h; ++ i) {
        memcpy((char*)out_buf + i * w*3, (char*)tmp_buf + ((h - 1) - i) * (pitch), w*3);
      }

      break;
    default:
      break;
  }

  free(tmp_buf);
  free(buf);
  return out_buf;
}

void* AMFrameBuffer::init_bmp_info(FILE *file, uint32_t &width, uint32_t &height,
                                       uint32_t &bmp_pitch, int16_t &bit_count)
{
  void *out_buf = nullptr;
  do {
    if (nullptr == file) {
      ERROR("bmp file is not loaded!");
      break;
    }
    void *img_buf = NULL;
    BmpFileHeader fileHeader = { 0 };
    BmpInfoHeader infoHeader = { 0 };
    fread(&fileHeader, sizeof(fileHeader), 1, file);
    fread(&infoHeader, sizeof(struct BmpInfoHeader), 1, file);
    bit_count = infoHeader.biBitCount;
    if (infoHeader.biBitCount != BMP_BIT_8 &&
        infoHeader.biBitCount != BMP_BIT_16 &&
        infoHeader.biBitCount != BMP_BIT_24) {
      ERROR("invalid [%u]bit. Only support 8|16|24 bit bitmap.", infoHeader.biBitCount);
      break;
    }
    if (infoHeader.biSizeImage != (infoHeader.biWidth * infoHeader.biHeight)*bit_count/8) {
      WARN("Invalid image size [%u]. Not equal to width[%u] X height[%u].\n",
             infoHeader.biSizeImage,
             infoHeader.biWidth,
             infoHeader.biHeight);
    }
    bmp_pitch = ((infoHeader.biWidth * infoHeader.biBitCount + 31) >> 5) << 2;
    uint32_t offbit = fileHeader.bfOffBits[3] << 24
        | fileHeader.bfOffBits[2] << 16 | fileHeader.bfOffBits[1] << 8
        | fileHeader.bfOffBits[0];

    width = infoHeader.biWidth;
    height = infoHeader.biHeight;
    switch (bit_count) {
      case BMP_BIT_8: {
        break;
      }
      case BMP_BIT_16: {
        img_buf = malloc(bmp_pitch*height);
        fseek(file, offbit-sizeof(struct BmpInfoHeader)-sizeof(struct BmpFileHeader), SEEK_CUR);
        fread(img_buf, bmp_pitch*height, 1, file);
        out_buf = obtain_image_pixels(file, width, height, bmp_pitch, img_buf, BMP_BIT_16);
        break;
      }
      case BMP_BIT_24: {
        img_buf = malloc(bmp_pitch*height);
        fseek(file, offbit-sizeof(struct BmpInfoHeader)-sizeof(struct BmpFileHeader), SEEK_CUR);
        fread(img_buf, bmp_pitch*height, 1, file);
        out_buf = obtain_image_pixels(file, width, height, bmp_pitch, img_buf, BMP_BIT_24);
        break;
      }
      default:
        break;
    }
  } while (0);
  return out_buf;
}
