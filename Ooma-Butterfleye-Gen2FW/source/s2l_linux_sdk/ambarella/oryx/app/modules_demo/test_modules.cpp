/*******************************************************************************
 * test_modules.cpp
 *
 * history:
 *   feb 10, 2017 - [Huaiqing Wang] created file
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

#include <sys/time.h>
#include <getopt.h>

#include "am_signal.h"
#include "modules.h"
#include "netlink.h"
#include "am_define.h"

static AMOryxModules *g_camera = nullptr;
static bool is_running = false;
static bool g_start_recording = false;
static bool g_stop_recording = false;

static inline const long long get_timestamp_ms()
{
  struct timeval tv;
  int now_us;
  time_t now_sec;
  gettimeofday(&tv, NULL);
  now_sec = tv.tv_sec;
  now_us = tv.tv_usec;

  long long ret = now_sec * 1000 + now_us/1000;
  return ret;
}

static void start_later_modules()
{
  //start md and lbr plugin
  g_camera->start_all_video_plugins();
  //start 3A
  g_camera->start_image();
}

static void signal_handler_normal(int sig)
{
  is_running = false;
}

static void signal_handler_fast(int sig)
{
  if (sig == 10) {
    if (!is_running) {
      is_running = true;
      start_later_modules();
    }
    g_start_recording = true;
  } else if (sig == 12) {
    g_stop_recording = true;
  }
}

static void show_normal_mode_menu(bool prompt)
{
  if (!prompt) {
    printf("> ");
    return;
  }
  printf("======================================================\n");
  printf(" r -- start file recording\n");
  printf(" s -- stop file recording\n");
  printf(" q -- quit this program\n");
  printf("======================================================\n");
}

static void normal_mode_loop()
{
  g_camera->start_file_recording(1);
  AM_RESULT ret = AM_RESULT_OK;
  bool show_menu = true;
  bool prompt = true;
  char opt = 0;
  while (is_running) {
    if (show_menu) {
      show_normal_mode_menu(prompt);
      prompt = true;
    } else {
      show_menu = true;
    }

    switch (opt = getchar()) {
      case 'r':
        if ((ret = g_camera->start_file_recording()) != AM_RESULT_OK) {
          is_running = false;
        }
        break;
      case 's':
        if ((ret = g_camera->stop_file_recording()) != AM_RESULT_OK) {
          is_running = false;
        }
        break;
      case 'q':
        is_running = false;
        break;
      case '\n':
        prompt = false;
        break;
      default:
        show_menu = false;
        break;
    }
  }
}

static AM_RESULT capture_video()
{
  AM_RESULT ret = AM_RESULT_OK;
  static unsigned int count = 0;
  while(true) {
    AMQueryFrameDesc frame_desc;
    AMAddress v_addr;
    AMStreamInfo s_info;

    if ((ret = g_camera->query_video_frame(frame_desc, 0))
        != AM_RESULT_OK) {
      if (ret == AM_RESULT_ERR_AGAIN) {
        continue;
      }
      ERROR("query video frame failed \n");
      break;
    }

    if ((ret = g_camera->video_addr_get(frame_desc, v_addr))
        != AM_RESULT_OK) {
      ERROR("Failed to get video address!");
      break;
    }
    AMVideoFrameDesc  &video = frame_desc.video;
    s_info.stream_id = AM_STREAM_ID(video.stream_id);
    if ((ret = g_camera->query_stream_info(s_info))
        != AM_RESULT_OK) {
      ERROR("Failed to query stream%d info!",s_info.stream_id);
      break;
    }

    AM_VIDEO_INFO vinfo = {0};
    AM_STREAM_TYPE vtype = AM_STREAM_TYPE_NONE;
    switch(video.stream_type) {
      case AM_STREAM_TYPE_MJPEG: {
        vtype = AM_STREAM_TYPE_MJPEG;
      }break;
      case AM_STREAM_TYPE_H264: {
        vtype = AM_STREAM_TYPE_H264;
      }break;
      case AM_STREAM_TYPE_H265: {
        vtype = AM_STREAM_TYPE_H265;
      }break;
      default:break; /* Won't come here */
    }
    vinfo.type      = vtype;
    vinfo.width     = video.width;
    vinfo.height    = video.height;
    vinfo.slice_num = video.tile_slice.slice_num;
    vinfo.tile_num  = video.tile_slice.tile_num;
    vinfo.M         = s_info.m;
    vinfo.N         = s_info.n;
    vinfo.mul       = s_info.mul;
    vinfo.div       = s_info.div;
    vinfo.rate      = s_info.rate;
    vinfo.scale     = s_info.scale;
    if (++count == 1) {
      printf("\n[%jd]##### First Video Packet #####"
             "\nVideo Stream%u Information:"
             "\nSessionID: %08X"
             "\n      PTS: %jd"
             "\n    Width: %u"
             "\n   Height: %u"
             "\n     Type: %s"
             "\n        M: %hu"
             "\n        N: %hu"
             "\nSlice Num: %hu"
             "\n Tile Num: %hu"
             "\n     Rate: %u"
             "\n    Scale: %u"
             "\n      Mul: %hu"
             "\n      Div: %hu\n",
             get_timestamp_ms(),
             video.stream_id,
             video.session_id,
             frame_desc.pts,
             video.width,
             video.height,
             AMVideoTrans::stream_type_enum_to_str(vtype).c_str(),
             vinfo.M,
             vinfo.N,
             vinfo.slice_num,
             vinfo.tile_num,
             vinfo.rate,
             vinfo.scale,
             vinfo.mul,
             vinfo.div);
    }
  };

  return ret;
}

static void static_audio_capture(AudioPacket *packet)
{
  static unsigned int count = 0;
  if (++count == 1) {
    printf("\n[%jd]##### First Audio Packet #####"
           "\nAudio Information:"
           "\nLength: %u"
           "\n   PTS: %jd\n",
           get_timestamp_ms(),
           packet->length,
           packet->pts);
  }
}

static AM_RESULT capture_audio()
{
  return g_camera->start_audio_capture(100);
}

static void fast_record_mode_loop()
{
  int fd = -1;
  std::string path = "/tmp/recording";
  while (true) {
    if (g_start_recording) {
      if (g_camera->start_file_recording(1) != AM_RESULT_OK) {
        break;
      }
      /*Used to sync with S90enter_hibernation script process during reboot*/
      if ((fd = open(path.c_str(), O_RDONLY|O_CREAT)) <= 0) {
        ERROR("Create tmp file failed!");
        break;
      }
      PRINTF("Start file recording success!");
      g_start_recording = false;
    }

    if (g_stop_recording) {
      if (g_camera->stop_file_recording(1) != AM_RESULT_OK) {
        break;
      }
      /*Used to sync with S90enter_hibernation script process during reboot*/
      if ((fd > 0) && (close(fd) || remove(path.c_str()))) {
        ERROR("Failed remove file!");
        break;
      }
      PRINTF("Stop file recording success!");
      g_stop_recording = false;
      fd = -1;
    }
    sleep(1);
  }
}

static void fast_mode_loop()
{
  if (capture_audio() != AM_RESULT_OK) {
    return;
  }

  if (capture_video() != AM_RESULT_OK) {
    return;
  }
}

struct hint {
    const char *args;
    const char *help;
};

void usage(option *options, hint *hints)
{
  option *opt = options;
  hint   *hts = hints;
  while(opt && opt->name) {
    if (isalpha(opt->val)) {
      printf("-%c, ", opt->val);
    } else {
      printf("    ");
    }
    printf("--%s ", opt->name);
    printf("%s",    hts->args);
    printf("%s\n",  hts->help);
    ++ opt;
    ++ hts;
  }
}

int main(int argc, char *argv[])
{
  AM_RESULT ret = AM_RESULT_OK;

  struct option long_options[] =
  {
    {"help",   no_argument,       nullptr, 'h'},
    {"normal", no_argument,       nullptr, 'n'},
    {"fast",   no_argument,       nullptr, 'f'},
    {"record", no_argument,       nullptr, 'r'},
    {"ac",     required_argument, nullptr, 'c'},
    {"ar",     required_argument, nullptr, 's'},
    {0, 0, 0, 0}
  };
  struct hint hints[] =
  {
    {"",                  "\n       Print this help message"              },
    {"Normal Mode",       "\n       Use record library to record file"    },
    {"Fast Mode",         "\n       Use fastboot + hibernation"           },
    {"Enable Record",     "\n       Enable file recording with fast mode" },
    {"Audio channel",     "\n       Specify audio channel number"         },
    {"Audio Sample Rate", "\n       Specify audio sample rate"            },
  };


  do {
    char short_options[] = ":hnfrc:s:";
    bool need_break = false;
    bool normal = false;
    bool fast = false;
    bool record = false;
    int val = -1;
    uint32_t audio_channel     = 1;
    uint32_t audio_sample_rate = 48000;
    ORYX_MODULE_MODE mode = ORYX_MODULE_MODE_UNKNOWN;
    while(!need_break && ((val = getopt_long(argc,
                                             argv,
                                             short_options,
                                             long_options,
                                             nullptr)) != -1)) {
      switch(val) {
        case 'n': {
          normal = true;
        }break;
        case 'f': {
          fast = true;
        }break;
        case 'r': {
          record = true;
        }break;
        case 'c': {
          uint32_t ch = strtoul(optarg, nullptr, 10);
          switch(ch) {
            case 1:
            case 2: audio_channel = ch; break;
            default: {
              ERROR("Invalid argument: -%c %s", val, optarg);
            }break;
          }
        }break;
        case 's': {
          uint32_t rate = strtoul(optarg, nullptr, 10);
          switch(rate) {
            case 16000:
            case 48000: audio_sample_rate = rate; break;
            default: {
              ERROR("Invalid argument: -%c %s", val, optarg);
              need_break = true;
            }break;
          }
        }break;
        case ':': {
          ERROR("Options -%c requires an argument!", optopt);
          need_break = true;
        }break;
        case 'h':
        default:  {
          usage(long_options, hints);
          need_break = true;
        }break;
      }
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
    if (AM_UNLIKELY(normal && fast)) {
      ERROR("Please run in normal mode or fast mode! Use either -n or -f");
      break;
    }
    if (AM_UNLIKELY(!fast && record)) {
      ERROR("-r --record can only work with -f --fast");
      break;
    }
    if (AM_LIKELY(normal)) {
      mode = ORYX_MODULE_MODE_NORMAL;
    }
    if (AM_LIKELY(fast)) {
      mode = ORYX_MODULE_MODE_FAST;
      if (AM_LIKELY(record)) {
        mode = ORYX_MODULE_MODE_FAST_RECORD;
      }
    }

    switch(mode) {
      case ORYX_MODULE_MODE_NORMAL: {
        signal(SIGINT,  signal_handler_normal);
        signal(SIGQUIT, signal_handler_normal);
        signal(SIGTERM, signal_handler_normal);
      }break;
      case ORYX_MODULE_MODE_FAST: {
        signal(SIGUSR1, signal_handler_fast);
      }break;
      case ORYX_MODULE_MODE_FAST_RECORD: {
        signal(SIGUSR1, signal_handler_fast);
        signal(SIGUSR2, signal_handler_fast);
      }break;
      case ORYX_MODULE_MODE_UNKNOWN:
      default: {
        usage(long_options, hints);
        need_break = true;
      }break;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }

    if (!(g_camera = AMOryxModules::create(mode))) {
      ret = AM_RESULT_ERR_MEM;
      PRINTF("Create oryx modules handle failed!\n");
      break;
    }

    if ((ret = g_camera->start()) != AM_RESULT_OK) {
      PRINTF("Start oryx modules failed!\n");
      break;
    }

    switch(mode) {
      case ORYX_MODULE_MODE_NORMAL: {
        is_running = true;
        normal_mode_loop();
      } break;
      case ORYX_MODULE_MODE_FAST:
        if ((ret = g_camera->set_audio_parameters(
            audio_channel, audio_sample_rate,
            1024 * audio_channel,
            AM_SAMPLE_S16LE,
            static_audio_capture)) != AM_RESULT_OK) {
          ERROR("set audio parameters failed!");
          break;
        }
        //no break here
      case ORYX_MODULE_MODE_FAST_RECORD: {
        bool done = false;
        nl_config_t conf;
        conf.msg.pid = getpid();
        conf.msg.port = NL_PORT_SYS;
        if (init_netlink(conf) < 0) {
          break;
        }

        do {
          if (receive_msg_from_kernel(conf) < 0) {
            break;
          }

          switch (conf.msg.type) {
            case NL_MSG_TYPE_REQUEST:
              PRINTF("Request!!!");
              break;
            case NL_MSG_TYPE_SESSION:
              PRINTF("Session!!!");
              break;
            case NL_MSG_TYPE_NOTICE:
              if (conf.msg.status == NL_NOTICE_SYS_THAW_DONE) {
                printf("\n[%jd]##### System Resume #####\n",
                       get_timestamp_ms());
                (mode == ORYX_MODULE_MODE_FAST) ?
                    fast_mode_loop():
                    fast_record_mode_loop();
                done = true;
              }
              PRINTF("Notice!!!");
              break;
            default:
              ERROR("Invalid MSG type: %d",conf.msg.type);
          }
        } while (!done);

        deinit_netlink(conf);
        g_camera->stop_all_video_plugins();
        g_camera->stop_image();
      } break;
      case ORYX_MODULE_MODE_UNKNOWN:
      default: break;
    }

  } while (0);
  if (g_camera) {
    g_camera->destory();
  }
  return ret;
}
