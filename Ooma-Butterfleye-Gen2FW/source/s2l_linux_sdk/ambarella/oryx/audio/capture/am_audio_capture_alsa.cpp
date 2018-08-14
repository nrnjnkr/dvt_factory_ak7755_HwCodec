/*******************************************************************************
 * am_audio_capture_alsa.cpp
 *
 * History:
 *   Mar 15, 2017 - [ypchang] created file
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

#include "am_audio_utility.h"
#include "am_audio_capture_alsa.h"

#define HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")

static inline snd_pcm_uframes_t bytes_to_frames(uint32_t &bytes,
                                                AudioSampleSpec &spec)
{
  uint32_t frame_size = snd_pcm_format_physical_width(spec.format) *
                        spec.channels / 8;
  return (ROUND_UP(bytes, frame_size) / frame_size);
}

static inline uint32_t frames_to_usec(snd_pcm_uframes_t &frames,
                                      AudioSampleSpec &spec)
{
  return (frames * 1000000 / spec.rate);
}

AMIAudioCapture* AMAudioCaptureAlsa::create(void *owner,
                                            const std::string& name,
                                            AudioCaptureCallback callback)
{
  AMAudioCaptureAlsa *result = new AMAudioCaptureAlsa();
  if (AM_UNLIKELY(result && (!result->init(owner, name, callback)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

bool AMAudioCaptureAlsa::set_capture_callback(AudioCaptureCallback callback)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio capture callback of %s is set to %p",
       m_context_name.c_str(), callback);
  m_cap_callback = callback;
  return (nullptr != m_cap_callback);
}

void AMAudioCaptureAlsa::destroy()
{
  delete this;
}

void AMAudioCaptureAlsa::set_echo_cancel_enabled(bool UNUSED(enabled))
{
  NOTICE("ALSA interface does NOT support software AEC!");
}

bool AMAudioCaptureAlsa::start(int32_t volume)
{
  AUTO_MEM_LOCK(m_lock);
  do {
    if (AM_LIKELY(m_is_started)) {
      INFO("Already started!");
      break;
    }

    if (AM_UNLIKELY((volume >= 0) && !set_volume(volume))) {
      WARN("Failed to set volume to %u!", volume);
    } else if (AM_LIKELY(volume < 0)) {
      NOTICE("Use default volume!");
    }

    m_is_started = true;
    m_event_run->signal();

  }while(0);

  return true;
}

bool AMAudioCaptureAlsa::stop()
{
  AUTO_MEM_LOCK(m_lock);

  if (AM_LIKELY(m_is_started)) {
    int err = -1;
    m_is_started = false;
    if (AM_UNLIKELY((err = snd_pcm_drain(m_handle)) < 0)) {
      ERROR("Failed to drain: %s", snd_strerror(err));
    }
    if (AM_LIKELY(m_is_waiting)) {
      m_event_run->signal();
    }
  } else {
    m_event_run->signal();
  }

  return true;
}

bool AMAudioCaptureAlsa::set_channel(uint32_t channel)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio channel of %s is set to %u.", m_context_name.c_str(), channel);
  m_sample_spec.channels = channel;

  return ((channel > 0) && (channel <= 2));
}

bool AMAudioCaptureAlsa::set_sample_rate(uint32_t sample_rate)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio sample rate of %s is set to %u.",
       m_context_name.c_str(), sample_rate);
  m_sample_spec.rate = sample_rate;
  return (sample_rate > 0);
}

bool AMAudioCaptureAlsa::set_chunk_bytes(uint32_t chunk_bytes)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio chunk bytes of %s is set to %u.",
       m_context_name.c_str(), chunk_bytes);
  if (AM_LIKELY((chunk_bytes > 0) && (m_chunk_bytes != chunk_bytes))) {
    delete[] m_chunk_buffer;
    m_chunk_buffer = new uint8_t[chunk_bytes];
    if (AM_UNLIKELY(!m_chunk_buffer)) {
      ERROR("Failed to allocate %u bytes buffer!", chunk_bytes);
    }
  }
  m_chunk_bytes = chunk_bytes;
  return (m_chunk_bytes > 0) && m_chunk_buffer;
}

bool AMAudioCaptureAlsa::set_sample_format(AM_AUDIO_SAMPLE_FORMAT format)
{
  AUTO_MEM_LOCK(m_lock);
  m_sample_format = format;
  m_sample_spec.format = snd_pcm_format_t(smp_fmt_to_pcm_fmt(format));
  m_sample_spec.sample_size = (m_sample_spec.format == SND_PCM_FORMAT_UNKNOWN) ?
      0 : (snd_pcm_format_width(m_sample_spec.format) / 8);

  INFO("Set sample format to %s, sample size: %u bytes",
       smp_fmt_to_str(format),
       m_sample_spec.sample_size);
  return (m_sample_spec.format != SND_PCM_FORMAT_UNKNOWN);
}

bool AMAudioCaptureAlsa::set_volume(uint32_t volume)
{
  bool ret = false;
  snd_mixer_t *handle = nullptr;
  snd_mixer_selem_id_t *sid = nullptr;
  do {
    snd_mixer_elem_t *elem = nullptr;
    int err = -1;
    long min = 0;
    long max = 0;

    if (AM_UNLIKELY((err = snd_mixer_open(&handle, 0)) < 0)) {
      ERROR("Failed to open mixer handler: %s", snd_strerror(err));
      break;
    }
    if (AM_UNLIKELY((err = snd_mixer_attach(handle, m_card.c_str())) < 0)) {
      ERROR("Failed to attach mixer to card(\"%s\"): %s",
            m_card.c_str(), snd_strerror(err));
      break;
    }
    if (AM_UNLIKELY((err = snd_mixer_selem_register(handle,
                                                    nullptr,
                                                    nullptr)) < 0)) {
      ERROR("Failed to register mixer simple element class: %s",
            snd_strerror(err));
      break;
    }
    if (AM_UNLIKELY((err = snd_mixer_load(handle)) < 0)) {
      ERROR("Failed to load mixer elements: %s", snd_strerror(err));
      break;
    }
    if (AM_UNLIKELY((err = snd_mixer_selem_id_malloc(&sid)) < 0)) {
      ERROR("Failed to allocate simple mixer element ID!");
      break;
    }
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, m_selement_name.c_str());
    elem = snd_mixer_find_selem(handle, sid);
    if (AM_UNLIKELY(!elem)) {
      ERROR("Simple Element \"%s\" is NOT found!", m_selement_name.c_str());
      break;
    }
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    err = snd_mixer_selem_set_playback_volume_all(elem, max * volume / 100);
    if (AM_UNLIKELY(err < 0)) {
      ERROR("Failed to set volume to %u: %s", volume, snd_strerror(err));
      break;
    }

    ret = true;
  }while(0);

  if (AM_LIKELY(handle)) {
    snd_mixer_close(handle);
  }
  if (AM_LIKELY(sid)) {
    snd_mixer_selem_id_free(sid);
  }

  return ret;
}

uint32_t AMAudioCaptureAlsa::get_channel()
{
  AUTO_MEM_LOCK(m_lock);
  return m_sample_spec.channels;
}

uint32_t AMAudioCaptureAlsa::get_sample_rate()
{
  AUTO_MEM_LOCK(m_lock);
  return m_sample_spec.rate;
}
uint32_t AMAudioCaptureAlsa::get_chunk_bytes()
{
  AUTO_MEM_LOCK(m_lock);
  return m_chunk_bytes;
}

uint32_t AMAudioCaptureAlsa::get_sample_size()
{
  AUTO_MEM_LOCK(m_lock);
  return m_sample_spec.sample_size;
}

uint32_t AMAudioCaptureAlsa::get_sample_physical_size()
{
  AUTO_MEM_LOCK(m_lock);
  return snd_pcm_format_physical_width(m_sample_spec.format) / 8;
}

int64_t  AMAudioCaptureAlsa::get_chunk_pts()
{
  AUTO_MEM_LOCK(m_lock);
  int64_t one_sec_frame_bytes =
      snd_pcm_format_physical_width(m_sample_spec.format) *
      m_sample_spec.channels *
      m_sample_spec.rate / 8;
  return ((int64_t)(90000LL * m_chunk_bytes) / one_sec_frame_bytes);
}

AM_AUDIO_SAMPLE_FORMAT AMAudioCaptureAlsa::get_sample_format()
{
  AUTO_MEM_LOCK(m_lock);
  return m_sample_format;
}

bool AMAudioCaptureAlsa::init_hw_param()
{
  bool ret = false;
    snd_pcm_hw_params_t *hw_params = nullptr;

    do {
      int err = -1;
      if (AM_UNLIKELY((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)) {
        ERROR ("Failed to allocate hardware parameter structure: %s",
               snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_any(m_handle, hw_params)) < 0)) {
        ERROR("Failed to initialize hardware parameter structure: %s",
              snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_access(
          m_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)) {
        ERROR("Failed to set access type: %s", snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_format(
          m_handle, hw_params, m_sample_spec.format)) < 0)) {
        ERROR("Failed to set sample format: %s", snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_rate(
          m_handle, hw_params, m_sample_spec.rate, 0)) < 0)) {
        ERROR("Failed to set sample rate to %u: %s",
              m_sample_spec.rate, snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_channels(
          m_handle, hw_params, m_sample_spec.channels)) < 0)) {
        ERROR("Failed to set channels to %u: %s",
              m_sample_spec.channels, snd_strerror(err));
        break;
      }

      uint32_t period_bytes = m_chunk_bytes / 4;
      snd_pcm_uframes_t min_frames = bytes_to_frames(period_bytes,
                                                    m_sample_spec);
      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_period_size_near(
          m_handle, hw_params, &min_frames, 0)) < 0)) {
        ERROR("Failed to set hardware period size to %u frames: %s",
              min_frames, snd_strerror(err));
        break;
      }

      min_frames *= 4;
      if (AM_UNLIKELY((err = snd_pcm_hw_params_set_buffer_size_near(
          m_handle, hw_params, &min_frames)) < 0)) {
        ERROR("Failed to set hardware buffer size to %u frames: %s",
              min_frames, snd_strerror(err));
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params(m_handle, hw_params)) < 0)) {
        ERROR("Failed to apply hardware parameters: %s",
              snd_strerror(err));
        break;
      }

      if (AM_UNLIKELY((err = snd_pcm_hw_params_get_period_size(hw_params,
                                                               &m_period_size,
                                                               0)) < 0)) {
        ERROR("Failed to get hardware period size: %s", snd_strerror(err));
        break;
      }

      snd_pcm_uframes_t buffer_size = 0;
      if (AM_UNLIKELY((err = snd_pcm_hw_params_get_buffer_size(
                      hw_params, &buffer_size)) < 0)) {
        ERROR("Failed to get hardware buffer size: %s", snd_strerror(err));
        break;
      }

      m_period_bytes = m_period_size *
                       m_sample_spec.channels *
                       snd_pcm_format_physical_width(m_sample_spec.format) / 8;
      INFO("Set period size to %u frames, actual period size is %u frames",
           min_frames / 4, m_period_size);
      INFO("Set buffer size to %u frames, actual buffer size is %u frames",
           min_frames, buffer_size);

      ret = true;
    }while(0);

    if (AM_LIKELY(hw_params)) {
      snd_pcm_hw_params_free(hw_params);
    }

    return ret;
}

bool AMAudioCaptureAlsa::init_sw_param()
{
  bool ret = false;
  snd_pcm_sw_params_t *sw_params = nullptr;

  do {
    int err = -1;

    if (AM_UNLIKELY((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)) {
      ERROR ("Failed to allocate software parameter structure: %s",
             snd_strerror(err));
      break;
    }

    if (AM_UNLIKELY((err = snd_pcm_sw_params_current(m_handle,
                                                     sw_params)) < 0)) {
      ERROR("Failed to initialize software parameter structure: %s",
            snd_strerror(err));
      break;
    }

    if (AM_UNLIKELY((err = snd_pcm_sw_params_set_avail_min(
        m_handle, sw_params, m_period_size)) < 0)) {
      ERROR("Failed to set avail min to %u frames: %s",
            m_period_size, snd_strerror(err));
      break;
    }
    INFO("Period size is set to %u frames!", m_period_size);

    if (AM_UNLIKELY((err = snd_pcm_sw_params_set_start_threshold(
        m_handle, sw_params, m_period_size)) < 0)) {
      ERROR("Failed to set start threshold to %u frames: %s",
            m_period_size, snd_strerror(err));
      break;
    }
    INFO("Buffer size is set to %u frames!", m_period_size);

    if (AM_UNLIKELY((err = snd_pcm_sw_params(m_handle, sw_params)) < 0)) {
      ERROR("Failed to apply software parameters: %s",
            snd_strerror(err));
      break;
    }
    ret = true;
  }while(0);

  if (AM_LIKELY(sw_params)) {
    snd_pcm_sw_params_free(sw_params);
  }

  return ret;
}

void AMAudioCaptureAlsa::finalize()
{
  int err = -1;
  if (AM_LIKELY(m_handle)) {
    snd_pcm_drop(m_handle);
    if (AM_UNLIKELY((err = snd_pcm_close(m_handle)) < 0)) {
      ERROR("Failed to close audio device: %s", snd_strerror(err));
    }
    m_handle = nullptr;
  }
  m_period_bytes = 0;
  m_is_started = false;

  INFO("ALSA resources are released");
}

int64_t AMAudioCaptureAlsa::get_current_pts()
{
  char pts[32] = {0};
  int64_t cur_pts = m_last_pts;
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    if (AM_UNLIKELY(read(m_hw_timer_fd, pts, sizeof(pts)) < 0)) {
      ERROR("read: %d %s", m_hw_timer_fd, strerror(errno));
    } else {
      cur_pts = strtoll(pts, nullptr, 10);
    }
  }

  return cur_pts;
}

void AMAudioCaptureAlsa::alsa_capture()
{
  uint32_t frame_bytes = 0;
  uint32_t  read_bytes = 0;

  m_run = true;
  while(m_run) {
    m_is_waiting = true;
    m_event_run->wait();
    m_is_waiting = false;

    frame_bytes = snd_pcm_format_physical_width(m_sample_spec.format) *
                  m_sample_spec.channels / 8;

    while (m_is_started) {
      int err = -1;
      snd_pcm_state_t state;
      if (AM_LIKELY(!m_handle)) {
        if (AM_UNLIKELY((err = snd_pcm_open(&m_handle,
                                            m_card.c_str(),
                                            SND_PCM_STREAM_CAPTURE, 0)) < 0)) {
          ERROR("Failed to open PCM: %s", snd_strerror(err));
          m_is_started = false;
          continue;
        }
      }
      state = snd_pcm_state(m_handle);
      switch(state) {
        case SND_PCM_STATE_OPEN: {
          INFO("PCM is open...");
          if (AM_UNLIKELY(!init_hw_param() || !init_sw_param())) {
            m_is_started = false;
          }
        }break;
        case SND_PCM_STATE_SETUP: {
          INFO("Preparing PCM...");
          if (AM_UNLIKELY((err = snd_pcm_prepare(m_handle)) < 0)) {
            ERROR("Failed to prepare audio interface for playing: %s",
                  snd_strerror(err));
            m_is_started = false;
          }
        }break;
        case SND_PCM_STATE_PREPARED: {
          INFO("Ready to run PCM...");
          if (AM_UNLIKELY((err = snd_pcm_start(m_handle)) < 0)) {
            ERROR("Failed to start: %s", snd_strerror(err));
            m_is_started = false;
          }
        }break;
        case SND_PCM_STATE_RUNNING: {
          snd_pcm_uframes_t read_frames =
              pcm_read(m_handle,
                       (m_chunk_buffer + (read_bytes % m_chunk_bytes)),
                       (m_chunk_bytes - read_bytes),
                       frame_bytes);
          read_bytes += read_frames * frame_bytes;
          if (AM_LIKELY((read_bytes == m_chunk_bytes))) {
            m_last_pts = get_current_pts();
            if (AM_LIKELY(m_owner && m_cap_callback && m_is_started)) {
              AudioCapture a_cap;
              a_cap.owner = m_owner;
              a_cap.packet.data = m_chunk_buffer;
              a_cap.packet.length = m_chunk_bytes;
              a_cap.packet.pts = m_last_pts;
              m_cap_callback(&a_cap);
            }
            read_bytes = 0;
          } else {
            snd_pcm_uframes_t remain_frame =
                (m_chunk_bytes / frame_bytes) - read_frames;
            snd_pcm_wait(m_handle, frames_to_usec(remain_frame,
                                                  m_sample_spec) / 1000);
          }
        }break;
        case SND_PCM_STATE_XRUN:
        case SND_PCM_STATE_SUSPENDED: {
          xrun_recovery(state);
        }break;
        case SND_PCM_STATE_DRAINING: {
          INFO("Draining...");
        }break;
        case SND_PCM_STATE_PAUSED: {
          /* Won't come here */
        }break;
        case SND_PCM_STATE_DISCONNECTED: {
          ERROR("Disconnected...");
          m_is_started = false;
        }break;
        default: break;
      }
    }
    finalize();
  }
  AM_DESTROY(m_event_run);
  delete[] m_chunk_buffer;
  m_chunk_buffer = nullptr;
}

bool AMAudioCaptureAlsa::xrun_recovery(snd_pcm_state_t state)
{
  bool ret = true;
  int err = -1;

  switch(state) {
    case SND_PCM_STATE_XRUN: {
      if (AM_UNLIKELY((err = snd_pcm_prepare(m_handle)) < 0)) {
        ERROR("Failed to recover from under run: %s", snd_strerror(err));
        ret = false;
      }
    }break;
    case SND_PCM_STATE_SUSPENDED: {
      while((err = snd_pcm_resume(m_handle)) == -EAGAIN) {
        usleep(100000);
      }
      if (AM_UNLIKELY(err < 0)) {
        err = snd_pcm_prepare(m_handle);
        if (AM_UNLIKELY(err < 0)) {
          ERROR("Failed to recover from suspend: %s", snd_strerror(err));
          ret = false;
        }
      }
    }break;
    default: break;
  }

  return ret;
}

snd_pcm_uframes_t AMAudioCaptureAlsa::pcm_read(snd_pcm_t *handle,
                                               uint8_t *buffer,
                                               uint32_t buf_size,
                                               uint32_t frame_bytes)
{
  snd_pcm_uframes_t ret = 0;
  if (AM_LIKELY(handle)) {
    uint8_t *buf = buffer;
    uint32_t read_bytes = 0;
    snd_pcm_sframes_t read_frame = 0;
    do {
      snd_pcm_uframes_t requested_frame = (buf_size - read_bytes) / frame_bytes;
      read_frame = snd_pcm_readi(handle, buf, requested_frame);
      if (AM_LIKELY(read_frame > 0)) {
        uint32_t read_frame_bytes = read_frame * frame_bytes;
        ret += read_frame;
        buf += read_frame_bytes;
        read_bytes += read_frame_bytes;
      } else {
        bool ret_val = true;
        if (AM_LIKELY(!m_is_started)) {
          NOTICE("Reading is aborted!");
          break;
        }
        switch(read_frame) {
          case -EBADFD: {
            ERROR("PCM is in bad state!");
            ret_val = false;
          }break;
          case -EPIPE: {
            NOTICE("Stream overrun, trying to recover!");
            ret_val = xrun_recovery(SND_PCM_STATE_XRUN);
          }break;
          case -ESTRPIPE: {
            NOTICE("Stream suspended, trying to recover!");
            ret_val = xrun_recovery(SND_PCM_STATE_SUSPENDED);
          }break;
          default: {
            ERROR("snd_pcm_readi: %s", snd_strerror(read_frame));
          }break;
        }
        if (AM_UNLIKELY(!ret_val)) {
          ERROR("Failed to recover from bad state!");
          break;
        }
      }
    } while((read_frame > 0) && (read_bytes < buf_size) && m_is_started);
  }

  return ret;
}

AMAudioCaptureAlsa::AMAudioCaptureAlsa()
{}

AMAudioCaptureAlsa::~AMAudioCaptureAlsa()
{
  m_run = false;
  stop();
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    close(m_hw_timer_fd);
  }
}

bool AMAudioCaptureAlsa::init(void *owner,
          const std::string &name,
          AudioCaptureCallback callback)
{
  bool ret = false;
  do {
    m_owner = owner;
    m_cap_callback = callback;
    m_context_name = name;

    if (AM_UNLIKELY(!m_owner)) {
      ERROR("Invalid owner of this object!");
      break;
    }

    if (AM_UNLIKELY(!m_cap_callback)) {
      WARN("Audio capture callback function is not set!");
    }

    if (AM_UNLIKELY(nullptr == (m_event_run = AMEvent::create()))) {
      ERROR("Failed to create AMEvent object!");
    }

    if (AM_UNLIKELY((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0)) {
      ERROR("Failed to open %s: %s", HW_TIMER, strerror(errno));
      break;
    }

    m_alsa_cap = std::bind(&AMAudioCaptureAlsa::alsa_capture, this);
    m_alsa_cap_thread = AlsaCaptureThreadType(
        new std::thread(m_alsa_cap),
        [](std::thread *p){p->join(), delete p;});

    if (AM_UNLIKELY(nullptr == m_alsa_cap_thread)) {
      ERROR("Failed to create ALSA audio capture thread!");
      break;
    }

    if (AM_UNLIKELY(pthread_setname_np(m_alsa_cap_thread->native_handle(),
                                       m_context_name.c_str()) < 0)) {
      WARN("Failed to set ALSA audio capture thread name!");
    }

    ret = true;
  }while(0);

  return ret;
}

