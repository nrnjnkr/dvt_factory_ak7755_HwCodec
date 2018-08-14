/*******************************************************************************
 * am_audio_capture_raw.h
 *
 * History:
 *   Dec 6, 2016 - [ypchang] created file
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
#ifndef AM_AUDIO_CAPTURE_RAW_H_
#define AM_AUDIO_CAPTURE_RAW_H_

#include "am_mutex.h"
#include "am_audio_capture_if.h"

#include <thread>
#include <memory>
#include <functional>

enum {
  /* Hard coded in amboot audio.c */
  RAW_AUDIO_SAMPLE_BITS = 16,
  RAW_AUDIO_CHANNEL_NUM = 2,
  RAW_AUDIO_SAMPLE_RATE = 48000,
  RAW_AUDIO_BLOCK_NUM   = 4,
};

struct RawAudioSampleSpec
{
    AM_AUDIO_SAMPLE_FORMAT format      = AM_SAMPLE_S16LE;
    uint32_t               rate        = 48000;
    uint16_t               channels    = 2;
    uint16_t               sample_size = 2;
};

struct AudioMemMapInfo;
class AMEvent;
class AMAudioCaptureRaw: public AMIAudioCapture
{
    typedef std::function<void()> RawCaptureFuncType;
    typedef std::shared_ptr<std::thread> RawCaptureThreadType;
  public:
    static AMIAudioCapture* create(void *owner,
                                   const std::string& name,
                                   AudioCaptureCallback callback);
  public:
    virtual bool set_capture_callback(AudioCaptureCallback callback);
    virtual void destroy();
    virtual void set_echo_cancel_enabled(bool enabled);
    virtual bool start(int32_t volume);
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
    bool map_audio_buf();
    bool unmap_audio_buf();
    void raw_audio_capture();

  private:
    AMAudioCaptureRaw();
    virtual ~AMAudioCaptureRaw();
    bool init(void *owner,
              const std::string& name,
              AudioCaptureCallback callback);

  private:
    int64_t              m_last_pts         = 900000LL;
    int64_t              m_fragment_pts     = 0LL;
    void                *m_owner            = nullptr; /* Owner of this obj */
    AudioMemMapInfo     *m_mem_info         = nullptr;
    AMEvent             *m_wait_event       = nullptr;
    AudioCaptureCallback m_capture_callback = nullptr;
    RawCaptureFuncType   m_capture_func     = nullptr;
    RawCaptureThreadType m_capture_thread   = nullptr;
    uint32_t             m_wait_interval    = 0;
    uint32_t             m_chunk_bytes      = 0;
    int                  m_iav_fd           = -1;
    RawAudioSampleSpec   m_sample_spec;
    RawAudioSampleSpec   m_def_sample_spec;
    std::atomic<bool>    m_run              = {false};
    std::atomic<bool>    m_wait             = {true};
    std::atomic<bool>    m_started          = {false};
    std::atomic<bool>    m_init_done        = {false};
    std::atomic<bool>    m_need_convert     = {false};
    std::string          m_cap_thread_name  = std::string("AudioCapRaw");
    std::string          m_context_name;
    AMMemLock            m_lock;
};

#endif /* AM_AUDIO_CAPTURE_RAW_H_ */
