/*******************************************************************************
 * test_media_service_dyn_air_api.cpp
 *
 * History:
 *   2016-12-5 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_log.h"
#include "am_define.h"

#include <getopt.h>
#include <signal.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>
#include <limits.h>
#include "am_api_helper.h"
#include "am_api_media.h"


using std::map;
using std::string;
using std::vector;

#define NO_ARG 0
#define HAS_ARG 1
#define PLAYBACK_INSTANCE_NUM 5

#define VERIFY_PARA_1(x, min) \
    do { \
      if ((x) < min) { \
        ERROR("Parameter wrong: %d", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        ERROR("Parameter wrong: %d", (x)); \
        return -1; \
      } \
    } while (0)

static AMAPIHelperPtr g_api_helper = nullptr;

/*muxer operation*/
uint32_t muxer_id_bit_map = 0;
AMMediaMuxerParam g_muxer_param;
bool muxer_operation_flag = false;
bool set_muxer_param_flag   = false;
bool start_file_writing_flag = false;
bool stop_file_writing_flag = false;
bool set_file_finish_notify_flag = false;
bool set_file_create_notify_flag = false;
bool cancel_file_finish_notify_flag = false;
bool cancel_file_create_notify_flag = false;
bool set_file_create_finish_notify_flag = false;
bool cancel_file_create_finish_notify_flag = false;
/*record engine operation*/
bool record_engine_operation_flag = false;
bool create_recording_flag = false;
bool start_recording_flag = false;
int32_t start_recording_value = 0;
bool stop_recording_flag = false;
bool destroy_recording_flag = false;
bool reload_recording_flag = false;
/*muxer filter operation*/
AMIApiMediaEvent *event_param = nullptr;
AMIAPiFileOperationParam* file_operation_param = nullptr;
bool muxer_filter_operation_flag = false;
bool muxer_filter_start_send_pkt_flag = false;
bool h26x_event_start_recording_flag = false;
bool h26x_event_stop_recording_flag = false;
bool jpeg_event_start_recording_flag = false;
bool periodic_mjpeg_start_flag = false;
uint32_t video_id_bit_map = 0;
uint32_t h26x_history_duration = 0;
uint32_t h26x_future_duration = 0;
int32_t h26x_history_voice_duration = -1;
uint8_t jpeg_history_num = 0;
uint8_t jpeg_future_num = 0;
uint8_t jpeg_closest_num = 0;
std::string start_time;
std::string end_time;
std::string duration;
uint32_t interval_second = 0;
uint32_t once_jpeg_num = 0;
/*playback engine operation*/
bool playback_engine_operation_flag = false;
bool create_playback_flag = false;
bool destroy_playback_flag = false;
bool set_playback_num_flag = false;
AMMediaPlaybackNum g_playback_num;
AMIApiPlaybackAudioFileList *playback_file_list = nullptr;
AMIApiPlaybackUnixDomainUri *playback_unix_domain = nullptr;
AMIApiPlaybackStartParam *playback_start_param = nullptr;
uint32_t playback_id_bit_map = 0;
bool playback_enable_aec = false;
bool add_audio_file_flag = false;
bool add_unix_domain_flag = false;
bool get_playback_id_flag = false;
bool release_playback_id_flag = false;
bool start_playback_flag = false;
bool stop_playback_flag  = false;
bool pause_playback_flag = false;
std::vector<string> audio_name_list;
std::string unix_domain_name;
std::string unix_domain_audio_type;
uint32_t unix_domain_audio_sample_rate = 0;
uint32_t unix_domain_audio_channel = 0;
/*Audio source filter operation*/
AMIApiAudioCodecParam * audio_codec_param = nullptr;
bool audio_source_filter_operation_flag = false;
bool enable_audio_codec_flag = false;
bool enable_audio_codec = false;
std::string audio_codec_name;
/*mainloop parameter*/
static bool exit_flag = false;
static bool main_loop_need_break = false;

enum short_opt {
  /*muxer operation*/
  SET_FILE_DURATION  = 1,
  SET_RECORDING_FILE_NUM,
  SET_RECORDING_DURATION,
  ENABLE_DIGITAL_SIGNATURE,
  ENABLE_GSENSOR,
  ENABLE_RECONSTRUCT,
  SET_MAX_FILE_SIZE,
  ENABLE_WRITE_SYNC,
  ENABLE_HLS,
  ENABLE_SCRAMBLE,
  SET_VIDEO_FRAME_RATE,
  SAVE_TO_CONFIG_FILE,
  START_FILE_WRITING,
  STOP_FILE_WRITING,
  SET_FILE_CREATE_NOTIFY,
  SET_FILE_FINISH_NOTIFY,
  SET_FILE_CREATE_FINISH_NOTIFY,
  CNACEL_FILE_CREATE_NOTIFY,
  CANCEL_FILE_FINISH_NOTIFY,
  CANCEL_FILE_CREATE_FINISH_NOTIFY,
  /*record engine operation*/
  CREATE_RECORDING,
  START_RECORDING,
  STOP_RECORDING,
  DESTROY_RECORDING,
  RELOAD_RECORDING,
  /*muxer filter operation*/
  MUXER_FILTER_START_SEND_PKT,
  HISTORY_DURATION,
  FUTURE_DURATION,
  HISTORY_VOICE_DURATION,
  JPEG_HIST_NUM,
  JPEG_FUTURE_NUM,
  JPEG_CLOSEST_NUM,
  H26X_EVENT_START,
  JPEG_EVENT_START,
  H26X_EVENT_STOP,
  PERIODIC_MJPEG_START,
  START_TIME,
  INTERVAL_SECOND,
  ONCE_JPEG_NUM,
  END_TIME,
  DURATION,
  /*playback engine operation*/
  CREATE_PLAYBACK,
  DESTROY_PLAYBACK,
  SET_PLAYBACK_NUM,
  ADD_AUDIO_FILE,
  ADD_UNIX_DOMAIN,
  UNIX_DOMAIN_AUDIO_TYPE,
  UNIX_DOMAIN_SAMPLE_RATE,
  UNIX_DOMAIN_AUDIO_CHANNEL,
  GET_PLAYBACK_ID,
  RELEASE_PLAYBACK_ID,
  ENABLE_AEC,
  START_PLAYBACK,
  STOP_PLAYBACK,
  PAUSE_PLAYBACK,
  /*Audio source filter operation*/
  ENABLE_AUDIO_CODEC,
  AUDIO_CODEC_NAME,
};

static void sigstop(int arg)
{
  PRINTF("catch sigstop, exit.");
  exit_flag = true;
}

static inline std::string file_finish_type_to_str(AM_API_MEDIA_FILE_FINISH_TYPE type)
{
  std::string str;
  switch (type) {
    case AM_API_MEDIA_FILE_FINISH_NULL : {
      str = "file finish null";
    } break;
    case AM_API_MEDIA_FILE_FINISH_STOP_RECORDING : {
      str = "file finish stop recording";
    } break;
    case AM_API_MEDIA_FILE_FINISH_STOP_FILE_WRITING : {
      str = "file finish stop file writing";
    } break;
    case AM_API_MEDIA_FILE_FINISH_SPLIT_FILE : {
      str = "file finish split file";
    } break;
    case AM_API_MEDIA_FILE_FINISH_EVENT_STOP : {
      str = "file finish event stop";
    } break;
    case AM_API_MEDIA_FILE_FINISH_EOS : {
      str = "file finish eos";
    } break;
    case AM_API_MEDIA_FILE_FINISH_NO_SPACE : {
      str = "file finish no space";
    } break;
    case AM_API_MEDIA_FILE_FINISH_REACH_RECORD_DURATION : {
      str = "file finish reach record duartion";
    } break;
    case AM_API_MEDIA_FILE_FINISH_REACH_RECORD_FILE_NUM : {
      str = "file finish reach record file number";
    } break;
    case AM_API_MEDIA_FILE_FINISH_IO_ERROR : {
      str = "file finish IO error";
    } break;
    default : {
      str = "file type error";
    } break;
  }
  return str;
}

static int on_notify(const void *msg_data, int msg_data_size)
{
  am_service_notify_payload *payload = (am_service_notify_payload *)msg_data;
  if (payload->type == AM_MEDIA_SERVICE_NOTIFY) {
    AMApiMediaFileInfo *file_info = (AMApiMediaFileInfo*)payload->data;
    if (file_info->type == AM_API_MEDIA_NOTIFY_TYPE_FILE_FINISH) {
      PRINTF("File finish callback func :\n"
             "  file finish type : %s\n"
             "         file name : %s\n"
             "create time string : %s\n"
             "finish time string : %s\n"
             "         stream id : %d\n"
             "          muxer id : %d\n",
           file_finish_type_to_str(file_info->finish_type).c_str(),
          file_info->file_name, file_info->create_time_str,
          file_info->finish_time_str,file_info->stream_id, file_info->muxer_id);
    } else if (file_info->type == AM_API_MEDIA_NOTIFY_TYPE_FILE_CREATE) {
      PRINTF("File create callback func :\n"
             "         file name : %s\n"
             "create time string : %s\n"
             "finish time string : %s\n"
             "         stream id : %d\n"
             "          muxer id : %d\n",
          file_info->file_name, file_info->create_time_str,
          file_info->finish_time_str, file_info->stream_id, file_info->muxer_id);
    } else {
      PRINTF("Unknown media notify type.");
    }
  } else {
    PRINTF("receive unknown nofity type.");
  }
  return 0;
}

static struct option long_options[] =
{
 {"help",                             NO_ARG,  0, 'h'},
 /*record engine operation*/
 {"create-recording",                 HAS_ARG, 0, CREATE_RECORDING},
 {"start-recording",                  NO_ARG,  0, START_RECORDING},
 {"stop-recording",                   NO_ARG,  0, STOP_RECORDING},
 {"destroy-recording",                NO_ARG,  0, DESTROY_RECORDING},
 {"reload-recording",                 NO_ARG,  0, RELOAD_RECORDING},
 /*muxer filter operation*/
 {"video-id",                         HAS_ARG, 0, 'v'},
 {"muxer-filter-start-send-pkt",      NO_ARG,  0, MUXER_FILTER_START_SEND_PKT},
 {"h26x-event-start",                 NO_ARG,  0, H26X_EVENT_START},
 {"history-duration",                 HAS_ARG, 0, HISTORY_DURATION},
 {"future-duration",                  HAS_ARG, 0, FUTURE_DURATION},
 {"history-voice-duration",           HAS_ARG, 0, HISTORY_VOICE_DURATION},
 {"h26x-event-stop",                  NO_ARG,  0, H26X_EVENT_STOP},
 {"jpeg-event-start",                 NO_ARG,  0, JPEG_EVENT_START},
 {"jpeg-history-num",                 HAS_ARG, 0, JPEG_HIST_NUM},
 {"jpeg-future-num",                  HAS_ARG, 0, JPEG_FUTURE_NUM},
 {"jpeg-closest-num",                 HAS_ARG, 0, JPEG_CLOSEST_NUM},
 {"periodic-mjpeg-start",             NO_ARG,  0, PERIODIC_MJPEG_START},
 {"start-time",                       HAS_ARG, 0, START_TIME},
 {"interval-second",                  HAS_ARG, 0, INTERVAL_SECOND},
 {"once-jpeg-num",                    HAS_ARG, 0, ONCE_JPEG_NUM},
 {"end-time",                         HAS_ARG, 0, END_TIME},
 {"duration",                         HAS_ARG, 0, DURATION},
 /*muxer operation*/
 {"muxer-id",                         HAS_ARG, 0, 'm'},
 {"set-file-duration",                HAS_ARG, 0, SET_FILE_DURATION},
 {"set-recording-file-num",           HAS_ARG, 0, SET_RECORDING_FILE_NUM},
 {"set-recording-duration",           HAS_ARG, 0, SET_RECORDING_DURATION},
 {"enable-digital-signature",         HAS_ARG, 0, ENABLE_DIGITAL_SIGNATURE},
 {"enable-gsensor",                   HAS_ARG, 0, ENABLE_GSENSOR},
 {"enable-reconstruct",               HAS_ARG, 0, ENABLE_RECONSTRUCT},
 {"set-max-file-size",                HAS_ARG, 0, SET_MAX_FILE_SIZE},
 {"set-video-frame-rate",             HAS_ARG, 0, SET_VIDEO_FRAME_RATE},
 {"enable-write-sync",                HAS_ARG, 0, ENABLE_WRITE_SYNC},
 {"enable-hls",                       HAS_ARG, 0, ENABLE_HLS},
 {"enable-scramble",                  HAS_ARG, 0, ENABLE_SCRAMBLE},
 {"save-to-config-file",              NO_ARG,  0, SAVE_TO_CONFIG_FILE},
 {"start-file-writing",               NO_ARG,  0, START_FILE_WRITING},
 {"stop-file-writing",                NO_ARG,  0, STOP_FILE_WRITING},
 {"set-file-create-notify",           NO_ARG,  0, SET_FILE_CREATE_NOTIFY},
 {"set-file-finish-notify",           NO_ARG,  0, SET_FILE_FINISH_NOTIFY},
 {"set-file-create-finish-notify",    NO_ARG,  0, SET_FILE_CREATE_FINISH_NOTIFY},
 {"cancel-file-create-notify",        NO_ARG,  0, CNACEL_FILE_CREATE_NOTIFY},
 {"cancel-file-finish-notify",        NO_ARG,  0, CANCEL_FILE_FINISH_NOTIFY},
 {"cancel-file-create-finish-notify", NO_ARG,  0, CANCEL_FILE_CREATE_FINISH_NOTIFY},
 /*playback engine operation*/
 {"create-playback",                  NO_ARG,  0, CREATE_PLAYBACK},
 {"destroy-playback",                 NO_ARG,  0, DESTROY_PLAYBACK},
 {"set-playback-num",                 HAS_ARG, 0, SET_PLAYBACK_NUM},
 {"get-playback-id",                  NO_ARG,  0, GET_PLAYBACK_ID},
 {"playback-id",                      HAS_ARG, 0, 'p'},
 {"add-audio-file",                   HAS_ARG, 0, ADD_AUDIO_FILE},
 {"add-unix-domain",                  HAS_ARG, 0, ADD_UNIX_DOMAIN},
 {"audio-type",                       HAS_ARG, 0, UNIX_DOMAIN_AUDIO_TYPE},
 {"sample-rate",                      HAS_ARG, 0, UNIX_DOMAIN_SAMPLE_RATE},
 {"channel",                          HAS_ARG, 0, UNIX_DOMAIN_AUDIO_CHANNEL},
 {"release-playback-id",              NO_ARG,  0, RELEASE_PLAYBACK_ID},
 {"start-playback",                   NO_ARG,  0, START_PLAYBACK},
 {"enable-aec",                       HAS_ARG, 0, ENABLE_AEC},
 {"stop-playback",                    NO_ARG,  0, STOP_PLAYBACK},
 {"pause-playback",                   NO_ARG,  0, PAUSE_PLAYBACK},
 /*Audio source filter operation*/
 {"enable-audio-codec",               HAS_ARG, 0, ENABLE_AUDIO_CODEC},
 {"audio-codec-name",                 HAS_ARG, 0, AUDIO_CODEC_NAME},
 {0,0,0,0}
};

static const char *short_options = "hm:v:p:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
 {"",                "\t\t\t\t" "Help information\n"},
 /*record engine operation*/
 {"0|1",             "\t\t"     "0:create recording  1:create and start recording "},
 {"",                "\t\t\t"   "Start recording"},
 {"",                "\t\t\t"   "Stop recording"},
 {"",                "\t\t\t"   "Destroy recording instance"},
 {"",                "\t\t\t"   "Reload recording instance\n"},
 /*muxer filter operation*/
 {"0-3",             "\t\t\t"   "Video id"},
 {"",                "\t"       "File muxer filter start to send pkt to muxers, use -v to specify video id"},
 {"",                "\t\t\t"   "H26x event start."},
 {"0-max_hist_dura", ""         "H26x event history duration, unit : second"},
 {"0-0xffffffff",    "\t"       "H26x event future duraiton, unit : second"},
 {"-1-max_hist_dura",""         "H26x event history voice duration, unit : second"},
 {"",                "\t\t\t"   "H26x event stop."},
 {"",                "\t\t\t"   "Jpeg event start."},
 {"0-max_hist_num",  "\t"       "Jpeg event history number."},
 {"0-255",           "\t\t"     "Jpeg event future number."},
 {"0-255",           "\t\t"     "Jpeg event closest number."},
 {"",                "\t\t"     "Periodic mjpeg start."},
 {"h-m-s",           "\t\t"     "Start time hour-minute-second."},
 {"1-0xffffffff",    "\t"       "Interval second."},
 {"1-0xffffffff",    "\t"       "Once jpeg number."},
 {"h-m-s",           "\t\t\t"   "End time hour-minute-second."},
 {"h-m-s",           "\t\t\t"   "Duration hour-minute-second\n"},
 /*muxer operation*/
 {"0-18",            "\t\t\t"   "Muxer id"},
 {"0-0x7fffffff",    "\t"       "Set file duration, 0:not split file automatically"},
 {"1-0x7fffffff",    ""         "Set recording file num, should not set it when file is writing"},
 {"1-0x7fffffff",    ""         "Set recording duration, should not set it when file is writing"},
 {"0|1",             "\t"       "0:disable, 1:enable, for av3 only"},
 {"0|1",             "\t\t"     "0:disable, 1:enable"},
 {"0|1",             "\t\t"     "0:disable, 1:enable"},
 {"1-2048",          "\t"       "Unit : MB"},
 {"1-90000",         "\t"       "Set video frame rate, only time-elapse-mp4 support this param currently"},
 {"0|1",             "\t\t"     "0:disable, 1:enable"},
 {"0|1",             "\t\t\t"   "0:disable, 1:enable"},
 {"0|1",             "\t\t"     "0:disable, 1:enable, for av3 only."},
 {"",                "\t\t"     "Save to config file"},
 {"",                "\t\t"     "Start file writing."},
 {"",                "\t\t\t"   "Stop file writing."},
 {"",                "\t\t"     "set file create notify."},
 {"",                "\t\t"     "set file finish notify."},
 {"",                "\t"       "set file create finish notify."},
 {"",                "\t\t"     "Cancel file create notify."},
 {"",                "\t\t"     "Cancel file finish notify."},
 {"",                "\t"       "Cancel file create  finish notify.\n"},
 /*playback engine operation*/
 {"",                "\t\t\t"   "Create playback instances."},
 {"",                "\t\t\t"   "Destroy playback instance."},
 {"1-5",             "\t\t"     "Set playback instance number."},
 {"",                "\t\t\t"   "Get valid playback instance id"},
 {"0-4",             "\t\t\t"   "Specify playback instance id"},
 {"file_name",       "\t"       "Add audio file to playback instance"},
 {"unix_domain_name",""         "Add unix domain to playback instance"},
 {"audio_type",      "\t\t"     "aac, opus, g711A, g711U, g726_40, g726_32, g726_24, g726_16, speex "},
 {"sample_rate",     "\t\t"     "8000, 16000, 48000"},
 {"channel",         "\t\t\t"   "Specify audio channel for unix domain playback"},
 {"",                "\t\t"     "Release playback instance id."},
 {"",                "\t\t\t"   "Start playback."},
 {"0|1",             "\t\t\t"   "Enable aec."},
 {"",                "\t\t\t"   "Stop playback."},
 {"",                "\t\t\t"   "Pause playback.\n"},
 {"0|1",             "\t\t"     "Enable audio codec."},
 {"name",            "\t\t"     "type-sampleRate. type:aac, opus, g711A, g711U, g726_40, g726_32, g726_24, g726_16, speex. sampleRate:48000,16000, 8000"},
};

static string enum_muxer_id_to_string(AM_MEDIA_MUXER_ID id)
{
  string muxer_str;
  switch (id) {
    case AM_MEDIA_MUXER_MP4_NORMAL_0 : {
      muxer_str = "   mp4-normal-0";
    } break;
    case AM_MEDIA_MUXER_MP4_NORMAL_1 : {
      muxer_str = "   mp4-normal-1";
    } break;
    case AM_MEDIA_MUXER_MP4_EVENT_0 : {
      muxer_str = "    mp4-event-0";
    } break;
    case AM_MEDIA_MUXER_MP4_EVENT_1 : {
      muxer_str = "    mp4-event-1";
    } break;
    case AM_MEDIA_MUXER_TS_NORMAL_0 : {
      muxer_str = "    ts-normal-0";
    } break;
    case AM_MEDIA_MUXER_TS_NORMAL_1 : {
      muxer_str = "    ts-normal-1";
    } break;
    case AM_MEDIA_MUXER_TS_EVENT_0 : {
      muxer_str = "     ts-event-0";
    } break;
    case AM_MEDIA_MUXER_TS_EVENT_1 : {
      muxer_str = "     ts-event-1";
    } break;
    case AM_MEDIA_MUXER_RTP : {
      muxer_str = "            rtp";
    } break;
    case AM_MEDIA_MUXER_JPEG_NORMAL : {
      muxer_str = "    jpeg-normal";
    } break;
    case AM_MEDIA_MUXER_JPEG_EVENT_0 : {
      muxer_str = "   jpeg-event-0";
    } break;
    case AM_MEDIA_MUXER_JPEG_EVENT_1 : {
      muxer_str = "   jpeg-event-1";
    } break;
    case AM_MEDIA_MUXER_PERIODIC_JPEG : {
      muxer_str = "  periodic-jpeg";
    } break;
    case AM_MEDIA_MUXER_TIME_ELAPSE_MP4 : {
      muxer_str = "time-elapse-mp4";
    } break;
    case AM_MEDIA_MUXER_AV3_NORMAL_0 : {
      muxer_str = "   av3-normal-0";
    } break;
    case AM_MEDIA_MUXER_AV3_NORMAL_1 : {
      muxer_str = "   av3-normal-1";
    } break;
    case AM_MEDIA_MUXER_AV3_EVENT_0 : {
      muxer_str = "    av3-event-0";
    } break;
    case AM_MEDIA_MUXER_AV3_EVENT_1 : {
      muxer_str = "    av3-event-1";
    } break;
    case AM_MEDIA_MUXER_EXPORT : {
      muxer_str = "         export";
    } break;
    default : {
      muxer_str = "Unkonwn muxer";
    } break;
  }
  return muxer_str;
}

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n", argv[0]);
  printf(" muxer id talbe : \n");
  for (int32_t i = 0; i < AM_MEDIA_MUXER_ID_MAX; ++ i) {
    printf("%s : %d\t", enum_muxer_id_to_string(AM_MEDIA_MUXER_ID(i)).c_str(), i);
    if ((i % 3) == 2) {
      printf("\n");
    }
  }
  printf("\n\n");
  for (uint32_t i = 0; i < sizeof(long_options)/sizeof(long_options[0])-1; ++i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c, ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
  printf("Examples: \n"
         "  %s -m 14 --set-file-duration 10 --enable-digital-signature 1\n"
         "  %s -m 13 -m 14 --set-file-duration 10 --enable-digital-signature 0\n"
         "  %s -m 0 --start-file-writing\n"
         "  %s -m 0 --set-file-create-notify\n"
         "  %s -v 0 -v 1 --muxer-filter-start-send-pkt\n"
         "  %s -v 0 -v 1 --h26x-event-start --history-duration 10 --future-duration -1 --history-voice-duration 5\n"
         "  %s -v 0 --h26x-event-stop\n"
         "  %s -v 3 --jpeg-event-start --jpeg-history-num 2 --jpeg-future-num 2\n"
         "  %s -v 3 --jpeg-event-start --jpeg-closest-num 3\n"
         "  %s -v 3 --periodic-mjpeg-start --start-time 18-30-30 --interval-second 1 --once-jpeg-num 1 --end-time 18-31-30\n"
         "  %s -v 3 --periodic-mjpeg-start --start-time 13-30-30 --interval-second 1 --once-jpeg-num 1 --duration 1-10-10\n"
         "  %s --get-playback-id\n"
         "  %s -p 0 --add-unix-domain unix_domain.service --audio-type aac --sample-rate 8000 --channel 2\n"
         "  %s -p 0 --add-audio-file test1.aac --add-audio-file test2.aac\n"
         "  %s -p 0 --start-playback\n"
         "  %s -p 0 --release-playback-id\n"
         "  %s --enable-audio-codec 0 --audio-codec-name aac-48000\n",
         argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
         argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
         argv[0]);
  printf("\n");
}

static int32_t init_param(int32_t argc, char **argv)
{
  int32_t ch;
  int32_t option_index = 0;
  int32_t ret = 0;
  memset(&g_muxer_param, 0, sizeof(AMMediaMuxerParam));
  while((ch = getopt_long(argc, argv,
                          short_options,
                          long_options,
                          &option_index)) != -1) {
    switch(ch) {
      case 'h' : {
        usage(argc, argv);
        main_loop_need_break = true;
      } break;
      /*muxer filter operation*/
      case 'v' : {
        int32_t video_id = atoi(optarg);
        VERIFY_PARA_2(video_id, 0, 3);
        video_id_bit_map |= 0x01 << video_id;
      } break;
      case MUXER_FILTER_START_SEND_PKT : {
        muxer_filter_start_send_pkt_flag = true;
        muxer_filter_operation_flag = true;
      } break;
      case HISTORY_DURATION : {
        h26x_history_duration = atoi(optarg);
        VERIFY_PARA_2(h26x_history_duration, 0, 0xffffffff);
      } break;
      case FUTURE_DURATION : {
        h26x_future_duration = atoi(optarg);
        VERIFY_PARA_2(h26x_future_duration, 0, 0xffffffff);
      } break;
      case HISTORY_VOICE_DURATION : {
        h26x_history_voice_duration = atoi(optarg);
        VERIFY_PARA_2(h26x_history_voice_duration, -1, 0x7fffffff);
      } break;
      case JPEG_HIST_NUM : {
        jpeg_history_num = atoi(optarg);
        VERIFY_PARA_2(jpeg_history_num, 0, 0xff);
      } break;
      case JPEG_FUTURE_NUM : {
        jpeg_future_num = atoi(optarg);
        VERIFY_PARA_2(jpeg_future_num, 0, 0xff);
      } break;
      case JPEG_CLOSEST_NUM : {
        jpeg_closest_num = atoi(optarg);
        VERIFY_PARA_2(jpeg_closest_num, 0, 0xff);
      } break;
      case H26X_EVENT_START : {
        muxer_filter_operation_flag = true;
        h26x_event_start_recording_flag = true;
      } break;
      case JPEG_EVENT_START : {
        muxer_filter_operation_flag = true;
        jpeg_event_start_recording_flag = true;
      } break;
      case H26X_EVENT_STOP : {
        muxer_filter_operation_flag = true;
        h26x_event_stop_recording_flag = true;
      } break;
      case PERIODIC_MJPEG_START : {
        periodic_mjpeg_start_flag = true;
        muxer_filter_operation_flag = true;
      } break;
      case START_TIME : {
        start_time = optarg;
      } break;
      case INTERVAL_SECOND : {
        interval_second = atoi(optarg);
        VERIFY_PARA_2(interval_second, 0, 0xffffffff);
      } break;
      case ONCE_JPEG_NUM : {
        once_jpeg_num = atoi(optarg);
        VERIFY_PARA_2(once_jpeg_num, 0, 0xffffffff);
      } break;
      case END_TIME : {
        end_time = optarg;
      } break;
      case DURATION : {
        duration = optarg;
      } break;
      /*record engine operation*/
      case CREATE_RECORDING : {
        create_recording_flag = true;
        record_engine_operation_flag = true;
        start_recording_value = atoi(optarg);
        VERIFY_PARA_2(start_recording_value, 0, 1);
      } break;
      case START_RECORDING : {
        start_recording_flag = true;
        record_engine_operation_flag = true;
      } break;
      case STOP_RECORDING : {
        stop_recording_flag = true;
        record_engine_operation_flag = true;
        } break;
      case DESTROY_RECORDING : {
        destroy_recording_flag = true;
        record_engine_operation_flag = true;
      } break;
      case RELOAD_RECORDING : {
        reload_recording_flag = true;
        record_engine_operation_flag = true;
      } break;
      /*playback engine operation*/
      case CREATE_PLAYBACK : {
        create_playback_flag = true;
        playback_engine_operation_flag = true;
      } break;
      case DESTROY_PLAYBACK : {
        destroy_playback_flag = true;
        playback_engine_operation_flag = true;
      } break;
      case SET_PLAYBACK_NUM : {
        set_playback_num_flag = true;
        playback_engine_operation_flag = true;
        g_playback_num.playback_num = atoi(optarg);
        VERIFY_PARA_2(g_playback_num.playback_num, 1, 5);
      } break;
      case 'p' : {
        VERIFY_PARA_2(atoi(optarg), 0, 4);
        playback_id_bit_map |= (0x00000001 << atoi(optarg));
      } break;
      case ADD_AUDIO_FILE : {
        playback_engine_operation_flag = true;
        add_audio_file_flag = true;
        audio_name_list.push_back(std::string(optarg));
      } break;
      case ADD_UNIX_DOMAIN : {
        playback_engine_operation_flag = true;
        add_unix_domain_flag = true;
        unix_domain_name = optarg;
      } break;
      case UNIX_DOMAIN_AUDIO_TYPE : {
        unix_domain_audio_type = optarg;
      } break;
      case UNIX_DOMAIN_SAMPLE_RATE : {
        unix_domain_audio_sample_rate = atoi(optarg);
      } break;
      case UNIX_DOMAIN_AUDIO_CHANNEL : {
        unix_domain_audio_channel = atoi(optarg);
      } break;
      case GET_PLAYBACK_ID : {
        playback_engine_operation_flag = true;
        get_playback_id_flag = true;
      } break;
      case RELEASE_PLAYBACK_ID : {
        playback_engine_operation_flag = true;
        release_playback_id_flag = true;
      } break;
      case START_PLAYBACK : {
        playback_engine_operation_flag = true;
        start_playback_flag = true;
      } break;
      case ENABLE_AEC : {
        playback_enable_aec = (atoi(optarg) > 0) ? true : false;
      } break;
      case STOP_PLAYBACK : {
        playback_engine_operation_flag = true;
        stop_playback_flag = true;
      } break;
      case PAUSE_PLAYBACK : {
        playback_engine_operation_flag = true;
        pause_playback_flag = true;
      } break;
      /*muxer operation*/
      case 'm' : {
        muxer_id_bit_map |= (0x00000001 << atoi(optarg));
        VERIFY_PARA_2(atoi(optarg), AM_MEDIA_MUXER_MP4_NORMAL_0,
                      AM_MEDIA_MUXER_ID_MAX - 1);
      } break;
      case SET_FILE_DURATION : {
        int32_t file_duration = atoi(optarg);
        VERIFY_PARA_2(file_duration, 0, 0x7fffffff);
        g_muxer_param.file_duration_int32.is_set = true;
        g_muxer_param.file_duration_int32.value.v_int32 = file_duration;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case SET_RECORDING_FILE_NUM : {
        int32_t file_num = atoi(optarg);
        VERIFY_PARA_2(file_num, 0, 0x7fffffff);
        g_muxer_param.recording_file_num_u32.is_set = true;
        g_muxer_param.recording_file_num_u32.value.v_u32 = (uint32_t)file_num;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case SET_RECORDING_DURATION : {
        int32_t duration = atoi(optarg);
        VERIFY_PARA_2(duration, 0, 0x7fffffff);
        g_muxer_param.recording_duration_u32.is_set = true;
        g_muxer_param.recording_duration_u32.value.v_u32 = (uint32_t)duration;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case ENABLE_DIGITAL_SIGNATURE : {
        int32_t digital_sig = atoi(optarg);
        VERIFY_PARA_2(digital_sig, 0, 1);
        g_muxer_param.digital_sig_enable_bool.is_set = true;
        g_muxer_param.digital_sig_enable_bool.value.v_bool =
            (digital_sig == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case ENABLE_GSENSOR : {
        int32_t enable_gsensor = atoi(optarg);
        VERIFY_PARA_2(enable_gsensor, 0, 1);
        g_muxer_param.gsensor_enable_bool.is_set = true;
        g_muxer_param.gsensor_enable_bool.value.v_bool =
            (enable_gsensor == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case ENABLE_RECONSTRUCT : {
        int32_t enable_reconstruct = atoi(optarg);
        VERIFY_PARA_2(enable_reconstruct, 0, 1);
        g_muxer_param.reconstruct_enable_bool.is_set = true;
        g_muxer_param.reconstruct_enable_bool.value.v_bool =
            (enable_reconstruct == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case SET_MAX_FILE_SIZE : {
        int32_t max_file_size = atoi(optarg);
        VERIFY_PARA_2(max_file_size, 0, 0x7fffffff);
        g_muxer_param.max_file_size_u32.is_set = true;
        g_muxer_param.max_file_size_u32.value.v_u32 = (uint32_t)max_file_size;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case SET_VIDEO_FRAME_RATE : {
        uint32_t video_frame_rate = atoi(optarg);
        VERIFY_PARA_2(video_frame_rate, 1, 90000);
        g_muxer_param.video_frame_rate_u32.is_set = true;
        g_muxer_param.video_frame_rate_u32.value.v_u32 = video_frame_rate;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      }  break;
      case ENABLE_WRITE_SYNC : {
        int32_t enable_sync = atoi(optarg);
        VERIFY_PARA_2(enable_sync, 0, 1);
        g_muxer_param.write_sync_enable_bool.is_set = true;
        g_muxer_param.write_sync_enable_bool.value.v_bool =
            (enable_sync == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case ENABLE_HLS : {
        int32_t hls = atoi(optarg);
        VERIFY_PARA_2(hls, 0, 1);
        g_muxer_param.hls_enable_bool.is_set = true;
        g_muxer_param.hls_enable_bool.value.v_bool =
            (hls == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case ENABLE_SCRAMBLE : {
        int32_t scramble = atoi(optarg);
        VERIFY_PARA_2(scramble, 0, 1);
        g_muxer_param.scramble_enable_bool.is_set = true;
        g_muxer_param.scramble_enable_bool.value.v_bool =
            (scramble == 0) ? false : true;
        set_muxer_param_flag = true;
        muxer_operation_flag = true;
      } break;
      case START_FILE_WRITING : {
        muxer_operation_flag = true;
        start_file_writing_flag = true;
      } break;
      case STOP_FILE_WRITING : {
        muxer_operation_flag = true;
        stop_file_writing_flag = true;
      } break;
      case SET_FILE_CREATE_NOTIFY : {
        muxer_operation_flag = true;
        set_file_create_notify_flag = true;
      } break;
      case SET_FILE_FINISH_NOTIFY : {
        muxer_operation_flag = true;
        set_file_finish_notify_flag = true;
      } break;
      case SET_FILE_CREATE_FINISH_NOTIFY : {
        muxer_operation_flag = true;
        set_file_create_finish_notify_flag = true;
      } break;
      case CNACEL_FILE_CREATE_NOTIFY : {
        muxer_operation_flag = true;
        cancel_file_create_notify_flag = true;
      } break;
      case CANCEL_FILE_FINISH_NOTIFY : {
        muxer_operation_flag = true;
        cancel_file_finish_notify_flag = true;
      } break;
      case CANCEL_FILE_CREATE_FINISH_NOTIFY : {
        muxer_operation_flag = true;
        cancel_file_create_finish_notify_flag = true;
      } break;
      /*save to config file*/
      case SAVE_TO_CONFIG_FILE : {
        if (set_muxer_param_flag) {
          g_muxer_param.save_to_config_file = true;
        }
        if (set_playback_num_flag) {
          g_playback_num.save_to_config_file = true;
        }
      } break;
      /*audio source filter operation*/
      case ENABLE_AUDIO_CODEC : {
        audio_source_filter_operation_flag = true;
        enable_audio_codec_flag = true;
        enable_audio_codec = (atoi(optarg) > 0) ? true : false;
      } break;
      case AUDIO_CODEC_NAME : {
        audio_codec_name = optarg;
      } break;
      default : {
        ERROR("Invalid short option");
        ret = -1;
        break;
      }
    }
  }
  return ret;
}

bool parse_periodic_mjpeg_arg()
{
  bool ret = true;
  do {
    std::string internal_flag = "-";
    std::string::size_type pos = 0;
    /*event attr*/
    event_param->set_attr_periodic_mjpeg();
    if (video_id_bit_map == 0) {
      ERROR("Please specify the video id");
      ret = false;
      break;
    }
    /*stream id bit map*/
    if (!event_param->set_stream_id_bit_map(video_id_bit_map)) {
      ERROR("Failed to set stream id bit map in parse periodic mjpeg arg");
      ret = false;
      break;
    }
    pos = 0;
    /*start time*/
    if (!event_param->set_start_time_hour(atoi(&(start_time[pos])))) {
      ERROR("Failed to set start time hour in periodic mjpeg arg function.");
      ret = false;
      break;
    }
    if ((pos = start_time.find(internal_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!event_param->set_start_time_minute(atoi(&(start_time[pos])))) {
        ERROR("Failed to set start time minute.");
        ret = false;
        break;
      }
      if ((pos = start_time.find(internal_flag, pos)) != std::string::npos) {
        pos += 1;
        if (!event_param->set_start_time_second(atoi(&(start_time[pos])))) {
          ERROR("Failed to set start time second");
          ret = false;
          break;
        }
      } else {
        ERROR("Periodic jepg recording param is invalid");
        ret = false;
        break;
      }
    } else {
      ERROR("Periodic jpeg recording param is invalid.");
      ret = false;
      break;
    }
    /*interval second*/
    if (!event_param->set_interval_second(interval_second)) {
      ERROR("Failed to set interval second.");
      ret = false;
      break;
    }
    /*once jpeg number*/
    if (!event_param->set_once_jpeg_num(once_jpeg_num)) {
      ERROR("Failed to set once jpeg number.");
      ret = false;
      break;
    }
    /*end time*/
    pos = 0;
    if (!event_param->set_end_time_hour(atoi(&(end_time[pos])))) {
      ERROR("Failed to set end time hour in periodic mjpeg arg function.");
      ret = false;
      break;
    }
    if ((pos = end_time.find(internal_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!event_param->set_end_time_minute(atoi(&(end_time[pos])))) {
        ERROR("Failed to set end time minute.");
        ret = false;
        break;
      }
      if ((pos = end_time.find(internal_flag, pos)) != std::string::npos) {
        pos += 1;
        if (!event_param->set_end_time_second(atoi(&(end_time[pos])))) {
          ERROR("Failed to set end time second");
          ret = false;
          break;
        }
      } else {
        ERROR("Periodic jepg recording param is invalid");
        ret = false;
        break;
      }
    }
    /*duration*/
    pos = 0;
    if (!event_param->set_duration_hour(atoi(&(duration[pos])))) {
      ERROR("Failed to set duration hour in periodic mjpeg arg function.");
      ret = false;
      break;
    }
    if ((pos = duration.find(internal_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!event_param->set_duration_minute(atoi(&(duration[pos])))) {
        ERROR("Failed to set duration minute.");
        ret = false;
        break;
      }
      if ((pos = duration.find(internal_flag, pos)) != std::string::npos) {
        pos += 1;
        if (!event_param->set_duration_second(atoi(&(duration[pos])))) {
          ERROR("Failed to set duration second");
          ret = false;
          break;
        }
      } else {
        ERROR("Periodic jepg recording param is invalid");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

static int32_t muxer_operation()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  do {
    g_muxer_param.muxer_id_bit_map = muxer_id_bit_map;
    if (g_muxer_param.muxer_id_bit_map == 0) {
      ERROR("Please specify muxer id.");
      ret = -1;
      break;
    } else {
      if (set_muxer_param_flag) {
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM,
                                  &g_muxer_param, sizeof(AMMediaMuxerParam),
                                  &service_result, sizeof(service_result));
        if (service_result.ret != AM_RESULT_OK) {
          ERROR("failed to get platform max stream number!\n");
          ret = -1;
          break;
        }
      }
      if (start_file_writing_flag) {
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
                                  &g_muxer_param.muxer_id_bit_map,
                                  sizeof(g_muxer_param.muxer_id_bit_map),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret != AM_RESULT_OK) {
          ERROR("Failed to start file writing!");
          ret = -1;
          break;
        }
      }
      if (stop_file_writing_flag) {
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
                                  &g_muxer_param.muxer_id_bit_map,
                                  sizeof(g_muxer_param.muxer_id_bit_map),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret != AM_RESULT_OK) {
          ERROR("Failed to stop file writing!");
          ret = -1;
          break;
        }
      }
      if (set_file_create_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->enable_callback_notify();
        file_operation_param->set_file_create_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to set file create callback!");
          ret = -1;
          break;
        } else {
          g_api_helper->register_notify_cb(on_notify);
          while(!exit_flag) {
            sleep(1);
          }
        }
      }
      if (set_file_finish_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->enable_callback_notify();
        file_operation_param->set_file_finish_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to set file finish callback!");
          ret = -1;
          break;
        } else {
          g_api_helper->register_notify_cb(on_notify);
          while(!exit_flag) {
            sleep(1);
          }
        }
      }
      if (set_file_create_finish_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->enable_callback_notify();
        file_operation_param->set_file_create_notify();
        file_operation_param->set_file_finish_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to set file create finish callback!");
          ret = -1;
          break;
        } else {
          g_api_helper->register_notify_cb(on_notify);
          while(!exit_flag) {
            sleep(1);
          }
        }
      }
      if (cancel_file_create_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->disable_callback_notify();
        file_operation_param->set_file_create_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to cancel file create callback!");
          ret = -1;
          break;
        }
      }
      if (cancel_file_finish_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->disable_callback_notify();
        file_operation_param->set_file_finish_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to cancel file finish callback!");
          ret = -1;
          break;
        }
      }
      if (cancel_file_create_finish_notify_flag) {
        file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
        file_operation_param->disable_callback_notify();
        file_operation_param->set_file_finish_notify();
        file_operation_param->set_file_create_notify();
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                  file_operation_param->get_data(),
                                  file_operation_param->get_data_size(),
                                  &service_result, sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to cancel file create finish callback!");
          ret = -1;
          break;
        }
      }
    }
  } while(0);
  return ret;
}

static int32_t record_engine_operation()
{
  int32_t ret = 0;
  do {
    if (create_recording_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_CREATE_RECORDING_INSTANCE,
                                &start_recording_value, sizeof(start_recording_value),
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to create recording instance!");
        ret = -1;
        break;
      }
    }
    if (start_recording_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_RECORDING,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to start recording engine!");
        ret = -1;
        break;
      }
    }
    if (stop_recording_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to stop recording engine!");
        ret = -1;
        break;
      }
    }
    if (destroy_recording_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_DESTROY_RECORDING_INSTANCE,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to destroy recording instance!");
        ret = -1;
        break;
      }
    }
    if (reload_recording_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_RELOAD_RECORDING_INSTANCE,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to reload recording instance!");
        ret = -1;
        break;
      }
    }
  } while(0);
  return ret;
}

static int32_t playback_engine_operation()
{
  int32_t ret = 0;
  do {
    if (create_playback_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_CREATE_PLAYBACK_INSTANCES,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to create playback instance!");
        ret = -1;
        break;
      }
    }
    if (destroy_playback_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_DESTROY_PLAYBACK_INSTANCES,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to destroy playback instance!");
        ret = -1;
        break;
      }
    }
    if (set_playback_num_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_PLAYBACK_INSTANCE_NUM,
                                &g_playback_num, sizeof(AMMediaPlaybackNum),
                                &service_result, sizeof(service_result));
      if (service_result.ret != AM_RESULT_OK) {
        ERROR("Failed to set playback instance number!");
        ret = -1;
        break;
      }
    }
    if (add_audio_file_flag) {
      am_service_result_t service_result;
      playback_file_list->clear_file();
      playback_file_list->set_playback_id_bit_map(playback_id_bit_map);
      for(auto &file : audio_name_list) {
        if (playback_file_list->is_full()) {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                                    playback_file_list->get_file_list(),
                                    playback_file_list->get_file_list_size(),
                                    &service_result,
                                    sizeof(am_service_result_t));
          if (service_result.ret < 0) {
            ERROR("Failed to add audio file.");
            ret = -1;
            break;
          }
          playback_file_list->clear_file();
        }
        char abs_path[PATH_MAX] = {0};
        if ((nullptr != realpath(file.c_str(), abs_path)) &&
            (!playback_file_list->add_file(std::string(abs_path)))) {
          ERROR("Failed to add file %s to file list, "
              "file name maybe too long, drop it.", abs_path);
          ret = -1;
          break;
        }
      }
      if (playback_file_list->get_file_number() > 0) {
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                                  playback_file_list->get_file_list(),
                                  playback_file_list->get_file_list_size(),
                                  &service_result,
                                  sizeof(am_service_result_t));
        if (service_result.ret < 0) {
          ERROR("Failed to add audio file.");
          ret = -1;
          break;
        }
      }
    }
    if (add_unix_domain_flag) {
      am_service_result_t service_result;
      if (playback_id_bit_map <= 0) {
        ERROR("Please specify playback id.");
        ret = -1;
        break;
      }
      if (!playback_unix_domain->set_playback_id_bit_map(playback_id_bit_map)) {
        ERROR("Failed to set playback id bit map.");
        ret = -1;
        break;
      }
      if (unix_domain_name.size() == 0) {
        ERROR("Please specify unix domain name.");
        ret = -1;
        break;
      }
      if (!playback_unix_domain->set_unix_domain_name(unix_domain_name.c_str(),
                                                      unix_domain_name.size())) {
        ERROR("Failed to set unix domain name.");
        ret = -1;
        break;
      }
      if (unix_domain_audio_type.size() == 0) {
        ERROR("Please specify unix domain audio type");
        ret = -1;
        break;
      }
      if (unix_domain_audio_type == std::string("aac")) {
        playback_unix_domain->set_audio_type_aac();
      } else if (unix_domain_audio_type == std::string("opus")) {
        playback_unix_domain->set_audio_type_opus();
      } else if (unix_domain_audio_type == std::string("g711A")) {
        playback_unix_domain->set_audio_type_g711A();
      } else if (unix_domain_audio_type == std::string("g711U")) {
        playback_unix_domain->set_audio_type_g711U();
      } else if (unix_domain_audio_type == std::string("g726_40")) {
        playback_unix_domain->set_audio_type_g726_40();
      } else if (unix_domain_audio_type == std::string("g726_32")) {
        playback_unix_domain->set_audio_type_g726_32();
      } else if (unix_domain_audio_type == std::string("g726_24")) {
        playback_unix_domain->set_audio_type_g726_24();
      } else if (unix_domain_audio_type == std::string("g726_16")) {
        playback_unix_domain->set_audio_type_g726_16();
      } else if (unix_domain_audio_type == std::string("speex")) {
        playback_unix_domain->set_audio_type_speex();
      } else {
        ERROR("Audio type invalid");
        ret = -1;
        break;
      }
      if (unix_domain_audio_sample_rate <= 0) {
        ERROR("Please specify unix domain audio sample rate.");
        ret = -1;
        break;
      }
      playback_unix_domain->set_sample_rate(unix_domain_audio_sample_rate);
      if (unix_domain_audio_channel <= 0) {
        ERROR("Please specify unix domain audio channel.");
        ret = -1;
        break;
      }
      playback_unix_domain->set_audio_channel(unix_domain_audio_channel);
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN,
                                playback_unix_domain->get_data(),
                                playback_unix_domain->get_data_size(),
                                &service_result, sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Failed to set playback unix domain uri!");
        ret = -1;
        break;
      }
    }
    if (get_playback_id_flag) {
      am_service_result_t service_result;
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID,
                                nullptr, 0, &service_result,
                                sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Get playback instance id error.");
        ret = -1;
        break;
      }
      int32_t playback_id = ((service_result.data[0] & 0x000000ff) << 24) |
          ((service_result.data[1] & 0x000000ff) << 16) |
          ((service_result.data[2] & 0x000000ff) << 8)  |
          ((service_result.data[3] & 0x000000ff));
      printf("playback id is %d\n", playback_id);
      if (playback_id < 0) {
        ERROR("Get playback id error.");
        ret = -1;
        break;
      }
    }
    if (release_playback_id_flag) {
      am_service_result_t service_result;
      if (playback_id_bit_map <= 0) {
        ERROR("Please specify playback id.");
        ret = -1;
        break;
      }
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID,
                                &playback_id_bit_map, sizeof(int32_t),
                                &service_result, sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Failed to release playback instance id bit map %u",
              playback_id_bit_map);
        ret = -1;
        break;
      }
    }
    if (start_playback_flag) {
      am_service_result_t service_result;
      playback_start_param = AMIApiPlaybackStartParam::create();
      if (!playback_start_param) {
        ERROR("Failed to create AMIApiPlaybackStartParam");
        ret = -1;
        break;
      }
      if (playback_id_bit_map <= 0) {
        ERROR("Please specify playback id.");
        ret = -1;
        break;
      }
      if (!playback_start_param->set_playback_id_bit_map(playback_id_bit_map)) {
        ERROR("Failed to set playback id bit map.");
        ret = -1;
        break;
      }
      playback_start_param->enable_aec(playback_enable_aec);
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,
          playback_start_param->get_data(), playback_start_param->get_data_size(),
          &service_result, sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Failed to start playback audio file.");
        ret = -1;
        break;
      }
    }
    if (stop_playback_flag) {
      am_service_result_t service_result;
      if (playback_id_bit_map <= 0) {
        ERROR("Please specify playback id.");
        ret = -1;
        break;
      }
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,
                                &playback_id_bit_map, sizeof(int32_t),
                                &service_result, sizeof(am_service_result_t));
      if(service_result.ret < 0) {
        ERROR("Failed to stop playback audio file.");
        ret = -1;
        break;;
      }
    }
    if (pause_playback_flag) {
      am_service_result_t service_result;
      if (playback_id_bit_map <= 0) {
        ERROR("Please specify playback id.");
        ret = -1;
        break;
      }
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,
                                &playback_id_bit_map, sizeof(int32_t),
                                &service_result, sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Failed to pause playback audio file.");
        ret = -1;
        break;;
      }
    }
  } while(0);
  return ret;
}

static int32_t h26x_event_start_recording()
{
  int32_t ret = 0;
  do {
    event_param->set_attr_h26X();
    if (!event_param->set_stream_id_bit_map(video_id_bit_map)) {
      ERROR("Failed to set stream id bit map for h26x event start recording.");
      ret = -1;
      break;
    }
    if (!event_param->set_history_duration(h26x_history_duration)) {
      ERROR("Failed to set history duration for h26x event start recording");
      ret = -1;
      break;
    }
    if (!event_param->set_future_duration(h26x_future_duration)) {
      ERROR("Failed to set future duration for h26x event start recording.");
      ret = -1;
      break;
    }
    if (!event_param->set_history_voice_duration(h26x_history_voice_duration)) {
      ERROR("Failed to set history voice duration for h26x event start recording.");
      ret = -1;
      break;
    }
    am_service_result_t service_result;
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              event_param->get_data(), event_param->get_data_size(),
                              &service_result, sizeof(am_service_result_t));
    if (service_result.ret < 0) {
      ERROR("Event start error!");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

static int32_t h26x_event_stop_recording()
{
  int32_t ret = 0;
  do {
    am_service_result_t service_result;
    event_param->set_attr_event_stop_cmd();
    if (!event_param->set_stream_id_bit_map(video_id_bit_map)) {
      ERROR("Failed to set stream id bit map for event stop recording.");
      ret = -1;
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP,
                              event_param->get_data(), event_param->get_data_size(),
                              &service_result, sizeof(am_service_result_t));
    if (service_result.ret < 0) {
      ERROR("Event stop error!");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

static int32_t jpeg_event_start_recording()
{
  int32_t ret = 0;
  do {
    am_service_result_t service_result;
    event_param->set_attr_mjpeg();
    if (!event_param->set_stream_id_bit_map(video_id_bit_map)) {
      ERROR("Failed to set stream id bit map for jpeg event start recording");
      ret = -1;
      break;
    }
    if (!event_param->set_pre_cur_pts_num(jpeg_history_num)) {
      ERROR("Failed to set pre current pts number for jpeg event start recording.");
      ret = -1;
      break;
    }
    if (!event_param->set_after_cur_pts_num(jpeg_future_num)) {
      ERROR("Failed to set after current pts number for jpeg event start recording");
      ret = -1;
      break;
    }
    if (!event_param->set_closest_cur_pts_num(jpeg_closest_num)) {
      ERROR("Failed to set closest current pts num for jpeg event start recording.");
      ret = -1;
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              event_param->get_data(), event_param->get_data_size(),
                              &service_result, sizeof(am_service_result_t));
    if (service_result.ret < 0) {
      ERROR("Event start error!");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

static int32_t periodic_mjpeg_start_recording()
{
  int32_t ret = 0;
  do {
    am_service_result_t service_result;
    if (!parse_periodic_mjpeg_arg()) {
      ERROR("Failed to parse periodic mjpeg arg");
      ret = -1;
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              event_param->get_data(), event_param->get_data_size(),
                              &service_result, sizeof(am_service_result_t));
    if (service_result.ret < 0) {
      ERROR("Event start error!");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

static int32_t muxer_filter_operation()
{
  int32_t ret = 0;
  do {
    if (video_id_bit_map == 0) {
      ERROR("Please specify video id");
      ret = -1;
      break;
    } else {
      if (muxer_filter_start_send_pkt_flag) {
        am_service_result_t service_result;
        g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_MUXER_FILTER_START_SEND_PKT,
                                  &video_id_bit_map, sizeof(video_id_bit_map),
                                  &service_result, sizeof(service_result));
        if (service_result.ret != AM_RESULT_OK) {
          ERROR("Failed to start to send pkt in file filter!");
          ret = -1;
          break;
        }
      }
      if (h26x_event_start_recording_flag) {
        ret = h26x_event_start_recording();
      } else if (h26x_event_stop_recording_flag) {
        ret = h26x_event_stop_recording();
      } else if (jpeg_event_start_recording_flag) {
        ret = jpeg_event_start_recording();
      } else if (periodic_mjpeg_start_flag) {
        ret = periodic_mjpeg_start_recording();
      }
    }
  } while(0);
  return ret;
}

static int32_t audio_source_filter_operation()
{
  int32_t ret = 0;
  do {
    if (enable_audio_codec_flag) {
      am_service_result_t service_result;
      audio_codec_param = AMIApiAudioCodecParam::create();
      if (!audio_codec_param) {
        ERROR("Failed to create audio codec param");
        ret = false;
        break;
      }
      std::string flag = "-";
      std::string::size_type pos = 0;
      std::string audio_type;
      if ((pos = audio_codec_name.find(flag, pos)) != std::string::npos) {
        audio_type = audio_codec_name.substr(0, (pos - 0));
        if (audio_type == std::string("aac")) {
          audio_codec_param->set_audio_type_aac();
        } else if (audio_type == std::string("opus")) {
          audio_codec_param->set_audio_type_opus();
        } else if (audio_type == std::string("g711A")) {
          audio_codec_param->set_audio_type_g711A();
        } else if (audio_type == std::string("g711U")) {
          audio_codec_param->set_audio_type_g711U();
        } else if (audio_type == std::string("g726_40")) {
          audio_codec_param->set_audio_type_g726_40();
        } else if (audio_type == std::string("g726_32")) {
          audio_codec_param->set_audio_type_g726_32();
        } else if (audio_type == std::string("g726_24")) {
          audio_codec_param->set_audio_type_g726_24();
        } else if (audio_type == std::string("g726_16")) {
          audio_codec_param->set_audio_type_g726_16();
        } else if (audio_type == std::string("speex")) {
          audio_codec_param->set_audio_type_speex();
        } else {
          ERROR("Audio type invalid");
          ret = false;
          break;
        }
      } else {
        ERROR("Enable audio codec param is invalid.");
        ret = false;
        break;
      }
      pos += 1;
      uint32_t sample_rate = atoi(&(audio_codec_name[pos]));
      if ((sample_rate != 8000) && (sample_rate != 16000) &&
          (sample_rate != 48000)) {
        ERROR("The sample rate is invalid, only support 8K 16K 48K currently");
        ret = false;
        break;
      }
      audio_codec_param->set_sample_rate(sample_rate);
      audio_codec_param->enable(enable_audio_codec);
      g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC,
                                audio_codec_param->get_data(),
                                audio_codec_param->get_data_size(),
                                &service_result, sizeof(am_service_result_t));
      if (service_result.ret < 0) {
        ERROR("Failed to enable audio codec!");
        ret = -1;
        break;
      }
    }
  } while(0);
  return ret;
}

static bool create_resource()
{
  bool ret = true;
  do {
    event_param = AMIApiMediaEvent::create();
    if (!event_param) {
      ERROR("Failed to create AMIApiMediaEventStruct");
      ret = -1;
      break;
    }
    file_operation_param = AMIAPiFileOperationParam::create();
    if (!file_operation_param) {
      ERROR("Failed to create file_operation_param");
      ret = -1;
      break;
    }
    playback_file_list = AMIApiPlaybackAudioFileList::create();
    if(!playback_file_list) {
      ERROR("Failed to create AMIApiPlaybackAudioFileList");
      ret = -1;
      break;
    }
    playback_unix_domain = AMIApiPlaybackUnixDomainUri::create();
    if (!playback_unix_domain) {
      ERROR("Failed to create playback_unix_domain");
      ret = -1;
      break;
    }
  } while(0);
  return ret;
}

static void release_resource()
{
  delete event_param;
  delete file_operation_param;
  delete playback_file_list;
  delete playback_start_param;
  delete audio_codec_param;
}

int32_t main(int32_t argc, char **argv)
{
  int32_t ret = 0;
  do {
    if (argc < 2) {
      usage(argc, argv);
      break;
    }
    if (!create_resource()) {
      ERROR("Failed to create resource.");
      ret = -1;
      break;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    if (init_param(argc, argv) < 0) {
      ERROR("Failed to init param.");
      ret = -1;
      break;
    }
    if (main_loop_need_break) {
      break;
    }
    g_api_helper = AMAPIHelper::get_instance();
    if (!g_api_helper) {
      ERROR("Unable to get AMAPIHelper instance\n");
      ret = -1;
      break;
    }
    if (muxer_operation_flag) {
      if (muxer_operation() < 0) {
        ERROR("Muxer operation error.");
        ret = -1;
        break;
      }
    }
    if (record_engine_operation_flag) {
      if (record_engine_operation() < 0) {
        ERROR("Record engine operation error.");
        ret = -1;
        break;
      }
    }
    if (playback_engine_operation_flag) {
      if (playback_engine_operation() < 0) {
        ERROR("Playback engine operation error.");
        ret = -1;
        break;
      }
    }
    if (muxer_filter_operation_flag) {
      if (muxer_filter_operation() < 0) {
        ERROR("Muxer filter operation error.");
        ret = -1;
        break;
      }
    }
    if (audio_source_filter_operation_flag) {
      if (audio_source_filter_operation() < 0) {
        ERROR("Audio source filter operation error.");
        ret = -1;
        break;
      }
    }
  } while(0);
  release_resource();
  return ret;
}
