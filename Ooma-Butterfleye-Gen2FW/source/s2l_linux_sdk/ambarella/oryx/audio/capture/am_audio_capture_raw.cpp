/*******************************************************************************
 * am_audio_capture_raw.cpp
 *
 * History:
 *   Dec 7, 2016 - [ypchang] created file
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

#include "am_event.h"
#include "am_io.h"

#include "am_audio_capture_raw.h"
#include "am_audio_utility.h"

#include "iav_ioctl.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <math.h>

#define HIBERNATION_FILE  ((const char*)"/tmp/hibernation")
#define HW_TIMER          ((const char*)"/proc/ambarella/ambarella_hwtimer")
#define DATA_PLACE_HOLDER 0xdeadbeaf
#define PTS_DIFF_DELTA    5 /* 5ms */
#define MAX_WAIT_COUNT    3
#define GAP_ON_BOUNDARY   0

struct AudioMemMapInfo
{
    uint8_t *addr   = nullptr;
    uint32_t length = 0;
};

AMIAudioCapture* AMAudioCaptureRaw::create(void *owner,
                                           const std::string& name,
                                           AudioCaptureCallback callback)
{
  AMAudioCaptureRaw *result = new AMAudioCaptureRaw();
  if (AM_UNLIKELY(result && (!result->init(owner, name, callback)))) {
    delete result;
    result = nullptr;
  }

  return ((AMIAudioCapture*)result);
}

bool AMAudioCaptureRaw::set_capture_callback(AudioCaptureCallback callback)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio capture callback of %s is set to %p",
       m_context_name.c_str(), callback);
  m_capture_callback = callback;
  return (nullptr != m_capture_callback);
}

void AMAudioCaptureRaw::destroy()
{
  delete this;
}

void AMAudioCaptureRaw::set_echo_cancel_enabled(bool UNUSED(enabled))
{
  WARN("Not supported in RAW audio capture!");
}

bool AMAudioCaptureRaw::start(int32_t UNUSED(volume))
{
  m_wait = false;
  if (AM_LIKELY(!m_started)) {
    m_wait_event->signal();
  }

  return true;
}

bool AMAudioCaptureRaw::stop()
{
  m_wait = true;
  return true;
}

bool AMAudioCaptureRaw::set_channel(uint32_t channel)
{
  bool ret = true;
  switch(channel) {
    case 2:
    case 1: {
      m_sample_spec.channels = channel;
    }break;
    default:
      ERROR("Audio RAW capture doesn't support such channel config: %u",
            channel);
      ret = false;
      break;
  }

  return ret;
}

bool AMAudioCaptureRaw::set_sample_rate(uint32_t sample_rate)
{
  bool ret = true;
  switch(sample_rate) {
    case 48000:
    case 16000:
    case  8000: m_sample_spec.rate = sample_rate; break;
    default: {
      ERROR("Audio RAW capture doesn't support such sample rate config: %u",
            sample_rate);
      ret = false;
    }break;
  }
  if (AM_UNLIKELY(m_def_sample_spec.rate != m_sample_spec.rate)) {
    ERROR("RAW Audio device is configured to sample rate %u, "
          "Current setting is %u!",
          m_def_sample_spec.rate,
          m_sample_spec.rate);
    ret = false;
  }

  return ret;
}

bool AMAudioCaptureRaw::set_chunk_bytes(uint32_t chunk_bytes)
{
  AUTO_MEM_LOCK(m_lock);
  m_chunk_bytes = chunk_bytes;
  INFO("Audio chunk bytes of %s is set to %u.",
       m_context_name.c_str(), m_chunk_bytes);

  return (m_chunk_bytes > 0);
}

bool AMAudioCaptureRaw::set_sample_format(AM_AUDIO_SAMPLE_FORMAT format)
{
  if (AM_UNLIKELY((format != AM_SAMPLE_S16LE) && (format != AM_SAMPLE_S32LE))) {
    ERROR("Audio RAW capture only supports S16LE / S32LE sample format!");
  } else {
    m_sample_spec.format = format;
    switch(m_sample_spec.format) {
      case AM_SAMPLE_S16LE: {
        m_sample_spec.sample_size = sizeof(int16_t);
      }break;
      case AM_SAMPLE_S32LE: {
        m_sample_spec.sample_size = sizeof(int32_t);
      }break;
      default: break;
    }
  }

  return ((format == AM_SAMPLE_S16LE) || (format == AM_SAMPLE_S32LE));
}

bool AMAudioCaptureRaw::set_volume(uint32_t volume)
{
  WARN("Not supported in RAW audio capture!");
  return false;
}

uint32_t AMAudioCaptureRaw::get_channel()
{
  return m_sample_spec.channels;
}

uint32_t AMAudioCaptureRaw::get_sample_rate()
{
  return m_sample_spec.rate;
}

uint32_t AMAudioCaptureRaw::get_chunk_bytes()
{
  AUTO_MEM_LOCK(m_lock);
  return m_chunk_bytes;
}

uint32_t AMAudioCaptureRaw::get_sample_size()
{
  return m_sample_spec.sample_size;
}

uint32_t AMAudioCaptureRaw::get_sample_physical_size()
{
  return m_sample_spec.sample_size;
}

int64_t  AMAudioCaptureRaw::get_chunk_pts()
{
  AUTO_MEM_LOCK(m_lock);
  m_fragment_pts = ((int64_t)(90000LL * m_chunk_bytes) /
                             (m_sample_spec.rate *
                              m_sample_spec.sample_size *
                              m_sample_spec.channels));

  return m_fragment_pts;
}

AM_AUDIO_SAMPLE_FORMAT AMAudioCaptureRaw::get_sample_format()
{
  return AM_SAMPLE_S16LE;
}

bool AMAudioCaptureRaw::map_audio_buf()
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(m_mem_info)) {
      INFO("Audio memory is already mapped!");
      ret = true;
      break;
    }

    m_mem_info = new AudioMemMapInfo();
    if (AM_UNLIKELY(!m_mem_info)) {
      ERROR("Failed to allocate memory for audio memory info!");
      break;
    }

    iav_querybuf audio_buf = {IAV_BUFFER_FB_AUDIO, 0, 0};
    if (AM_UNLIKELY(ioctl(m_iav_fd, IAV_IOC_QUERY_BUF, &audio_buf) < 0)) {
      PERROR("IAV_IOC_QUERY_BUF");
      break;
    }
    m_mem_info->length = audio_buf.length;
    m_mem_info->addr = (uint8_t*)mmap(nullptr,
                                      audio_buf.length,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      m_iav_fd,
                                      audio_buf.offset);
    if (AM_UNLIKELY(m_mem_info->addr == MAP_FAILED)) {
      PERROR("mmap");
      delete m_mem_info;
      m_mem_info = nullptr;
      break;
    }

    ret = true;
  }while(0);

  return ret;
}

bool AMAudioCaptureRaw::unmap_audio_buf()
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(!m_mem_info)) {
      NOTICE("Audio memory is NOT mapped!");
      ret = true;
      break;
    }

    if (AM_UNLIKELY(munmap(m_mem_info->addr, m_mem_info->length) < 0)) {
      PERROR("munmap");
      break;
    }
    delete m_mem_info;
    m_mem_info = nullptr;
    ret = true;
  }while(0);

  return ret;
}

void AMAudioCaptureRaw::raw_audio_capture()
{
  volatile uint8_t  *audio_ptr_s = nullptr;
  volatile uint32_t *audio_ptr_r = nullptr;
  uint8_t             *audio_buf = nullptr;
  uint8_t            *data_ptr_w = nullptr;
  uint64_t       sent_packet_num = 0ULL;
  uint64_t      total_read_bytes = 0ULL;
  uint32_t       last_read_bytes = 0;
  uint32_t            read_bytes = 0;
  uint32_t        audio_blk_size = 0;
  uint32_t        audio_read_len = 0;
  uint32_t            wait_count = 0;
  uint32_t             wait_time = 0;
  uint32_t            frame_size = 0;
  bool              need_to_skip = false;
#if GAP_ON_BOUNDARY
  bool            found_real_gap = false;
#endif

  do {
    m_run = true;
    m_wait = true;
    if (AM_UNLIKELY((m_iav_fd < 0) &&
                    ((m_iav_fd = open("/dev/iav", O_RDWR)) < 0))) {
      ERROR("Failed to open IAV device: %s", strerror(errno));
      m_run = false;
      break;
    }
    if (AM_UNLIKELY(!map_audio_buf())) {
      ERROR("Failed to map audio buffer!");
      m_run = false;
      break;
    }
  }while(0);

  m_init_done = true;
  while (m_run && m_wait) {
    m_wait_event->wait();
  }

  m_wait = false;
  m_started = true;

  m_fragment_pts = get_chunk_pts();
  m_wait_interval = (1000 * m_chunk_bytes) /
                    (m_sample_spec.rate *
                     m_sample_spec.sample_size *
                     m_sample_spec.channels);

  if (AM_LIKELY(m_run)) {
    audio_read_len = m_mem_info->length;
    audio_ptr_s = ((uint8_t*)m_mem_info->addr);
    audio_ptr_r = ((uint32_t*)(audio_ptr_s + audio_blk_size));
    audio_buf = new uint8_t[m_chunk_bytes];
    data_ptr_w = audio_buf;
    NOTICE("Audio start address:    %p", audio_ptr_s);
    NOTICE("Audio data buffer size: %u", audio_read_len);
    if (AM_UNLIKELY(!audio_buf)) {
      ERROR("Failed to allocate audio buffer!");
      m_run = false;
    }
  }

  m_need_convert = (m_def_sample_spec.format != m_sample_spec.format);
  if (AM_LIKELY(m_need_convert)) {
    NOTICE("RAW Audio Capture converts %s --> %s!",
           smp_fmt_to_str(m_def_sample_spec.format),
           smp_fmt_to_str(m_sample_spec.format));
  }
  frame_size = m_sample_spec.sample_size * m_sample_spec.channels;
  while (m_run) {
#if GAP_ON_BOUNDARY
    bool is_buf_boundary = ((((uintptr_t)audio_ptr_r -
                             (uintptr_t)audio_ptr_s) %
                             (audio_read_len / RAW_AUDIO_BLOCK_NUM)) == 0);
#endif
    if (AM_LIKELY(*audio_ptr_r != DATA_PLACE_HOLDER)) {
      wait_count = 0;
      wait_time  = 0;
      if (AM_LIKELY(data_ptr_w)) {
        switch(m_sample_spec.sample_size) {
          case 2: { /* S16LE */
            if (AM_LIKELY(m_need_convert)) {
              /* S32LE -> S16LE */
              switch(m_sample_spec.channels) {
                case 1: {
                  int32_t left = audio_ptr_r[0];
                  *((int16_t*)data_ptr_w) = (int16_t)lrintf(
                      AM_MAX(-32768, \
                             AM_MIN((left / 2147483648.0f) * 32768.0f,
                                    32767)));
                }break;
                case 2: {
                  int32_t src_left  = audio_ptr_r[0];
                  int32_t src_right = audio_ptr_r[1];
                  int16_t left = (int16_t)lrintf(
                      AM_MAX(-32768, \
                             AM_MIN((src_left / 2147483648.0f) * 32768.0f,
                                    32767)));
                  int16_t right = (int16_t)lrintf(
                      AM_MAX(-32768, \
                             AM_MIN((src_right / 2147483648.0f) * 32768.0f,
                                    32767)));
                  *((int32_t*)data_ptr_w) = ((right << 16) | left);
                }break;
              }
            } else {
              uint32_t read_data = *audio_ptr_r;
              switch(m_sample_spec.channels) {
                case 1: {
                  *((uint16_t*)data_ptr_w) = (uint16_t)(read_data & 0x0000ffff);
                }break;
                case 2: {
                  *((uint32_t*)data_ptr_w) = read_data;
                }break;
              }
            }
          }break;
          case 4: { /* S32LE */
            if (AM_LIKELY(m_need_convert)) {
              /* S16LE -> S32LE */
              switch(m_sample_spec.channels) {
                case 1: {
                  int16_t left = (int16_t)(audio_ptr_r[0] & 0x0000ffff);
                  *((int32_t*)data_ptr_w) = (int32_t)lrintf(
                      AM_MAX(-2147483648, \
                             AM_MIN((left / 32768.0f) * 2147483648.0f,
                                    2147483647)));
                }break;
                case 2: {
                  int32_t *write_addr = (int32_t*)data_ptr_w;
                  int16_t left = (int16_t)(audio_ptr_r[0] & 0x0000ffff);
                  int16_t right = (int16_t)(audio_ptr_r[0] >> 16);
                  write_addr[0] = (int32_t)lrintf(
                      AM_MAX(-2147483648, \
                             AM_MIN((left / 32768.0f) * 2147483648.0f,
                                    2147483647)));
                  write_addr[1] = (int32_t)lrintf(
                      AM_MAX(-2147483648, \
                             AM_MIN((right / 32768.0f) * 2147483648.0f,
                                    2147483647)));
                }break;
              }
            } else {
              uint32_t *write_addr = (uint32_t*)data_ptr_w;
              switch(m_sample_spec.channels) {
                case 1: {
                  write_addr[0] = audio_ptr_r[0];
                }break;
                case 2: {
                  write_addr[0] = audio_ptr_r[0];
                  write_addr[1] = audio_ptr_r[1];
                }break;
              }
            }
          }break;
        }
        data_ptr_w += frame_size;
        read_bytes += frame_size;
        total_read_bytes += frame_size;
        if (AM_LIKELY(read_bytes == m_chunk_bytes)) {
          AudioCapture a_capture;
          m_last_pts += m_fragment_pts;
          a_capture.packet.data   = audio_buf;
          a_capture.packet.length = m_chunk_bytes;
          a_capture.packet.pts    = m_last_pts;
          ++ sent_packet_num;
          if (AM_LIKELY(!m_wait && m_owner && m_capture_callback)) {
            a_capture.owner = m_owner;
            m_capture_callback(&a_capture);
          }
          data_ptr_w = audio_buf;
          read_bytes = 0;
        }
      }
      *audio_ptr_r = DATA_PLACE_HOLDER;
      audio_ptr_r = (uint32_t*)(audio_ptr_s + audio_blk_size +
                               ((((uintptr_t)audio_ptr_r -
                                  (uintptr_t)audio_ptr_s) +
                                 m_def_sample_spec.sample_size * 2) %
                                audio_read_len));
    } else if (AM_UNLIKELY(
#if GAP_ON_BOUNDARY
                           !found_real_gap &&
                           is_buf_boundary &&
#endif
                           need_to_skip)) {
      /* Skip Place-holder gap in DMA buffer */
      volatile uint32_t *last_read_ptr = audio_ptr_r;
      bool found_gap = true;
      INFO("Read %u bytes before meeting data-place-holder!!", read_bytes);
      while(m_run && (*audio_ptr_r == DATA_PLACE_HOLDER)) {
        uintptr_t read_len = ((((uintptr_t)audio_ptr_r -
            (uintptr_t)audio_ptr_s) + sizeof(uint32_t)) %
            audio_read_len);
        audio_ptr_r = (uint32_t*)(audio_ptr_s + audio_blk_size + read_len);
        if (AM_UNLIKELY(audio_ptr_r == last_read_ptr)) {
          NOTICE("All buffer has been searched! Already read %u bytes! "
                 "Searched from %p, buffer start address is %p",
                 read_bytes, last_read_ptr, audio_ptr_s);
          found_gap = false;
          break;
        }
      }
      if (AM_LIKELY(found_gap && (last_read_ptr != audio_ptr_r))) {
        uint32_t skipped_size = sizeof(uint32_t) *
            ((audio_ptr_r > last_read_ptr) ?
                (audio_ptr_r - last_read_ptr) :
                (audio_read_len - (last_read_ptr - audio_ptr_r)));
        if (AM_LIKELY(audio_ptr_r > last_read_ptr)) {
          NOTICE("Skipped %u bytes of data gap, "
                 "start addr %p, jump from %p to %p!!",
                 skipped_size, audio_ptr_s, last_read_ptr, audio_ptr_r);
        } else {
          NOTICE("Pointer wrapped around! Skipped %u bytes of data gap, "
                 "start addr %p, jump from %p to %p!!",
                 skipped_size, audio_ptr_s, last_read_ptr, audio_ptr_r);
        }
#if GAP_ON_BOUNDARY
        found_real_gap = ((skipped_size %
            (m_mem_info->length / RAW_AUDIO_BLOCK_NUM)) == 0);
#endif
      } else {
        NOTICE("Data gap is NOT found, need to skip it next time!");
      }
#if GAP_ON_BOUNDARY
      if (AM_UNLIKELY(!found_real_gap)) {
        /* If this data gap is not a real gap,
         * reset read pointer to where it starts to search data gap
         */
        audio_ptr_r = last_read_ptr;
      }
#endif
      need_to_skip = false;
    } else {
      uint32_t wait_value = 1000 * m_wait_interval *
          (m_chunk_bytes - read_bytes) / m_chunk_bytes;
      if (AM_LIKELY((last_read_bytes == read_bytes)
#if GAP_ON_BOUNDARY
                    && !found_real_gap
                    && is_buf_boundary
#endif
                    )) {
        ++ wait_count;
        if (AM_UNLIKELY(wait_count >= MAX_WAIT_COUNT)) {
          need_to_skip = true;
          wait_count = 0;
          WARN("\nNo valid data is found after %uus! It's probably a data gap. "
               "Trying to skip it. Current read address %p, "
               "%ju bytes are totally read!",
               wait_time,
               audio_ptr_r,
               total_read_bytes);
          wait_time = 0;
        } else {
          uint32_t wait_interval = wait_value / wait_count;
          usleep(wait_interval);
          wait_time += wait_interval;
        }
      } else {
        need_to_skip = false;
        usleep(wait_value);
      }
      last_read_bytes = read_bytes;
    }
  }

  if (AM_UNLIKELY(!unmap_audio_buf())) {
    ERROR("Failed to unmap audio buffer!");
  }
  delete m_mem_info;
  delete[] audio_buf;

  INFO("%s exits mainloop!", m_cap_thread_name.c_str());
}

AMAudioCaptureRaw::AMAudioCaptureRaw()
{
#if defined(AMBOOT_AUDIO_32BIT)
  m_def_sample_spec.format = AM_SAMPLE_S32LE;
  m_def_sample_spec.sample_size = sizeof(int32_t);
#elif defined(AMBOOT_AUDIO_16BIT)
  m_def_sample_spec.format = AM_SAMPLE_S16LE;
  m_def_sample_spec.sample_size = sizeof(int16_t);
#endif

#if defined(AMBOOT_AUDIO_48000)
  m_def_sample_spec.rate = 48000;
#elif defined(AMBOOT_AUDIO_16000)
  m_def_sample_spec.rate = 16000;
#elif defined(AMBOOT_AUDIO_8000)
  m_def_sample_spec.rate = 8000;
#endif

  m_context_name.clear();
}

AMAudioCaptureRaw::~AMAudioCaptureRaw()
{
  m_run = false;
  if (AM_LIKELY(!m_started)) {
    m_wait_event->signal();
  }
  m_capture_thread = nullptr;

  if (AM_LIKELY(m_iav_fd >= 0)) {
    close(m_iav_fd);
  }
  AM_DESTROY(m_wait_event);
}

bool AMAudioCaptureRaw::init(void *owner,
                             const std::string& name,
                             AudioCaptureCallback callback)
{
  bool ret = false;
  do {
    m_owner = owner;
    m_capture_callback = callback;
    m_context_name = name;
    if (AM_UNLIKELY(!m_owner)) {
      ERROR("Invalid owner of this object!");
      break;
    }

    m_wait_event = AMEvent::create();
    if (AM_UNLIKELY(!m_wait_event)) {
      ERROR("Failed to create AMEvent!");
      break;
    }
    if (AM_UNLIKELY((m_iav_fd = open("/dev/iav", O_RDWR)) < 0)) {
      if (AM_LIKELY((errno == ENXIO)  ||
                    (errno == ENOENT) ||
                    (errno == ENODEV))) {
        WARN("IAV driver is probably not loaded! Try to open IAV later");
      } else {
        ERROR("Failed to open IAV device: %s", strerror(errno));
        break;
      }
    }

    m_capture_func = std::bind(&AMAudioCaptureRaw::raw_audio_capture, this);
    m_capture_thread = RawCaptureThreadType(
        new std::thread(m_capture_func),
        [](std::thread *p){p->join(); delete p;});

    if (AM_UNLIKELY(nullptr == m_capture_thread)) {
      ERROR("Failed to create RAW audio capture thread!");
      break;
    }

    if (AM_UNLIKELY(pthread_setname_np(m_capture_thread->native_handle(),
                                       m_cap_thread_name.c_str()) < 0)) {
      WARN("Failed to set RAW audio capture thread name!");
    }
    while (!m_init_done) {
      usleep(10);
    }
    ret = m_run.load();
  }while(0);

  return ret;
}
