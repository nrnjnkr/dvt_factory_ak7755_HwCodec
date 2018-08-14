/*******************************************************************************
 * am_avqueue.h
 *
 * History:
 *   Sep 21, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_
#define ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_

#include <map>
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <condition_variable>

#include "am_sei_define.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_muxer_codec_info.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_queue.h"
using std::map;
using std::pair;
using std::vector;
using std::list;

struct ExPayload;
class AMAVQueue;
class AMRingQueue;
class AMAVQueuePacketPool;
struct AMEventStruct;
struct AM_VIDEO_INFO;

struct AVQueueConfig
{
    uint32_t      pkt_pool_size                     = 0;
    uint32_t      video_id                          = 0;
    //map<audio_type, vector<sample_rate>>
    map<AM_AUDIO_TYPE, vector<uint32_t>> audio_types;
    bool          event_enable                      = false;
    bool          event_gsensor_enable              = false;
    //pair<audio_type, sample_rate>
    pair<AM_AUDIO_TYPE, uint32_t> event_audio       = {AM_AUDIO_AAC, 48000};
    uint32_t      event_max_history_duration        = 0;
    uint32_t      persistent_buf_duration           = 0;
};

enum AM_AVQUEUE_MEDIA_TYPE{
  AM_AVQUEUE_MEDIA_NULL = -1,
  AM_AVQUEUE_MEDIA_VIDEO,
  AM_AVQUEUE_MEDIA_AUDIO,
  AM_AVQUEUE_MEDIA_GSENSOR,
};
#define AUDIO_STREAM_ID_MAX 32
typedef std::function<void(AMPacket *packet)> recv_cb_t;

typedef std::shared_ptr<AMAVQueue> AMAVQueuePtr;
class AMAVQueue final
{
    friend AMAVQueuePacketPool;
    typedef AMSafeDeque<AMPacket*>                    packet_queue;
    typedef pair<std::atomic_char, AMRingQueue*>      ring_que_pair;//<ref_count, AMRingQueue*>
    typedef pair<std::atomic_char, AMPacket::Payload> payload_pair;
    typedef pair<std::atomic_char, AM_VIDEO_INFO>     vinfo_pair;
    typedef pair<std::atomic_bool, AM_GSENSOR_INFO>   ginfo_pair;
    typedef AMSafeDeque<ring_que_pair*>               ring_queue;
    typedef pair<uint32_t, AM_AUDIO_INFO>             ainfo_pair;//<stream_id, audio_info>
    typedef pair<uint32_t, AMRingQueue*>              stream_que_pair;//<stream_id, AMRingQueue*>
    typedef pair<uint32_t, AMPacket::Payload>         stream_id_payload_pair;//<stream_id, payload>
    typedef pair<uint32_t, std::atomic_bool>          block_pair;
    typedef map<uint32_t, AM_PTS>                     pts_map;
    typedef AMSafeDeque<payload_pair*>                payload_pair_que;
  public:
    static AMAVQueuePtr create(AVQueueConfig &config,
                               std::string name,
                               bool start_send_normal_pkt,
                               recv_cb_t callback);

  public:
    AM_STATE process_packet(AMPacket *packet);
    bool is_ready_for_event(AMEventStruct& event);
    bool is_sending_normal_pkt();
    bool start_send_normal_pkt();
  private:
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_event(AMPacket *packet);
    inline AM_STATE on_eos(AMPacket *packet);

    inline AM_STATE send_normal_packet();
    inline AM_STATE send_event_packet();
    inline AM_STATE send_event_info();
    inline AM_STATE send_normal_info(AM_AVQUEUE_MEDIA_TYPE type,
                                     uint32_t stream_id);
    AM_STATE process_info_pkt(AMPacket *packet);
    void release_packet(AMPacket *packet);
    static void     static_send_normal_packet_thread(void *p);
    static void     static_send_event_packet_thread(void *p);
    void            send_normal_packet_thread();
    void            send_event_packet_thread();
    bool is_h265_IDR_first_nalu(ExPayload *video_payload,
                                int32_t slices_per_info);
    bool is_video_frame_last_nalu(ExPayload *video_payload);
    inline AM_STATE process_h26x_event(AMPacket *packet);
    inline void set_event_end_pts(int64_t pts);
    inline int64_t get_event_end_pts();
    inline uint32_t get_audio_queue_size();
    inline AMRingQueue* get_audio_ring_que(uint32_t stream_id);
    inline AM_AUDIO_INFO* get_audio_info(uint32_t stream_id);
    inline AMPacket::Payload *get_audio_info_payload(uint32_t stream_id);
    inline void remove_video_event_front_que();
    inline void remove_video_normal_front_que();
    inline AM_PTS get_current_pts();
  private:
    AMAVQueue();
    ~AMAVQueue();
    AM_STATE construct(AVQueueConfig &config,
                       std::string name,
                       bool start_send_normal_pkt,
                       recv_cb_t callback);

  private:
    AM_PTS                        m_event_end_pts          = 0;
    AM_PTS                        m_last_pts               = 0;
    AM_PTS                        m_g_last_pts             = 0;//gsensor
    AM_PTS                        m_first_v_pts            = 0;
    AMEvent                      *m_event_wait             = nullptr;
    AMEvent                      *m_send_normal_pkt_wait   = nullptr;
    AMThread                     *m_send_normal_pkt_thread = nullptr;
    AMThread                     *m_send_event_pkt_thread  = nullptr;
    AMAVQueuePacketPool          *m_packet_pool            = nullptr;
    AMPacket                     *m_event_pkt              = nullptr;
    recv_cb_t                     m_recv_cb                = nullptr;
    int                           m_hw_timer_fd            = -1;
    uint32_t                      m_g_pts_increment        = 0;//gsensor
    uint32_t                      m_v_id                   = 0;//video
    uint32_t                      m_g_id                   = 0;//gsensor
    uint32_t                      m_send_g_state           = 0;//gsensor
    uint32_t                      m_write_v_state          = 0;//video
    uint32_t                      m_write_g_state          = 0;//gsensor
    uint32_t                      m_a_pts_increment[AUDIO_STREAM_ID_MAX] = {0};//audio
    std::atomic<uint8_t>          m_send_a_state[AUDIO_STREAM_ID_MAX]    = {{0}};//audio
    std::atomic<uint8_t>          m_write_a_state[AUDIO_STREAM_ID_MAX]   = {{0}};//audio
    std::atomic<uint8_t>          m_a_normal_eos[AUDIO_STREAM_ID_MAX]    = {{0}};
    std::atomic<uint8_t>          m_a_event_eos            = {0};
    std::atomic<uint8_t>          m_g_normal_eos           = {0};
    std::atomic<uint8_t>          m_g_event_eos            = {0};
    std::atomic<uint8_t>          m_event_a_id             = {0xff};
    std::atomic_bool              m_event_v_end            = {false};//video
    std::atomic_bool              m_event_v_block          = {false};//video
    std::atomic_bool              m_event_a_block          = {false};//audio
    std::atomic_bool              m_event_g_block          = {false};//gsensor
    std::atomic_bool              m_v_info_sent            = {false};//video
    std::atomic_bool              m_event_v_info_sent      = {false};
    std::atomic_bool              m_event_a_info_sent      = {false};
    std::atomic_bool              m_event_g_info_sent      = {false};
    std::atomic_bool              m_g_info_sent            = {false};
    std::atomic_bool              m_a_info_sent[AUDIO_STREAM_ID_MAX] = {{false}};
    std::atomic_bool              m_is_sending_normal_pkt  = {true};
    std::atomic_bool              m_in_event               = {false};
    std::atomic_bool              m_stop                   = {false};
    AVQueueConfig                 m_config;
    std::mutex                    m_send_mutex;
    std::condition_variable       m_send_cond;
    std::mutex                    m_normal_eos_mutex;
    std::condition_variable       m_normal_eos_cond;
    std::mutex                    m_event_eos_mutex;
    std::condition_variable       m_event_eos_cond;
    pts_map                       m_a_last_pts;
    AMSafeDeque<vinfo_pair*>      m_v_normal_info_que;
    AMSafeDeque<vinfo_pair*>      m_v_event_info_que;
    list<ainfo_pair*>             m_a_info_que;
    ginfo_pair                    m_g_info;
    ring_queue                    m_v_normal_que;
    ring_queue                    m_v_event_que;
    list<stream_que_pair>         m_a_que;
    stream_que_pair               m_g_que;
    block_pair                    m_v_send_block;
    block_pair                    m_a_send_block;
    block_pair                    m_g_send_block;
    std::mutex                    m_event_send_mutex;
    std::condition_variable       m_event_send_cond;
    payload_pair_que              m_v_normal_info_payload_que;
    payload_pair_que              m_v_event_info_payload_que;
    list<stream_id_payload_pair*> m_a_info_payload;
    AMPacket::Payload             m_g_info_payload;
    std::string                   m_name;
    AMMemLock                     m_pts_lock;
    AMMemLock                     m_a_queue_lock;
    AMMemLock                     m_v_que_lock;
};

class AMAVQueuePacketPool final: public AMSimplePacketPool
{
    typedef AMSimplePacketPool inherited;

  public:
    static AMAVQueuePacketPool* create(AMAVQueue *q,
                                       const char *name,
                                       uint32_t count)
    {
      AMAVQueuePacketPool *result = new AMAVQueuePacketPool();
      if (result && (AM_STATE_OK != result->init(q, name, count))) {
        delete result;
        result = nullptr;
      }
      return result;
    }

    bool alloc_packet(AMPacket*& packet, uint32_t size = 0) override
    {
      return AMSimplePacketPool::alloc_packet(packet, size);
    }

  public:
    void on_release_packet(AMPacket *packet) override
    {
      if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA ||
          packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
        m_avqueue->release_packet(packet);
      }
    }

  private:
    AMAVQueuePacketPool(){};
    virtual ~AMAVQueuePacketPool(){};
    AM_STATE init(AMAVQueue *q, const char *name, uint32_t count)
    {
      AM_STATE state = inherited::init(name, count, sizeof(AMPacket));
      if (AM_STATE_OK == state) {
        m_avqueue = q;
      }
      return state;
    }

  private:
    AMAVQueue           *m_avqueue = nullptr;
    AMPacket::Payload   *m_payload = nullptr;
};

#endif /* ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_ */
