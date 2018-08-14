/*******************************************************************************
 * am_audio_player_alsa.h
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
#ifndef AM_AUDIO_PLAYER_ALSA_H_
#define AM_AUDIO_PLAYER_ALSA_H_

#include "am_event.h"
#include "am_mutex.h"
#include "am_audio_player_if.h"

#include <thread>
#include <memory>
#include <functional>
#include <alsa/asoundlib.h>

struct AudioSampleSpec
{
    snd_pcm_format_t format      = SND_PCM_FORMAT_UNKNOWN;
    uint32_t         rate        = 48000;
    uint16_t         channels    = 2;
    uint16_t         sample_size = 2;
};

class AMAudioPlayerAlsa: public AMIAudioPlayer
{
    typedef std::function<void()>        AlsaPlayFuncType;
    typedef std::shared_ptr<std::thread> AlsaPlayThreadType;
  public:
    static AMIAudioPlayer* create(void *owner,
                                  const std::string& name,
                                  AudioPlayerCallback callback);

  public:
    virtual void destroy();
    virtual void set_echo_cancel_enabled(bool enabled);
    virtual bool start(int32_t volume = -1);
    virtual bool stop(bool wait = true);
    virtual bool pause(bool enable);
    virtual bool set_volume(int32_t volume);
    virtual bool is_player_running();
    virtual void set_audio_info(AM_AUDIO_INFO &ainfo);
    virtual void set_player_default_latency(uint32_t ms);
    virtual void set_player_callback(AudioPlayerCallback callback);

  private:
    inline bool init_hw_param();
    inline bool init_sw_param();
    void finalize();

  private:
    void alsa_play();
    bool xrun_recovery(snd_pcm_state_t state);
    snd_pcm_uframes_t pcm_write(snd_pcm_t *handle,
                                uint8_t *data,
                                snd_pcm_uframes_t frames);

  private:
    AMAudioPlayerAlsa();
    virtual ~AMAudioPlayerAlsa();
    bool init(void *owner,
              const std::string& name,
              AudioPlayerCallback callback);

  private:
    AlsaPlayFuncType     m_alsa_play         = nullptr;
    AlsaPlayThreadType   m_alsa_play_thread  = nullptr;
    AudioPlayerCallback  m_player_callback   = nullptr;
    void                *m_owner             = nullptr;
    snd_pcm_t           *m_handle            = nullptr;
    uint8_t             *m_period_buffer     = nullptr;
    AMEvent             *m_event             = nullptr;
    AMEvent             *m_event_run         = nullptr;
    AMEvent             *m_event_pause       = nullptr;
    snd_pcm_uframes_t    m_period_size       = 0;
    snd_pcm_uframes_t    m_buffer_size       = 0;
    uint32_t             m_period_buf_size   = 0;
    uint32_t             m_play_latency_ms   = 0;
    std::atomic<bool>    m_is_player_started = {false};
    std::atomic<bool>    m_is_player_running = {false};
    std::atomic<bool>    m_is_restart_needed = {false};
    std::atomic<bool>    m_is_initialized    = {false};
    std::atomic<bool>    m_is_draining       = {false};
    std::atomic<bool>    m_is_waiting        = {false};
    std::atomic<bool>    m_run               = {true};
    std::string          m_card              = std::string("default");
    std::string          m_selement_name     = std::string("Speaker Output");
    std::string          m_context_name;
    AudioSampleSpec      m_sample_spec;
    AM_AUDIO_INFO        m_audio_info;
    AMMemLock            m_lock;
    AMMemLock            m_lock_cb;
    AMMemLock            m_lock_info;
};

#endif /* AM_AUDIO_PLAYER_ALSA_H_ */
