/*******************************************************************************
 * am_audio_player_alsa.cpp
 *
 * History:
 *   Mar 8, 2017 - [ypchang] created file
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
#include "am_audio_player_alsa.h"

static inline snd_pcm_uframes_t msec_to_frames(uint32_t usec,
                                               AudioSampleSpec &spec)
{
  return (usec * spec.rate / 1000);
}

AMIAudioPlayer* AMAudioPlayerAlsa::create(void *owner,
                                          const std::string &name,
                                          AudioPlayerCallback callback)
{
  AMAudioPlayerAlsa *result = new AMAudioPlayerAlsa();
  if (AM_UNLIKELY(result && (!result->init(owner, name, callback)))) {
    delete result;
    result = nullptr;
  }

  return ((AMIAudioPlayer*)result);
}

void AMAudioPlayerAlsa::destroy()
{
  delete this;
}

void AMAudioPlayerAlsa::set_echo_cancel_enabled(bool UNUSED(enabled))
{
  NOTICE("ALSA interface doesn't support Software AEC!");
}

bool AMAudioPlayerAlsa::start(int32_t volume)
{
  AUTO_MEM_LOCK(m_lock);
  AUTO_MEM_LOCK(m_lock_info);
  do {
    if (AM_LIKELY(m_is_player_started)) {
      INFO("Already started!");
      break;
    }
    m_sample_spec.channels    = m_audio_info.channels;
    m_sample_spec.rate        = m_audio_info.sample_rate;
    m_sample_spec.format      = snd_pcm_format_t(smp_fmt_to_pcm_fmt(
        AM_AUDIO_SAMPLE_FORMAT(m_audio_info.sample_format)));
    m_sample_spec.sample_size =
        snd_pcm_format_physical_width(m_sample_spec.format) / 8 ;

    if (AM_UNLIKELY((volume >= 0) && !set_volume(volume))) {
      WARN("Failed to set volume to %u!", volume);
    } else if (AM_LIKELY(volume < 0)) {
      NOTICE("Use default volume!");
    }

    m_is_player_started = true;
    m_event_run->signal();

  }while(0);

  return m_is_player_started.load();
}

bool AMAudioPlayerAlsa::stop(bool wait)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);

  if (AM_LIKELY(m_is_player_started)) {
    if (AM_LIKELY(wait)) {
      m_event->wait();
    }
    m_is_player_started = false;
    if (AM_LIKELY(SND_PCM_STATE_PAUSED == snd_pcm_state(m_handle))) {
      m_event_pause->signal();
    }
    while (m_is_initialized) {
      /* Wait play loop to exit and do resource release */
      usleep(10000);
    }
    if (AM_LIKELY(m_is_waiting)) {
      m_event_run->signal();
    }
  } else {
    m_event_run->signal();
  }

  return ret;
}

bool AMAudioPlayerAlsa::pause(bool enable)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;

  if (AM_LIKELY(m_is_player_started)) {
    snd_pcm_state_t state = snd_pcm_state(m_handle);
    /* Wait PCM to enter correct state if PAUSE is needed */
    while (enable &&
           m_is_player_started &&
           !m_is_player_running &&
           (state != SND_PCM_STATE_RUNNING)) {
      usleep(10000);
      state = snd_pcm_state(m_handle);
    }
    if (AM_LIKELY((enable && (state == SND_PCM_STATE_RUNNING)) ||
                  (!enable && (state == SND_PCM_STATE_PAUSED)))) {
      int err = snd_pcm_pause(m_handle, enable ? 1 : 0);
      if (AM_UNLIKELY(err < 0)) {
        ret = false;
        ERROR("Failed to %s: %s",
              enable ? "PAUSE" : "RESUME", snd_strerror(err));
      }
      if (AM_LIKELY(!enable)) {
        m_event_pause->signal();
      }
    } else {
      ERROR("PCM state is %u", state);
    }
  } else {
    NOTICE("Invalid operation! Player is already stopped!");
    ret = false;
  }

  return ret;
}

bool AMAudioPlayerAlsa::set_volume(int32_t volume)
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

bool AMAudioPlayerAlsa::is_player_running()
{
  AUTO_MEM_LOCK(m_lock);
  return m_is_player_started.load();
}

void AMAudioPlayerAlsa::set_audio_info(AM_AUDIO_INFO &ainfo)
{
  AUTO_MEM_LOCK(m_lock_info);
  m_is_restart_needed = ((m_audio_info.channels != ainfo.channels) ||
                         (m_audio_info.sample_rate != ainfo.sample_rate) ||
                         (m_audio_info.sample_format != ainfo.sample_format));
  memcpy(&m_audio_info, &ainfo, sizeof(m_audio_info));
}

void AMAudioPlayerAlsa::set_player_default_latency(uint32_t ms)
{
  AUTO_MEM_LOCK(m_lock_info);
  m_play_latency_ms = ms;
}

void AMAudioPlayerAlsa::set_player_callback(AudioPlayerCallback callback)
{
  AUTO_MEM_LOCK(m_lock_cb);
  m_player_callback = callback;
}

bool AMAudioPlayerAlsa::init_hw_param()
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

    uint32_t buffer_time = 0;
    uint32_t period_time = 0;
    if (AM_UNLIKELY((err = snd_pcm_hw_params_get_buffer_time_max(
        hw_params, &buffer_time, 0) < 0))) {
      ERROR("Failed to get hardware max buffer time: %s", snd_strerror(err));
      break;
    }

    period_time = buffer_time / 4;
    if (AM_UNLIKELY((err = snd_pcm_hw_params_set_period_time_near(
        m_handle, hw_params, &period_time, 0)) < 0)) {
      ERROR("Failed to set hardware period time to %u milliseconds: %s",
            period_time, snd_strerror(err));
      break;
    }

    if (AM_UNLIKELY((err = snd_pcm_hw_params_set_buffer_time_near(
        m_handle, hw_params, &buffer_time, 0)) < 0)) {
      ERROR("Failed to set hardware buffer time to %u milliseconds: %s",
            buffer_time, snd_strerror(err));
      break;
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

    m_period_buf_size = m_period_size *
                        m_sample_spec.channels *
                        m_sample_spec.sample_size;
    m_period_buffer = new uint8_t[m_period_buf_size];
    if (AM_UNLIKELY(!m_period_buffer)) {
      ERROR("Failed to allocate data buffer!");
      break;
    }

    if (AM_UNLIKELY((err = snd_pcm_hw_params_get_buffer_size(
        hw_params, &m_buffer_size)) < 0)) {
      ERROR("Failed to get hardware buffer size: %s", snd_strerror(err));
      break;
    }
    INFO("Buffer size is set to %u frames!", m_buffer_size);

    ret = true;
  }while(0);

  if (AM_LIKELY(hw_params)) {
    snd_pcm_hw_params_free(hw_params);
  }

  if (AM_UNLIKELY(!ret)) {
    delete[] m_period_buffer;
    m_period_buffer = nullptr;
    m_period_buf_size = 0;
  }

  return ret;
}

bool AMAudioPlayerAlsa::init_sw_param()
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
    uint32_t start_threashold = msec_to_frames(m_play_latency_ms,
                                               m_sample_spec);
    start_threashold = (start_threashold < 1) ? 1 :
        (start_threashold > m_buffer_size) ? m_buffer_size : start_threashold;

    if (AM_UNLIKELY((err = snd_pcm_sw_params_set_start_threshold(
        m_handle, sw_params, start_threashold)) < 0)) {
      ERROR("Failed to set start threshold to %u frames: %s",
            start_threashold,
            snd_strerror(err));
      break;
    }
    INFO("Start threashold is set to %u frames!", start_threashold);

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

void AMAudioPlayerAlsa::finalize()
{
  int err = -1;
  if (AM_LIKELY(m_handle)) {
    snd_pcm_drop(m_handle);
    if (AM_UNLIKELY((err = snd_pcm_close(m_handle)) < 0)) {
      ERROR("Failed to close audio device: %s", snd_strerror(err));
    }
    m_handle = nullptr;
  }
  m_period_buf_size = 0;
  delete[] m_period_buffer;
  m_period_buffer = nullptr;

  if (AM_LIKELY(m_is_restart_needed)) {
    usleep(10000);
  }
  m_is_restart_needed = false;
  m_is_draining = false;
  m_is_player_started = false;
  m_is_player_running = false;
  m_is_initialized = false;

  INFO("ALSA resources are released");
}

void AMAudioPlayerAlsa::alsa_play()
{
  m_run = true;
  while(m_run) {
    m_is_waiting = true;
    m_event_run->wait();
    m_is_waiting = false;

    while (m_is_player_started) {
      int err = -1;
      snd_pcm_state_t state;
      if (AM_LIKELY(!m_handle)) {
        if (AM_UNLIKELY((err = snd_pcm_open(&m_handle,
                                            m_card.c_str(),
                                            SND_PCM_STREAM_PLAYBACK, 0)) < 0)) {
          ERROR("Failed to open PCM: %s", snd_strerror(err));
          m_is_player_started = false;
          continue;
        }
      }
      state = snd_pcm_state(m_handle);
      switch(state) {
        case SND_PCM_STATE_OPEN: {
          INFO("PCM is open...");
          if (AM_UNLIKELY(!init_hw_param() || !init_sw_param())) {
            m_is_player_started = false;
          } else {
            m_is_initialized = true;
          }
        }break;
        case SND_PCM_STATE_SETUP: {
          if (AM_LIKELY(m_is_draining)) {
            m_event->signal();
            m_is_draining = false;
            INFO("Stream is drained...");
          } else if (m_is_initialized) {
            INFO("Preparing PCM...");
            if (AM_UNLIKELY((err = snd_pcm_prepare(m_handle)) < 0)) {
              ERROR("Failed to prepare audio interface for playing: %s",
                    snd_strerror(err));
              m_is_player_started = false;
            }
          }
        }break;
        case SND_PCM_STATE_PREPARED: {
          uint32_t frames = msec_to_frames(m_play_latency_ms / 2,
                                           m_sample_spec);
          snd_pcm_format_set_silence(m_sample_spec.format,
                                     m_period_buffer,
                                     frames * m_sample_spec.channels);
          if (AM_UNLIKELY(pcm_write(m_handle,
                                    m_period_buffer,
                                    frames) != frames)) {
            ERROR("Failed to write data %u bytes!", m_period_buf_size);
            m_is_player_started = false;
            break;
          }
          if (AM_UNLIKELY((err = snd_pcm_start(m_handle)) < 0)) {
            ERROR("Failed to start: %s", snd_strerror(err));
            m_is_player_started = false;
          } else {
            m_is_player_running = true;
          }
          INFO("Ready to run PCM...");
        }break;
        case SND_PCM_STATE_RUNNING: {
          while (!m_is_draining &&
                 m_is_player_started &&
                 (SND_PCM_STATE_RUNNING == state)) {
            uint32_t frames = 0;
            AudioPlayer a_player;
            a_player.data.data      = m_period_buffer;
            a_player.data.need_size = m_period_buf_size;
            m_lock_cb.lock();
            if (AM_LIKELY(m_owner && m_player_callback)) {
              a_player.owner             = m_owner;
              a_player.data.written_size = 0;
              a_player.data.drain        = false;
              m_player_callback(&a_player); /* get audio data */
            } else {
              /* Just write silence */
              snd_pcm_format_set_silence(m_sample_spec.format,
                                         m_period_buffer,
                                         m_period_size *
                                         m_sample_spec.channels);
              a_player.data.written_size = a_player.data.need_size;
            }
            m_lock_cb.unlock();
            if (AM_LIKELY(a_player.data.written_size > 0)) {
              frames = a_player.data.written_size /
                  m_sample_spec.channels / m_sample_spec.sample_size;
              if (AM_UNLIKELY(pcm_write(m_handle,
                                        m_period_buffer,
                                        frames) != frames)) {
                ERROR("Failed to write data %u bytes!",
                      a_player.data.written_size);
                break;
              }
            }
            if (AM_LIKELY(a_player.data.drain)) {
              INFO("Start to drain!");
              m_is_draining = true;
              err = snd_pcm_drain(m_handle);
              if (AM_UNLIKELY(err < 0)) {
                ERROR("Failed to drain: %s", snd_strerror(err));
                snd_pcm_drop(m_handle);
                m_is_draining = false;
                m_is_player_started = false;
                m_event->signal();
                break;
              }
            }
            state = snd_pcm_state(m_handle);
          }
        }break;
        case SND_PCM_STATE_XRUN:
        case SND_PCM_STATE_SUSPENDED: {
          m_is_player_running = false;
          xrun_recovery(state);
        }break;
        case SND_PCM_STATE_DRAINING: {
          m_is_player_running = false;
          INFO("Draining...");
        }break;
        case SND_PCM_STATE_PAUSED: {
          INFO("Paused...");
          m_is_player_running = false;
          m_event_pause->wait();
        }break;
        case SND_PCM_STATE_DISCONNECTED: {
          ERROR("Disconnected...");
          m_is_player_running = false;
          m_is_player_started = false;
        }break;
        default: break;
      }
    }
    finalize();
  }
  AM_DESTROY(m_event);
  AM_DESTROY(m_event_run);
  AM_DESTROY(m_event_pause);
}

bool AMAudioPlayerAlsa::xrun_recovery(snd_pcm_state_t state)
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

snd_pcm_uframes_t AMAudioPlayerAlsa::pcm_write(snd_pcm_t *handle,
                                               uint8_t *data,
                                               snd_pcm_uframes_t frames)
{
  snd_pcm_uframes_t frames_remain  = frames;
  snd_pcm_uframes_t frames_written = 0;
  uint8_t                     *src = data;

  while (handle && (frames_remain > 0)) {
    snd_pcm_sframes_t written = snd_pcm_writei(handle, src, frames_remain);
    if (AM_LIKELY(written > 0)) {
      frames_remain -= written;
      frames_written += written;
      src += (written * m_sample_spec.sample_size * m_sample_spec.channels);
    } else {
      bool ret = true;
      switch(written) {
        case -EBADFD: {
          ERROR("PCM is in bad state!");
          ret = false;
        }break;
        case -EPIPE: {
          NOTICE("Stream underrun, trying to recover!");
          ret = xrun_recovery(SND_PCM_STATE_XRUN);
        }break;
        case -ESTRPIPE: {
          NOTICE("Stream suspended, trying to recover!");
          ret = xrun_recovery(SND_PCM_STATE_SUSPENDED);
        }break;
        default: {
          ERROR("snd_pcm_writei: %s", snd_strerror(written));
        }break;
      }
      if (AM_UNLIKELY(!ret)) {
        ERROR("Failed to recover from bad state!");
        break;
      }
    }
  }

  return frames_written;
}

AMAudioPlayerAlsa::AMAudioPlayerAlsa()
{}

AMAudioPlayerAlsa::~AMAudioPlayerAlsa()
{
  m_run = false;
  stop(false);
}

bool AMAudioPlayerAlsa::init(void *owner,
                             const std::string& name,
                             AudioPlayerCallback callback)
{
  bool ret = false;
  do {
    m_owner = owner;
    m_player_callback = callback;
    m_context_name = name;

    if (AM_UNLIKELY(!m_owner)) {
      ERROR("Invalid owner of this object!");
      break;
    }

    if (AM_UNLIKELY(!m_player_callback)) {
      WARN("Audio player callback function is not set!");
    }

    if (AM_UNLIKELY(nullptr == (m_event = AMEvent::create()))) {
      ERROR("Failed to create AMEvent object!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_event_run = AMEvent::create()))) {
      ERROR("Failed to create AMEvent object!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_event_pause = AMEvent::create()))) {
      ERROR("Failed to create AMEvent object!");
      break;
    }

    m_alsa_play = std::bind(&AMAudioPlayerAlsa::alsa_play, this);
    m_alsa_play_thread = AlsaPlayThreadType(
        new std::thread(m_alsa_play),
        [](std::thread *p){p->join(); delete p;});

    if (AM_UNLIKELY(nullptr == m_alsa_play_thread)) {
      ERROR("Failed to create ALSA audio player thread!");
      break;
    }

    if (AM_UNLIKELY(pthread_setname_np(m_alsa_play_thread->native_handle(),
                                       m_context_name.c_str()) < 0)) {
      WARN("Failed to set ALSA audio player thread name!");
    }

    ret = true;
  }while(0);

  return ret;
}
