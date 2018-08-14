/*******************************************************************************
 * am_audio_capture_alsa.h
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
#ifndef AM_AUDIO_CAPTURE_ALSA_H_
#define AM_AUDIO_CAPTURE_ALSA_H_

#include "am_event.h"
#include "am_mutex.h"
#include "am_audio_capture_if.h"

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

class AMAudioCaptureAlsa: public AMIAudioCapture
{
    typedef std::function<void()>        AlsaCaptureFuncType;
    typedef std::shared_ptr<std::thread> AlsaCaptureThreadType;
  public:
    static AMIAudioCapture* create(void *owner,
                                   const std::string& name,
                                   AudioCaptureCallback callback);

  public:
    virtual bool set_capture_callback(AudioCaptureCallback callback);
    virtual void destroy();
    virtual void set_echo_cancel_enabled(bool enabled);
    virtual bool start(int32_t volume = -1);
    virtual bool stop();
    virtual bool set_channel(uint32_t channel);
    virtual bool set_sample_rate(uint32_t sample_rate);
    virtual bool set_chunk_bytes(uint32_t chunk_bytes);
    virtual bool set_sample_format(AM_AUDIO_SAMPLE_FORMAT format);
    virtual bool set_volume(uint32_t volume);
    virtual uint32_t get_channel();
    virtual uint32_t get_sample_rate();
    virtual uint32_t get_chunk_bytes();
    virtual uint32_t get_sample_size();
    virtual uint32_t get_sample_physical_size();
    virtual int64_t  get_chunk_pts();
    virtual AM_AUDIO_SAMPLE_FORMAT get_sample_format();

  private:
    inline bool init_hw_param();
    inline bool init_sw_param();
    void finalize();
    int64_t get_current_pts();

  private:
    void alsa_capture();
    bool xrun_recovery(snd_pcm_state_t state);
    snd_pcm_uframes_t pcm_read(snd_pcm_t *handle,
                               uint8_t *buffer,
                               uint32_t buffer_size,
                               uint32_t frame_bytes);

  private:
    AMAudioCaptureAlsa();
    virtual ~AMAudioCaptureAlsa();
    bool init(void *owner,
              const std::string &name,
              AudioCaptureCallback callback);

  private:
    int64_t                m_last_pts        = 0LL;
    AlsaCaptureFuncType    m_alsa_cap        = nullptr;
    AlsaCaptureThreadType  m_alsa_cap_thread = nullptr;
    AudioCaptureCallback   m_cap_callback    = nullptr;
    void                  *m_owner           = nullptr;
    snd_pcm_t             *m_handle          = nullptr;
    AMEvent               *m_event_run       = nullptr;
    uint8_t               *m_chunk_buffer    = nullptr;
    snd_pcm_uframes_t      m_period_size     = 0;
    uint32_t               m_period_bytes    = 0;
    uint32_t               m_chunk_bytes     = 0;
    int                    m_hw_timer_fd     = -1;
    AM_AUDIO_SAMPLE_FORMAT m_sample_format   = AM_SAMPLE_INVALID;
    std::atomic<bool>      m_run             = {false};
    std::atomic<bool>      m_is_waiting      = {false};
    std::atomic<bool>      m_is_started      = {false};
    std::string            m_card            = std::string("default");
    std::string            m_selement_name   = std::string("Mic Gain Control");
    std::string            m_context_name;
    AudioSampleSpec        m_sample_spec;
    AM_AUDIO_INFO          m_audio_info;
    AMMemLock              m_lock;
};

#endif /* AM_AUDIO_CAPTURE_ALSA_H_ */
