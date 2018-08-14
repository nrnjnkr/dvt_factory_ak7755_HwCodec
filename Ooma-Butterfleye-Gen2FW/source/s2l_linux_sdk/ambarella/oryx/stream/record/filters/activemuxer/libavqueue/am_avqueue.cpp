/*******************************************************************************
 * am_avqueue.cpp
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
#include "am_base_include.h"
#include <algorithm>
#include "am_log.h"
#include "am_define.h"
#include "am_thread.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_packet.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet_pool.h"
#include "h265.h"

#include "am_record_event_sender.h"
#include "am_ring_queue.h"
#include "am_avqueue.h"

#define PTS_SCALE 90000
#define EVENT_MAX_QUE_SIZE 3
#define HW_TIMER  ((const char*)"/proc/ambarella/ambarella_hwtimer")
enum {
  VIDEO_MAX_NUM = 4,
  AUDIO_MAX_NUM = 16,
};

using namespace std::placeholders;

static std::mutex m_mutex;

AMAVQueuePtr AMAVQueue::create(AVQueueConfig &config,
                               std::string name,
                               bool start_send_normal_pkt,
                               recv_cb_t callback)
{
  AMAVQueue *result = new AMAVQueue();
  if (result && (result->construct(config, name, start_send_normal_pkt,
                                   callback) != AM_STATE_OK)) {
    delete result;
    result = nullptr;
  }
  return AMAVQueuePtr(result, [](AMAVQueue *p){delete p;});
}

AM_STATE AMAVQueue::construct(AVQueueConfig &config,
                              std::string name,
                              bool start_send_normal_pkt,
                              recv_cb_t callback)
{
  AM_STATE state = AM_STATE_OK;
  do {
    m_name = name + std::string("-avqueue");
    if (!(m_recv_cb = callback)) {
      state = AM_STATE_NOT_SUPPORTED;
      ERROR("Callback is nullptr in %s, please check!", m_name.c_str());
      break;
    }
    m_config = config;
    m_v_id = m_config.video_id;
    m_is_sending_normal_pkt = start_send_normal_pkt;
    std::string pool_name = m_name + "Pool";
    if (!(m_packet_pool = AMAVQueuePacketPool::create(this, pool_name.c_str(),
                                               m_config.pkt_pool_size))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create avqueue packet pool in %s!", m_name.c_str());
      break;
    }
    if (!(m_event_wait = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent in %s!", m_name.c_str());
      break;
    }
    if (!(m_send_normal_pkt_wait = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent in %s!",  m_name.c_str());
      break;
    }
    std::string normal_thread_name = m_name + std::string("normal_pkt_thread");
    if (!(m_send_normal_pkt_thread = AMThread::create(normal_thread_name.c_str(),
                                               static_send_normal_packet_thread,
                                               this))) {
      state = AM_STATE_OS_ERROR;
      ERROR("Failed to create send_normal_packet_thread!");
      break;
    }
    std::string event_thread_name = m_name + std::string("event_pkt_thread");
    if (!(m_send_event_pkt_thread = AMThread::create(event_thread_name.c_str(),
                                              static_send_event_packet_thread,
                                              this))) {
      state = AM_STATE_OS_ERROR;
      ERROR("Failed to create send_event_packet_thread in %s!", m_name.c_str());
      break;
    }
  } while (0);
  return state;
}

AM_STATE AMAVQueue::process_packet(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;

  if (!m_stop.load()) {
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        packet->add_ref();
        state = on_info(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        packet->add_ref();
        state = on_data(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EVENT: {
        packet->add_ref();
        state = on_event(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        packet->add_ref();
        state = on_eos(packet);
      } break;
      default:
        break;
    }
  }
  packet->release();
  return state;
}

void AMAVQueue::release_packet(AMPacket *packet)
{
  uint32_t stream_id = packet->get_stream_id();
  ExPayload *payload = dynamic_cast<ExPayload*>(packet->get_payload());
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        m_v_event_que.front()->second->event_release(payload);
        if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS)) {
          if (!m_v_event_que.front()->second->event_in_use()) {
            std::unique_lock<std::mutex> lk(m_event_eos_mutex);
            remove_video_event_front_que();
            m_event_eos_cond.notify_one();
          } else {
            ERROR("Release video[%u] eos, but event in use.",
                  packet->get_stream_id());
          }
        }
        if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_STOP) != 0) {
          if (!m_v_event_que.front()->second->event_in_use()) {
            m_in_event = false;
          }
        }
      } else {
        m_v_normal_que.front()->second->normal_release(payload);
        if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
          if (!m_v_normal_que.front()->second->normal_in_use()) {
            std::unique_lock<std::mutex> lk(m_normal_eos_mutex);
            remove_video_normal_front_que();
            m_normal_eos_cond.notify_one();
          } else {
            ERROR("Release video[%u] eos, but normal in use.",
                  packet->get_stream_id());
          }
        }
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      AMRingQueue* audio_que = nullptr;
      audio_que = get_audio_ring_que(stream_id);
      if (audio_que) {
        if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
          audio_que->event_release(payload);
        } else {
          audio_que->normal_release(payload);
        }
      } else {
        ERROR("Can not find audio[%u] in %s", stream_id, m_name.c_str());
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR : {
      if (m_g_que.second) {
        if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
          m_g_que.second->event_release(payload);
        } else {
          m_g_que.second->normal_release(payload);
        }
      } else {
        ERROR("Can not find gsensor[%u] in %s", stream_id, m_name.c_str());
      }
    } break;
    default: break;
  }
}

void AMAVQueue::static_send_normal_packet_thread(void *p)
{
  ((AMAVQueue*)p)->send_normal_packet_thread();
}

void AMAVQueue::static_send_event_packet_thread(void *p)
{
  ((AMAVQueue*)p)->send_event_packet_thread();
}

void AMAVQueue::send_normal_packet_thread()
{
  if (!m_is_sending_normal_pkt.load()) {
    m_send_normal_pkt_wait->wait();
  }
  uint32_t count = 0;
  while (true) {
    //Send normal packets
    if (!m_stop.load()) {
      if (send_normal_packet() != AM_STATE_OK) {
        break;
      }
    } else {
      uint32_t audio_count = 0;
      bool need_break = true;
      {
        AUTO_MEM_LOCK(m_v_que_lock);
        while(!m_v_normal_que.empty()) {
          if (!m_v_normal_que.front()->second->in_use()) {
            if ((-- m_v_normal_que.front()->first) == 0) {
              AM_DESTROY(m_v_normal_que.front()->second);
              delete m_v_normal_que.front();
            }
            m_v_normal_que.pop_front();
            if ((-- m_v_normal_info_payload_que.front()->first) == 0) {
              delete m_v_normal_info_payload_que.front();
            }
            m_v_normal_info_payload_que.pop_front();
            if ((-- m_v_normal_info_que.front()->first) == 0) {
              delete m_v_normal_info_que.front();
            }
            m_v_normal_info_que.pop_front();
          } else {
            need_break = false;
            break;
          }
        }
        while(!m_v_event_que.empty()) {//clear all old ring queues.
          if (!m_v_event_que.front()->second->in_use()) {
            if ((-- m_v_event_que.front()->first) == 0) {
              AM_DESTROY(m_v_event_que.front()->second);
              delete m_v_event_que.front();
            }
            m_v_event_que.pop_front();
            if ((-- m_v_event_info_payload_que.front()->first) == 0) {
              delete m_v_event_info_payload_que.front();
            }
            m_v_event_info_payload_que.pop_front();
            if ((-- m_v_event_info_que.front()->first) == 0) {
              delete m_v_event_info_que.front();
            }
            m_v_event_info_que.pop_front();
          } else {
            need_break = false;
            break;
          }
        }
      }
      {
        AUTO_MEM_LOCK(m_a_queue_lock);
        for (auto &m : m_a_que) {
          if (!m.second->in_use()) {
            ++ audio_count;
          }
        }
        if (audio_count != m_a_que.size()) {
          need_break = false;
        }
      }
      if (m_g_que.second) {
        if (m_g_que.second->in_use()) {
          need_break = false;
        }
      }
      if (need_break) {
        break;
      } else {
        usleep(50000);
        if (++ count > 20) {
          NOTICE("Count is bigger than 20, exit send normal pkt thread in %s",
                 m_name.c_str());
          break;
        }
      }
    }
  }
  INFO("%s exits!", m_send_normal_pkt_thread->name());
}

void AMAVQueue::send_event_packet_thread()
{
  while (true) {
    //Send event packets
    if (m_in_event.load() && (!m_event_v_end)) {
      if (m_event_pkt != nullptr) {
        if (process_h26x_event(m_event_pkt) != AM_STATE_OK) {
          ERROR("Process h26x event error.");
          m_in_event = false;
          m_event_pkt->release();
          m_event_pkt = nullptr;
          continue;
        }
        m_event_pkt->release();
        m_event_pkt = nullptr;
      }
      if (send_event_packet() != AM_STATE_OK) {
        break;
      }
    } else {
      m_event_wait->wait();
    }
    if (m_stop.load()) {
      m_in_event = false;
      break;
    }
  };
  INFO("%s exits!", m_send_event_pkt_thread->name());
}

AM_STATE AMAVQueue::send_normal_packet()
{
  AM_STATE       state          = AM_STATE_OK;
  AM_PTS         min_pts        = INT64_MAX;//(0x7fffffffffffffffLL)
  AMPacket      *send_packet    = nullptr;
  AMRingQueue   *queue_ptr      = nullptr;
  AMPacket::AM_PAYLOAD_TYPE payload_type;
  AMPacket::AM_PAYLOAD_ATTR payload_attr;
  do {
    bool need_break = false;
    if (m_v_normal_que.size() > 0) {
      /* compare all kinds of data packets, find a minimum PTS packet to send */
      /* process audio data */
      uint32_t audio_queue_size = get_audio_queue_size();
      uint32_t audio_queue_count = 0;
      for (auto &audio_que_pair : m_a_que) {
        uint32_t    &audio_stream_id = audio_que_pair.first;
        AMRingQueue        *&audio_q = audio_que_pair.second;
        if ((++ audio_queue_count) > audio_queue_size) {
          break;
        }
        pts_map::iterator a_last_pts = m_a_last_pts.find(audio_stream_id);
        if (a_last_pts == m_a_last_pts.end()) {
          std::pair<pts_map::iterator, bool> ret =
              m_a_last_pts.emplace(std::make_pair(audio_stream_id, 0));
          a_last_pts = ret.first;
        }
        if (!m_a_info_sent[audio_stream_id]) {
          if (send_normal_info(AM_AVQUEUE_MEDIA_AUDIO, audio_stream_id)
              != AM_STATE_OK) {
            ERROR("Failed to send normal info pkt in avqueue.");
            state = AM_STATE_ERROR;
            break;
          }
          m_a_info_sent[audio_stream_id] = true;
        }
        if (m_a_normal_eos[audio_stream_id] == 1) {
          if (audio_q->empty(queue_mode::normal)) {
            continue;
          } else {
            m_a_normal_eos[audio_stream_id] = 0;
            m_send_a_state[audio_stream_id] = 0;
          }
        }
        {
          std::unique_lock<std::mutex> lk(m_send_mutex);
          if (audio_q->empty(queue_mode::normal)) {
            switch (m_send_a_state[audio_stream_id].load()) {
              case 0: //Normal
                m_send_a_state[audio_stream_id] = 1;
                a_last_pts->second += m_a_pts_increment[audio_stream_id] - 100;
                break;
              case 1: //Try to get audio
                break;
              case 2: //Need block to wait audio packet
                while (!m_stop && audio_q->empty(queue_mode::normal)) {
                  m_a_send_block.first = audio_stream_id;
                  m_a_send_block.second = true;
                  m_send_cond.wait(lk);
                  m_a_send_block.second = false;
                }
                a_last_pts->second = audio_q->get()->m_data.m_payload_pts;
                m_send_a_state[audio_stream_id] = 0;
                break;
              default:
                break;
            }
          } else {
            a_last_pts->second = audio_q->get()->m_data.m_payload_pts;
            if (m_send_a_state[audio_stream_id] > 0) {
              m_send_a_state[audio_stream_id] = 0;
            }
          }
        }
        if (m_stop) {
          break;
        }
        if (a_last_pts->second < min_pts) {
          min_pts = a_last_pts->second;
          queue_ptr = audio_q;
        }
      }
      if (m_stop) {
        break;
      }
      /*process gsensor data*/
      if (m_g_info.first) {
        if (!m_g_info_sent) {
          if (send_normal_info(AM_AVQUEUE_MEDIA_GSENSOR, m_g_que.first)
              != AM_STATE_OK) {
            ERROR("Failed to send normal info pkt in avqueue.");
            state = AM_STATE_ERROR;
            break;
          }
          m_g_info_sent = true;
        }
        do {
          if (m_g_normal_eos == 1) {
            if (m_g_que.second->empty(queue_mode::normal)) {
              break;
            } else {
              m_g_normal_eos = 0;
            }
          }
          std::unique_lock<std::mutex> lk(m_send_mutex);
          if (m_g_que.second->empty(queue_mode::normal)) {
            switch (m_send_g_state) {
              case 0: //Normal
                m_send_g_state = 1;
                m_g_last_pts += m_g_pts_increment - 100;
                break;
              case 1: //Try to get gsensor
                break;
              case 2: //Need block to wait gsensor packet
                while (!m_stop && m_g_que.second->empty(queue_mode::normal)) {
                  m_g_send_block.first = m_g_que.first;
                  m_g_send_block.second = true;
                  m_send_cond.wait(lk);
                  m_g_send_block.second = false;
                }
                m_g_last_pts = m_g_que.second->get()->m_data.m_payload_pts;
                m_send_g_state = 0;
                break;
              default:
                break;
            }
          } else {
            m_g_last_pts = m_g_que.second->get()->m_data.m_payload_pts;
            if (m_send_g_state) {
              m_send_g_state = 0;
            }
          }
        } while(0);
        if (m_stop) {
          break;
        }

        if (m_g_last_pts < min_pts) {
          min_pts = m_g_last_pts;
          queue_ptr = m_g_que.second;
        }
      }
      /*process video data*/
      if (!m_v_info_sent) {
        if (send_normal_info(AM_AVQUEUE_MEDIA_VIDEO, m_v_id) != AM_STATE_OK) {
          ERROR("Failed to send normal info pkt in avqueue.");
          state = AM_STATE_ERROR;
          break;
        }
        m_v_info_sent = true;
      }
      {
        std::unique_lock<std::mutex> lk(m_send_mutex);
        while (!m_stop &&
               m_v_normal_que.front()->second->empty(queue_mode::normal)) {
          m_v_send_block.first = m_v_id;
          m_v_send_block.second = true;
          m_send_cond.wait(lk);
          m_v_send_block.second = false;
        }
      }
      if (m_stop) {
        break;
      }
      if (m_v_normal_que.front()->second->get()->m_data.m_payload_pts < min_pts) {
        min_pts = m_v_normal_que.front()->second->get()->m_data.m_payload_pts;
        queue_ptr = m_v_normal_que.front()->second;
      }
      /*process audio data*/
      audio_queue_count = 0;
      for (auto &audio_que_pair : m_a_que) {
        if ((++ audio_queue_count) > audio_queue_size) {
          break;
        }
        if ((queue_ptr == audio_que_pair.second) &&
            (m_send_a_state[audio_que_pair.first] > 0)) {
          m_send_a_state[audio_que_pair.first] = 2; //Need block to wait
          need_break = true;
          break;
        }
      }
      if (need_break) {
        break;
      }
      if ((queue_ptr == m_g_que.second) &&
          m_send_g_state) {
        m_send_g_state = 2; //Need block to wait
        break;
      }
      if (queue_ptr) {
        if (!m_packet_pool->alloc_packet(send_packet)) {
          WARN("Failed to allocate packet!");
          state = AM_STATE_ERROR;
          break;
        }
        send_packet->set_payload(queue_ptr->get());
        payload_type = send_packet->get_type();
        payload_attr = send_packet->get_attr();
        queue_ptr->read_pos_inc(queue_mode::normal);
        if (payload_type == AMPacket::AM_PAYLOAD_TYPE_EOS) {//wait eos pkt release
          std::unique_lock<std::mutex> lk(m_normal_eos_mutex);
          switch (payload_attr) {
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
              m_recv_cb(send_packet);
              m_v_info_sent = false;
              m_normal_eos_cond.wait(lk);
              break;
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
              m_a_normal_eos[send_packet->get_stream_id()] = 1;
              NOTICE("Send audio[%u] eos to muxer in %s",
                    send_packet->get_stream_id(), m_name.c_str());
              lk.unlock();
              m_recv_cb(send_packet);
              break;
            case AMPacket::AM_PAYLOAD_ATTR_GSENSOR:
              m_g_normal_eos = 1;
              lk.unlock();
              m_recv_cb(send_packet);
              NOTICE("Send gsensor eos to muxer in %s.", m_name.c_str());
              break;
            default:
              break;
          }
        } else {
          m_recv_cb(send_packet);
        }
      } else {
        usleep(30000);
      }
    } else {
      usleep(30000);
    }
  } while(0);
  return state;
}

AM_STATE AMAVQueue::send_event_packet()
{
  AM_STATE state = AM_STATE_OK;
  do {
    AM_PTS       min_pts        = INT64_MAX;
    AMPacket    *send_packet    = nullptr;
    ExPayload   *send_payload   = nullptr;
    AMRingQueue *queue_ptr      = nullptr;
    bool need_break = false;
    /*process video*/
    if (m_v_event_que.size() > 0) {
      if (send_event_info() != AM_STATE_OK) {
        state = AM_STATE_ERROR;
        ERROR("Failed to send event information in %s!", m_name.c_str());
        break;
      }
      while (!m_stop.load() &&
          m_v_event_que.front()->second->empty(queue_mode::event)) {
        std::unique_lock<std::mutex> lk(m_event_send_mutex);
        m_event_v_block = true;
        m_event_send_cond.wait(lk);
        m_event_v_block = false;
      }
      if (m_stop.load()) {break;}

      send_payload = m_v_event_que.front()->second->event_get();
      if (send_payload->m_data.m_payload_pts < min_pts) {
        min_pts = send_payload->m_data.m_payload_pts;
        queue_ptr = m_v_event_que.front()->second;
      }
      /*process audio*/
      do {
        AMRingQueue *audio_ring_que = get_audio_ring_que(m_event_a_id);
        if (audio_ring_que && m_event_a_info_sent) {
          if (m_a_event_eos == 1) {
            if (audio_ring_que->empty(queue_mode::event)) {
              break;
            } else {
              m_a_event_eos = 0;
            }
          }
          while (!m_stop.load() && audio_ring_que->empty(queue_mode::event)) {
            std::unique_lock<std::mutex> lk(m_event_send_mutex);
            m_event_a_block = true;
            m_event_send_cond.wait(lk);
            m_event_a_block = false;
          }
          if (m_stop.load()) {
            need_break = true;
            break;
          }
          send_payload = audio_ring_que->event_get();
          if (send_payload->m_data.m_payload_pts < min_pts) {
            min_pts = send_payload->m_data.m_payload_pts;
            queue_ptr = audio_ring_que;
          }
        }
      } while(0);
      if (need_break) {
        break;
      }
      /*process G-sensor*/
      do {
        if (m_config.event_gsensor_enable && m_g_info.first &&
            m_event_g_info_sent) {
          if (m_g_event_eos == 1) {
            if (m_g_que.second->empty(queue_mode::event)) {
              break;
            } else {
              m_g_event_eos = 0;
            }
          }
          while (!m_stop.load() &&
              m_g_que.second->empty(queue_mode::event)) {
            std::unique_lock<std::mutex> lk(m_event_send_mutex);
            m_event_g_block = true;
            m_event_send_cond.wait(lk);
            m_event_g_block = false;
          }
          if (m_stop.load()) {
            need_break = true;
            break;
          }
          send_payload = m_g_que.second->event_get();
          if (send_payload->m_data.m_payload_pts < min_pts) {
            min_pts = send_payload->m_data.m_payload_pts;
            queue_ptr = m_g_que.second;
          }
        }
      } while(0);
      if (need_break) {
        break;
      }
      send_payload = queue_ptr->event_get();
      if (queue_ptr) {
        if (!m_packet_pool->alloc_packet(send_packet)) {
          WARN("Failed to allocate event packet in %s!", m_name.c_str());
          return AM_STATE_ERROR;
        }
        send_packet->set_payload(send_payload);
        if ((min_pts >= get_event_end_pts()) &&
            (send_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) &&
            (is_video_frame_last_nalu(send_payload))) {
          m_event_v_end = true;
          send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT |
                                       AMPacket::AM_PACKET_TYPE_STOP);
          INFO("Event stop PTS: %jd in %s.", min_pts, m_name.c_str());
        } else {
          send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
        }
        queue_ptr->read_pos_inc(queue_mode::event);
        if (send_packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {//wait eos pkt release
          std::unique_lock<std::mutex> lk(m_event_eos_mutex);
          switch (send_packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
              m_recv_cb(send_packet);
              m_event_v_info_sent = false;
              m_event_eos_cond.wait(lk);
              break;
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
              m_a_event_eos = 1;
              lk.unlock();
              m_recv_cb(send_packet);
              NOTICE("Send audio[%u] eos pkt to muxer in %s",
                     send_packet->get_stream_id(), m_name.c_str());
              break;
            case AMPacket::AM_PAYLOAD_ATTR_GSENSOR:
              m_g_event_eos = 1;
              lk.unlock();
              m_recv_cb(send_packet);
              NOTICE("Send gsensor eos pkt sent to muxer in %s.", m_name.c_str());
              break;
            default:
              break;
          }
        } else {
          m_recv_cb(send_packet);
        }
      }
    } else {
      usleep(30000);
      if (get_event_end_pts() < get_current_pts()) {
        m_event_v_end = true;
        m_in_event = false;
      }
    }
  } while (0);
  return state;
}

AM_STATE AMAVQueue::send_event_info()
{
  AM_STATE state = AM_STATE_OK;
  do {
    /*send video latest info*/
    if (!m_event_v_info_sent) {
      AMPacket *video_info_pkt = nullptr;
      if (!m_packet_pool->alloc_packet(video_info_pkt)) {
        WARN("Failed to allocate event packet in %s!", m_name.c_str());
        state = AM_STATE_ERROR;
        break;
      }
      video_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      video_info_pkt->set_payload(&m_v_event_info_payload_que.front()->second);
      video_info_pkt->set_data_ptr((uint8_t*)&m_v_event_info_que.front()->second);
      video_info_pkt->set_data_size(sizeof(AM_VIDEO_INFO));
      video_info_pkt->set_stream_id(m_v_id);
      m_recv_cb(video_info_pkt);
      m_event_v_info_sent = true;
    }
    /*send audio info*/
    if ((!m_event_a_info_sent) && get_audio_info(m_event_a_id)) {
      AMPacket *audio_info_pkt = nullptr;
      if (!m_packet_pool->alloc_packet(audio_info_pkt)) {
        WARN("Failed to allocate event packet in %s!", m_name.c_str());
        state = AM_STATE_ERROR;
        break;
      }
      audio_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      audio_info_pkt->set_payload(get_audio_info_payload(m_event_a_id));
      audio_info_pkt->set_data_ptr((uint8_t*)get_audio_info(m_event_a_id));
      audio_info_pkt->set_data_size(sizeof(AM_AUDIO_INFO));
      audio_info_pkt->set_stream_id(m_event_a_id);
      m_recv_cb(audio_info_pkt);
      m_event_a_info_sent = true;
    }
    /*send g-sensor info*/
    if (!m_event_g_info_sent) {
      if (m_config.event_gsensor_enable) {
        AMPacket *gsensor_info_pkt = nullptr;
        if (!m_packet_pool->alloc_packet(gsensor_info_pkt)) {
          WARN("Failed to allocate event packet in %s!", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        gsensor_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
        gsensor_info_pkt->set_payload(&m_g_info_payload);
        gsensor_info_pkt->set_data_ptr((uint8_t*)&m_g_info.second);
        gsensor_info_pkt->set_data_size(sizeof(AM_GSENSOR_INFO));
        gsensor_info_pkt->set_stream_id(m_g_id);
        m_recv_cb(gsensor_info_pkt);
        m_event_g_info_sent = true;
      }
    }
  } while (0);

  return state;
}

AM_STATE AMAVQueue::send_normal_info(AM_AVQUEUE_MEDIA_TYPE type, uint32_t stream_id)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    AMPacket *info_pkt = nullptr;
    if (!m_packet_pool->alloc_packet(info_pkt)) {
      WARN("Failed to allocate event packet in %s!", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    switch (type) {
      case AM_AVQUEUE_MEDIA_VIDEO : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(&m_v_normal_info_payload_que.front()->second);
        info_pkt->set_data_ptr((uint8_t*)&m_v_normal_info_que.front()->second);
        info_pkt->set_data_size(sizeof(AM_VIDEO_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      case AM_AVQUEUE_MEDIA_AUDIO : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(get_audio_info_payload(stream_id));
        info_pkt->set_data_ptr((uint8_t*)get_audio_info(stream_id));
        info_pkt->set_data_size(sizeof(AM_AUDIO_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      case AM_AVQUEUE_MEDIA_GSENSOR : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(&m_g_info_payload);
        info_pkt->set_data_ptr((uint8_t*)&m_g_info.second);
        info_pkt->set_data_size(sizeof(AM_GSENSOR_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      default : {
        ERROR("media type error.");
        ret = AM_STATE_ERROR;
        info_pkt->release();
        info_pkt = nullptr;
      } break;
    }
    if (info_pkt) {
      m_recv_cb(info_pkt);
    }
  }  while(0);
  return ret;
}

bool AMAVQueue::is_h265_IDR_first_nalu(ExPayload *video_payload,
                                       int32_t slices_per_info)
{
  bool ret = false;
  do {
    if (video_payload->m_data.m_frame_type != AM_VIDEO_FRAME_TYPE_IDR) {
      break;
    }
    if (video_payload->m_data.m_video_type != AM_STREAM_TYPE_H265) {
      break;
    }
    uint32_t frame_count = video_payload->m_data.m_frame_count;
    uint8_t slice_num = (uint8_t)((frame_count >> 24) & 0x000000ff);
    uint8_t slice_id = (uint8_t)((frame_count >> 16) & 0x000000ff);
    uint8_t tile_num = (uint8_t)((frame_count >> 8) & 0x000000ff);
    uint8_t tile_id = (uint8_t)((frame_count) & 0x000000ff);
    DEBUG("slice num : %u\n"
          " slice id : %u\n"
          " tile num : %u\n"
          "  tile id : %u", slice_num, slice_id, tile_num, tile_id);
    switch (slices_per_info) {
      case AM_VIDEO_BITS_INFO_PER_TILE : {
        ret = ((slice_id == 0) && (tile_id == 0));
      } break;
      case AM_VIDEO_BITS_INFO_PER_FRAME : {
        ret = ((slice_id == slice_num - 1) && (tile_id == tile_num -1));
      } break;
      default : {
        ret = (slice_id == slices_per_info - 1);
      } break;
    }
  } while(0);
  return ret;
}

bool AMAVQueue::is_video_frame_last_nalu(ExPayload *video_payload)
{
  bool ret = false;
  do {
    uint32_t frame_count = video_payload->m_data.m_frame_count;
    uint8_t slice_num = (uint8_t)((frame_count >> 24) & 0x000000ff);
    uint8_t slice_id = (uint8_t)((frame_count >> 16) & 0x000000ff);
    uint8_t tile_num = (uint8_t)((frame_count >> 8) & 0x000000ff);
    uint8_t tile_id = (uint8_t)((frame_count) & 0x000000ff);
    DEBUG("slice num : %u\n"
          " slice id : %u\n"
          " tile num : %u\n"
          "  tile id : %u", slice_num, slice_id, tile_num, tile_id);
    if ((slice_id == (slice_num - 1)) && (tile_id == (tile_num - 1))) {
      ret = true;
      break;
    }
  } while(0);
  return ret;
}

AM_STATE AMAVQueue::on_info(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t stream_id = packet->get_stream_id();
  AM_AUDIO_INFO *audio_info = nullptr;
  AM_VIDEO_INFO *video_info = nullptr;
  bool valid = false;
  do {
    if (!packet) {
      ERROR("Packet is null!");
      ret = AM_STATE_ERROR;
      break;
    }
    switch (packet->get_attr()) {
      case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
        video_info = (AM_VIDEO_INFO*)packet->get_data_ptr();
        if ((stream_id == m_config.video_id) &&
            ((video_info->type == AM_STREAM_TYPE_H264) ||
                (video_info->type == AM_STREAM_TYPE_H265))) {
          valid = true;
        }
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
        audio_info = (AM_AUDIO_INFO*)packet->get_data_ptr();
        for (auto &m : m_config.audio_types) {
          if (m.first == audio_info->type) {
            for (auto &v : m.second) {
              if (v == audio_info->sample_rate) {
                valid = true;
              }
            }
          }
        }
        if ((m_config.event_audio.first == audio_info->type) &&
            (m_config.event_audio.second == audio_info->sample_rate)) {
          valid = true;
        }
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
        valid = true;
      } break;
      default : break;
    }
    if (!valid) {
      break;
    }
    packet->add_ref();
    ret = process_info_pkt(packet);
  } while(0);
  packet->release();

  return ret;
}

AM_STATE AMAVQueue::process_info_pkt(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (!packet) {
      ERROR("Packet is null!");
      state = AM_STATE_ERROR;
      break;
    }
    uint32_t stream_id = packet->get_stream_id();
    AM_AUDIO_INFO *audio_info = nullptr;
    AM_VIDEO_INFO *video_info = nullptr;
    AM_GSENSOR_INFO *gsensor_info = nullptr;
    switch (packet->get_attr()) {
      case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
        video_info = (AM_VIDEO_INFO*)packet->get_data_ptr();
        switch (video_info->type) {
          case AM_STREAM_TYPE_H264:
            INFO("%s receive H264 INFO[%d]: Width: %d, Height: %d",
                 m_name.c_str(),
                 stream_id,
                 video_info->width,
                 video_info->height);
            break;
          case AM_STREAM_TYPE_H265:
            INFO("%s receive H265 INFO[%d]: Width: %d, Height: %d",
                 m_name.c_str(),
                 stream_id,
                 video_info->width,
                 video_info->height);
            break;
          case AM_STREAM_TYPE_MJPEG:
            INFO("%s receive MJPEG INFO[%d]: Width: %d, Height: %d",
                 m_name.c_str(),
                 stream_id,
                 video_info->width,
                 video_info->height);
            break;
          default:
            state = AM_STATE_ERROR;
            ERROR("Unknown video type: %d in %s!",
                  video_info->type, m_name.c_str());
            break;
        }
        if (state != AM_STATE_OK) {
          break;
        }
        uint32_t video_payload_count = m_config.persistent_buf_duration *
            ((video_info->scale * video_info->mul) /
                (video_info->rate * video_info->div));
        if (m_config.event_enable) {
          video_payload_count += (m_config.event_max_history_duration *
              ((video_info->scale * video_info->mul) /
                  (video_info->rate * video_info->div)));
        }
        video_payload_count *= (video_info->slice_num * video_info->tile_num);
        AMRingQueue *ring_queue = AMRingQueue::create(video_payload_count);
        if (!ring_queue) {
          ERROR("Failed to create video queue[%d] in %s!",
                stream_id, m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        ring_que_pair *video_q_pair = new ring_que_pair();
        if (!video_q_pair) {
          ERROR("Failed to create ring queue pair in %s.", m_name.c_str());
          state =AM_STATE_ERROR;
          break;
        }
        video_q_pair->first = 2;//ref count
        video_q_pair->second = ring_queue;
        while((m_v_event_que.size() > EVENT_MAX_QUE_SIZE) &&
            (!m_v_event_que.front()->second->in_use())) {
          remove_video_event_front_que();
        }
        vinfo_pair *tmp_info_pair = new vinfo_pair();
        if (!tmp_info_pair) {
          ERROR("Failed to create video info pair in %s", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        memcpy(&tmp_info_pair->second, video_info, sizeof(AM_VIDEO_INFO));
        tmp_info_pair->first = 2;
        payload_pair *tmp_payload_pair = new payload_pair();
        if (!tmp_payload_pair) {
          ERROR("Failed to create payload pair in %s", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        tmp_payload_pair->second = *packet->get_payload();
        tmp_payload_pair->first = 2;
        {
          AUTO_MEM_LOCK(m_v_que_lock);
          m_v_normal_info_que.push_back(tmp_info_pair);
          m_v_event_info_que.push_back(tmp_info_pair);
          m_v_normal_info_payload_que.push_back(tmp_payload_pair);
          m_v_event_info_payload_que.push_back(tmp_payload_pair);
          m_v_normal_que.push_back(video_q_pair);
          m_v_event_que.push_back(video_q_pair);
        }
        INFO("Video Queue[%d] count: %d in %s", stream_id, video_payload_count
             , m_name.c_str());
        m_write_v_state = 0;
        m_first_v_pts = 0;
      } break;

      case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
        if (get_audio_ring_que(stream_id)) {
          NOTICE("The audio[%u] ring queue is exist.", stream_id);
          break;
        }
        audio_info = (AM_AUDIO_INFO*)packet->get_data_ptr();
        bool valid_type = false;
        for (auto &m : m_config.audio_types) {
          if (m.first == audio_info->type) {
            for (auto &v : m.second) {
              if (v == audio_info->sample_rate) {
                valid_type = true;
              }
            }
          }
        }
        if ((m_config.event_audio.first == audio_info->type) &&
            (m_config.event_audio.second == audio_info->sample_rate)) {
          m_event_a_id = packet->get_stream_id();
          valid_type = true;
        }
        if (!valid_type) {
          break;
        }
        INFO("%s : Audio[%d]: channels: %d, sample rate: %d, "
            "chunk size: %d, pts_increment: %d, sample size : %d",
            m_name.c_str(),
            stream_id,
            audio_info->channels,
            audio_info->sample_rate,
            audio_info->chunk_size,
            audio_info->pkt_pts_increment,
            audio_info->sample_size);
        if (m_a_pts_increment[stream_id] == 0) {
          m_a_pts_increment[stream_id] = audio_info->pkt_pts_increment;
          NOTICE("audio[%u] pkt pts inc %u",
                 stream_id,
                 m_a_pts_increment[stream_id]);
        }
        uint32_t audio_payload_count = m_config.persistent_buf_duration *
            (PTS_SCALE / audio_info->pkt_pts_increment);
        stream_id_payload_pair * payload_pair = new stream_id_payload_pair();
        if (!payload_pair) {
          ERROR("Failed to create audio payload pair in %s", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        payload_pair->first = stream_id;
        payload_pair->second = *packet->get_payload();
        if (m_config.event_enable &&
            (m_config.event_audio.first == audio_info->type) &&
            (m_config.event_audio.second == audio_info->sample_rate)) {
          audio_payload_count +=  (PTS_SCALE / audio_info->pkt_pts_increment) *
              m_config.event_max_history_duration;
        }
        stream_que_pair audio_queue_pair;
        if (!(audio_queue_pair.second =
            AMRingQueue::create(audio_payload_count))) {
          ERROR("Failed to create audio queue[%d] in %s!",
                stream_id, m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        audio_queue_pair.first = stream_id;
        INFO("Audio Queue[%d] count: %d in %s",
             stream_id, audio_payload_count, m_name.c_str());
        ainfo_pair *audio_info_pair = new ainfo_pair();
        if (!audio_info_pair) {
          ERROR("Failed to create audio info pair");
          state = AM_STATE_ERROR;
          break;
        }
        audio_info_pair->first = stream_id;
        memcpy(&audio_info_pair->second, audio_info, sizeof(AM_AUDIO_INFO));
        m_send_a_state[stream_id] = 0;
        m_write_a_state[stream_id] = 0;
        m_a_queue_lock.lock();
        m_a_info_payload.push_back(payload_pair);
        m_a_info_sent[stream_id] = false;
        m_a_info_que.push_back(audio_info_pair);
        m_a_que.push_back(audio_queue_pair);
        m_a_queue_lock.unlock();
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
        if (m_g_info.first) {
          NOTICE("The gsensor info is exist");
          break;
        }
        gsensor_info = (AM_GSENSOR_INFO*)packet->get_data_ptr();
        memcpy(&m_g_info.second, gsensor_info, sizeof(AM_GSENSOR_INFO));
        INFO("%s : gsensor[%d]: sample rate: %d,"
            "sample size: %d, pkt_pts_increment: %d",
            m_name.c_str(),
            stream_id,
            gsensor_info->sample_rate,
            gsensor_info->sample_size,
            gsensor_info->pkt_pts_increment);
        m_g_pts_increment = gsensor_info->pkt_pts_increment;
        uint32_t gsensor_payload_count = m_config.persistent_buf_duration *
            (PTS_SCALE / gsensor_info->pkt_pts_increment);
        m_g_info_payload = *packet->get_payload();
        m_g_info_sent = false;
        m_g_id = packet->get_stream_id();
        if (m_config.event_gsensor_enable) {
          gsensor_payload_count += (PTS_SCALE / gsensor_info->pkt_pts_increment) *
              m_config.event_max_history_duration;
        }
        if (!(m_g_que.second =
            AMRingQueue::create(gsensor_payload_count))) {
          ERROR("Failed to create gsensor queue[%d] in %s!",
                stream_id, m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
        INFO("gsensor Queue[%d] count: %d in %s",
             stream_id, gsensor_payload_count, m_name.c_str());
        m_send_g_state = 0;
        m_write_g_state = 0;
        m_g_info.first = true;
      } break;
      default: {
        ERROR("Packet attr error in %s.", m_name.c_str());
      } break;
    }
  } while(0);
  packet->release();
  return state;
}

AM_STATE AMAVQueue::on_data(AMPacket* packet)
{
  std::unique_lock<std::mutex> lk(m_mutex);
  AM_STATE state = AM_STATE_OK;
  do {
    if (packet->get_pts() < 0) {
      ERROR("Packet pts: %jd in %s!", packet->get_pts(), m_name.c_str());
      state = AM_STATE_ERROR;
      break;
    }
    queue_mode event_mode;
    uint32_t stream_id = packet->get_stream_id();
    AMPacket::Payload *payload = packet->get_payload();
    switch (packet->get_attr()) {
      case AMPacket::AM_PAYLOAD_ATTR_SEI: {
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_GPS: {
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
        if (!m_g_info.first) {
          break;
        }
        event_mode = (m_in_event && m_config.event_gsensor_enable) ?
            queue_mode::event : queue_mode::normal;

        switch (m_write_g_state) {
          case 1: //gsensor queue is full, wait for sending
            if (m_g_que.second->full(event_mode) &&
                  (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              break;
            } else {
              m_write_g_state = 0;
              WARN("gsensor[%d] stop dropping packet in %s!", stream_id,
                   m_name.c_str());
            }
          case 0: { //Normal write
            if (m_g_que.second->full(event_mode) &&
                (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              WARN("gsensor[%d] queue is full, start to drop packet in %s",
                   stream_id, m_name.c_str());
              m_write_g_state = 1;
            } else {
              if (!m_g_que.second->write(payload, event_mode)) {
                ERROR("Failed to write payload to gsensor queue[%d] in %s",
                      stream_id, m_name.c_str());
                state = AM_STATE_ERROR;
                break;
              }

              {
                std::unique_lock<std::mutex> lk(m_send_mutex);
                if (m_g_send_block.second &&
                    (m_g_send_block.first == stream_id)) {
                  m_send_cond.notify_one();
                }
              }

              {
                std::unique_lock<std::mutex> lk(m_event_send_mutex);
                if (m_event_g_block && (stream_id == m_g_id)) {
                  m_event_send_cond.notify_one();
                }
              }
            }
          } break;
          default: break;
        }
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
        if (m_v_id != stream_id) {
          break;
        }
        if (m_v_normal_que.size() == 0) {
          state = AM_STATE_ERROR;
          ERROR("There is no video ring que in %s", m_name.c_str());
          break;
        }
        event_mode = m_in_event ?
            queue_mode::event : queue_mode::normal;

        switch (m_write_v_state) {
          case 1: //Video queue is full, wait for sending
            if (m_v_normal_que.back()->second->full(event_mode) &&
                (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              break;
            } else {
              m_write_v_state = 2;
              WARN("Video[%d] queue wait I frame in %s!",
                   stream_id, m_name.c_str());
            }
          case 2: //Video Queue is not full, wait I frame
            if ((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR) ||
                (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_I) ||
                (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              WARN("Video[%d] I frame or eos pkt comes, stop dropping packet in %s!",
                   stream_id, m_name.c_str());
              m_write_v_state = 0;
            } else {
              break;
            }
          case 0: //Normal write
            if (m_v_normal_que.back()->second->full(event_mode) &&
                (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              m_write_v_state = 1;
              WARN("Video[%d] queue is full, start to drop packet in %s",
                   stream_id, m_name.c_str());
            } else {
              if (!m_v_normal_que.back()->second->write(payload, event_mode)) {
                ERROR("Failed to write payload to video queue[%d] in %s!",
                      stream_id, m_name.c_str());
                state = AM_STATE_ERROR;
                break;
              }

              if (m_first_v_pts == 0) {
                m_first_v_pts = packet->get_pts();
              }

              {
                std::unique_lock<std::mutex> lk(m_send_mutex);
                if (m_v_send_block.second &&
                    (m_v_send_block.first == stream_id)) {
                  m_send_cond.notify_one();
                }
              }

              {
                std::unique_lock<std::mutex> lk(m_event_send_mutex);
                if (m_event_v_block && (m_v_id == stream_id)) {
                  m_event_send_cond.notify_one();
                }
              }
            }
            break;
          default: break;
        }
      } break;

      case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
        AMRingQueue* audio_ring_que = get_audio_ring_que(stream_id);
        if (!audio_ring_que) {
          break;
        }
        event_mode = (m_in_event && (stream_id == m_event_a_id)) ?
            queue_mode::event : queue_mode::normal;
        switch (m_write_a_state[stream_id]) {
          case 1: //Audio queue is full, wait for sending
            if (audio_ring_que->full(event_mode) &&
                (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              break;
            } else {
              m_write_a_state[stream_id] = 0;
              WARN("Audio[%d] stop dropping packet in %s!", stream_id,
                   m_name.c_str());
            }
          case 0: { //Normal write
            if (audio_ring_que->full(event_mode) &&
                (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_EOS)) {
              WARN("Audio[%d] queue is full, start to drop packet in %s",
                   stream_id, m_name.c_str());
              m_write_a_state[stream_id] = 1;
            } else {
              if (!audio_ring_que->write(payload, event_mode)) {
                ERROR("Failed to write payload to audio queue[%d] in %s",
                      stream_id, m_name.c_str());
                state = AM_STATE_ERROR;
                break;
              }

              {
                std::unique_lock<std::mutex> lk(m_send_mutex);
                if (m_a_send_block.second &&
                    (m_a_send_block.first == stream_id)) {
                  m_send_cond.notify_one();
                }
              }

              {
                std::unique_lock<std::mutex> lk(m_event_send_mutex);
                if (m_event_a_block && (stream_id == m_event_a_id)) {
                  m_event_send_cond.notify_one();
                }
              }
            }
          } break;
          default: break;
        }
      } break;

      default:
        ERROR("Invalid data type in %s!", m_name.c_str());
        state = AM_STATE_ERROR;
        break;
    }
  } while(0);
  packet->release();
  return state;
}

AM_STATE AMAVQueue::process_h26x_event(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  do {
    AMEventStruct *event = (AMEventStruct*)(packet->get_data_ptr());
    AMRingQueue *audio_ring_que = nullptr;
    if (event->h26x.history_duration > m_config.event_max_history_duration) {
      event->h26x.history_duration = m_config.event_max_history_duration;
    }
    if (m_v_event_que.size() > 0) {
      while(m_v_event_que.size() > 1) {//clear all old ring queues.
        remove_video_event_front_que();
      }
      m_v_event_que.front()->second->event_reset();
    } else {
      ERROR("There is no video event ring queue");
      state = AM_STATE_ERROR;
      break;
    }
    audio_ring_que = get_audio_ring_que(m_event_a_id);
    if (audio_ring_que) {
      audio_ring_que->event_reset();
    }
    if (m_config.event_gsensor_enable && m_g_que.second) {
      m_g_que.second->event_reset();
    }
    AM_PTS event_pts = packet->get_pts();
    INFO("Event occurrence PTS: %jd in %s.", event_pts, m_name.c_str());

    AM_PTS video_start_pts = m_first_v_pts;
    int64_t history_pts = (int64_t)(((uint64_t)event->h26x.history_duration) * PTS_SCALE);
    int64_t future_pts = (int64_t)(((uint64_t)event->h26x.future_duration) * PTS_SCALE);
    if (event_pts > (video_start_pts + history_pts)) {
      video_start_pts = event_pts - history_pts;
    }
    set_event_end_pts(event_pts + future_pts);
    INFO("\n%s:\n"
          "video start pts is : %jd\n"
          "      event pts is : %jd\n"
          "    history pts is : %jd\n"
          "     future pts is : %jd\n"
          "  event_end pts is : %jd\n",
          m_name.c_str(), video_start_pts, event_pts, history_pts,
          future_pts, get_event_end_pts());
    AM_PTS video_pts = 0;
    AM_PTS audio_pts = 0;
    AM_PTS gsensor_pts = 0;
    ExPayload *video_payload = nullptr;
    ExPayload *audio_payload = nullptr;
    ExPayload *gsensor_payload = nullptr;

    //Set Video ReadPos
    while (!m_v_event_que.front()->second->event_empty()) {
      m_v_event_que.front()->second->event_backtrack();
      video_payload = m_v_event_que.front()->second->event_get();
      video_pts = video_payload->m_data.m_payload_pts;

      // Set Audio ReadPos
      if (audio_ring_que) {
        while (!audio_ring_que->event_empty()) {
          audio_payload = audio_ring_que->event_get_prev();
          audio_pts = audio_payload->m_data.m_payload_pts;
          if (audio_pts < video_pts) {
            break;
          }
          audio_ring_que->event_backtrack();
        }
      }
      // Set gsensor ReadPos
      if (m_config.event_gsensor_enable && m_g_que.second) {
        while (!m_g_que.second->event_empty()) {
          gsensor_payload = m_g_que.second->event_get_prev();
          gsensor_pts = gsensor_payload->m_data.m_payload_pts;
          if (gsensor_pts < video_pts) {
            break;
          }
          m_g_que.second->event_backtrack();
        }
      }
      if ((video_pts <= video_start_pts) &&
          (video_payload->m_data.m_frame_type == AM_VIDEO_FRAME_TYPE_IDR)) {
        if (m_v_event_info_que.front()->second.type == AM_STREAM_TYPE_H264) {
          break;
        } else if (m_v_event_info_que.front()->second.type == AM_STREAM_TYPE_H265) {
          if (is_h265_IDR_first_nalu(video_payload,
              m_v_event_info_que.front()->second.slices_per_info)) {
            break;
          }
        } else {
          ERROR("video type error in %s.", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
    INFO("Event: current audio PTS: %jd, video PTS: %jd, "
        "g-sensor pts: %jd in %s ",
         audio_pts, video_pts, gsensor_pts, m_name.c_str());
    packet->add_ref();
    m_recv_cb(packet);
    m_event_v_info_sent  = false;
    m_event_a_info_sent  = false;
    m_event_g_info_sent = false;
  } while(0);
  return state;
}

AM_STATE AMAVQueue::on_event(AMPacket *packet)
{
  std::unique_lock<std::mutex> lk(m_mutex);
  AM_STATE state = AM_STATE_OK;
  do {
    AMEventStruct *event = (AMEventStruct*)(packet->get_data_ptr());
    switch (event->attr) {
      case AM_EVENT_MJPEG : break;
      case AM_EVENT_PERIODIC_MJPEG : break;
      case AM_EVENT_H26X : {
        if (packet->get_stream_id() != m_config.video_id) {
          break;
        }
        INFO("%s receive event packet, event attr is AM_EVENT_H26X,"
            "stream id is %u, history duration is %u, future duration is %u,"
            "history voice duration is %d",
            m_name.c_str(), packet->get_stream_id(),
            event->h26x.history_duration, event->h26x.future_duration,
            event->h26x.history_voice_duration);
        if (is_ready_for_event(*event)) {
          m_event_pkt = packet;
          packet->add_ref();
          m_in_event = true;
          m_event_v_end = false;
          m_event_wait->signal();
        } else {
          NOTICE("%s is not ready for event.", m_name.c_str());
          break;
        }
      } break;
      case AM_EVENT_STOP_CMD : {
        if (packet->get_stream_id() == m_config.video_id) {
          if (m_in_event) {
            INFO("%s is in event, receive stop cmd, stop it", m_name.c_str());
            set_event_end_pts(packet->get_pts());
          } else {
            ERROR("%s is not in event now, ignore the stop cmd.", m_name.c_str());
            break;
          }
        }
      } break;
      default : {
        ERROR("Event attr error in %s.", m_name.c_str());
        state = AM_STATE_ERROR;
      } break;
    }
  } while(0);
  if (packet) {
    packet->release();
  }
  return state;
}

bool AMAVQueue::is_ready_for_event(AMEventStruct& event)
{
  bool ret = false;
  do {
    if ((event.attr == AM_EVENT_MJPEG) || (event.attr == AM_EVENT_PERIODIC_MJPEG)) {
      ret = true;
      break;
    } else if (event.attr == AM_EVENT_H26X) {
      if (m_in_event) {
        NOTICE("%s is in event", m_name.c_str());
        break;
      }
      if (!m_config.event_enable) {
        NOTICE("Event is not enabled in %s", m_name.c_str());
      }
      if (!((0x00000001 << m_v_id) & event.h26x.stream_id_bit_map)) {
        NOTICE("the video id[%u] is not included in stream id bit map [%u] in %s",
               m_config.video_id, event.h26x.stream_id_bit_map, m_name.c_str());
        break;
      }
      if (m_v_event_que.size() == 0) {
        NOTICE("Can not find video[%u] in event buffer in %s.",
              m_v_id, m_name.c_str());
        break;
      }
      if (m_event_a_id != 0xff) {
        if (!get_audio_ring_que(m_event_a_id)) {
          WARN("Can not find audio[%u] in event buffer in %s.",
               m_event_a_id.load(), m_name.c_str());
        }
      }
      if (m_config.event_gsensor_enable) {
        if (!m_g_info.first) {
          WARN("Can not find gsensor[%u] in event buffer in %s.",
                 m_g_id, m_name.c_str());
        }
      }
      ret = true;
    } else if (event.attr == AM_EVENT_STOP_CMD) {
      ret = (m_in_event && ((0x00000001 << m_v_id) & event.h26x.stream_id_bit_map));
    } else {
      ERROR("Event attr error in %s.", m_name.c_str());
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

void AMAVQueue::set_event_end_pts(int64_t pts)
{
  AUTO_MEM_LOCK(m_pts_lock);
  m_event_end_pts = pts;
}

int64_t AMAVQueue::get_event_end_pts()
{
  AUTO_MEM_LOCK(m_pts_lock);
  return m_event_end_pts;
}

uint32_t AMAVQueue::get_audio_queue_size()
{
  AUTO_MEM_LOCK(m_a_queue_lock);
  return m_a_que.size();
}

AMRingQueue* AMAVQueue::get_audio_ring_que(uint32_t stream_id)
{
  AUTO_MEM_LOCK(m_a_queue_lock);
  AMRingQueue* queue_ptr = nullptr;
  for (auto &audio_que_pair : m_a_que) {
    if (audio_que_pair.first == stream_id) {
      queue_ptr = audio_que_pair.second;
      break;
    }
  }
  return queue_ptr;
}

AM_AUDIO_INFO* AMAVQueue::get_audio_info(uint32_t stream_id)
{
  AUTO_MEM_LOCK(m_a_queue_lock);
  AM_AUDIO_INFO* audio_info_ptr = nullptr;
  for (auto &audio_info : m_a_info_que)
  {
    if (audio_info->first == stream_id) {
      audio_info_ptr = &audio_info->second;
      break;
    }
  }
  return audio_info_ptr;
}

AMPacket::Payload *AMAVQueue::get_audio_info_payload(uint32_t stream_id)
{
  AUTO_MEM_LOCK(m_a_queue_lock);
  AMPacket::Payload *payload = nullptr;
  for (auto &audio_info_payload_pair : m_a_info_payload) {
    if (audio_info_payload_pair->first == stream_id) {
      payload = &audio_info_payload_pair->second;
      break;
    }
  }
  return payload;
}

bool AMAVQueue::is_sending_normal_pkt()
{
  return m_is_sending_normal_pkt.load();
}

void AMAVQueue::remove_video_event_front_que()
{
  do {
    AUTO_MEM_LOCK(m_v_que_lock);
    if (m_v_event_que.size() == 0) {
      break;
    }
    if ((-- m_v_event_que.front()->first) == 0) {
      AM_DESTROY(m_v_event_que.front()->second);
      delete m_v_event_que.front();
    }
    m_v_event_que.pop_front();
    if ((-- m_v_event_info_payload_que.front()->first) == 0) {
      delete m_v_event_info_payload_que.front();
    }
    m_v_event_info_payload_que.pop_front();
    if ((-- m_v_event_info_que.front()->first) == 0) {
      delete m_v_event_info_que.front();
    }
    m_v_event_info_que.pop_front();
  } while(0);
}

void AMAVQueue::remove_video_normal_front_que()
{
  do {
    AUTO_MEM_LOCK(m_v_que_lock);
    if (m_v_normal_que.size() == 0) {
      break;
    }
    if ((-- m_v_normal_que.front()->first) == 0) {
      AM_DESTROY(m_v_normal_que.front()->second);
      delete m_v_normal_que.front();
    }
    m_v_normal_que.pop_front();
    if ((-- m_v_normal_info_payload_que.front()->first) == 0) {
      delete m_v_normal_info_payload_que.front();
    }
    m_v_normal_info_payload_que.pop_front();
    if ((-- m_v_normal_info_que.front()->first) == 0) {
      delete m_v_normal_info_que.front();
    }
    m_v_normal_info_que.pop_front();
  } while(0);
}

AM_PTS AMAVQueue::get_current_pts()
{
  uint8_t pts[32] = {0};
  AM_PTS current_pts = m_last_pts;
  do {
    if (m_hw_timer_fd  < 0) {
      m_hw_timer_fd = open(HW_TIMER, O_RDONLY);
      if (m_hw_timer_fd < 0) {
        PERROR("Open hardware timer error :");
        break;
      }
    }
    if (read(m_hw_timer_fd, pts, sizeof(pts)) < 0) {
      PERROR("read current pts error:");
      break;
    } else {
      current_pts = strtoull((const char*)pts, (char**)nullptr, 10);
      m_last_pts = current_pts;
    }
  } while(0);
  return current_pts;
}

bool AMAVQueue::start_send_normal_pkt()
{
  bool ret = true;
  do {
    if (m_is_sending_normal_pkt.load()) {
      ERROR("Send normal pkt function has been started already in %s.",
            m_name.c_str());
      break;
    } else {
      m_is_sending_normal_pkt = true;
      m_send_normal_pkt_wait->signal();
    }
  } while(0);
  return ret;
}

AM_STATE AMAVQueue::on_eos(AMPacket* packet)
{
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
      INFO("Video[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
      INFO("Audio[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR:
      INFO("gsensor[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    default:
      break;
  }
  return on_data(packet);
}

AMAVQueue::AMAVQueue()
{
  m_a_last_pts.clear();
  m_a_info_que.clear();
  m_g_info.first = false;
  m_a_que.clear();
  m_v_normal_que.clear();
  m_v_event_que.clear();
  m_g_que.first = 0;
  m_g_que.second = nullptr;
  m_v_send_block.first = 0;
  m_v_send_block.second = false;
  m_a_send_block.first = 0;
  m_a_send_block.second = false;
  m_g_send_block.first = 0;
  m_g_send_block.second = false;
  m_a_info_payload.clear();
}

AMAVQueue::~AMAVQueue()
{
  m_stop = true;
  if (m_event_pkt) {
    m_event_pkt->release();
  }
  m_send_cond.notify_one();
  m_send_normal_pkt_wait->signal();
  m_event_wait->signal();
  m_normal_eos_cond.notify_one();
  m_event_eos_cond.notify_one();
  m_event_send_cond.notify_one();
  AM_DESTROY(m_send_normal_pkt_thread);
  AM_DESTROY(m_send_event_pkt_thread);
  AM_DESTROY(m_send_normal_pkt_wait);
  AM_DESTROY(m_packet_pool);
  AM_DESTROY(m_event_wait);
  while(!m_v_normal_que.empty()) {
    remove_video_normal_front_que();
  }
  while(!m_v_event_que.empty()) {
    remove_video_event_front_que();
  }
  AM_DESTROY(m_g_que.second);
  for (auto &m : m_a_que) {
    AM_DESTROY(m.second);
  }
  for (auto &m : m_a_info_que) {
    delete m;
  }
  for (auto &m : m_a_info_payload) {
    delete m;
  }
  if (m_hw_timer_fd > 0) {
    close(m_hw_timer_fd);
  }
  INFO("%s is destroyed.", m_name.c_str());
}
