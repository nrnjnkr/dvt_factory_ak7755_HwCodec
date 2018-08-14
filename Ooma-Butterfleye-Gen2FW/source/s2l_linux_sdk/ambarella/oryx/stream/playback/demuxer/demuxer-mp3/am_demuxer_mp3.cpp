/*******************************************************************************
 * am_demuxer_mp3.cpp
 *
 * History:
 *   2017年9月11日 - [ypchang] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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

#include "am_file.h"
#include "am_mutex.h"
#include "am_audio_type.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_pool.h"

#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_demuxer_mp3.h"
#include "am_audio_define.h"

#include "am_queue.h"

#include "id3v2.h"
#include "mp3.h"

#define PACKET_POOL_SIZE   32
#define MAX_MP3_FRAME_SIZE 4096
#define FILE_BUFFER_SIZE   1024

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_MP3;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerMp3::create(streamid);
}

AMIDemuxerCodec* AMDemuxerMp3::create(uint32_t streamid)
{
  AMDemuxerMp3 *demuxer = new AMDemuxerMp3(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = nullptr;
  }

  return demuxer;
}

bool AMDemuxerMp3::is_drained()
{
  return (m_packet_pool->get_avail_packet_num() == PACKET_POOL_SIZE);
}

AM_DEMUXER_STATE AMDemuxerMp3::get_packet(AMPacket *&packet)
{
  AM_DEMUXER_STATE ret = AM_DEMUXER_OK;
  packet = nullptr;
  while(!packet) {
    if (AM_UNLIKELY(!m_media)) {
      m_need_to_read = true;
      m_avail_size = 0;
      m_sent_size = 0;
      m_is_new_file = (nullptr != (m_media = get_new_file()));
      m_is_tag_parsed = false;
      if (AM_UNLIKELY(!m_media)) {
        ret = AM_DEMUXER_NO_FILE;
        break;
      } else {
        m_remain_size = m_file_size;
      }
    }

    if (AM_LIKELY(m_media && (m_media->is_open() ||
                              m_media->open(AMFile::AM_FILE_READONLY)))) {
      MP3Header *mp3_hdr = nullptr;
      if (AM_UNLIKELY(!m_is_tag_parsed)) {
        int32_t tag_size = mp3_parse_tag(*m_media);
        if (AM_UNLIKELY(-1 == tag_size)) {
          ERROR("Invalid MP3 file: %s, skip!", m_media->name());
          delete m_media;
          m_media = nullptr;
          continue;
        } else {
          m_remain_size -= tag_size;
        }
        m_is_tag_parsed = true;
      }

      if (AM_LIKELY(m_need_to_read)) {
        ssize_t read_size = m_media->read_reliable(m_buffer, m_read_buf_size);
        if (AM_UNLIKELY(read_size <= 0)) {
          if (read_size < 0) {
            ERROR("%s: %s! Skip!", m_media->name(), strerror(errno));
          } else {
            INFO("%s EOF", m_media->name());
          }
          delete m_media;
          m_media = nullptr;
          continue;
        } else {
          m_sent_size = 0;
          m_avail_size = read_size;
          m_need_to_read = false;
        }
      }
      mp3_hdr = (MP3Header*)&m_buffer[m_sent_size];
      if (AM_LIKELY((m_avail_size > sizeof(MP3Header)) &&
                    mp3_hdr->is_sync_ok() &&
                    (m_avail_size >= mp3_hdr->frame_bytes()))) {
        if (AM_LIKELY(allocate_packet(packet))) {
          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->set_frame_type(AM_AUDIO_CODEC_MP3);
          packet->set_stream_id(m_stream_id);
          packet->set_pts(0LL);
          if (AM_UNLIKELY(m_is_new_file)) {
            AM_AUDIO_INFO *aInfo = ((AM_AUDIO_INFO*)packet->get_data_ptr());
            uint32_t frame_len = mp3_hdr->frame_bytes();
            uint32_t max_frame_count = (frame_len > 0) ?
                    (MAX_MP3_FRAME_SIZE / frame_len) : 1;
            uint32_t max_read_buf_size = max_frame_count * frame_len;
            memcpy(m_mp3_header, mp3_hdr, sizeof(*m_mp3_header));
            INFO("%s (Layer %s, Version %s, Bitrate %ukbps)",
                 m_media->name(),
                 m_mp3_header->layer_str(),
                 m_mp3_header->version_str(),
                 m_mp3_header->bitrate() / 1000);
            aInfo->channels = mp3_hdr->channel_number();
            aInfo->sample_rate = mp3_hdr->sample_rate();
            aInfo->codec_info = m_mp3_header;
            aInfo->type = AM_AUDIO_MP3;
            aInfo->chunk_size = max_read_buf_size;
            aInfo->sample_format = AM_SAMPLE_INVALID;
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
            packet->set_data_size(sizeof(AM_AUDIO_INFO));
            m_media->seek(-((long)m_read_buf_size), AMFile::AM_FILE_SEEK_CUR);
            m_is_new_file = false;
            m_need_to_read = true;
            if (AM_LIKELY(m_read_buf_size != max_read_buf_size)) {
              m_read_buf_size = max_read_buf_size;
              delete[] m_buffer;
              m_buffer = new char[m_read_buf_size];
              if (AM_UNLIKELY(!m_buffer)) {
                ERROR("Failed to create read buffer!");
                delete m_media;
                m_media = nullptr;
                continue;
              }
            }
          } else {
            uint32_t framelen = mp3_hdr->frame_bytes();
            uint32_t sent_len = 0;
            uint8_t *data = packet->get_data_ptr();
            int count = 0;
            while ((mp3_hdr->is_sync_ok()) && (m_avail_size >= framelen) &&
                   (((sent_len + framelen) <= MAX_MP3_FRAME_SIZE))) {
              memcpy((data + sent_len), &m_buffer[m_sent_size], framelen);
              sent_len += framelen;
              m_sent_size += framelen;
              m_avail_size -= framelen;
              m_remain_size -= framelen;
              mp3_hdr = (MP3Header*)&m_buffer[m_sent_size];
              framelen = mp3_hdr->frame_bytes();
              ++ count;
            }
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
            packet->set_data_size(sent_len);
            packet->set_data_offset(0);
            m_need_to_read = (m_avail_size == 0);
          }
        } else {
          ret = AM_DEMUXER_NO_PACKET;
          break;
        }
      } else {
        uint32_t skipped = 0;
        while((m_avail_size >= sizeof(MP3Header)) &&
              !mp3_hdr->is_sync_ok()) {
          -- m_avail_size;
          -- m_remain_size;
          ++ m_sent_size;
          ++ skipped;
          mp3_hdr = (MP3Header*)&m_buffer[m_sent_size];
        }
        if (AM_LIKELY(skipped > 0)) {
          ERROR("Invalid MP3 frame, skipped %u bytes data!", skipped);
        }
        if (AM_LIKELY(m_remain_size <= sizeof(MP3Header))) {
          ERROR("%s is incomplete, remaining file size %ju is not larger than "
                "MP3 frame header size%u",
                m_media->name(), m_remain_size, sizeof(MP3Header));
          delete m_media;
          m_media = nullptr;
          continue;
        }
        if (AM_LIKELY((m_avail_size >= sizeof(MP3Header)) &&
                      mp3_hdr->is_sync_ok() &&
                      (m_remain_size < mp3_hdr->frame_bytes()))) {
          WARN("%s is incomplete, the last MP3 frame reports length is %u "
               "bytes, but available file length is %ju bytes!",
               m_media->name(), mp3_hdr->frame_bytes(), m_remain_size);
          delete m_media;
          m_media = nullptr;
          continue;
        }
        m_media->seek(-((long)m_avail_size), AMFile::AM_FILE_SEEK_CUR);
        m_need_to_read = true;
      }
    }
  }

  return ret;
}

void AMDemuxerMp3::destroy()
{
  enable(false);
  inherited::destroy();
}

int32_t AMDemuxerMp3::mp3_parse_tag(AMFile &mp3)
{
  int32_t tag_size = -1;
  do {
    uint8_t header[128] = {0};
    ssize_t   read_size = 0;
    ID3v2Header  *id3v2 = nullptr;
    if (AM_UNLIKELY(!mp3.is_open())) {
      ERROR("File %s is not open!", mp3.name());
      break;
    }
    if (AM_UNLIKELY(mp3.size() <= sizeof(header))) {
      ERROR("Failed %s is too small!", mp3.name());
      break;
    }
    read_size = mp3.read_reliable((char*)header, sizeof(header));
    if (AM_LIKELY(read_size != sizeof(header))) {
      ERROR("Read %s error: %s", mp3.name(), strerror(errno));
      break;
    }
    id3v2 = (ID3v2Header*)header;
    if (AM_LIKELY(id3v2->is_id3v2())) {
      tag_size = id3v2->tag_size();
      INFO("Found ID3v2 Tag in %s!", mp3.name());
      if (AM_UNLIKELY(!mp3.seek(tag_size, AMFile::AM_FILE_SEEK_SET))) {
        ERROR("Failed to skip %u bytes of ID3v2 Tag!", tag_size);
        tag_size = -1;
        break;
      }
    } else if (AM_LIKELY((header[0] == 'T') &&
                         (header[1] == 'A') &&
                         (header[2] == 'G'))) {
      tag_size = 128;
      INFO("Found ID3v1 Tag in %s!", mp3.name());
      if (AM_UNLIKELY(!mp3.seek(tag_size, AMFile::AM_FILE_SEEK_SET))) {
        ERROR("Failed to skip 128 bytes ID3v1 Tag!");
        tag_size = -1;
      }
      /* todo: print detailed information */
    } else if (!mp3.seek(0, AMFile::AM_FILE_SEEK_SET)) {
      ERROR("Failed to reset file pointer of %s to the beginning!",
            mp3.name());
      tag_size = -1;
      break;
    } else {
      tag_size = 0;
    }
  }while(0);

  return tag_size;
}

AMDemuxerMp3::AMDemuxerMp3(uint32_t streamid) :
    inherited(AM_DEMUXER_MP3, streamid)
{}

AMDemuxerMp3::~AMDemuxerMp3()
{
  delete m_mp3_header;
  delete[] m_buffer;
  DEBUG("~AMDemuxerMp3");
}

AM_STATE AMDemuxerMp3::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    state = inherited::init();
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      break;
    }

    m_packet_pool = AMFixedPacketPool::create("Mp3DemuxerPacketPool",
                                              PACKET_POOL_SIZE,
                                              MAX_MP3_FRAME_SIZE);
    if (AM_UNLIKELY(!m_packet_pool)) {
      ERROR("Failed to create packet pool for MP3 demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_read_buf_size = FILE_BUFFER_SIZE;
    m_buffer = new char[m_read_buf_size];
    if (AM_UNLIKELY(!m_buffer)) {
      ERROR("Failed to create file read buffer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_mp3_header = new MP3Header;
    if (AM_UNLIKELY(!m_mp3_header)) {
      ERROR("Failed to allocate buffer for MP3 header!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}
