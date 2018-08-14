/*******************************************************************************
 * video_service_operation_guide.cpp
 *
 * History:
 *   May 24, 2017 - [Guohua Zheng] created file
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

#include<getopt.h>
#include<signal.h>
#include<math.h>
#include<string>

#include "am_log.h"
#include "am_define.h"
#include "am_api_helper.h"
#include "am_video_types.h"
#include "am_api_video.h"
#include "am_api_media.h"
#include "am_configure.h"
#include "am_record_event_sender.h"

enum BUFFER_TYPE {
  OFF = 0,
  ENCODE = 1,
  PREVIEW = 2,
};

enum STREAM_TYPE {
  NONE = 0,
  H264 = 1,
  H265 = 2,
  MJPEG = 3,
};

struct setting_option {
    int32_t value;
    bool is_set;
};

struct buffer_fmt_setting {
    bool is_set;
    setting_option type;
    setting_option input_crop;
    setting_option input_width;
    setting_option input_height;
    setting_option input_offset_x;
    setting_option input_offset_y;
    setting_option width;
    setting_option height;
    setting_option prewarp;
    setting_option cap_skip_itvl;
    setting_option auto_stop;
};

struct stream_fmt_setting {
    bool is_set;
    setting_option enable;
    setting_option type;
    setting_option source;
    setting_option framerate;
    setting_option width;
    setting_option heigth;
    setting_option offset_x;
    setting_option offset_y;
    setting_option hflip;
    setting_option vflip;
    setting_option rotate;
};

buffer_fmt_setting g_main_buf_set =
{
 true,
 {ENCODE, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {1920, true},
 {1080, true},
 {0, false},
 {0, false},
 {0, false}
};

buffer_fmt_setting g_sec_buf_set =
{
 true,
 {ENCODE, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {720, true},
 {480, true},
 {0, false},
 {0, false},
 {0, false}
};

buffer_fmt_setting g_thd_buf_set =
{
 true,
 {OFF, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
};

buffer_fmt_setting g_fth_buf_set =
{
 true,
 {ENCODE, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {1280, true},
 {720, true},
 {0, false},
 {0, false},
 {0, false}
};

stream_fmt_setting g_a_stream_set =
{
 true,
 {true, true},
 {H265, true},
 {0, true},
 {30, true},
 {1920, true},
 {1080, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false}
};

stream_fmt_setting g_b_stream_set =
{
 true,
 {true, true},
 {H264, true},
 {1, true},
 {30, true},
 {720, true},
 {480, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false}
};

stream_fmt_setting g_c_stream_set =
{
 true,
 {true, true},
 {MJPEG, true},
 {3, true},
 {15, true},
 {1280, true},
 {720, true},
 {0, false},
 {0, false},
 {0, false},
 {0, false},
 {0, false}
};

std::map<uint32_t,buffer_fmt_setting> g_buf_fmt_map =
{
 {0, g_main_buf_set},
 {1, g_sec_buf_set},
 {2, g_thd_buf_set},
 {3, g_fth_buf_set}
};

std::map<uint32_t, stream_fmt_setting> g_stream_fmt_map =
{
 {0, g_a_stream_set},
 {1, g_b_stream_set},
 {2, g_c_stream_set}
};

static AMAPIHelperPtr g_api_helper = nullptr;
static bool g_mp4_muxer_id_bit_map = 1;


static void sigstop(int32_t arg)
{
  INFO("signal comes!\n");
  exit(1);
}

static void feature_usage()
{
  PRINTF("===========================\n");
  PRINTF("m ------ Record mp4 file\n");
  PRINTF("j ------ JPEG event\n");
  PRINTF("e ------ Mp4 event\n");
  PRINTF("q ------ Quit\n");
  PRINTF("===========================\n");
}

static void file_record_usage()
{
  PRINTF("=============================\n");
  PRINTF("b ------ Start file recording\n");
  PRINTF("c ------ Stop file recording\n");
  PRINTF("q ------ Quit\n");
  PRINTF("=============================\n");
}

static void event_record_usage()
{
  PRINTF("===========================\n");
  PRINTF("b ------ Start event file recording\n");
  PRINTF("q ------ Quit\n");
  PRINTF("=============================\n");
}

static int32_t set_buffer_fmt()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_buffer_fmt_t buffer_fmt = { 0 };
  bool has_setting = false;

  for (auto &m: g_buf_fmt_map) {
    if (!m.second.is_set) {
      continue;
    }
    buffer_fmt.buffer_id = m.first;

    if (m.second.type.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_TYPE_EN_BIT);
      buffer_fmt.type = m.second.type.value;
      has_setting = true;
    }

    if (m.second.input_crop.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_CROP_EN_BIT);
      buffer_fmt.input_crop = m.second.input_crop.value;
      has_setting = true;
    }

    if (m.second.input_width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT);
      buffer_fmt.input_width = m.second.input_width.value;
      has_setting = true;
    }

    if (m.second.input_height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
      buffer_fmt.input_height = m.second.input_height.value;
      has_setting = true;
    }

    if (m.second.input_offset_x.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT);
      buffer_fmt.input_offset_x = m.second.input_offset_x.value;
      has_setting = true;
    }

    if (m.second.input_offset_y.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT);
      buffer_fmt.input_offset_y = m.second.input_offset_y.value;
      has_setting = true;
    }

    if (m.second.width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
      buffer_fmt.width = m.second.width.value;
      has_setting = true;
    }
    if (m.second.height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
      buffer_fmt.height = m.second.height.value;
      has_setting = true;
    }
    if (m.second.prewarp.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_PREWARP_EN_BIT);
      buffer_fmt.prewarp = m.second.prewarp.value;
      has_setting = true;
    }
    if (m.second.cap_skip_itvl.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_CAP_SKIP_ITVL_EN_BIT);
      buffer_fmt.cap_skip_itvl = m.second.cap_skip_itvl.value;
      has_setting = true;
    }
    if (m.second.auto_stop.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_AUTO_STOP_EN_BIT);
      buffer_fmt.auto_stop = m.second.auto_stop.value;
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,
                                &buffer_fmt, sizeof(buffer_fmt),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("Failed to set Stream buffer format!");
        break;
      }
    }
  }

  return ret;
}

static int32_t set_stream_fmt()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_stream_fmt_t stream_fmt = { 0 };
  bool has_setting = false;

  for (auto &m : g_stream_fmt_map) {
    if (!m.second.is_set) {
      continue;
    }
    stream_fmt.stream_id = m.first;

    if (m.second.enable.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_ENABLE_EN_BIT);
      stream_fmt.enable = m.second.enable.value;
      has_setting = true;
    }
    if (m.second.type.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_TYPE_EN_BIT);
      stream_fmt.type = m.second.type.value;
      has_setting = true;
    }
    if (m.second.source.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_SOURCE_EN_BIT);
      stream_fmt.source = m.second.source.value;
      has_setting = true;
    }
    if (m.second.framerate.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_FRAME_RATE_EN_BIT);
      stream_fmt.frame_rate = m.second.framerate.value;
      has_setting = true;
    }
    if (m.second.width.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_WIDTH_EN_BIT);
      stream_fmt.width = m.second.width.value;
      has_setting = true;
    }
    if (m.second.heigth.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_HEIGHT_EN_BIT);
      stream_fmt.height = m.second.heigth.value;
      has_setting = true;
    }
    if (m.second.offset_x.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_OFFSET_X_EN_BIT);
      stream_fmt.offset_x = m.second.offset_x.value;
      has_setting = true;
    }
    if (m.second.offset_y.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_OFFSET_Y_EN_BIT);
      stream_fmt.offset_y = m.second.offset_y.value;
      has_setting = true;
    }
    if (m.second.hflip.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_HFLIP_EN_BIT);
      stream_fmt.hflip = m.second.hflip.value;
      has_setting = true;
    }
    if (m.second.vflip.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_VFLIP_EN_BIT);
      stream_fmt.vflip = m.second.vflip.value;
      has_setting = true;
    }
    if (m.second.rotate.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_ROTATE_EN_BIT);
      stream_fmt.rotate = m.second.rotate.value;
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET,
                                &stream_fmt, sizeof(stream_fmt),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("Failed to set stream format config");
        break;
      }
    }
  }
  return ret;
}

static int32_t start_encode()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START,
                            nullptr, 0,
                            &service_result, sizeof(service_result));

  if((ret = service_result.ret) != 0) {
    ERROR("Failed to start encoding");
  }
  return ret;
}

static int32_t stop_encode()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP,
                            nullptr, 0,
                            &service_result, sizeof(service_result));

  if((ret = service_result.ret) != 0) {
    ERROR("Failed to stop encoding");
  }
  return ret;
}

static int32_t load_all_cfg()
{
  int32_t ret = 0;
  INFO("Load all configures!!!\n");
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to load all configs!\n");
  }

  return ret;
}

static int32_t mp4_start_file_recording()
{
  uint32_t ret = 0;

  am_service_result_t service_result;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
                              &g_mp4_muxer_id_bit_map,
                              sizeof(g_mp4_muxer_id_bit_map),
                              &service_result, sizeof(service_result));
    if(service_result.ret != AM_RESULT_OK) {
      ERROR("Failed to start mp4 file writing");
      ret = -1;
      break;
    }
  } while(0);

  return ret;
}

static int32_t mp4_stop_file_recording()
{
  uint32_t ret = 0;

  am_service_result_t service_result;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
                              &g_mp4_muxer_id_bit_map,
                              sizeof(g_mp4_muxer_id_bit_map),
                              &service_result, sizeof(service_result));

    if (service_result.ret != AM_RESULT_OK) {
      ERROR("Failed to stop mp4 file writing");
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

static int32_t mp4_event_start()
{
  uint32_t ret = 0;
  AMEventStruct param;

  am_service_result_t service_result;
  param.attr = AM_EVENT_H26X;
  param.h26x.stream_id_bit_map = 1;
  param.h26x.history_duration = 1;
  param.h26x.future_duration = 0;
  param.h26x.history_voice_duration = -1;
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              &param, sizeof(param),
                              &service_result, sizeof(service_result));

    if (service_result.ret != AM_RESULT_OK) {
      ERROR("Failed to start mp4 event");
      ret = -1;
      break;
    }
  } while(0);

  return ret;
}

static int32_t jpeg_event_start()
{
  uint32_t ret = 0;
  AMEventStruct param;

  am_service_result_t service_result;
  param.attr = AM_EVENT_MJPEG;
  param.mjpeg.stream_id_bit_map = 4;
  param.mjpeg.after_cur_pts_num = 0;
  param.mjpeg.pre_cur_pts_num = 0;
  param.mjpeg.closest_cur_pts_num = 1;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              &param, sizeof(param),
                              &service_result, sizeof(service_result));

    if (service_result.ret != AM_RESULT_OK) {
      ERROR("Failed to start JPEG event");
      ret = -1;
      break;
    }

  } while (0);

  return ret;

}

static void mp4_record_process()
{
  bool run = true;
  file_record_usage();
  while (run) {
    char sub_char = getchar();
    switch (sub_char) {
      case 'b':
        mp4_start_file_recording();
        break;
      case 'c':
        mp4_stop_file_recording();
        break;
      case 'q':
        run = false;
        break;
      case '\n':
        break;
      default :
        file_record_usage();
        break;
    }
  }
}

static void mp4_event_record_process()
{
  bool run = true;
  event_record_usage();
  while (run) {
    char sub_char = getchar();
    switch (sub_char) {
      case 'b':
        mp4_event_start();
        break;
      case 'q':
        run = false;
        break;
      case '\n':
        break;
      default:
        event_record_usage();
        break;
    }
  }
}

static void jpeg_event_record_process()
{
  bool run = true;
  event_record_usage();
  while (run) {
    char sub_char = getchar();
    switch (sub_char) {
      case 'b':
        jpeg_event_start();
        break;
      case 'q':
        run = false;
        break;
      case '\n':
        break;
      default:
        event_record_usage();
        break;
    }
  }
}

static void fea_process()
{
  bool run_flag = true;
  while (run_flag) {
    char read_char = getchar();
    switch (read_char) {
      case 'm':
        mp4_record_process();
        break;
      case 'e':
        mp4_event_record_process();
        break;
      case 'j':
        jpeg_event_record_process();
        break;
      case 'q':
        run_flag = false;
        break;
      default:
        feature_usage();
        break;
    }
  }
}

int main(int argc, char** argv)
{
  int ret = 1;
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  g_api_helper = AMAPIHelper::get_instance();

  if (!g_api_helper) {
    ERROR("Unable to get AMAPIHelper instance");
    return -1;
  }

  do {

    if ((ret = set_buffer_fmt()) != 0) {
      break;
    }

    if ((ret = set_stream_fmt()) != 0) {
      break;
    }

    if ((ret = stop_encode()) != 0) {
      break;
    }

    if ((ret = load_all_cfg()) != 0) {
      break;
    }

    if ((ret = start_encode()) != 0) {
      break;
    }

    feature_usage();

    fea_process();
  } while (0);
  return ret;
}
