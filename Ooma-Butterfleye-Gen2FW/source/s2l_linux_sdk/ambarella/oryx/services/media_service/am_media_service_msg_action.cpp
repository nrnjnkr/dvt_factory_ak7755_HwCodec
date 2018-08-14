/*******************************************************************************
 * am_media_service_msg_action.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#include "am_define.h"
#include "am_log.h"
#include "am_service_impl.h"
#include "am_service_frame_if.h"
#include "am_media_service_instance.h"
#include "am_media_service_data_structure.h"
#include "commands/am_api_cmd_common.h"
#include "am_image_quality_data.h"
#include "am_api_image.h"
#include "am_record_msg.h"
#include "am_api_media.h"

extern AM_SERVICE_STATE g_service_state;
extern AMMemLock        g_file_operation_cb_lock;
extern AMMediaService  *g_media_instance;
extern AMIServiceFrame *g_service_frame;
extern AMIPCSyncCmdServer g_ipc;

inline std::string file_finish_type_to_str(AM_RECORD_FILE_FINISH_TYPE type)
{
  std::string str;
  switch (type) {
    case AM_RECORD_FILE_FINISH_NULL : {
      str = "file finish null";
    } break;
    case AM_RECORD_FILE_FINISH_STOP_RECORDING : {
      str = "file finish stop recording";
    } break;
    case AM_RECORD_FILE_FINISH_STOP_FILE_WRITING : {
      str = "file finish stop file writing";
    } break;
    case AM_RECORD_FILE_FINISH_SPLIT_FILE : {
      str = "file finish split file";
    } break;
    case AM_RECORD_FILE_FINISH_EVENT_STOP : {
      str = "file finish event stop";
    } break;
    case AM_RECORD_FILE_FINISH_EOS : {
      str = "file finish eos";
    } break;
    case AM_RECORD_FILE_FINISH_NO_SPACE : {
      str = "file finish no space";
    } break;
    case AM_RECORD_FILE_FINISH_REACH_RECORD_DURATION : {
      str = "file finish reach record duartion";
    } break;
    case AM_RECORD_FILE_FINISH_REACH_RECORD_FILE_NUM : {
      str = "file finish reach record file number";
    } break;
    case AM_RECORD_FILE_FINISH_IO_ERROR : {
      str = "file finish IO error";
    } break;
    default : {
      str = "file type error";
    } break;
  }
  return str;
}

static void file_operation_callback_func(AMRecordFileInfo &file_info)
{
  AUTO_MEM_LOCK(g_file_operation_cb_lock);
  AMApiMediaFileInfo media_info;
  am_service_notify_payload payload;
  memset(&payload, 0, sizeof(am_service_notify_payload));
  SET_BIT(payload.dest_bits, AM_SERVICE_TYPE_GENERIC);
  payload.type = AM_MEDIA_SERVICE_NOTIFY;
  payload.msg_id = AM_IPC_MW_CMD_MEDIA_FILE_OPERATION_TO_APP;
  payload.data_size = sizeof(AMApiMediaFileInfo);
  media_info.muxer_id = file_info.muxer_id;
  media_info.stream_id = file_info.stream_id;
  memcpy(media_info.file_name, file_info.file_name, sizeof(file_info.file_name));
  memcpy(media_info.create_time_str, file_info.create_time_str,
         sizeof(file_info.create_time_str));
  memcpy(media_info.finish_time_str, file_info.finish_time_str,
         sizeof(file_info.finish_time_str));
  if (file_info.type == AM_RECORD_FILE_FINISH_INFO) {
    INFO("\nFile finish callback func :\n"
        "  file finish type : %s\n"
        "         file name : %s\n"
        "create time string : %s\n"
        "finish time string : %s\n"
        "         stream id : %d\n"
        "          muxer id : %d",
        file_finish_type_to_str(file_info.finish_type).c_str(),
        media_info.file_name, media_info.create_time_str,
        media_info.finish_time_str, media_info.stream_id, media_info.muxer_id);
    media_info.type = AM_API_MEDIA_NOTIFY_TYPE_FILE_FINISH;
    media_info.finish_type =
        AM_API_MEDIA_FILE_FINISH_TYPE((int32_t)file_info.finish_type);
  } else if (file_info.type == AM_RECORD_FILE_CREATE_INFO) {
    INFO("\nFile create callback func :\n"
        "         file name : %s\n"
        "create time string : %s\n"
        "finish time string : %s\n"
        "         stream id : %d\n"
        "          muxer id : %d",
        media_info.file_name, media_info.create_time_str,
        media_info.finish_time_str, media_info.stream_id, media_info.muxer_id);
    media_info.type = AM_API_MEDIA_NOTIFY_TYPE_FILE_CREATE;
  } else {
    ERROR("Record file info type error.");
  }
  memcpy(payload.data, &media_info, payload.data_size);
  g_ipc.notify(AM_IPC_SERVICE_NOTIF, &payload,
               payload.header_size() + payload.data_size);
}

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  //if it's init done, then
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("Media service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize Media service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Media service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int ret = 0;
  PRINTF("ON MEDIA SERVICE DESTROY.");
  if (g_media_instance) {
    g_media_instance->stop_media();
    g_service_state = AM_SERVICE_STATE_STOPPED;
  } else {
    ERROR("Media service: Failed to get AMMediaService instance!");
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
  g_service_frame->quit();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  PRINTF("ON MEDIA SERVICE START.");
  if (!g_media_instance) {
    ERROR("media instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if (g_service_state == AM_SERVICE_STATE_STARTED) {
      INFO("media instance is already started.");
    } else {
      if (!g_media_instance->start_media()) {
        ERROR("Media service: start media failed!");
        g_service_state = AM_SERVICE_STATE_ERROR;
      } else {
        g_service_state = AM_SERVICE_STATE_STARTED;
      }
    }
  }
  //if it's init done, then
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE STOP.");
  if (!g_media_instance) {
    ERROR("AMMediaInstance instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if(!g_media_instance->stop_media()) {
      ERROR("Media service: stop media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STOPPED;
    }
  }
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_RESTART(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE RESTART.");
  if (!g_media_instance) {
    ERROR("media instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if (!g_media_instance->stop_media()) {
      ERROR("Media service: stop media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else if(!g_media_instance->start_media()) {
      ERROR("Media service: start media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STARTED;
    }
  }
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_STATUS(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE STATUS.");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_MUXER_FILTER_START_SEND_PKT(void* msg_data,
                                                        int msg_data_size,
                                                        void *result_addr,
                                                        int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_MUXER_FILTER_START_SEND_PKT");
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  int32_t ret = 0;
  uint32_t video_id_bit_map = *((uint32_t*)msg_data);
  if (g_record != nullptr) {
    if (!g_record->muxer_filter_start_send_pkt(video_id_bit_map)) {
      ERROR("Muxer filter start send pkt error.");
      ret = -1;
    }
  } else {
    ERROR("Failed to get stream record instance");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START(void* msg_data,
                                                  int msg_data_size,
                                                  void *result_addr,
                                                  int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START");
  int32_t ret = 0;
  AMEventStruct* event = (AMEventStruct*)msg_data;
  if (!g_media_instance->send_event(*event)) {
    ERROR("Failed to send event.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP(void* msg_data,
                                                 int msg_data_size,
                                                 void *result_addr,
                                                 int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP");
  int32_t ret = 0;
  AMEventStruct* event = (AMEventStruct*)msg_data;
  if (!g_media_instance->send_event(*event)) {
    ERROR("Failed to send event.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING(void* msg_data,
                                                    int msg_data_size,
                                                    void *result_addr,
                                                    int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING");
  int32_t ret = 0;
  AMEventStruct* event = (AMEventStruct*)msg_data;
  if (!g_media_instance->send_event(*event)) {
    ERROR("Faile to send event.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID(void *msg_data,
                                                     int msg_data_size,
                                                     void *result_addr,
                                                     int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID");
  int32_t ret = 0;
  int32_t player_id = -1;
  player_id = g_media_instance->get_valid_playback_id();
  if (player_id < 0) {
    ERROR("Failed to get valid playback id");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->data[0] =
      (uint8_t)((player_id & 0xff000000) >> 24);
  ((am_service_result_t*)result_addr)->data[1] =
      (uint8_t)((player_id & 0x00ff0000) >> 16);
  ((am_service_result_t*)result_addr)->data[2] =
      (uint8_t)((player_id & 0x0000ff00) >> 8);
  ((am_service_result_t*)result_addr)->data[3] =
      (uint8_t)((player_id & 0x000000ff));
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID(void *msg_data,
                                                         int msg_data_size,
                                                         void *result_addr,
                                                         int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID");
  int ret = 0;
  do {
    if (!msg_data) {
      ret = -1;
      ERROR("Invalid argument! Playback instance ID is not specified!");
      break;
    }
    uint32_t id_bit_map = *((int32_t*)msg_data);
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; ++ i) {
      if (id_bit_map & (0x00000001 << i)) {
        if (!g_media_instance->release_playback_instance(i)) {
          ERROR("Failed to release playback instance, id is %d", i);
          ret = -1;
          break;
        }
      }
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE(void* msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE");
  int ret = 0;
  do {
    AudioFileList* audio_file = nullptr;
    uint32_t playback_id_bit_map = 0;

    if (!msg_data) {
      ret = -1;
      ERROR("Invalid argument! AudioFileList is not specified!");
      break;
    }
    audio_file = (AudioFileList*)msg_data;
    playback_id_bit_map = audio_file->playback_id_bit_map;
    if(audio_file->file_number == 0) {
      ERROR("file list is empty\n");
      ret = -1;
      break;
    }
    for (uint32_t i = 0; i < sizeof(uint32_t) * 4; ++ i) {
      if (playback_id_bit_map & (0x00000001 << i)) {
        AMIPlaybackPtr playback = g_media_instance->get_playback_instance(i);
        if (playback == nullptr) {
          ERROR("Failed to get valid playback instance.");
          ret = -1;
          break;
        }
        INFO("audio file num: %d", audio_file->file_number);
        for(uint32_t i = 0; i< audio_file->file_number; ++ i) {
          INFO("file name :%s",audio_file->file_list[i]);
          AMPlaybackUri uri;
          uri.type = AM_PLAYBACK_URI_FILE;
          memcpy(uri.media.file, audio_file->file_list[i],
                 sizeof(audio_file->file_list[i]));
          if(!playback->add_uri(uri)) {
            ERROR("Failed to add %s to play list",
                  audio_file->file_list[i]);
            ret = -1;
            break;
          }
        }
        if(ret == -1) {
          break;
        }
      }
    }
    if(ret == -1) {
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN(void* msg_data,
                                                     int msg_data_size,
                                                     void *result_addr,
                                                     int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN");
  int ret = 0;
  AMPlaybackUnixDomainUri* unix_domain_uri = (AMPlaybackUnixDomainUri*)msg_data;
  uint32_t bit_map = unix_domain_uri->playback_id_bit_map;
  do{
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; ++ i) {
      if (bit_map & (0x00000001 << i)) {
        AMIPlaybackPtr playback = g_media_instance->get_playback_instance(i);
        if(playback == nullptr) {
          ERROR("Failed to get valid playback instance, playback id is %d", i);
          ret = -1;
          break;
        }
        if (playback->is_paused() || playback->is_playing()) {
          ERROR("The playback %d is paused or playing, stop it first.", i);
          ret = -1;
          break;
        }
        AMPlaybackUri uri;
        uri.type = AM_PLAYBACK_URI_UNIX_DOMAIN;
        memcpy((char*)(&uri.media.unix_domain), (char*)unix_domain_uri, msg_data_size);
        if(!playback->add_uri(uri)) {
          ERROR("Failed to add unix domain uri to playback");
          ret = -1;
          break;
        }
      }
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  PlaybackStartParam *param = (PlaybackStartParam*)msg_data;
  do{
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; ++ i) {
      if (param->playback_id_bit_map & (0x00000001 << i)) {
        AMIPlaybackPtr playback = g_media_instance->get_playback_instance(i);
        if (playback == nullptr) {
          ERROR("Failed to get valid playback instance, id is %d", i);
          ret = -1;
          break;
        }
        if(playback->is_paused()) {
          if(!playback->pause(false)) {
            PRINTF("Failed to resume playback.");
            ret = -1;
            break;
          }
        } else if(playback->is_playing()) {
          PRINTF("Playback is already playing.");
          break;
        } else {
          playback->set_aec_enabled(param->aec_enable);
          if(!playback->play()) {
            PRINTF("Failed to start playing.");
            ret = -1;
            break;
          }
        }
      }
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  uint32_t playback_id_bit_map = *((int32_t*)msg_data);
  do{
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; ++ i) {
      if (playback_id_bit_map & (0x00000001 << i)) {
        AMIPlaybackPtr playback = g_media_instance->get_playback_instance(i);
        if (playback == nullptr) {
          ERROR("Failed to get valid playback instance, id is %d", i);
          ret = -1;
          break;
        }
        if(!playback->pause(true)) {
          PRINTF("Failed to pause playing.");
          ret = -1;
          break;
        }
      }
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                     int msg_data_size,
                                                     void *result_addr,
                                                     int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  uint32_t playback_id_bit_map = *((int32_t*)msg_data);
  do {
    for (uint32_t i = 0; i < sizeof(uint32_t) * 8; ++ i) {
      if (playback_id_bit_map & (0x00000001 << i)) {
        AMIPlaybackPtr playback = g_media_instance->get_playback_instance(i);
        if (playback == nullptr) {
          ERROR("Failed to get valid playback instance, id is %d", i);
          ret = -1;
          break;
        }
        if(!playback->stop()) {
          PRINTF("Failed to stop playing.");
          ret = -1;
          break;
        }
      }
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get record instance");
      ret = -1;
      break;
    }
    if(g_record->is_recording()) {
      PRINTF("The media service is already recording.");
      break;
    }
    g_record->set_aec_enabled(msg_data ? (0 != (*((uint32_t*)msg_data)))
                                       : false);
    if(!g_record->start()) {
      PRINTF("Failed to start recording.");
      ret = -1;
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if(!g_record->is_recording()) {
      PRINTF("The media service recording is already stopped.");
      break;
    }
    if(!g_record->stop()) {
      PRINTF("Failed to stop recording.");
      ret = -1;
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING(void *msg_data,
                                                 int msg_data_size,
                                                 void *result_addr,
                                                 int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING");
  int ret = 0;
  uint32_t muxer_id_bit_map = 0xffffffff;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if (msg_data) {
      muxer_id_bit_map = *((uint32_t*) msg_data);
    } else {
      ERROR("The msg data of start file recording is invalid.");
      ret = -1;
      break;
    }
    if (!g_record->start_file_recording(muxer_id_bit_map)) {
      PRINTF("Failed to start file recording.");
      ret = -1;
      break;
    }
  } while (0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING(void *msg_data,
                                                int msg_data_size,
                                                void *result_addr,
                                                int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING");
  int ret = 0;
  uint32_t muxer_id_bit_map = 0xffffffff;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  if(msg_data) {
    muxer_id_bit_map = *((uint32_t*)msg_data);
  }
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if(!g_record->stop_file_recording(muxer_id_bit_map)) {
      PRINTF("Failed to stop file recording.");
      ret = -1;
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  AMMuxerParam *param = nullptr;
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if(msg_data) {
      param = ((AMMuxerParam*)msg_data);
    } else {
      ERROR("The msg data is invalid in ON_AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM");
      ret = -1;
      break;
    }
    if(!g_record->set_muxer_param(*param)) {
      PRINTF("Failed to set muxer param");
      ret = -1;
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC(void *msg_data,
                                               int msg_data_size,
                                               void *result_addr,
                                               int result_max_size)
{
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  AudioCodecParam *param = nullptr;
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if(msg_data) {
      param = ((AudioCodecParam*)msg_data);
    } else {
      ERROR("The msg data is invalid in ON_AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC");
      ret = -1;
      break;
    }
    if (param->enable) {
      PRINTF("ON_AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC");
    } else {
      PRINTF("ON_AM_IPC_MW_CMD_MEDIA_DISABLE_AUDIO_CODEC");
    }
    if(!g_record->enable_audio_codec(param->type, param->sample_rate,
                                     param->enable)) {
      PRINTF("Failed to enable audio codec.");
      ret = -1;
      break;
    }
  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK(void *msg_data,
                                                        int msg_data_size,
                                                        void *result_addr,
                                                        int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  FileOperationParam *param = nullptr;
  do {
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if(msg_data) {
      param = (FileOperationParam*)msg_data;
    } else {
      ERROR("The msg data is invalid for file operation callback");
      ret = -1;
      break;
    }
    for (int32_t i = 0; i < AM_OPERATION_CB_TYPE_NUM; ++ i) {
      if ((param->type_bit_map & (0x00000001 << i)) > 0) {
        if (param->enable) {
          INFO("Set bitmap %u file operation callback", param->muxer_id_bit_map);
          if(!g_record->set_file_operation_callback(param->muxer_id_bit_map,
                                                 AM_FILE_OPERATION_CB_TYPE(i),
                                                 file_operation_callback_func)) {
            PRINTF("Failed to set file operation callback function.");
            ret = -1;
            break;
          }
        } else {
          INFO("Cancel bitmap %u file operation callback", param->muxer_id_bit_map);
          if(!g_record->set_file_operation_callback(param->muxer_id_bit_map,
                                                    AM_FILE_OPERATION_CB_TYPE(i),
                                                    nullptr)) {
            PRINTF("Failed to cancel file operation callback notify.");
            ret = -1;
            break;
          }
        }
      }
    }

  } while(0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO");
  int ret = 0;
  do {
    if (!msg_data) {
      ERROR("msg_data is invalid");
      ret = -1;
      break;
    }
    am_image_3A_info_s *image_3A = (am_image_3A_info_s *)msg_data;
    AMImage3AInfo image_3A_inner;
    memcpy(&image_3A_inner, image_3A, sizeof(AMImage3AInfo));
    AMIRecordPtr g_record = g_media_instance->get_record_instance();
    if (g_record == nullptr) {
      ERROR("Failed to get g_record instance");
      ret = -1;
      break;
    }
    if (!g_record->update_image_3A_info(&image_3A_inner)) {
      ERROR("update_image_3A_info failed");
      ret = -1;
      break;
    }
  } while (0);
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_RELOAD_RECORDING_INSTANCE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE.");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  if (g_record) {
    if (!g_record->reload()) {
      ERROR("Failed to reload record engine.");
      ret = -1;
    }
  } else {
    ERROR("Failed to get record instance.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_CREATE_RECORDING_INSTANCE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_CREATE_RECORDING_INSTANCE.");
  int ret = 0;
  bool start = *((int32_t*)msg_data) > 0 ? true : false;
  if (!g_media_instance->create_recording_instance(start)) {
    ERROR("Failed to create recording instance in media service.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_DESTROY_RECORDING_INSTANCE(void *msg_data,
                                                       int msg_data_size,
                                                       void *result_addr,
                                                       int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_CREATE_RECORDING_INSTANCE.");
  int ret = 0;
  if (!g_media_instance->destroy_recording_instance()) {
    ERROR("Failed to destroy recording instance in media service.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_CREATE_PLAYBACK_INSTANCES(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_CREATE_PLAYBACK_INSTANCES.");
  int ret = 0;
  if (!g_media_instance->create_playback_instances()) {
    ERROR("Failed to create playback instances in media service.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_SET_PLAYBACK_INSTANCE_NUM(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_SET_PLAYBACK_INSTANCE_NUM.");
  int ret = 0;
  AMPlaybackInstanceNum *num = (AMPlaybackInstanceNum*)msg_data;
  if (!g_media_instance->set_playback_instances_num(num->playback_num,
                                                    num->save_to_config_file)) {
    ERROR("Failed to set playback instances number in media service.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_DESTROY_PLAYBACK_INSTANCES(void *msg_data,
                                                       int msg_data_size,
                                                       void *result_addr,
                                                       int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_DESTROY_PLAYBACK_INSTANCES.");
  int ret = 0;
  if (!g_media_instance->destroy_playback_instances()) {
    ERROR("Failed to destroy playback instances in media service.");
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}
