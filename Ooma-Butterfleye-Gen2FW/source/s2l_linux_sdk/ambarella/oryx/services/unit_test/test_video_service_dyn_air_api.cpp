/*
 * test_video_service_dyn_air_api.cpp
 *
 *  History:
 *    Nov 27, 2015 - [Shupeng Ren] created file
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
 */
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>
#include "uuid.h"

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_api_helper.h"
#include "am_api_video.h"
#include "am_video_types.h"
#include "am_signal.h"

using std::map;
using std::string;
using std::vector;
using std::pair;
using std::make_pair;

#define NO_ARG 0
#define HAS_ARG 1

#define MAX_STREAM_NUM 4
#define MAX_MD_ROI_NUM 4

#define VERIFY_PARA_1(x, min) \
    do { \
      if ((x) < min) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_1_FLOAT(x, min) \
    do { \
      if ((x) < min) { \
        printf("Parameter wrong: %f\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2_FLOAT(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        printf("Parameter wrong: %f\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_CHANNELID(x)   do {    \
      if (((x) < 0) || ((x) >= AM_MULTI_VIN_CHAN_MAX_NUM)) {  \
        printf ("channel id wrong %d \n", (x));     \
        return -1;  \
      } \
    } while (0)

#define VERIFY_BUFFERID(x)  do {    \
      if ((x) <= AM_SOURCE_BUFFER_INVALID || ((x) >= AM_SOURCE_BUFFER_MAX)) {  \
        printf ("buffer id wrong %d\n", (x));     \
        return -1;  \
      } \
    } while (0)

static AMAPIHelperPtr g_api_helper = nullptr;
static bool show_lock_state = false;
static bool show_status_flag = false;
static bool show_vin_info_flag = false;
static bool show_buffer_flag = false;
static bool show_canvas_flag = false;
static bool save_buffer_cfg_flag = false;
static bool save_stream_cfg_flag = false;
static bool save_warp_cfg_flag = false;
static bool save_eis_cfg_flag = false;
static bool save_md_cfg_flag = false;
static bool save_lbr_cfg_flag = false;
static bool show_info_flag = false;
static bool show_warp_flag = false;
static bool show_dptz_ratio_flag = false;
static bool show_dptz_size_flag = false;
static bool set_dptz_ratio_flag = false;
static bool set_dptz_size_flag = false;
static bool show_lbr_flag = false;
static bool show_eis_flag = false;
static bool show_motion_detect_flag = false;
static bool show_stream_info_flag = false;
static bool show_avail_cpu_flag = false;
static bool show_cur_cpu_flag = false;
static bool set_cur_cpu_flag = false;
static bool show_working_mode_flag = false;
static bool start_flag = false;
static bool stop_flag = false;
static bool goto_vca_mode_flag = false;
static bool goto_iav_unloaded_mode_flag = false;
static bool goto_iav_current_mode_flag = false;
static bool save_overlay_flag = false;
static bool get_platform_stream_num_max_flag = false;
static bool get_platform_buffer_num_max_flag = false;
static bool get_overlay_max_flag = false;
static bool destroy_overlay_flag = false;
static bool fd_recog_enable = false;
static bool fd_recog_disable =  false;
static uint32_t resume_streaming_id = 0;
static uint32_t pause_streaming_id = 0;
static uint32_t restart_streaming_id = 0;
static uint32_t force_idr_id = 0;
AM_STREAM_ID current_stream_id = AM_STREAM_ID_INVALID;
AM_MULTI_VIN_CHAN_ID current_channel_id = AM_MULTI_VIN_CHAN_0;
AM_SOURCE_BUFFER_ID current_buffer_id = AM_SOURCE_BUFFER_INVALID;
uint32_t current_md_roi_id = 0;
int32_t cpu_index = -1;

static void sigstop(int32_t arg)
{
  INFO("signal comes!\n");
  exit(1);
}

struct setting_option {
    bool is_set;
    union {
        bool     v_bool;
        float    v_float;
        int32_t  v_int;
        uint32_t v_uint;
    } value;

    string str;
};

static setting_option vout_stop;

struct buffer_setting {
  bool is_set;
  setting_option width;
  setting_option height;
  setting_option output_x;
  setting_option output_y;
  setting_option input_x;
  setting_option input_y;
  setting_option input_w;
  setting_option input_h;
};

struct dptz_ratio_setting {
    bool is_set;
    setting_option pan;
    setting_option tilt;
    setting_option zoom;
};

struct dptz_size_setting {
    bool is_set;
    setting_option x;
    setting_option y;
    setting_option w;
    setting_option h;
};

struct warp_setting {
    bool is_set;
    setting_option region_id;
    setting_option warp_mode;
    setting_option max_radius;
    setting_option ldc_strength;
    setting_option pano_hfov_degree;
    setting_option ver_pano_fov_degree;
    setting_option warp_region_yaw;
    setting_option warp_region_pitch;
    setting_option warp_region_rotate;
    setting_option warp_zoom;
    setting_option hor_zoom;
    setting_option ver_zoom;
    setting_option buffer_id;
    setting_option pan_angle;
    setting_option tilt_angle;
    setting_option sub_roi_offset_x;
    setting_option sub_roi_offset_y;
    warp_region_dptz_t warp_dptz[MAX_WARP_AREAS];
};

typedef pair<int32_t, int32_t> rtg_point;
typedef pair<rtg_point, rtg_point> rtg_point_pair;

struct overlay_data_setting {
    setting_option w;
    setting_option h;
    setting_option x;
    setting_option y;
    setting_option type;
    setting_option str;
    setting_option pre_str;
    setting_option suf_str;
    setting_option spacing;
    setting_option en_msec;
    setting_option time_format;
    setting_option is_12h;
    setting_option bg_color;
    setting_option font_name;
    setting_option font_size;
    setting_option font_color;
    setting_option font_outline_w;
    setting_option font_outline_color;
    setting_option font_hor_bold;
    setting_option font_ver_bold;
    setting_option font_italic;
    setting_option bmp;
    setting_option color_key;
    setting_option color_range;
    setting_option bmp_num;
    setting_option interval;
    setting_option line_color;
    setting_option thickness;
    setting_option points_num;
    setting_option rtg_thickness;
    setting_option rtg_color;
    pair<uint32_t,uint32_t> p[OVERLAY_MAX_POINT];
    vector<rtg_point_pair> rtg_point_vector;
    int16_t data_idx;
    bool is_set;
};

struct overlay_attr_setting {
    bool is_set;
    setting_option  x;
    setting_option  y;
    setting_option  w;
    setting_option  h;
    setting_option  rotate;
    setting_option  buf_num;
    setting_option  bg_color;
};

struct overlay_area_setting {
    bool    is_set   = false;
    int32_t area_id  = 0;
    int32_t data_idx = 0;
    uint32_t type    = -1;
    overlay_attr_setting         attr;
    vector<overlay_data_setting> data;
};

struct overlay_setting {
    bool                  is_set;
    overlay_area_setting  init;
    overlay_area_setting  add_data;
    overlay_area_setting  update_data;
    overlay_area_setting  remove_data;
    overlay_area_setting  remove;
    overlay_area_setting  enable;
    overlay_area_setting  disable;
    overlay_area_setting  show;
};

struct lbr_setting {
    bool is_set;
    setting_option lbr_enable;
    setting_option lbr_auto_bitrate_target;
    setting_option lbr_bitrate_ceiling;
    setting_option lbr_frame_drop;
};

struct eis_setting {
    bool is_set;
    setting_option eis_mode;
};

struct md_roi_config {
    bool is_set;
    setting_option threshold0;
    setting_option threshold1;
    setting_option level_change_delay0;
    setting_option level_change_delay1;
    setting_option roi_valid;
    setting_option roi_left;
    setting_option roi_right;
    setting_option roi_top;
    setting_option roi_bottom;
};

struct motion_detect_setting
{
    bool is_set;
    setting_option enable;
    setting_option buf_id;
    setting_option buf_type;
    md_roi_config  md_roi_cfg[MAX_MD_ROI_NUM];
};

struct stream_params_setting {
    bool is_set;
    setting_option source;
    setting_option type;
    setting_option framerate;
    setting_option target_bitrate;
    setting_option absolute_bitrate;
    setting_option w;
    setting_option h;
    setting_option x;
    setting_option y;
    setting_option flip;
    setting_option rotate;
    setting_option profile;
    setting_option gop_n;
    setting_option gop_idr;
    setting_option quality;
    setting_option latency;
    setting_option bandwidth;
};

struct stream_lock_setting {
    bool is_set;
    setting_option operation;
    setting_option op_result;
    char           uuid[40] = {0};
};

typedef map<AM_STREAM_ID, stream_lock_setting> stream_lock_setting_map;
static stream_lock_setting_map g_stream_lock_setting;
typedef map<AM_STREAM_ID, overlay_setting> overlay_setting_map;
static overlay_setting_map g_overlay_setting;
static lbr_setting g_lbr_cfg[MAX_STREAM_NUM];
static warp_setting g_warp_cfg;
typedef map<AM_SOURCE_BUFFER_ID, dptz_ratio_setting> buf_dptz_ratio_map;
typedef map<AM_MULTI_VIN_CHAN_ID, buf_dptz_ratio_map> chan_dptz_ration_map;
static chan_dptz_ration_map g_dptz_ratio_cfg;
typedef map<AM_SOURCE_BUFFER_ID, dptz_size_setting> buf_dptz_size_map;
typedef map<AM_MULTI_VIN_CHAN_ID, buf_dptz_size_map> chan_dptz_size_map;
static chan_dptz_size_map g_dptz_size_cfg;
static eis_setting g_eis_cfg;
static motion_detect_setting g_motion_detect_cfg;
typedef map<AM_SOURCE_BUFFER_ID, buffer_setting> buffer_setting_map;
typedef map<AM_MULTI_VIN_CHAN_ID, buffer_setting_map> channel_setting_map;
static channel_setting_map g_channel_cfg;
typedef map<AM_STREAM_ID, stream_params_setting> stream_params_setting_map;
static stream_params_setting_map g_stream_params_setting;

enum numeric_short_options {
  NONE = 0,
  H264 = 1,
  H265 = 2,
  MJPEG = 3,

  OFF = 0,
  ENCODE = 1,
  PREVIEW = 2,

  CHROMA_420 = 0,
  CHROMA_422 = 1,
  CHROMA_MONO = 2,

  NEMERIC_BEGIN = 'z',

  STREAM_NUM_MAX,
  BUFFER_NUM_MAX,

  SHOW_LOCK_STATE,
  LOCK_STREAM,
  UNLOCK_STREAM,

  //dynamic control
  VOUT_HALT,

  FORCE_IDR,

  //DPTZ Warp config
  REGION_ID,
  LDC_WARP_MODE,
  LDC_MAX_RADIUS,
  LDC_STRENGTH,
  PANO_HFOV_DEGREE,
  VER_PANO_FOV_DEGREE,
  WARP_REGION_YAW,
  WARP_REGION_PITCH,
  WARP_REGION_ROTATE,
  WARP_ZOOM,
  WARP_HOR_ZOOM,
  WARP_VER_ZOOM,
  PAN_ANGLE,
  TILT_ANGLE,
  SUB_ROI_OFFSET,
  WARP_SAVE_CFG,
  WARP_DPTZ_INPUT_SIZE,
  WARP_DPTZ_INPUT_OFFSET,
  WARP_DPTZ_OUTPUT_SIZE,
  WARP_DPTZ_OUTPUT_OFFSET,
  PAN_RATIO,
  TILT_RATIO,
  ZOOM_RATIO,
  DPTZ_X,
  DPTZ_Y,
  DPTZ_W,
  DPTZ_H,

  //Overlay config
  OVERLAY_MAX,
  OVERLAY_INIT,
  OVERLAY_DATA_ADD,
  OVERLAY_DATA_UPDATE,
  OVERLAY_DATA_REMOVE,
  OVERLAY_REMOVE,
  OVERLAY_ENABLE,
  OVERLAY_DISABLE,
  OVERLAY_SHOW,
  OVERLAY_SAVE,
  OVERLAY_DESTROY,
  OVERLAY_AREA_BG_COLOR,
  OVERLAY_AREA_BUF_NUM,
  OVERLAY_AREA_ROTATE,
  OVERLAY_W,
  OVERLAY_H,
  OVERLAY_X,
  OVERLAY_Y,
  OVERLAY_DATA_INDEX,
  OVERLAY_DATA_TYPE,
  OVERLAY_TEXT_STRING,
  OVERLAY_TEXT_BG_COLOR,
  OVERLAY_TEXT_SPACING,
  OVERLAY_TIME_PREFIX_STRING,
  OVERLAY_TIME_SUFFIX_STRING,
  OVERLAY_TIME_MSEC_EN,
  OVERLAY_TIME_TIME_FORMAT,
  OVERLAY_TIME_IS_12H,
  OVERLAY_FONT_TYPE,
  OVERLAY_FONT_SIZE,
  OVERLAY_FONT_COLOR,
  OVERLAY_FONT_OUTLINE_WIDTH,
  OVERLAY_FONT_OUTLINE_COLOR,
  OVERLAY_FONT_HOR_BOLD,
  OVERLAY_FONT_VER_BOLD,
  OVERLAY_FONT_ITALIC,
  OVERLAY_BMP,
  OVERLAY_BMP_NUM,
  OVERLAY_BMP_INTERVAL,
  OVERLAY_COLOR_KEY,
  OVERLAY_COLOR_RANGE,
  OVERLAY_LINE_COLOR,
  OVERLAY_LINE_THICKNESS,
  OVERLAY_LINE_POINT,
  OVERLAY_RTG_COLOR,
  OVERLAY_RTG_THICKNESS,
  OVERLAY_H_RTG_POINT_PAIR,//hollow rectangle
  OVERLAY_S_RTG_POINT_PAIR,//solid rectangle

  //LBR
  LBR_ENABLE,
  LBR_AUTO_BITRATE_TARGET,
  LBR_BITRATE_CEILING,
  LBR_FRAME_DROP,
  LBR_SAVE_CFG,

  //EIS
  EIS_MODE,
  EIS_SAVE_CFG,

  //Motion Detect
  MD_ROI_INDEX,
  MD_ENABLE,
  MD_BUF_ID,
  MD_BUF_TYPE,
  MD_TH0,
  MD_TH1,
  MD_LCD0,
  MD_LCD1,
  MD_ROI_VALID,
  MD_ROI_LEFT,
  MD_ROI_RIGHT,
  MD_ROI_TOP,
  MD_ROI_BOTTOM,
  MD_SAVE_CFG,

  //FACE
  FD_RECOG_ENABLE,
  FD_RECOG_DISABLE,

  //Buffer
  BUFFER_INPUT_X,
  BUFFER_INPUT_Y,
  BUFFER_INPUT_W,
  BUFFER_INPUT_H,
  BUFFER_OUTPUT_X,
  BUFFER_OUTPUT_Y,
  BUFFER_SAVE_CFG,

  //stream bitrate/frame factor
  BITRATE_SET,
  FRAMERATE_SET,
  MJPEG_QUALITY_SET,
  H26x_GOP_N_SET,
  H26x_GOP_IDR_SET,
  STREAM_SOURCE_SET,
  STREAM_TYPE_SET,
  STREAM_SIZE_W,
  STREAM_SIZE_H,
  STREAM_OFFSET_X,
  STREAM_OFFSET_Y,
  ABS_BITRATE_SET,
  STREAM_FLIP,
  STREAM_ROTATE,
  STREAM_PROFILE,
  STREAM_SAVE_CFG,
  LATENCY,
  BANDWIDTH,

  GET_CPU_CLKS,
  GET_CPU_CLK,
  SET_CPU_CLK,

  SET_VCA_MODE,
  SET_IAV_UNLOADED_MODE,
  SET_IAV_CURRENT_MODE,

  SAVE_ALL_CFG,
  STOP_ENCODE,
  START_ENCODE,
  STOP_STREAMING,
  START_STREAMING,
  RESTART_STREAMING,

  SHOW_WORKING_MODE,
  SHOW_VIN_INFO,
  SHOW_BUFFER_INFO,
  SHOW_CANVAS_INFO,
  SHOW_STREAM_STATUS,
  SHOW_STREAM_INFO,
  SHOW_WARP_CFG,
  SHOW_DPTZ_RATIO_CFG,
  SHOW_DPTZ_SIZE_CFG,
  SHOW_LBR_CFG,
  SHOW_EIS_CFG,
  SHOW_MD_CFG,
  SHOW_ALL_INFO
};

static struct option long_options[] =
{
 {"help",               NO_ARG,   0,  'h'},

 {"channel",            HAS_ARG,  0,  'c'},

 {"buffer-max-n",       NO_ARG,   0,  BUFFER_NUM_MAX},
 {"buffer",             HAS_ARG,  0,  'b'},

 {"width",              HAS_ARG,  0,  'W'},
 {"height",             HAS_ARG,  0,  'H'},
 {"output-x",           HAS_ARG,  0,  BUFFER_OUTPUT_X},
 {"output-y",           HAS_ARG,  0,  BUFFER_OUTPUT_Y},

 {"input-x",            HAS_ARG,  0,  BUFFER_INPUT_X},
 {"input-y",            HAS_ARG,  0,  BUFFER_INPUT_Y},
 {"input-w",            HAS_ARG,  0,  BUFFER_INPUT_W},
 {"input-h",            HAS_ARG,  0,  BUFFER_INPUT_H},
 {"bf-save-cfg",        NO_ARG,   0,  BUFFER_SAVE_CFG},

 {"stream-max-n",       NO_ARG,   0,  STREAM_NUM_MAX},
 {"stream",             HAS_ARG,  0,  's'},

 {"show-lock-state",    NO_ARG,   0,  SHOW_LOCK_STATE},
 {"lock",               HAS_ARG,  0,  LOCK_STREAM},
 {"unlock",             HAS_ARG,  0,  UNLOCK_STREAM},

 {"vout-halt",          HAS_ARG,  0,  VOUT_HALT},
 {"force-idr",          NO_ARG,   0,  FORCE_IDR},

 {"region-id",          HAS_ARG,  0,  REGION_ID},
 {"ldc-mode",           HAS_ARG,  0,  LDC_WARP_MODE},
 {"max-radius",         HAS_ARG,  0,  LDC_MAX_RADIUS},
 {"ldc-strength",       HAS_ARG,  0,  LDC_STRENGTH},
 {"pano-hfov-degree",   HAS_ARG,  0,  PANO_HFOV_DEGREE},
 {"ver-pano-fov-degree",HAS_ARG,  0,  VER_PANO_FOV_DEGREE},
 {"warp-region-yaw",    HAS_ARG,  0,  WARP_REGION_YAW},
 {"warp-region-pitch",  HAS_ARG,  0,  WARP_REGION_PITCH},
 {"warp-region-rotate", HAS_ARG,  0,  WARP_REGION_ROTATE},
 {"warp-zoom",          HAS_ARG,  0,  WARP_ZOOM},
 {"warp-hor-zoom",      HAS_ARG,  0,  WARP_HOR_ZOOM},
 {"warp-ver-zoom",      HAS_ARG,  0,  WARP_VER_ZOOM},
 {"pan-angle",          HAS_ARG,  0,  PAN_ANGLE},
 {"tilt-angle",         HAS_ARG,  0,  TILT_ANGLE},
 {"sub-roi-offset",     HAS_ARG,  0,  SUB_ROI_OFFSET},
 {"warp-save-cfg",      NO_ARG,   0,  WARP_SAVE_CFG},
 {"dptz-input-size",    HAS_ARG,  0,  WARP_DPTZ_INPUT_SIZE},
 {"dptz-input-offset",  HAS_ARG,  0,  WARP_DPTZ_INPUT_OFFSET},
 {"dptz-output-size",   HAS_ARG,  0,  WARP_DPTZ_OUTPUT_SIZE},
 {"dptz-output-offset", HAS_ARG,  0,  WARP_DPTZ_OUTPUT_OFFSET},
 {"pan-ratio",          HAS_ARG,  0,  PAN_RATIO},
 {"tilt-ratio",         HAS_ARG,  0,  TILT_RATIO},
 {"zoom-ratio",         HAS_ARG,  0,  ZOOM_RATIO},
 {"dptz-x",             HAS_ARG,  0,  DPTZ_X},
 {"dptz-y",             HAS_ARG,  0,  DPTZ_Y},
 {"dptz-w",             HAS_ARG,  0,  DPTZ_W},
 {"dptz-h",             HAS_ARG,  0,  DPTZ_H},

 //below is overlay option
 //get platform max area number for overlay
 {"overlay-max",        NO_ARG,  0, OVERLAY_MAX},
 {"overlay-init",       NO_ARG,  0, OVERLAY_INIT}, //init a overlay area
 {"overlay-data-add",   HAS_ARG, 0, OVERLAY_DATA_ADD}, //add a data to area
 {"overlay-data-update",HAS_ARG, 0, OVERLAY_DATA_UPDATE}, //update a area data
 {"overlay-data-remove",HAS_ARG, 0, OVERLAY_DATA_REMOVE}, //remove a area data
 {"overlay-remove",     HAS_ARG, 0, OVERLAY_REMOVE}, //remove a area
 {"overlay-enable",     HAS_ARG, 0, OVERLAY_ENABLE}, //enable a area
 {"overlay-disable",    HAS_ARG, 0, OVERLAY_DISABLE}, //disable a area
 {"overlay-show",       HAS_ARG, 0, OVERLAY_SHOW}, //show a area parameters
 {"overlay-save",       NO_ARG,  0, OVERLAY_SAVE}, //save user setting
 {"overlay-destroy",    NO_ARG,  0, OVERLAY_DESTROY}, //destory all overlay
 {"overlay-w",          HAS_ARG, 0, OVERLAY_W}, //area or data block width
 {"overlay-h",          HAS_ARG, 0, OVERLAY_H}, //area or data block height
 {"overlay-x",          HAS_ARG, 0, OVERLAY_X}, //area or data block offset x
 {"overlay-y",          HAS_ARG, 0, OVERLAY_Y}, //area or data block offset y
 {"overlay-area-buf-n", HAS_ARG, 0, OVERLAY_AREA_BUF_NUM}, //area background color
 {"overlay-area-bg-c",  HAS_ARG, 0, OVERLAY_AREA_BG_COLOR}, //area background color
 {"overlay-area-rotate",HAS_ARG, 0, OVERLAY_AREA_ROTATE}, //rotate
 {"overlay-data-type",  HAS_ARG, 0, OVERLAY_DATA_TYPE}, //data block type
 {"overlay-data-idx",   HAS_ARG, 0, OVERLAY_DATA_INDEX}, //data block index in area
 {"overlay-text-str",   HAS_ARG, 0, OVERLAY_TEXT_STRING}, //string
 {"overlay-text-spacing",HAS_ARG, 0, OVERLAY_TEXT_SPACING}, //spacing for text type
 {"overlay-text-bg-c",  HAS_ARG, 0, OVERLAY_TEXT_BG_COLOR}, //text type background color
 {"overlay-time-prestr",HAS_ARG, 0, OVERLAY_TIME_PREFIX_STRING}, //prefix string add to timestamp
 {"overlay-time-sufstr",HAS_ARG, 0, OVERLAY_TIME_SUFFIX_STRING}, //suffix string add to timestamp
 {"overlay-time-msec",  HAS_ARG, 0, OVERLAY_TIME_MSEC_EN}, //whether enable msec display
 {"overlay-time-format",HAS_ARG, 0, OVERLAY_TIME_TIME_FORMAT}, //time display format
 {"overlay-time-12h",   HAS_ARG, 0, OVERLAY_TIME_IS_12H}, //whether use 12 hours notation
 {"overlay-font-t",     HAS_ARG, 0, OVERLAY_FONT_TYPE}, //font type name
 {"overlay-font-s",     HAS_ARG, 0, OVERLAY_FONT_SIZE}, //font size
 {"overlay-font-c",     HAS_ARG, 0, OVERLAY_FONT_COLOR}, //font color
 {"overlay-font-ol-w",  HAS_ARG, 0, OVERLAY_FONT_OUTLINE_WIDTH}, //font outline width
 {"overlay-font-ol-c",  HAS_ARG, 0, OVERLAY_FONT_OUTLINE_COLOR}, //text type outline color
 {"overlay-font-hb",    HAS_ARG, 0, OVERLAY_FONT_HOR_BOLD}, //font hor_bold
 {"overlay-font-vb",    HAS_ARG, 0, OVERLAY_FONT_VER_BOLD}, //font ver_bold
 {"overlay-font-i",     HAS_ARG, 0, OVERLAY_FONT_ITALIC}, //font italic
 {"overlay-bmp",        HAS_ARG, 0, OVERLAY_BMP}, //bmp file path
 {"overlay-bmp-num",    HAS_ARG, 0, OVERLAY_BMP_NUM}, //bmp number in the animation file
 {"overlay-bmp-interval",HAS_ARG, 0,OVERLAY_BMP_INTERVAL}, //frame interval to display animation
 {"overlay-color-k",    HAS_ARG, 0, OVERLAY_COLOR_KEY}, //color to transparent
 {"overlay-color-r",    HAS_ARG, 0, OVERLAY_COLOR_RANGE}, //color range to transparent
 {"overlay-line-c",     HAS_ARG, 0, OVERLAY_LINE_COLOR}, //color used for draw line
 {"overlay-line-t",     HAS_ARG, 0, OVERLAY_LINE_THICKNESS}, //thickness used for draw line
 {"overlay-line-p",     HAS_ARG, 0, OVERLAY_LINE_POINT}, //point used for draw line
 {"overlay-rtg-color",  HAS_ARG, 0, OVERLAY_RTG_COLOR},//color used for rectangle
 {"overlay-rtg-thick",  HAS_ARG, 0, OVERLAY_RTG_THICKNESS},//thickness used for rectangle
 {"overlay-h-rtg-p-pair", HAS_ARG, 0, OVERLAY_H_RTG_POINT_PAIR},//point pair used for hollow rectangle
 {"overlay-s-rtg-p-pair", HAS_ARG, 0, OVERLAY_S_RTG_POINT_PAIR},//point pair used for solid rectangle

 {"lbr-en",             HAS_ARG,  0,  LBR_ENABLE},
 {"lbr-abt",            HAS_ARG,  0,  LBR_AUTO_BITRATE_TARGET},
 {"lbr-bc",             HAS_ARG,  0,  LBR_BITRATE_CEILING},
 {"lbr-fd",             HAS_ARG,  0,  LBR_FRAME_DROP},
 {"lbr-sc",             NO_ARG,   0,  LBR_SAVE_CFG},

 {"eis-mode",           HAS_ARG,  0,  EIS_MODE},
 {"eis-save-cfg",       NO_ARG,   0,  EIS_SAVE_CFG},

 {"md-roi-index",       HAS_ARG, 0, MD_ROI_INDEX},
 {"md-enable",          HAS_ARG, 0, MD_ENABLE},
 {"md-buf-id",          HAS_ARG, 0, MD_BUF_ID},
 {"md-buf-type",        HAS_ARG, 0, MD_BUF_TYPE},
 {"md-th0",             HAS_ARG, 0, MD_TH0},
 {"md-th1",             HAS_ARG, 0, MD_TH1},
 {"md-lcd0",            HAS_ARG, 0, MD_LCD0},
 {"md-lcd1",            HAS_ARG, 0, MD_LCD1},
 {"md-roi-valid",       HAS_ARG, 0, MD_ROI_VALID},
 {"md-roi-left",        HAS_ARG, 0, MD_ROI_LEFT},
 {"md-roi-right",       HAS_ARG, 0, MD_ROI_RIGHT},
 {"md-roi-top",         HAS_ARG, 0, MD_ROI_TOP},
 {"md-roi-bottom",      HAS_ARG, 0, MD_ROI_BOTTOM},
 {"md-save-cfg",        NO_ARG,  0, MD_SAVE_CFG},

 {"fd-recog-enable",    NO_ARG, 0, FD_RECOG_ENABLE},
 {"fd-recog-disable",   NO_ARG, 0, FD_RECOG_DISABLE},

 {"stream-bitrate",       HAS_ARG,  0,  BITRATE_SET},
 {"stream-framerate",     HAS_ARG,  0,  FRAMERATE_SET},
 {"stream-mjpeg-quality", HAS_ARG,  0,  MJPEG_QUALITY_SET},
 {"stream-h26x-gop-n",    HAS_ARG,  0,  H26x_GOP_N_SET},
 {"stream-h26x-gop-idr",  HAS_ARG,  0,  H26x_GOP_IDR_SET},
 {"stream-source",        HAS_ARG,  0,  STREAM_SOURCE_SET},
 {"stream-type",          HAS_ARG,  0,  STREAM_TYPE_SET},
 {"stream-size-w",        HAS_ARG,  0,  STREAM_SIZE_W},
 {"stream-size-h",        HAS_ARG,  0,  STREAM_SIZE_H},
 {"stream-offset-x",      HAS_ARG,  0,  STREAM_OFFSET_X},
 {"stream-offset-y",      HAS_ARG,  0,  STREAM_OFFSET_Y},
 {"stream-abs-bitrate",   HAS_ARG,  0,  ABS_BITRATE_SET},
 {"stream-flip",          HAS_ARG,  0,  STREAM_FLIP},
 {"stream-rotate",        HAS_ARG,  0,  STREAM_ROTATE},
 {"stream-profile",       HAS_ARG,  0,  STREAM_PROFILE},
 {"stream-save-cfg",      NO_ARG,   0,  STREAM_SAVE_CFG},
 {"latency",              HAS_ARG,  0,  LATENCY},
 {"bandwidth",            HAS_ARG,  0,  BANDWIDTH},

 {"get-avail-cpu-clk",    NO_ARG,   0,  GET_CPU_CLKS},
 {"get-cur-cpu-clk",      NO_ARG,   0,  GET_CPU_CLK},
 {"set-cur-cpu-clk",      HAS_ARG,  0,  SET_CPU_CLK},

 {"vca-mode",             NO_ARG,   0,  SET_VCA_MODE},
 {"iav-unloaded-mode",    NO_ARG,   0,  SET_IAV_UNLOADED_MODE},
 {"iav-current-mode",     NO_ARG,   0,  SET_IAV_CURRENT_MODE},

 {"stop",               NO_ARG,   0,  STOP_ENCODE},
 {"start",              NO_ARG,   0,  START_ENCODE},
 {"pause",              NO_ARG,   0,  STOP_STREAMING},
 {"resume",             NO_ARG,   0,  START_STREAMING},
 {"stream-restart",     NO_ARG,   0,  RESTART_STREAMING},

 {"save-all-cfg",       NO_ARG,   0,  SAVE_ALL_CFG},
 {"show-working-mode",  NO_ARG,   0,  SHOW_WORKING_MODE},
 {"show-vin-info",      NO_ARG,   0,  SHOW_VIN_INFO},
 {"show-buffer-info",   NO_ARG,   0,  SHOW_BUFFER_INFO},
 {"show-canvas-info",   NO_ARG,   0,  SHOW_CANVAS_INFO},
 {"show-stream-status", NO_ARG,   0,  SHOW_STREAM_STATUS},
 {"show-stream-info",   NO_ARG,   0,  SHOW_STREAM_INFO},
 {"show-warp-cfg",      NO_ARG,   0,  SHOW_WARP_CFG},
 {"show-dptz-ratio",    NO_ARG,   0,  SHOW_DPTZ_RATIO_CFG},
 {"show-dptz-size",     NO_ARG,   0,  SHOW_DPTZ_SIZE_CFG},
 {"show-lbr-cfg",       NO_ARG,   0,  SHOW_LBR_CFG},
 {"show-eis-cfg",       NO_ARG,   0,  SHOW_EIS_CFG},
 {"show-md-cfg",        NO_ARG,   0,  SHOW_MD_CFG},
 {"show-all-info",      NO_ARG,   0,  SHOW_ALL_INFO},
 {0, 0, 0, 0}
};

static const char *short_options = "hc:b:W:H:s:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
 {"",     "\t\t\t" "Show usage\n"},

 {"",     "\t\t\t" "Specify Channel ID"},

 {"",     "\t\t"   "Show platform support max buffer number"},
 {"0-3",  "\t\t"   "Source buffer ID. 0: Main buffer; 1: 2nd buffer; "
                   "2: 3rd buffer; 3: 4th buffer"},
 {"",     "\t\t\t" "Source buffer width."},
 {"",     "\t\t\t" "Source buffer height."},
 {"",     "\t\t\t" "Source buffer output offset X in corresponding canvas."},
 {"",     "\t\t\t" "Source buffer output offset Y in corresponding canvas."},

 {"",     "\t\t\t" "Source buffer input window offset X."},
 {"",     "\t\t\t" "Source buffer input window offset Y."},
 {"",     "\t\t\t" "Source buffer input window width."},
 {"",     "\t\t\t" "Source buffer input window height."},
 {"",     "\t\t"   "save current buffer parameter to config file\n"},

 {"",     "\t\t"   "Show platform support max stream number"},
 {"0-3",  "\t\t"   "Stream ID\n"},

 {"",     "\t\t"   "Show streams lock state."},
 {"",     "\t\t\t" "lock the stream, please specify UUID."},
 {"",     "\t\t\t" "unlock the stream, please specify UUID\n"},

 {"0|1",  "\t\t"   "shutdown vout device,0:VOUTB,1:VOUTA"},

 {"",     "\t\t\t" "force IDR at once for current stream\n"},

 {"0-7",  "\t\t"   "region number"},
 {"0-8",  "\t\t"   "LDC warp mode.0:no transform;1:normal transform;2:panorama;3:mercator;"
                   "4:sub region;5:virtical panorama;6:transverse mercator; 7:cylinder panorama;"
                   "8:equirectangular;9:fullframe"},
 {"",     "\t\t"   "Full FOV circle radius (pixel) in vin, if = 0, use vin_width/2"},
 {"0.0-36.0", "\t" "LDC strength"},
 {"1.0-180.0", ""  "Panorama HFOV degree"},
 {"1.0-180.0", ""  "Vertical panorama or transverse mercator FOV degree"},
 {"-90-90",   "\t" "Lens warp region yaw in degree(0.1 degree on S5L, -900-900)"},
 {"-90-90",   ""   "Lens warp region pitch in degree(0.1 degree on S5L, -900-900)"},
 {"-20-20",   ""   "Lens warp region rotate in degree(0.1 degree on S5L, -200-200)"},
 {"a/b",  "\t\t"   "Zoom factor. format: numerator/denumerator. a<b: zoom out, a>b: zoom in"},
 {"a/b",  "\t"   "Specify horizontal zoom ratio. format: numerator/denumerator"},
 {"a/b",  "\t"   "Specify vertical zoom ratio. format: numerator/denumerator"},
 {"-90-90", "\t"   "(Wall Subregion)Pan angle"},
 {"-90-90", "\t"   "(Wall Subregion)Tilt angle"},
 {"axb", "\t"      "(Wall Subregion) ROI center offset to the circle center. Negative is left/top and positive is right/bottom"},
 {"",     "\t\t"   "save current warp parameter to config file\n"},
 {"W x H", "\t"  "specify warp DPTZ area input size"},
 {"X x Y", "\t"  "specify warp DPTZ area input offset"},
 {"W x H", "\t"  "specify warp DPTZ area output size"},
 {"X x Y",       "specify warp DPTZ area output offset\n"},
 {"-1.0-1.0", "\t" "pan ratio"},
 {"-1.0-1.0", "\t" "tilt ratio"},
 {"0.1-8.0", "\t" "zoom ratio\n"
                   "\t\t\t\t\t0.1 zoom out to FOV\n"
                   "\t\t\t\t\tDPTZ-I  1st buffer scalability 1/4X  - 8X\n"
                   "\t\t\t\t\tDPTZ-II 2nd buffer scalability 1/16X - 1X\n"
                   "\t\t\t\t\tDPTZ-II 3rd buffer scalability 1/8X  - 8X\n"
                   "\t\t\t\t\tDPTZ-II 4th buffer scalability 1/16X - 8X"},
 {"",     "\t\t\t" "dptz offset.x"},
 {"",     "\t\t\t" "dptz offset.y"},
 {"",     "\t\t\t" "dptz width"},
 {"",     "\t\t\t" "dptz height\n"},

 //overlay
 {"",     "\t\t"   "get the platform support max overlay area number"},
 {"",     "\t\t"   "init a area for overlay, the return value is the area id "
                   "of this handle which will used by next action"},
 {"0~max-1",""     "add a data block to area, 0~max-1 means the area id for the stream."
                   " The return value is the data block index in the area"},
 {"0~max-1",""     "update a data block of this area, 0~max-1 "
                   "means the area id for the stream"},
 {"0~max-1",""     "remove a data block from a area, 0~max-1 means the area id"
                   " for the stream"},
 {"0~max-1","\t"   "remove a area, 0~max-1 means the area id for the stream"},
 {"0~max-1","\t"   "enable a area, 0~max-1 means the area id for the stream"},
 {"0~max-1","\t"   "disable a area, 0~max-1 means the area id for the stream"},
 {"0~max-1","\t"   "show a area parameters, 0~max-1 means the area id for the stream"},
 {"",     "\t\t"   "save all setting which are using to configure file"
                   "(/etc/oryx/video/osd_overlay.acs)"},
 {"",     "\t\t"   "delete all overlay for all stream"},
 {"",     "\t\t\t" "area or data block width"},
 {"",     "\t\t\t" "area or data block height"},
 {"",     "\t\t\t" "area or data block offset x"},
 {"",     "\t\t\t" "area or data block offset y"},
 {"",     "\t"     "area buffer number, it is useful when data type is animation"
                   " or picture which frequently do update manipulation!\n\t\t\t\t\t"
                   "Note: Don't put more than one Animation data in one area "
                   "when buffer number > 1!"},
 {"",     "\t\t"   "area background color, v:24-31,u:16-23,y:8-15,0-7:alpha"},
 {"",     "\t"     "When rotate clockwise degree is 90/180/270, area whether "
                   "consistent with stream orientation."},
 {"",     "\t\t"   "data block type for add or update to area, 0:string; 1:picture;"
                   " 2:time; 3:animation; 4:line; 5:hollow rectangle; 6:solid rectangle"},
 {"",     "\t\t"   "data block index in area, which will used in "
                   "data block update and data block remove"},
 {"",     "\t\t"   "string to be inserted when data block type is string"},
 {"",     "\t"     "char spacing when data block type is string or time"},
 {"",     "\t\t"   "data block(which is string or time) background color,"
                   " v:24-31,u:16-23,y:8-15,0-7:alpha"},
 {"",     "\t"     "prefix string to be inserted when data block type is time"},
 {"",     "\t"     "suffix string to be inserted when data block type is time"},
 {"",     "\t\t"   "whether enable msec display when data block is time,"
                   "0:disable;1:enable"},
 {"",     "\t"     "choose format to display when data block type is time.0:YYYY-MM-DD hh:mm:ss;"
                   "1:MM/DD/YYYY hh:mm:ss; 2:DD/MM/YYYY hh:mm:ss."},
 {"",     "\t\t"   "whether enable 12H notation display format when data block type is time"},
 {"",     "\t\t"   "font type name"},
 {"",     "\t\t"   "font size"},
 {"",     "\t\t"   "font color,0-7 is predefine color: 0,white;1,black;2,red;"
                   "3,blue;4,green;5,yellow;6,cyan;7,magenta; "
                   ">7,user custom(v:24-31,u:16-23,y:8-15,0-7:alpha)"},
 {"",     "\t\t"   "outline size"},
 {"",     "\t\t"   "outline color, v:24-31,u:16-23,y:8-15,0-7:alpha"},
 {"",     "\t\t"   "n percentage HOR bold of font size, positive is bold, negetive is thin"},
 {"",     "\t\t"   "n percentage VER bold of font size, positive is bold, negetive is thin"},
 {"",     "\t\t"   "n percentage italic of font size"},
 {"",     "\t\t"   "bmp file to be inserted when data block type is picture"},
 {"",     "\t\t"   "bmp number in the file when data block type is animation"},
 {"",     "\t"     "frame interval to display picture when data block type is animation"},
 {"",     "\t\t"   "color used to transparent when data block type is picture,"
                   "v:24-31,u:16-23,y:8-15,a:0-7"},
 {"",     "\t\t"   "color range used to transparent with color key"},
 {"",     "\t\t"   "color used to draw line, 0-7 is predefine color: 0,white;1,black;2,red;"
                   "3,blue;4,green;5,yellow;6,cyan;7,magenta; "
                   ">7,user custom(v:24-31,u:16-23,y:8-15,0-7:alpha)"},
 {"",     "\t\t"   "thickness used to draw line"},
 {"",     "\t\t"   "add a point which will used to draw line, cann't add more than 5 points!"
                   " format: x,y"},
 {"",     "\t\t"   "rectangle color, 0-7 is predefine color: 0,white;1,black;2,red;"
                   "3,blue;4,green;5,yellow;6,cyan;7,magenta;"
                   ">7,user custom(v:24-31,u:16-23,y:8-15,0-7:alpha)"},
 {"",     "\t\t"   "rectangle thickness"},
 {"",     "\t"     "Hollow rectangle point pair, format: 10x10,100x100\n"},
 {"",     "\t"     "Solid rectangle point pair, format: 10x10\n"},

 {"0|1",  "\t\t"   "low bitrate enable"},
 {"0|1",  "\t\t"   "low bitrate auto bitrate target"},
 {"", "\t\t\t"     "low bitrate bitrate ceiling(bps/MB)"},
 {"0|1",  "\t\t"   "low bitrate frame drop"},
 {"",  "\t\t\t"    "low bitrate save current parameter to config file\n"},

 {"",     "\t\t\t" "EIS mode"},
 {"",     "\t\t" "save current eis parameter to config file\n"},

 {"0|1|2|3", "\t" "motion detect ROI index"},
 {"0|1", "\t\t" "disable or enable motion detect"},
 {"0|1|2|3", "\t" "buffer ID. 0: main buffer, 1: second buffer, 2: third buffer, 3: fourth buffer"},
 {"1|3|4", "\t" "buffer type 1: YUV buffer, 3: ME0 buffer, 4: ME1 buffer"},
 {"", "\t\t\t" "threshold0 of motion detect"},
 {"", "\t\t\t" "threshold1 of motion detect"},
 {"", "\t\t\t" "level0 change delay of motion detect"},
 {"", "\t\t\t" "level1 change delay of motion detect"},
 {"0|1", "\t" "ROI valid. 0: invalid, 1:valid"},
 {"", "\t\t" "ROI width-offset of left border line"},
 {"", "\t\t" "ROI width-offset of right border line"},
 {"", "\t\t" "ROI height-offset of top border line"},
 {"", "\t\t" "ROI height-offset of bottom border line"},
 {"", "\t\t" "save current motion detect parameter to config file\n"},

 {"", "\t\t" "enable face recongnition in face detect"},
 {"", "\t\t" "disable face recongnition in face detect"},

 {"",     "\t\t" "bitrate set"},
 {"",     "\t\t" "frame rate set"},
 {"",     "\t"   "mjpeg quality set"},
 {"",     "\t\t" "h264 or h265 gop N set"},
 {"",     "\t"   "h264 or H265 gop idr set"},
 {"",     "\t\t" "stream source buffer set, must restart stream to apply it"},
 {"H264|H265|MJPEG","" "stream type set, must restart stream to apply it"},
 {"",     "\t\t" "stream width set, must restart stream to apply it"},
 {"",     "\t\t" "stream height set, must restart stream to apply it"},
 {"",     "\t\t" "stream offset x set, must restart stream to apply it"},
 {"",     "\t\t" "stream offset y set, must restart stream to apply it"},
 {"",     "\t"   "absolute bitrate set, must restart stream to apply it"},
 {"",     "\t\t" "stream flip set. 0:none; 1:vflip; 2:hflip; 3:both flip."
                  " must restart stream to apply it"},
 {"",     "\t\t" "stream rotate set, 0:none; else: clock-wise rotation 90 degrees. "
                 "must restart stream to apply it"},
 {"",     "\t\t" "stream h26x profile set, 0:base line; 1:main line; 2:high line. h265 just support main line."
                 " must restart stream to apply it"},
 {"",     "\t\t" "save current stream paramater to config file\n"},

 {"",     "\t\t\t" "Network latency(ms), app will use it and bandwidth value to calculate the i frame max size value and set it to the stream"},
 {"",     "\t\t\t" "Nerwork throughput(Kbit/s), app will use it and latency value to calculate the i frame max size value and set it to the stream\n"},

 {"",     "\t\t"   "Get available CPU frequencies"},
 {"",     "\t\t"   "Get current CPU frequency"},
 {"",     "\t\t"   "Set current CPU frequency\n"},

 {"",     "\t\t\t" "Goto VCA mode"},
 {"",     "\t\t"   "Goto iav unloaded mode"},
 {"",     "\t\t"   "Goto iav current mode, just recovery to iav current state"},
 {"",     "\t\t\t" "Stop encode(apply to all streams)"},
 {"",     "\t\t\t" "Start encode(apply to all streams,can use it goto normal mode from vca mode)"},
 {"",     "\t\t\t" "Pause streaming"},
 {"",     "\t\t\t" "Resume streaming"},
 {"",     "\t\t"   "Restart streaming\n"},

 {"",     "\t\t"   "Save all modules current parameter to config file"},
 {"",     "\t\t"   "Show current working mode"},
 {"",     "\t\t"   "Show current vin's info"},
 {"",     "\t\t"   "Show all buffers' info"},
 {"",     "\t\t"   "Show all canvas' info"},
 {"",     "\t"     "Show all streams' status"},
 {"",     "\t\t"   "Show all streams' parameters"},
 {"",     "\t\t"   "Show Warp configs"},
 {"",     "\t\t"   "Show DPTZ ratio configs"},
 {"",     "\t\t"   "Show DPTZ size configs"},
 {"",     "\t\t"   "Show lbr configs"},
 {"",     "\t\t"   "Show EIS configs"},
 {"",     "\t\t"   "Show motion detect configs"},
 {"",     "\t\t"   "Show all information"}
};

// first and second values must be in format of "AxB"
static int get_two_int(const char *name, int *first, int *second, char delimiter)
{
  char tmp_string[16];
  char * separator;

  separator = strchr((char*)name, delimiter);
  if (!separator) {
    printf("two int should be like A%cB.\n", delimiter);
    return -1;
  }

  strncpy(tmp_string, name, separator - name);
  tmp_string[separator-name] = '\0';
  *first = atoi(tmp_string);
  strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
  *second = atoi(tmp_string);

  return 0;
}

// first and second values must be in format of "AxB"
static int get_two_float(const char *name, float *first, float *second,
                         char delimiter)
{
  char tmp_string[16];
  char * separator;

  separator = strchr((char*)name, delimiter);
  if (!separator) {
    printf("two int should be like A%cB.\n", delimiter);
    return -1;
  }

  strncpy(tmp_string, name, separator - name);
  tmp_string[separator - name] = '\0';
  *first = atof(tmp_string);
  strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
  *second = atof(tmp_string);

  return 0;
}

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
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
  printf("\n");
}

static int32_t init_param(int32_t argc, char **argv)
{
  int32_t ch;
  int32_t option_index = 0;
  int32_t num = 0;
  int32_t den = 0;
  int32_t ret = 0;
  int32_t val = 0;
  int32_t pn = 0;
  int32_t rtg_point_num = 0;
  int32_t x = 0;
  int32_t y = 0;
  opterr = 0;
  int32_t first = 0;
  int32_t second = 0;
  float first_f = 0.0;
  float second_f = 0.0;
  int area = -1;
  int32_t ol_cur_add_data_idx = -1;
  int32_t ol_cur_up_data_idx = -1;
  enum {
    set_none = 0,
    set_stream,
    set_buffer
  } set_state = set_none;

  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        usage(argc, argv);
        return -1;

      case BUFFER_NUM_MAX:
        get_platform_buffer_num_max_flag = true;
        break;

      case STREAM_NUM_MAX:
        get_platform_stream_num_max_flag = true;
        break;

      case 'c':
        current_channel_id = AM_MULTI_VIN_CHAN_ID(atoi(optarg));
        break;

      case 'b':
        current_buffer_id = AM_SOURCE_BUFFER_ID(atoi(optarg));
        current_stream_id = AM_STREAM_ID_INVALID;
        set_state = set_buffer;
        break;

      case 's':
        current_stream_id = AM_STREAM_ID(atoi(optarg));
        current_buffer_id = AM_SOURCE_BUFFER_INVALID;
        set_state = set_stream;
        break;

      case SHOW_LOCK_STATE:
        show_lock_state = true;
        break;
      case LOCK_STREAM:
        if (set_state == set_stream) {
          if (strlen(optarg) != 36) {
            ERROR("UUID format is wrong: %s, length isn't enough: %d",
                  optarg, strlen(optarg));
            break;
          }
          g_stream_lock_setting[current_stream_id].is_set = true;
          g_stream_lock_setting[current_stream_id].operation.is_set = true;
          g_stream_lock_setting[current_stream_id].operation.value.v_int = 1;
          strncpy(g_stream_lock_setting[current_stream_id].uuid, optarg, 40);
        }
        break;

      case UNLOCK_STREAM:
        if (set_state == set_stream) {
          if (strlen(optarg) != 36) {
            ERROR("UUID format is wrong: %s, length isn't enough: %d",
                  optarg, strlen(optarg));
            break;
          }
          g_stream_lock_setting[current_stream_id].is_set = true;
          g_stream_lock_setting[current_stream_id].operation.is_set = true;
          g_stream_lock_setting[current_stream_id].operation.value.v_int = 0;
          strncpy(g_stream_lock_setting[current_stream_id].uuid, optarg, 40);
        }
        break;

      case 'W':
        if (set_state == set_buffer) {
          VERIFY_CHANNELID(current_channel_id);
          VERIFY_BUFFERID(current_buffer_id);
          g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
          g_channel_cfg[current_channel_id][current_buffer_id].width.is_set = true;
          g_channel_cfg[current_channel_id][current_buffer_id].width.value.v_int =
              atoi(optarg);
        }
        break;

      case 'H':
        if (set_state == set_buffer) {
          VERIFY_CHANNELID(current_channel_id);
          VERIFY_BUFFERID(current_buffer_id);
          g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
          g_channel_cfg[current_channel_id][current_buffer_id].height.is_set = true;
          g_channel_cfg[current_channel_id][current_buffer_id].height.value.v_int =
              atoi(optarg);
        }
        break;

      case BUFFER_OUTPUT_X:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].output_x.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].output_x.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_OUTPUT_Y:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].output_y.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].output_y.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_INPUT_X:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_x.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_x.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_INPUT_Y:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_y.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_y.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_INPUT_W:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_w.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_w.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_INPUT_H:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        g_channel_cfg[current_channel_id][current_buffer_id].is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_h.is_set = true;
        g_channel_cfg[current_channel_id][current_buffer_id].input_h.value.v_int =
            atoi(optarg);
        break;

      case BUFFER_SAVE_CFG:
        save_buffer_cfg_flag = true;
        break;
      case VOUT_HALT:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        vout_stop.value.v_int = atoi(optarg);
        vout_stop.is_set = true;
        break;

      case FORCE_IDR:
        force_idr_id |= (1 << current_stream_id);
        break;
      case REGION_ID:
        VERIFY_PARA_2(atoi(optarg), 0, 7);
        g_warp_cfg.region_id.value.v_int = atoi(optarg);
        g_warp_cfg.region_id.is_set = true;
        g_warp_cfg.is_set = true;
        area = g_warp_cfg.region_id.value.v_int;
        break;
      case LDC_WARP_MODE:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2(atoi(optarg), 0, 9);
        g_warp_cfg.warp_mode.value.v_int = atoi(optarg);
        g_warp_cfg.warp_mode.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case LDC_MAX_RADIUS:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_1(atoi(optarg), 0);
        g_warp_cfg.max_radius.value.v_int = atoi(optarg);
        g_warp_cfg.max_radius.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case LDC_STRENGTH:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2_FLOAT(atof(optarg), 0.0, 36.0);
        g_warp_cfg.ldc_strength.value.v_float = atof(optarg);
        g_warp_cfg.ldc_strength.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case PANO_HFOV_DEGREE:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2_FLOAT(atof(optarg), 1.0, 180.0);
        g_warp_cfg.pano_hfov_degree.value.v_float = atof(optarg);
        g_warp_cfg.pano_hfov_degree.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case VER_PANO_FOV_DEGREE:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2_FLOAT(atof(optarg), 1.0, 180.0);
        g_warp_cfg.ver_pano_fov_degree.value.v_float = atof(optarg);
        g_warp_cfg.ver_pano_fov_degree.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_REGION_YAW:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2(atoi(optarg), -900, 900);
        g_warp_cfg.warp_region_yaw.value.v_int = atoi(optarg);
        g_warp_cfg.warp_region_yaw.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_REGION_PITCH:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2(atoi(optarg), -900, 900);
        g_warp_cfg.warp_region_pitch.value.v_int = atoi(optarg);
        g_warp_cfg.warp_region_pitch.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_REGION_ROTATE:
        VERIFY_PARA_2(atoi(optarg), -200, 200);
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        g_warp_cfg.warp_region_rotate.value.v_int = atoi(optarg);
        g_warp_cfg.warp_region_rotate.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_ZOOM:
        if (get_two_int(optarg, &first, &second, '/') < 0) {
          ERROR("get parameter of WARP_ZOOM error!");
          break;
        }
        if (first <= 0 || second <= 0) {
          printf("Invalid parameter: %s\n",optarg);
          break;
        }
        g_warp_cfg.warp_zoom.value.v_uint = (first << 16) | (second & 0xffff);
        g_warp_cfg.warp_zoom.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_HOR_ZOOM:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        if (sscanf(optarg, "%d/%d", &num, &den) != 2) {
          printf("Invalid parameter: %s\n",optarg);
          break;
        }
        if (num <= 0 || den <= 0) {
          printf("Invalid parameter: %s\n",optarg);
          break;
        }
        g_warp_cfg.hor_zoom.value.v_uint = (num << 16) | (den & 0xffff);
        g_warp_cfg.hor_zoom.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_VER_ZOOM:
        current_buffer_id = AM_SOURCE_BUFFER_MAIN;
        if (sscanf(optarg, "%d/%d", &num, &den) != 2) {
          printf("Invalid parameter: %s\n",optarg);
          break;
        }
        if (num <= 0 || den <= 0) {
          printf("Invalid parameter: %s\n",optarg);
          break;
        }
        g_warp_cfg.ver_zoom.value.v_uint = (num << 16) | (den & 0xffff);
        g_warp_cfg.ver_zoom.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case PAN_ANGLE:
        VERIFY_PARA_2_FLOAT(atof(optarg), -90, 90);
        g_warp_cfg.pan_angle.value.v_float = atof(optarg);
        g_warp_cfg.pan_angle.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case TILT_ANGLE:
        VERIFY_PARA_2_FLOAT(atof(optarg), -90, 90);
        g_warp_cfg.tilt_angle.value.v_float = atof(optarg);
        g_warp_cfg.tilt_angle.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case SUB_ROI_OFFSET:
        if (get_two_float(optarg, &first_f, &second_f, 'x') < 0) {
          ERROR("get parameter of SUB_ROI_OFFSET error!");
          break;
        }
        g_warp_cfg.sub_roi_offset_x.value.v_float = first_f;
        g_warp_cfg.sub_roi_offset_x.is_set = true;
        g_warp_cfg.sub_roi_offset_y.value.v_float = second_f;
        g_warp_cfg.sub_roi_offset_y.is_set = true;
        g_warp_cfg.is_set = true;
        break;
      case WARP_SAVE_CFG:
        save_warp_cfg_flag = true;
        break;
      case WARP_DPTZ_INPUT_SIZE:
        if (get_two_int(optarg, &first, &second, 'x') < 0) {
          ERROR("get parameter of WARP_DPTZ_INPUT_SIZE error!");
          break;
        }
        if ((current_buffer_id != 1) && (current_buffer_id != 3)) {
          ERROR("Invalid buffer id for warp DPTZ area! "
                "1 for 2nd buffer, 3 for 4th buffer");
          break;
        }
        g_warp_cfg.buffer_id.value.v_int = current_buffer_id;
        g_warp_cfg.buffer_id.is_set = true;
        VERIFY_PARA_2(area, 0, MAX_WARP_AREAS);
        g_warp_cfg.warp_dptz[area].input_size_flag = true;
        g_warp_cfg.warp_dptz[area].input_w = first;
        g_warp_cfg.warp_dptz[area].input_h = second;
        break;
      case WARP_DPTZ_INPUT_OFFSET:
        if (get_two_int(optarg, &first, &second, 'x') < 0) {
          ERROR("get parameter of WARP_DPTZ_INPUT_OFFSET error!");
          break;
        }
        if ((current_buffer_id != 1) && (current_buffer_id != 3)) {
          ERROR("Invalid buffer id for warp DPTZ area! "
                "1 for 2nd buffer, 3 for 4th buffer");
          break;
        }
        g_warp_cfg.buffer_id.value.v_int = current_buffer_id;
        g_warp_cfg.buffer_id.is_set = true;
        VERIFY_PARA_2(area, 0, MAX_WARP_AREAS);
        g_warp_cfg.warp_dptz[area].input_offset_flag = true;
        g_warp_cfg.warp_dptz[area].input_x = first;
        g_warp_cfg.warp_dptz[area].input_y = second;
        break;
      case WARP_DPTZ_OUTPUT_SIZE:
        if (get_two_int(optarg, &first, &second, 'x') < 0) {
          ERROR("get parameter of WARP_DPTZ_OUTPUT_SIZE error!");
          break;
        }
        if ((current_buffer_id != 1) && (current_buffer_id != 3)) {
          ERROR("Invalid buffer id for warp DPTZ area! "
                "1 for 2nd buffer, 3 for 4th buffer");
          break;
        }
        g_warp_cfg.buffer_id.value.v_int = current_buffer_id;
        g_warp_cfg.buffer_id.is_set = true;
        VERIFY_PARA_2(area, 0, MAX_WARP_AREAS);
        g_warp_cfg.warp_dptz[area].output_size_flag = true;
        g_warp_cfg.warp_dptz[area].output_w = first;
        g_warp_cfg.warp_dptz[area].output_h = second;
        break;
      case WARP_DPTZ_OUTPUT_OFFSET:
        if (get_two_int(optarg, &first, &second, 'x') < 0) {
          ERROR("get parameter of WARP_DPTZ_OUTPUT_OFFSET error!");
          break;
        }
        if ((current_buffer_id != 1) && (current_buffer_id != 3)) {
          ERROR("Invalid buffer id for warp DPTZ area! "
                "1 for 2nd buffer, 3 for 4th buffer");
          break;
        }
        g_warp_cfg.buffer_id.value.v_int = current_buffer_id;
        g_warp_cfg.buffer_id.is_set = true;
        VERIFY_PARA_2(area, 0, MAX_WARP_AREAS);
        g_warp_cfg.warp_dptz[area].output_offset_flag = true;
        g_warp_cfg.warp_dptz[area].output_x = first;
        g_warp_cfg.warp_dptz[area].output_y = second;
        break;
      case PAN_RATIO:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_2_FLOAT(atof(optarg), -1.0, 1.0);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].pan.value.
          v_float = atof(optarg);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].pan.is_set = true;
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_ratio_flag = true;
        break;
      case TILT_RATIO:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_2_FLOAT(atof(optarg), -1.0, 1.0);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].tilt.value.
          v_float = atof(optarg);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].tilt.is_set = true;
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_ratio_flag = true;
        break;
      case ZOOM_RATIO:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_2_FLOAT(atof(optarg), 0.0625, 8);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].zoom.value.
          v_float = atof(optarg);
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].zoom.is_set = true;
        g_dptz_ratio_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_ratio_flag = true;
        break;
      case DPTZ_X:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_1(atoi(optarg), 0);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].x.value.v_int =
            atoi(optarg);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].x.is_set = true;
        g_dptz_size_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_size_flag = true;
        break;
      case DPTZ_Y:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_1(atoi(optarg), 0);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].y.value.v_int =
            atoi(optarg);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].y.is_set = true;
        g_dptz_size_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_size_flag = true;
        break;
      case DPTZ_W:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_1(atoi(optarg), 0);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].w.value.v_int =
            atoi(optarg);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].w.is_set = true;
        g_dptz_size_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_size_flag = true;
        break;
      case DPTZ_H:
        VERIFY_CHANNELID(current_channel_id);
        VERIFY_BUFFERID(current_buffer_id);
        VERIFY_PARA_1(atoi(optarg), 0);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].h.value.v_int =
            atoi(optarg);
        g_dptz_size_cfg[current_channel_id][current_buffer_id].h.is_set = true;
        g_dptz_size_cfg[current_channel_id][current_buffer_id].is_set = true;
        set_dptz_size_flag = true;
        break;

        //overlay parameter
      case OVERLAY_MAX:
        get_overlay_max_flag = true;
        break;
      case OVERLAY_SAVE:
        save_overlay_flag = true;
        break;
      case OVERLAY_DESTROY:
        destroy_overlay_flag = true;
        break;
      case OVERLAY_INIT:
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].init.is_set = true;
        break;
      case OVERLAY_DATA_ADD: {
        overlay_data_setting data = {0};
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].add_data.is_set = true;
        g_overlay_setting[current_stream_id].add_data.area_id = atoi(optarg);
        g_overlay_setting[current_stream_id].add_data.data.push_back(data);
        ++ ol_cur_add_data_idx;
        pn = 0;
        rtg_point_num = 0;
      } break;
      case OVERLAY_DATA_UPDATE: {
        overlay_data_setting data = {0};
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].update_data.is_set = true;
        g_overlay_setting[current_stream_id].update_data.area_id = atoi(optarg);
        g_overlay_setting[current_stream_id].update_data.data.push_back(data);
        ++ ol_cur_up_data_idx;
        pn = 0;
        rtg_point_num = 0;
      } break;
      case OVERLAY_DATA_REMOVE: {
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].remove_data.is_set = true;
        g_overlay_setting[current_stream_id].remove_data.area_id = atoi(optarg);
      } break;
      case OVERLAY_REMOVE:
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].remove.is_set = true;
        g_overlay_setting[current_stream_id].remove.area_id = atoi(optarg);
        break;
      case OVERLAY_ENABLE:
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].enable.is_set = true;
        g_overlay_setting[current_stream_id].enable.area_id = atoi(optarg);
        break;
      case OVERLAY_DISABLE:
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].disable.is_set = true;
        g_overlay_setting[current_stream_id].disable.area_id = atoi(optarg);
        break;
      case OVERLAY_SHOW:
        g_overlay_setting[current_stream_id].is_set = true;
        g_overlay_setting[current_stream_id].show.is_set = true;
        g_overlay_setting[current_stream_id].show.area_id = atoi(optarg);
        break;
      case OVERLAY_AREA_BUF_NUM:
        g_overlay_setting[current_stream_id].init.attr.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.buf_num.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.buf_num.value.v_int =
            atoi(optarg);
        break;
      case OVERLAY_AREA_ROTATE:
        g_overlay_setting[current_stream_id].init.attr.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.rotate.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.rotate.value.v_int =
            atoi(optarg);
        break;
      case OVERLAY_AREA_BG_COLOR:
        g_overlay_setting[current_stream_id].init.attr.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.bg_color.is_set = true;
        g_overlay_setting[current_stream_id].init.attr.bg_color.value.v_uint =
            (uint32_t)atoll(optarg);
        break;
      case OVERLAY_W:
        val = atoi(optarg);
        if (g_overlay_setting[current_stream_id].init.is_set) {
          g_overlay_setting[current_stream_id].init.attr.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.w.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.w.value.v_int = val;
        } else if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].w.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].w.value.v_int = val;
        }
        break;
      case OVERLAY_H:
        val = atoi(optarg);
        if (g_overlay_setting[current_stream_id].init.is_set) {
          g_overlay_setting[current_stream_id].init.attr.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.h.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.h.value.v_int = val;
        } else if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].h.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].h.value.v_int = val;
        }
        break;
      case OVERLAY_X:
        val = atoi(optarg);
        if (g_overlay_setting[current_stream_id].init.is_set) {
          g_overlay_setting[current_stream_id].init.attr.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.x.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.x.value.v_int = val;
        } else if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].x.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].x.value.v_int = val;
        }
        break;
      case OVERLAY_Y:
        val = atoi(optarg);
        if (g_overlay_setting[current_stream_id].init.is_set) {
          g_overlay_setting[current_stream_id].init.attr.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.y.is_set = true;
          g_overlay_setting[current_stream_id].init.attr.y.value.v_int = val;
        } else if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].y.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].y.value.v_int = val;
        }
        break;
      case OVERLAY_DATA_INDEX:
        if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].data_idx = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].remove_data.is_set) {
          overlay_data_setting data = {0};
          data.data_idx = atoi(optarg);
          g_overlay_setting[current_stream_id].remove_data.data.push_back(data);
        }
        break;
      case OVERLAY_DATA_TYPE:
        VERIFY_PARA_2(atoi(optarg), 0, 6);
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].type.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].type.value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].type.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].type.value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_TEXT_STRING:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].str.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].str.str = optarg;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].str.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].str.str = optarg;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        }
        break;
      case OVERLAY_TIME_PREFIX_STRING:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].pre_str.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].pre_str.str = optarg;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].pre_str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].pre_str.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].pre_str.str = optarg;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].pre_str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        }
        break;
      case OVERLAY_TIME_SUFFIX_STRING:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].suf_str.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].suf_str.str = optarg;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].suf_str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].suf_str.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].suf_str.str = optarg;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].suf_str.
          str[OVERLAY_MAX_STRING-1] = '\0';
        }
        break;
      case OVERLAY_TIME_MSEC_EN:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].en_msec.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].en_msec.value.v_int =
              atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].en_msec.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].en_msec.value.v_int =
              atoi(optarg);
        }
        break;
      case OVERLAY_TIME_TIME_FORMAT:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].time_format.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].time_format.value.v_int =
              atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].time_format.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].time_format.value.v_int =
              atoi(optarg);
        }
        break;
      case OVERLAY_TIME_IS_12H:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_12h.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_12h.value.v_int =
              atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_12h.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_12h.value.v_int =
              atoi(optarg);
        }
        break;
      case OVERLAY_TEXT_BG_COLOR:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bg_color.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bg_color.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bg_color.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bg_color.
          value.v_uint = (uint32_t)atoll(optarg);
        }
        break;
      case OVERLAY_TEXT_SPACING:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].spacing.is_set =
              true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].spacing.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].spacing.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].spacing.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_FONT_TYPE:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_name.is_set =
              true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_name.str =
              optarg;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_name.
          str[OVERLAY_MAX_FILENAME-1] = '\0';
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_name.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_name.str =
              optarg;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_name.
          str[OVERLAY_MAX_FILENAME-1] = '\0';
        }
        break;
      case OVERLAY_FONT_SIZE:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_size.is_set =
              true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_size.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_size.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_size.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_FONT_COLOR:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_color.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_color.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_color.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_color.
          value.v_uint = (uint32_t)atoll(optarg);
        }
        break;
      case OVERLAY_FONT_OUTLINE_WIDTH:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_outline_w.
          is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_outline_w.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_outline_w.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_outline_w.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_FONT_OUTLINE_COLOR:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_outline_color.
          is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_outline_color.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_outline_color.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_outline_color.
          value.v_uint = (uint32_t)atoll(optarg);
        }
        break;
      case OVERLAY_FONT_HOR_BOLD:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_hor_bold.is_set =
              true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_hor_bold.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_hor_bold.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_hor_bold.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_FONT_VER_BOLD:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_ver_bold.is_set =
              true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_ver_bold.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_ver_bold.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_ver_bold.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_FONT_ITALIC:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_italic.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].font_italic.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_italic.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].font_italic.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_BMP:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bmp.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bmp.str = optarg;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bmp.
          str[OVERLAY_MAX_FILENAME-1] = '\0';
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bmp.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bmp.str = optarg;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bmp.
          str[OVERLAY_MAX_FILENAME-1] = '\0';
        }
        break;
      case OVERLAY_BMP_NUM:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bmp_num.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].bmp_num.value.v_int
          = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bmp_num.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].bmp_num.value.v_int
          = atoi(optarg);
        }
        break;
      case OVERLAY_BMP_INTERVAL:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].interval.is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].interval.value.v_int
          = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].interval.is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].interval.value.v_int
          = atoi(optarg);
        }
        break;
      case OVERLAY_COLOR_KEY:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].color_key.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].color_key.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].color_key.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].color_key.
          value.v_uint = (uint32_t)atoll(optarg);
        }
        break;
      case OVERLAY_COLOR_RANGE:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].color_range.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].color_range.
          value.v_int = atoi(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].color_range.
          is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].color_range.
          value.v_int = atoi(optarg);
        }
        break;
      case OVERLAY_LINE_COLOR:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].line_color.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].line_color.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].line_color.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].line_color.
          value.v_uint = (uint32_t)atoll(optarg);
        }
        break;
      case OVERLAY_LINE_THICKNESS:
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].thickness.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].thickness.
          value.v_int = (int32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].thickness.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].thickness.
          value.v_int = (int32_t)atoll(optarg);
        }
        break;
      case OVERLAY_LINE_POINT:
        ++ pn;
        if (pn > OVERLAY_MAX_POINT) {
          ERROR("Cann't add more than %d points!", OVERLAY_MAX_POINT);
          break;
        }
        if (sscanf(optarg,"%d,%d",&x,&y) != 2) {
          ERROR("Invalid point parameter:%s\n",optarg);
          return -1;
        }
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].points_num.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].points_num.
          value.v_int = pn;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].p[pn-1] = make_pair(x,y);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].points_num.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].points_num.
          value.v_int = pn;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].p[pn-1] = make_pair(x,y);
        }
        break;
      case OVERLAY_RTG_COLOR : {
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].rtg_color.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].rtg_color.
          value.v_uint = (uint32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].rtg_color.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].rtg_color.
          value.v_uint = (uint32_t)atoll(optarg);
        }
      } break;
      case OVERLAY_RTG_THICKNESS : {
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].rtg_thickness.is_set
          = true;
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].rtg_thickness.
          value.v_int = (int32_t)atoll(optarg);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].rtg_thickness.is_set
          = true;
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].rtg_thickness.
          value.v_int = (int32_t)atoll(optarg);
        }
      } break;
      case OVERLAY_H_RTG_POINT_PAIR : {
        if ((++ rtg_point_num) > OVERLAY_RTG_MAX_POINT_PAIR) {
          ERROR("Cann't add more than %d point pairs!", OVERLAY_RTG_MAX_POINT_PAIR);
          break;
        }
        int32_t first_x = 0;
        int32_t first_y = 0;
        int32_t second_x = 0;
        int32_t second_y = 0;
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          if (sscanf(optarg,"%dx%d,%dx%d", &first_x, &first_y, &second_x, &second_y) != 4) {
            ERROR("Invalid point parameter:%s\n",optarg);
            return -1;
          }
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].is_set = true;
          rtg_point_pair point_pair = make_pair(make_pair(first_x, first_y),
                                                make_pair(second_x, second_y));
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].\
          rtg_point_vector.push_back(point_pair);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          if (sscanf(optarg,"%dx%d,%dx%d", &first_x, &first_y, &second_x, &second_y) != 4) {
            ERROR("Invalid point parameter:%s\n",optarg);
            return -1;
          }
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].is_set = true;
          rtg_point_pair point_pair = make_pair(make_pair(first_x, first_y),
                                                make_pair(second_x, second_y));
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].\
          rtg_point_vector.push_back(point_pair);
        }
      } break;
      case OVERLAY_S_RTG_POINT_PAIR : {
        if ((++ rtg_point_num) > OVERLAY_RTG_MAX_POINT_PAIR) {
          ERROR("Cann't add more than %d point pairs!", OVERLAY_RTG_MAX_POINT_PAIR);
          break;
        }
        int32_t first_x = 0;
        int32_t first_y = 0;
        if (g_overlay_setting[current_stream_id].add_data.is_set) {
          if (sscanf(optarg,"%dx%d", &first_x, &first_y) != 2) {
            ERROR("Invalid point parameter:%s\n",optarg);
            return -1;
          }
          rtg_point_pair point_pair = make_pair(make_pair(first_x, first_y),
                                                make_pair(0, 0));
          g_overlay_setting[current_stream_id].add_data.data[ol_cur_add_data_idx].\
          rtg_point_vector.push_back(point_pair);
        } else if (g_overlay_setting[current_stream_id].update_data.is_set) {
          if (sscanf(optarg,"%dx%d", &first_x, &first_y) != 2) {
            ERROR("Invalid point parameter:%s\n",optarg);
            return -1;
          }
          rtg_point_pair point_pair = make_pair(make_pair(first_x, first_y),
                                                make_pair(0, 0));
          g_overlay_setting[current_stream_id].update_data.data[ol_cur_up_data_idx].\
          rtg_point_vector.push_back(point_pair);
        }
      } break;
      case LBR_ENABLE:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].is_set = true;
        g_lbr_cfg[current_stream_id].lbr_enable.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_enable.is_set = true;
        break;
      case LBR_AUTO_BITRATE_TARGET:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].is_set = true;
        g_lbr_cfg[current_stream_id].lbr_auto_bitrate_target.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_auto_bitrate_target.is_set = true;
        break;
      case LBR_BITRATE_CEILING:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].is_set = true;
        g_lbr_cfg[current_stream_id].lbr_bitrate_ceiling.value.v_int = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_bitrate_ceiling.is_set = true;
        break;
      case LBR_FRAME_DROP:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].is_set = true;
        g_lbr_cfg[current_stream_id].lbr_frame_drop.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_frame_drop.is_set = true;
        break;
      case LBR_SAVE_CFG:
        save_lbr_cfg_flag = true;
        break;

      case EIS_MODE:
        g_eis_cfg.is_set = true;
        g_eis_cfg.eis_mode.is_set = true;
        g_eis_cfg.eis_mode.value.v_int = atoi(optarg);
        break;

      case EIS_SAVE_CFG:
        save_eis_cfg_flag = true;
        break;

      case MD_ENABLE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.enable.is_set = true;
        g_motion_detect_cfg.enable.value.v_int = atoi(optarg);
        break;
      case MD_BUF_ID:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.buf_id.is_set = true;
        g_motion_detect_cfg.buf_id.value.v_uint = atoi(optarg);
        break;
      case MD_BUF_TYPE:
        VERIFY_PARA_1(atoi(optarg), 1);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.buf_type.is_set = true;
        g_motion_detect_cfg.buf_type.value.v_uint = atoi(optarg);
        break;
      case MD_ROI_INDEX:
        VERIFY_PARA_2(atoi(optarg), 0, MAX_MD_ROI_NUM - 1);
        current_md_roi_id = atoi(optarg);
        break;
      case MD_TH0:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].threshold0.is_set
        = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].threshold0.value.v_int
        = atoi(optarg);
        break;
      case MD_TH1:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].threshold1.is_set
        = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].threshold1.value.v_int
        = atoi(optarg);
        break;
      case MD_LCD0:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].level_change_delay0.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].level_change_delay0.
        value.v_int = atoi(optarg);
        break;
      case MD_LCD1:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].level_change_delay1.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].level_change_delay1.
        value.v_int = atoi(optarg);
        break;
      case MD_ROI_VALID:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_valid.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_valid.
        value.v_uint = atoi(optarg);
        break;
      case MD_ROI_LEFT:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_left.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_left.
        value.v_uint = atoi(optarg);
        break;
      case MD_ROI_RIGHT:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_right.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_right.
        value.v_uint = atoi(optarg);
        break;
      case MD_ROI_TOP:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_top.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_top.
        value.v_uint = atoi(optarg);
        break;
      case MD_ROI_BOTTOM:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_motion_detect_cfg.is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_bottom.
        is_set = true;
        g_motion_detect_cfg.md_roi_cfg[current_md_roi_id].roi_bottom.
        value.v_uint = atoi(optarg);
        break;
      case MD_SAVE_CFG:
        save_md_cfg_flag = true;
        break;

      case FD_RECOG_ENABLE:
        fd_recog_enable = true;
        break;

      case FD_RECOG_DISABLE:
        fd_recog_disable = true;
        break;

      case BITRATE_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].target_bitrate.is_set = true;
        g_stream_params_setting[current_stream_id].target_bitrate.value.v_int =
            atoi(optarg);
        break;
      case ABS_BITRATE_SET:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].absolute_bitrate.is_set = true;
        g_stream_params_setting[current_stream_id].absolute_bitrate.value.v_uint =
            atoi(optarg);
        break;
      case FRAMERATE_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].framerate.is_set = true;
        g_stream_params_setting[current_stream_id].framerate.value.v_int =
            atoi(optarg);
        break;
      case MJPEG_QUALITY_SET:
        VERIFY_PARA_2(atoi(optarg), 1, 100);
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].quality.is_set = true;
        g_stream_params_setting[current_stream_id].quality.value.v_int =
            atoi(optarg);
        break;
      case H26x_GOP_N_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].gop_n.is_set = true;
        g_stream_params_setting[current_stream_id].gop_n.value.v_int =
            atoi(optarg);
        break;
      case H26x_GOP_IDR_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].gop_idr.is_set = true;
        g_stream_params_setting[current_stream_id].gop_idr.value.v_int =
            atoi(optarg);
        break;
      case STREAM_SOURCE_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].source.is_set = true;
        g_stream_params_setting[current_stream_id].source.value.v_int =
            atoi(optarg);
        break;
      case STREAM_TYPE_SET:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].type.is_set = true;
        if (!strcasecmp(optarg, "h264")) {
          g_stream_params_setting[current_stream_id].type.value.v_int = H264;
        } else if (!strcasecmp(optarg, "h265")) {
          g_stream_params_setting[current_stream_id].type.value.v_int = H265;
        } else if (!strcasecmp(optarg, "mjpeg")) {
          g_stream_params_setting[current_stream_id].type.value.v_int = MJPEG;
        } else {
          ERROR("Wrong stream type: %s", optarg);
          return -1;
        }
        break;
      case STREAM_SIZE_W:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].w.is_set = true;
        g_stream_params_setting[current_stream_id].w.value.v_int = atoi(optarg);
        break;
      case STREAM_SIZE_H:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].h.is_set = true;
        g_stream_params_setting[current_stream_id].h.value.v_int = atoi(optarg);
        break;
      case STREAM_OFFSET_X:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].x.is_set = true;
        g_stream_params_setting[current_stream_id].x.value.v_int = atoi(optarg);
        break;
      case STREAM_OFFSET_Y:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].y.is_set = true;
        g_stream_params_setting[current_stream_id].y.value.v_int = atoi(optarg);
        break;
      case STREAM_FLIP:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].flip.is_set = true;
        g_stream_params_setting[current_stream_id].flip.value.v_int = atoi(optarg);
        break;
      case STREAM_ROTATE:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].rotate.is_set = true;
        g_stream_params_setting[current_stream_id].rotate.value.v_bool = atoi(optarg);
        break;
      case STREAM_PROFILE:
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].profile.is_set = true;
        g_stream_params_setting[current_stream_id].profile.value.v_int = atoi(optarg);
        break;
      case STREAM_SAVE_CFG:
        save_stream_cfg_flag = true;
        break;
      case LATENCY:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].latency.is_set = true;
        g_stream_params_setting[current_stream_id].latency.value.v_int = atoi(optarg);
        break;
      case BANDWIDTH:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_stream_params_setting[current_stream_id].is_set = true;
        g_stream_params_setting[current_stream_id].bandwidth.is_set = true;
        g_stream_params_setting[current_stream_id].bandwidth.value.v_int = atoi(optarg);
        break;

      case SET_VCA_MODE:
        goto_vca_mode_flag = true;
        break;
      case SET_IAV_UNLOADED_MODE:
        goto_iav_unloaded_mode_flag = true;
        break;
      case SET_IAV_CURRENT_MODE:
        goto_iav_current_mode_flag = true;
        break;
      case GET_CPU_CLKS:
        show_avail_cpu_flag = true;
        break;
      case GET_CPU_CLK:
        show_cur_cpu_flag = true;
        break;
      case SET_CPU_CLK:
        set_cur_cpu_flag = true;
        cpu_index = atoi(optarg);
        break;
      case SAVE_ALL_CFG:
        save_buffer_cfg_flag = true;
        save_stream_cfg_flag = true;
        save_warp_cfg_flag = true;
        save_eis_cfg_flag = true;
        save_overlay_flag = true;
        save_md_cfg_flag = true;
        save_lbr_cfg_flag = true;
        break;
      case STOP_ENCODE:
        stop_flag = true;
        break;
      case START_ENCODE:
        start_flag = true;
        break;
      case STOP_STREAMING:
        pause_streaming_id |= (1 << current_stream_id);
        break;
      case START_STREAMING:
        resume_streaming_id |= (1 << current_stream_id);
        break;
      case RESTART_STREAMING:
        restart_streaming_id |= (1 << current_stream_id);
        break;

      case SHOW_WORKING_MODE:
        show_working_mode_flag = true;
        break;
      case SHOW_VIN_INFO:
        show_vin_info_flag = true;
        break;
      case SHOW_BUFFER_INFO:
        show_buffer_flag = true;
        break;
      case SHOW_CANVAS_INFO:
        show_canvas_flag = true;
        break;
      case SHOW_STREAM_STATUS:
        show_status_flag = true;
        break;
      case SHOW_STREAM_INFO:
        show_stream_info_flag = true;
        break;
      case SHOW_WARP_CFG:
        show_warp_flag = true;
        break;
      case SHOW_DPTZ_RATIO_CFG:
        show_dptz_ratio_flag = true;
        break;
      case SHOW_DPTZ_SIZE_CFG:
        show_dptz_size_flag = true;
        break;
      case SHOW_LBR_CFG:
        show_lbr_flag = true;
        break;
      case SHOW_EIS_CFG:
        show_eis_flag = true;
        break;
      case SHOW_MD_CFG:
        show_motion_detect_flag = true;
        break;

      case SHOW_ALL_INFO:
        show_info_flag = true;
        break;
      default:
        ret = -1;
        printf("unknown option found: %d\n", ch);
        break;
    }
  }

  return ret;
}

static int32_t get_stream_number_max()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if (service_result.ret != AM_RESULT_OK) {
    ERROR("failed to get platform max stream number!\n");
    return -1;
  }
  uint32_t *val = (uint32_t*)service_result.data;

  return *val;
}

static AM_RESULT stop_vout()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (vout_stop.is_set) {
      am_service_result_t service_result;
      int32_t vout_id = vout_stop.value.v_int;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT,
                                &vout_id, sizeof(vout_id),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to stop VOUT device!\n");
      }
    }
  } while(0);

  return ret;
}

static AM_RESULT set_buffer_fmt()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};

  for (auto &channel : g_channel_cfg) {
    for (auto &buffer : channel.second) {
      if (!buffer.second.is_set) {
        continue;
      }
      am_buffer_fmt_t buffer_fmt = {0};
      buffer_fmt.channel_id = channel.first;
      buffer_fmt.buffer_id = buffer.first;

      if (buffer.second.width.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
        buffer_fmt.width = buffer.second.width.value.v_int;
      }
      if (buffer.second.height.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
        buffer_fmt.height = buffer.second.height.value.v_int;
      }
      if (buffer.second.output_x.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_OUTPUT_X_EN_BIT);
        buffer_fmt.output_offset_x = buffer.second.output_x.value.v_int;
      }
      if (buffer.second.output_y.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_OUTPUT_Y_EN_BIT);
        buffer_fmt.output_offset_y = buffer.second.output_y.value.v_int;
      }
      if (buffer.second.input_x.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT);
        buffer_fmt.input_offset_x = buffer.second.input_x.value.v_int;
      }
      if (buffer.second.input_y.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT);
        buffer_fmt.input_offset_y = buffer.second.input_y.value.v_int;
      }
      if (buffer.second.input_w.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT);
        buffer_fmt.input_width = buffer.second.input_w.value.v_int;
      }
      if (buffer.second.input_h.is_set) {
        SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
        buffer_fmt.input_height = buffer.second.input_h.value.v_int;
      }

      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,
                                &buffer_fmt, sizeof(buffer_fmt),
                                &service_result, sizeof(service_result));
      if (AM_UNLIKELY((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK)) {
        ERROR("Failed to set buffer format!");
      }
    }
  }

  return ret;
}

static AM_RESULT show_vin_info()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CHANNEL_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get channel number!\n");
      break;
    }
    uint32_t *val_ptr = (uint32_t*)service_result.data;
    uint32_t channel_num = *val_ptr;
    uint32_t vsrc_bit_map = 0;

    for (uint32_t channel_id = 0; channel_id < channel_num; ++channel_id) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CHANNEL_TO_VSRC,
                                &channel_id, sizeof(channel_id),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to get vsrc id");
        break;
      }
      val_ptr = (uint32_t*)service_result.data;
      uint32_t vsrc_id = *val_ptr;

      if (vsrc_bit_map & (1 << vsrc_id)) {
        continue;
      }

      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET,
                                &vsrc_id, sizeof(vsrc_id),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("Failed to get vin[%u] parameters", vsrc_id);
        break;
      }
      am_vin_info_s *info = (am_vin_info_s *) service_result.data;
      //fps
      uint16_t hz = info->fps;
      string fps_str;
      char tem_str[32];
      switch(AM_VIDEO_FPS(hz)) {
        case AM_VIDEO_FPS_AUTO:
          fps_str = "AUTO";
          break;
        case AM_VIDEO_FPS_29_97:
          fps_str = "29.97";
          break;
        case AM_VIDEO_FPS_59_94:
          fps_str = "59.94";
          break;
        default:
          snprintf(tem_str, 32, "%u", hz);
          fps_str = tem_str;
          break;
      }
      //type
      string type;
      switch(AM_VIN_TYPE_CUR(info->type)) {
        case AM_VIN_TYPE_AUTO:
          snprintf(tem_str, 32, "%s", "AUTO");
          break;
        case AM_VIN_TYPE_RGB_RAW:
          snprintf(tem_str, 32, "%s", "RGB");
          break;
        case AM_VIN_TYPE_YUV_601:
          snprintf(tem_str, 32, "%s", "YUV BT601");
          break;
        case AM_VIN_TYPE_YUV_656:
          snprintf(tem_str, 32, "%s", "YUV BT656");
          break;
        case AM_VIN_TYPE_YUV_BT1120:
          snprintf(tem_str, 32, "%s", "YUV BT1120");
          break;
        case AM_VIN_TYPE_RGB_601:
          snprintf(tem_str, 32, "%s", "RGB BT601");
          break;
        case AM_VIN_TYPE_RGB_656:
          snprintf(tem_str, 32, "%s", "RGB BT656");
          break;
        case AM_VIN_TYPE_RGB_BT1120:
          snprintf(tem_str, 32, "%s", "RGB BT1120");
          break;
        default:
          snprintf(tem_str, 32, "type?%d", info->type);
          break;
      }
      type = tem_str;
      //ratio
      string ratio;
      switch(info->ratio) {
        case AM_VIN_RATIO_AUTO:
          snprintf(tem_str, 32, "%s", "AUTO");
          break;
        case AM_VIN_RATIO_4_3:
          snprintf(tem_str, 32, "%s", "4:3");
          break;
        case AM_VIN_RATIO_16_9:
          snprintf(tem_str, 32, "%s", "16:9");
          break;
        case AM_VIN_RATIO_1_1:
          snprintf(tem_str, 32, "%s", "1:1");
          break;
        default:
          snprintf(tem_str, 32, "ratio?%d", info->ratio);
          break;
        }
      ratio = tem_str;
      //system
      string system;
      switch(info->system) {
      case AM_VIN_SYSTEM_AUTO:
        snprintf(tem_str, 32, "%s", "AUTO");
        break;
      case AM_VIN_SYSTEM_NTSC:
        snprintf(tem_str, 32, "%s", "NTSC");
        break;
      case AM_VIN_SYSTEM_PAL:
        snprintf(tem_str, 32, "%s", "PAL");
        break;
      case AM_VIN_SYSTEM_SECAM:
        snprintf(tem_str, 32, "%s", "SECAM");
        break;
      case AM_VIN_SYSTEM_ALL:
        snprintf(tem_str, 32, "%s", "ALL");
        break;
      default:
        snprintf(tem_str, 32, "system?%d", info->system);
        break;
      }
      system = tem_str;
      string hdr_mode;
      switch(info->hdr_mode) {

      case AM_HDR_2_EXPOSURE:
        snprintf(tem_str, 32, "%s", "(HDR 2x)");
        break;
      case AM_HDR_3_EXPOSURE:
        snprintf(tem_str, 32, "%s", "(HDR 3x)");
        break;
      case AM_HDR_4_EXPOSURE:
        snprintf(tem_str, 32, "%s", "(HDR 4x)");
        break;
      case AM_HDR_SENSOR_INTERNAL:
        snprintf(tem_str, 32, "%s", "(WDR In)");
        break;
      case AM_HDR_SINGLE_EXPOSURE:
      default:
        snprintf(tem_str, 32, "%s", "Null");
        break;
      }
      hdr_mode = tem_str;

      printf("\n");
      PRINTF("[VIN%d]", info->vin_id);
      PRINTF("  Mode:\t\t\t%ux%uP", info->width, info->height);
      PRINTF("  HDR:\t\t\t%s", hdr_mode.c_str());
      PRINTF("  FPS:\t\t\t%s", fps_str.c_str());
      PRINTF("  Type:\t\t\t%s", type.c_str());
      PRINTF("  Bits:\t\t\t%ubits", info->bits);
      PRINTF("  Ratio:\t\t\t%s", ratio.c_str());
      PRINTF("  System:\t\t\t%s", system.c_str());

      vsrc_bit_map |= 1 << vsrc_id;
    }
  } while(0);
  return ret;
}

static AM_RESULT show_buffer_info()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};
  uint32_t *val_ptr = nullptr;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CHANNEL_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get channel number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t channel_num = *val_ptr;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get platform max buffer number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t buffer_num = *val_ptr;

    for (uint32_t channel_id = 0; channel_id < channel_num; ++channel_id) {
      for (uint32_t buffer_id = 0; buffer_id < buffer_num; ++buffer_id) {
        am_buffer_id_t buffer = {0};
        buffer.channel_id = channel_id;
        buffer.buffer_id = buffer_id;
        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET,
                                  &buffer, sizeof(buffer),
                                  &service_result, sizeof(service_result));
        if (service_result.ret != AM_RESULT_OK) {
          ERROR("Failed to get buffer[%d.%d] format!", buffer.channel_id,
                 buffer.buffer_id);
          continue;
        }

        am_buffer_fmt_t *buffer_fmt = (am_buffer_fmt_t*)service_result.data;
        printf("\n");
        PRINTF("[Channel %d Buffer %d]", channel_id, buffer_id);
        switch (buffer_fmt->type) {
          case 0:
            PRINTF("  Type:\t\t\tOFF");
            break;
          case 1:
            PRINTF("  Type:\t\t\tENCODE");
            break;
          case 2:
            PRINTF("  Type:\t\t\tPREVIEW");
            break;
          default:
            PRINTF("  Type:\t\t\t%d, error!", buffer_fmt->type);
            break;
        }
        if (buffer_fmt->type != 0) {
          PRINTF("  Canvas ID:\t\t%d", buffer_fmt->canvas_id);
          PRINTF("  Size:\t\t\t%dx%d", buffer_fmt->width, buffer_fmt->height);
          PRINTF("  Output offset:\t\t%dx%d",
                 buffer_fmt->output_offset_x, buffer_fmt->output_offset_y);
          PRINTF("  Input offset:\t\t%dx%d",
                 buffer_fmt->input_offset_x, buffer_fmt->input_offset_y);
          PRINTF("  Input size:\t\t%dx%d",
                 buffer_fmt->input_width, buffer_fmt->input_height);
          PRINTF("  Cap_skip_interval:\t%d",buffer_fmt->cap_skip_itvl);
          PRINTF("  Cap_auto_stop:\t\t%s",buffer_fmt->auto_stop ? "true" : "false");

          g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET,
                                     &buffer_id, sizeof(buffer_id),
                                     &service_result, sizeof(service_result));
           if (service_result.ret != AM_RESULT_OK) {
             ERROR("Failed to get buffer[%d] state!", buffer_id);
             continue;
           }
           am_buffer_state_t *buffer_state = (am_buffer_state_t*)service_result.data;
           switch (buffer_state->state) {
             case 0:
               PRINTF("  State:\t\t\tidle");
               break;
             case 1:
               PRINTF("  State:\t\t\tbusy");
               break;
             case 2:
               PRINTF("  State:\t\t\terror");
               break;
             case 3:
             default:
               PRINTF("  State:\t\t\tunknown");
               break;
           }
        }
      }
    }
  } while (0);

  return ret;
}

static AM_RESULT show_canvas_info()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CANVAS_INFO_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get canvas info!\n");
      break;
    }

    am_canvas_info_t *canvas_info = (am_canvas_info_t*)service_result.data;

    for (uint32_t num = 0; num < MAX_CANVAS_NUM; ++num) {
      am_canvas_info_t canvas_cfg = canvas_info[num];
      if ((canvas_cfg.width > 0) && (canvas_cfg.height > 0)) {
        PRINTF("Canvas[%d]", canvas_cfg.canvas_id);
        switch (canvas_cfg.canvas_type) {
          case AM_MULTI_VIN_CANVAS_TYPE_OFF:
            PRINTF("  Type:\t\tOFF");
            break;
          case AM_MULTI_VIN_CANVAS_TYPE_ENC:
            PRINTF("  Type:\t\tENCODE");
            break;
          case AM_MULTI_VIN_CANVAS_TYPE_PREV:
            PRINTF("  Type:\t\tPREVIEW");
            break;
          default:
            PRINTF("  Type:\t\tUNKNOWN");
            break;
        }
        PRINTF("  Width:\t\t\%d", canvas_cfg.width);
        PRINTF("  Height:\t\t\%d", canvas_cfg.height);

        uint8_t canvas_id = canvas_cfg.canvas_id;
        am_service_result_t svc_ret = {0};

        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CANVAS_STATE_GET,
                                  &canvas_id, sizeof(canvas_id),
                                  &svc_ret, sizeof(svc_ret));
        if ((ret = AM_RESULT(svc_ret.ret)) != AM_RESULT_OK) {
          ERROR("Failed to get canvas state!\n");
          break;
        }
        am_canvas_state_t *canvas_state =
            (am_canvas_state_t*)svc_ret.data;
        switch (canvas_state->state) {
          case 0: {
            PRINTF("  State:\t\tUNKNOWN");
          }break;
          case 1: {
            PRINTF("  State:\t\tIDLE");
          }break;
          case 2: {
            PRINTF("  State:\t\tBUSY");
          }break;
          case 3: {
            PRINTF("  State:\t\tERROR");
          }break;
          default: {
            PRINTF(" ERROR State:\t\t%d", canvas_state->state);
          }break;
        }
      }
    }
  } while(0);

  return ret;
}

static AM_RESULT show_stream_lock_state()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  uint32_t *lock_bitmap = (uint32_t*)service_result.data;

  for (uint32_t i = 0; i < MAX_STREAM_NUM; ++i) {
    if (*lock_bitmap & (1 << i)) {
      PRINTF("Stream[%d] lock state: locked.", i);
    } else {
      PRINTF("Stream[%d] lock state: free.", i);
    }
  }

  return ret;
}

static AM_RESULT stream_lock_operation()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_stream_lock_t *lock_result = (am_stream_lock_t*)service_result.data;

  for (auto &m : g_stream_lock_setting) {
    if (!m.second.is_set) {
      continue;
    }
    am_stream_lock_t lock_cfg = {0};
    lock_cfg.stream_id = m.first;
    lock_cfg.operation = m.second.operation.value.v_int;
    uuid_parse(m.second.uuid, lock_cfg.uuid);
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK,
                              &lock_cfg, sizeof(lock_cfg),
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to do stream lock operation!");
    } else {
      if (!lock_result->op_result) {
        PRINTF("\nStream[%d] lock operation is OK!\n\n", m.first);
      } else {
        PRINTF("\nStream[%d] is occupied by another APP!\n\n", m.first);
      }
    }
  }
  return ret;
}

static AM_RESULT set_dptz_ratio()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  for (auto &chan_dptz : g_dptz_ratio_cfg) {
    for (auto &buf_dptz : chan_dptz.second) {
      if (!(buf_dptz.second.is_set)) {
        continue;
      }
      am_dptz_ratio_t dptz_cfg = {0};
      dptz_cfg.buffer_id = buf_dptz.first;
      dptz_cfg.channel_id = chan_dptz.first;

      if (buf_dptz.second.pan.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_PAN_RATIO_EN_BIT);
        dptz_cfg.pan_ratio = buf_dptz.second.pan.value.v_float;
      }
      if (buf_dptz.second.tilt.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_TILT_RATIO_EN_BIT);
        dptz_cfg.tilt_ratio = buf_dptz.second.tilt.value.v_float;
      }
      if (buf_dptz.second.zoom.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_ZOOM_RATIO_EN_BIT);
        dptz_cfg.zoom_ratio = buf_dptz.second.zoom.value.v_float;
      }

      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET,
                                &dptz_cfg, sizeof(dptz_cfg),
                                &service_result, sizeof(service_result));
      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("DPTZ plugin is not loaded!\n");
      } else if (ret != AM_RESULT_OK) {
        ERROR("failed to set dptz_ratio\n");
      }
    }
  }

  return ret;
}

static AM_RESULT set_dptz_size()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  for (auto &chan_dptz : g_dptz_size_cfg) {
    for (auto &buf_dptz : chan_dptz.second) {
      if (!(buf_dptz.second.is_set)) {
        continue;
      }
      am_dptz_size_t dptz_cfg = {0};
      dptz_cfg.buffer_id = buf_dptz.first;
      dptz_cfg.channel_id = chan_dptz.first;

      if (buf_dptz.second.x.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_SIZE_X_EN_BIT);
        dptz_cfg.x = buf_dptz.second.x.value.v_int;
      }
      if (buf_dptz.second.y.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_SIZE_Y_EN_BIT);
        dptz_cfg.y = buf_dptz.second.y.value.v_int;
      }
      if (buf_dptz.second.w.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_SIZE_W_EN_BIT);
        dptz_cfg.w = buf_dptz.second.w.value.v_int;
      }
      if (buf_dptz.second.h.is_set) {
        SET_BIT(dptz_cfg.enable_bits, AM_DPTZ_SIZE_H_EN_BIT);
        dptz_cfg.h = buf_dptz.second.h.value.v_int;
      }

      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET,
                                &dptz_cfg, sizeof(dptz_cfg),
                                &service_result, sizeof(service_result));
      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("DPTZ plugin is not loaded!\n");
      } else if (ret != AM_RESULT_OK) {
        ERROR("failed to set dptz size config!\n");
      }
    }
  }

  return ret;
}

static AM_RESULT set_warp_config()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_warp_t warp_cfg = {0};
  bool has_warp_setting = false;

  if (g_warp_cfg.region_id.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_ID_EN_BIT);
    warp_cfg.region_id = g_warp_cfg.region_id.value.v_int;
  } else {
    warp_cfg.region_id = -1;
  }

  if (g_warp_cfg.warp_mode.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_LDC_WARP_MODE_EN_BIT);
    warp_cfg.warp_mode = g_warp_cfg.warp_mode.value.v_int;
    has_warp_setting = true;
  }
  if (g_warp_cfg.max_radius.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_MAX_RADIUS_EN_BIT);
    warp_cfg.max_radius = g_warp_cfg.max_radius.value.v_int;
    has_warp_setting = true;
  }
  if (g_warp_cfg.ldc_strength.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_LDC_STRENGTH_EN_BIT);
    warp_cfg.ldc_strength = g_warp_cfg.ldc_strength.value.v_float;
    has_warp_setting = true;
  }
  if (g_warp_cfg.pano_hfov_degree.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_PANO_HFOV_DEGREE_EN_BIT);
    warp_cfg.pano_hfov_degree = g_warp_cfg.pano_hfov_degree.value.v_float;
    has_warp_setting = true;
  }
  if (g_warp_cfg.ver_pano_fov_degree.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_VER_PANO_FOV_DEGREE_EN_BIT);
    warp_cfg.ver_pano_fov_degree = g_warp_cfg.ver_pano_fov_degree.value.v_float;
    has_warp_setting = true;
  }
  if (g_warp_cfg.warp_region_yaw.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_YAW_PITCH_EN_BIT);
    warp_cfg.warp_region_yaw = g_warp_cfg.warp_region_yaw.value.v_int;
    has_warp_setting = true;
  }
  if (g_warp_cfg.warp_region_pitch.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_YAW_PITCH_EN_BIT);
    warp_cfg.warp_region_pitch = g_warp_cfg.warp_region_pitch.value.v_int;
    has_warp_setting = true;
  }
  if (g_warp_cfg.warp_region_rotate.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_ROTATE_EN_BIT);
    warp_cfg.warp_region_rotate = g_warp_cfg.warp_region_rotate.value.v_int;
    has_warp_setting = true;
  }
  if (g_warp_cfg.warp_zoom.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_ZOOM_EN_BIT);
    warp_cfg.warp_zoom = g_warp_cfg.warp_zoom.value.v_uint;
    has_warp_setting = true;
  }

  if (g_warp_cfg.hor_zoom.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_HOR_VER_ZOOM_EN_BIT);
    warp_cfg.hor_zoom = g_warp_cfg.hor_zoom.value.v_uint;
    has_warp_setting = true;
  }
  if (g_warp_cfg.ver_zoom.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_HOR_VER_ZOOM_EN_BIT);
    warp_cfg.ver_zoom = g_warp_cfg.ver_zoom.value.v_uint;
    has_warp_setting = true;
  }

  if (g_warp_cfg.buffer_id.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_DPTZ_EN_BIT);
    warp_cfg.buffer_id = g_warp_cfg.buffer_id.value.v_int;
    memcpy(warp_cfg.warp_region_dptz, g_warp_cfg.warp_dptz,
           sizeof(warp_cfg.warp_region_dptz));
    has_warp_setting = true;
  }

  if (g_warp_cfg.pan_angle.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_PAN_ANGLE_EN_BIT);
    warp_cfg.pan_angle = g_warp_cfg.pan_angle.value.v_float;
    has_warp_setting = true;
  }

  if (g_warp_cfg.tilt_angle.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_TILT_ANGLE_EN_BIT);
    warp_cfg.tilt_angle = g_warp_cfg.tilt_angle.value.v_float;
    has_warp_setting = true;
  }

  if (g_warp_cfg.sub_roi_offset_x.is_set && g_warp_cfg.sub_roi_offset_y.is_set) {
    SET_BIT(warp_cfg.enable_bits, AM_WARP_SUB_ROI_OFFSET_EN_BIT);
    warp_cfg.sub_roi_offset_x = g_warp_cfg.sub_roi_offset_x.value.v_float;
    warp_cfg.sub_roi_offset_y = g_warp_cfg.sub_roi_offset_y.value.v_float;
    has_warp_setting = true;
  }

  if (has_warp_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,
                              &warp_cfg, sizeof(warp_cfg),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Warp plugin is not loaded!\n");
    } else if (ret == AM_RESULT_ERR_PLATFORM_SUPPORT) {
      ERROR("This platform not support warp mode%d", warp_cfg.warp_mode);
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to set warp config!\n");
    }
  }

  return ret;
}

static AM_RESULT set_lbr_config()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_encode_lbr_ctrl_t lbr_cfg = {0};
  bool has_setting = false;
  for (uint32_t i = 0; i < MAX_STREAM_NUM; ++i) {
    if (!g_lbr_cfg[i].is_set) {
      continue;
    }
    lbr_cfg.stream_id = i;
    if (g_lbr_cfg[i].lbr_enable.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_LBR_ENABLE_LBR_EN_BIT);
      lbr_cfg.enable_lbr = g_lbr_cfg[i].lbr_enable.value.v_bool;
      has_setting = true;
    }

    if (g_lbr_cfg[i].lbr_auto_bitrate_target.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_LBR_AUTO_BITRATE_CEILING_EN_BIT);
      lbr_cfg.auto_bitrate_ceiling = g_lbr_cfg[i].lbr_auto_bitrate_target.value.v_bool;
      has_setting = true;
    }

    if (g_lbr_cfg[i].lbr_bitrate_ceiling.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_LBR_BITRATE_CEILING_EN_BIT);
      lbr_cfg.bitrate_ceiling = g_lbr_cfg[i].lbr_bitrate_ceiling.value.v_int;
      has_setting = true;
    }

    if (g_lbr_cfg[i].lbr_frame_drop.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_LBR_DROP_FRAME_EN_BIT);
      lbr_cfg.drop_frame = g_lbr_cfg[i].lbr_frame_drop.value.v_bool;
      has_setting = true;
    }
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET,
                              &lbr_cfg, sizeof(lbr_cfg),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("LBR plugin is not loaded!\n");
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to set lbr cfg config!\n");
    }
  }

  return ret;
}

static AM_RESULT set_eis_config()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_encode_eis_ctrl_t eis_config = {0};
  bool has_eis_setting = false;

  if (g_eis_cfg.eis_mode.is_set) {
    SET_BIT(eis_config.enable_bits, AM_ENCODE_EIS_MODE_EN_BIT);
    eis_config.eis_mode = g_eis_cfg.eis_mode.value.v_int;
    has_eis_setting = true;
  }

  if (has_eis_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET,
                              &eis_config, sizeof(eis_config),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("EIS plugin is not loaded!\n");
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to set eis config!\n");
    }
  }

  return ret;
}

static AM_RESULT save_current_md_config_to_file()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = { 0 };
  am_video_md_config_s md_cfg = { 0 };
  do {
    SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_SAVE_CURRENT_CONFIG);

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET,
                              &md_cfg,
                              sizeof(md_cfg),
                              &service_result,
                              sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Motion detect plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to save motion detect config to file!\n");
      break;
    }
  } while(0);

  return ret;
}

static AM_RESULT set_motion_detect_config()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = { 0 };
  am_video_md_config_s md_cfg = { 0 };
  bool has_setting = false;
  bool restart_flag = false;

  do {
    if (!g_motion_detect_cfg.is_set) {
      break;
    }
    if (g_motion_detect_cfg.enable.is_set) {
      SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_ENABLE);
      md_cfg.enable = g_motion_detect_cfg.enable.value.v_bool;
      has_setting = true;
    }
    if (g_motion_detect_cfg.buf_id.is_set) {
      SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_BUFFER_ID);
      md_cfg.buffer_id = g_motion_detect_cfg.buf_id.value.v_uint;
      has_setting = true;
    }
    if (g_motion_detect_cfg.buf_type.is_set) {
      SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_BUFFER_TYPE);
      md_cfg.buffer_type = g_motion_detect_cfg.buf_type.value.v_uint;
      if ((md_cfg.buffer_type != 1) &&
          (md_cfg.buffer_type != 3) &&
          (md_cfg.buffer_type != 4)) {
        ERROR("Wrong buffer type: %d!\n", md_cfg.buffer_type);
        ret = AM_RESULT_ERR_INVALID;
        break;
      }
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP,
                                nullptr, 0,
                                &service_result, sizeof(service_result));
      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("Motion detect plugin is not loaded!\n");
        break;
      } else if (ret != AM_RESULT_OK) {
        ERROR("failed to stop motion detect!\n");
        break;
      }
      restart_flag = true;

      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET,
                                &md_cfg,
                                sizeof(md_cfg),
                                &service_result,
                                sizeof(service_result));
      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("Motion detect plugin is not loaded!\n");
        break;
      } else if (ret != AM_RESULT_OK) {
        ERROR("failed to set motion detect config!\n");
        break;
      }
      has_setting = false;
      CLEAR_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_ENABLE)
      CLEAR_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_BUFFER_ID)
      CLEAR_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_BUFFER_TYPE)
    }

    for (uint32_t i = 0; i < MAX_MD_ROI_NUM; i ++) {
      if (!g_motion_detect_cfg.md_roi_cfg[i].is_set) {
        continue;
      }
      INFO("roi#%d\n", i);
      bool need_stop = false;
      if (g_motion_detect_cfg.md_roi_cfg[i].roi_valid.is_set ||
          g_motion_detect_cfg.md_roi_cfg[i].roi_left.is_set ||
          g_motion_detect_cfg.md_roi_cfg[i].roi_right.is_set ||
          g_motion_detect_cfg.md_roi_cfg[i].roi_top.is_set ||
          g_motion_detect_cfg.md_roi_cfg[i].roi_bottom.is_set) {
        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET,
                                  &i,
                                  sizeof(i),
                                  &service_result,
                                  sizeof(service_result));
        ret = AM_RESULT(service_result.ret);
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          ERROR("Motion detect plugin is not loaded!\n");
          break;
        } else if (ret != AM_RESULT_OK) {
          ERROR("Failed to get event config!");
          break;
        }

        am_video_md_config_s *cfg = (am_video_md_config_s*) service_result.data;
        memcpy(&md_cfg.roi, &cfg->roi, sizeof(md_cfg.roi));
        md_cfg.roi.roi_id = i;
        SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_ROI);
        has_setting = true;
        need_stop = true;
        if (g_motion_detect_cfg.md_roi_cfg[i].roi_valid.is_set) {
          md_cfg.roi.valid = g_motion_detect_cfg.
              md_roi_cfg[i].roi_valid.value.v_uint;
        }
        if (g_motion_detect_cfg.md_roi_cfg[i].roi_left.is_set) {
          md_cfg.roi.left = g_motion_detect_cfg.
              md_roi_cfg[i].roi_left.value.v_uint;
        }
        if (g_motion_detect_cfg.md_roi_cfg[i].roi_right.is_set) {
          md_cfg.roi.right = g_motion_detect_cfg.
              md_roi_cfg[i].roi_right.value.v_uint;
        }
        if (g_motion_detect_cfg.md_roi_cfg[i].roi_top.is_set) {
          md_cfg.roi.top = g_motion_detect_cfg.
              md_roi_cfg[i].roi_top.value.v_uint;
        }
        if (g_motion_detect_cfg.md_roi_cfg[i].roi_bottom.is_set) {
          md_cfg.roi.bottom = g_motion_detect_cfg.
              md_roi_cfg[i].roi_bottom.value.v_uint;
        }
        if ((md_cfg.roi.right < md_cfg.roi.left) ||
            (md_cfg.roi.bottom < md_cfg.roi.top) ||
            (md_cfg.roi.right - md_cfg.roi.left) < 2 ||
            (md_cfg.roi.bottom - md_cfg.roi.top < 2)) {
          ERROR("Wrong roi parameter: left=%d, right=%d, top=%d, bottom=%d, "
              "(right - left) and (bottom - top) value must greater than 1",
              md_cfg.roi.left, md_cfg.roi.right, md_cfg.roi.top, md_cfg.roi.bottom);
          ret = AM_RESULT_ERR_INVALID;
          break;
        }
      }

      if (g_motion_detect_cfg.md_roi_cfg[i].threshold0.is_set) {
        SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_THRESHOLD0);
        md_cfg.threshold.threshold[0] = g_motion_detect_cfg.
            md_roi_cfg[i].threshold0.value.v_int;
        md_cfg.threshold.roi_id = i;
        has_setting = true;
      }
      if (g_motion_detect_cfg.md_roi_cfg[i].threshold1.is_set) {
        SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_THRESHOLD1);
        md_cfg.threshold.threshold[1] = g_motion_detect_cfg.
            md_roi_cfg[i].threshold1.value.v_int;
        md_cfg.threshold.roi_id = i;
        has_setting = true;
      }
      if (g_motion_detect_cfg.md_roi_cfg[i].
          level_change_delay0.is_set) {
        SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_LEVEL0_CHANGE_DELAY);
        md_cfg.level_change_delay.lc_delay[0] = g_motion_detect_cfg.
            md_roi_cfg[i].
            level_change_delay0.value.v_int;
        md_cfg.level_change_delay.roi_id = i;
        has_setting = true;
      }
      if (g_motion_detect_cfg.md_roi_cfg[i].
          level_change_delay1.is_set) {
        SET_BIT(md_cfg.enable_bits, AM_VIDEO_MD_CONFIG_LEVEL1_CHANGE_DELAY);
        md_cfg.level_change_delay.lc_delay[1] = g_motion_detect_cfg.
            md_roi_cfg[i].level_change_delay1.value.v_int;
        md_cfg.level_change_delay.roi_id = i;
        has_setting = true;
      }

      if (has_setting) {
        if ((!restart_flag) && need_stop) {
          g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP,
                                    nullptr, 0,
                                    &service_result, sizeof(service_result));
          ret = AM_RESULT(service_result.ret);
          if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
            ERROR("Motion detect plugin is not loaded!\n");
            break;
          } else if (ret != AM_RESULT_OK) {
            ERROR("failed to stop motion detect!\n");
            break;
          }
          restart_flag = true;
        }
        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET,
                                  &md_cfg,
                                  sizeof(md_cfg),
                                  &service_result,
                                  sizeof(service_result));
        ret = AM_RESULT(service_result.ret);
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          ERROR("Motion detect plugin is not loaded!\n");
          break;
        } else if (ret != AM_RESULT_OK) {
          ERROR("failed to set motion detect config!\n");
          break;
        }
      }
    }

  } while(0);

  if (restart_flag) {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Motion detect plugin is not loaded!\n");
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to start motion detect!\n");
    }
  }
  return ret;
}

static AM_RESULT enable_face_recognition()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_FACE_RECOGNITION_START,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to enable face recognition!\n");
  }
  return ret;
}

static AM_RESULT disable_face_recognition()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_FACE_RECOGNITION_STOP,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to enable face recognition!\n");
  }
  return ret;
}

static AM_RESULT goto_vca_mode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_VCA_MODE,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to set to vca mode!\n");
  }
  return ret;
}

static AM_RESULT goto_iav_unloaded_mode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_IAV_UNLOADED_MODE,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to set to iav unloaded mode!\n");
  }
  return ret;
}

static AM_RESULT goto_iav_current_mode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_IAV_CURRENT_MODE,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to set to iav current mode!\n");
  }
  return ret;
}

static AM_RESULT show_working_mode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_WORKING_MODE_GET, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to set dsp power mode!\n");
  } else {
    int32_t *mode = (int32_t*)service_result.data;
    PRINTF("[Current Working]");
    PRINTF("Mode:\t\t\t%s",*mode == 0 ? "Normal" : (*mode == 1 ?
        "VCA" : (*mode == 2 ? "IAV Unloaded" : "Unknown")));
  }
  return ret;
}

static AM_RESULT show_avail_cpu_clk()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to get cpu frequencies");
  } else {
    for (int i = 0; i < service_result.data[0]; i++) {
      PRINTF("%d : %dMHZ", i, service_result.data[i + 1]*24);
    }
  }
  return ret;
}

static AM_RESULT show_cur_cpu_clk()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET, nullptr,
                            0, &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to get current CPU frequency");
  }
  PRINTF("Current CPU frequency: %dMHZ", service_result.data[0]*24);
  return ret;
}

static AM_RESULT set_cur_cpu_clk()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET, &cpu_index,
                            sizeof(cpu_index), &service_result,
                            sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to set CPU frequency");
  }
  return ret;
}

static AM_RESULT start_encode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to start encoding!\n");
  }
  return ret;
}

static AM_RESULT stop_encode()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to stop encoding!\n");
  }
  return ret;
}

static AM_RESULT start_streaming()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  int32_t stream_max = get_stream_number_max();
  for (int32_t i = 0; i < stream_max; i++) {
    if (resume_streaming_id & (1 << i)) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to resume stream%d!\n",i);
      }
    }
  }
  return ret;
}

static AM_RESULT stop_streaming()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  int32_t stream_max = get_stream_number_max();
  for (int32_t i = 0; i < stream_max; i++) {
    if (pause_streaming_id & (1 << i)) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to pause stream%d!\n",i);
      }
    }
  }
  return ret;
}

static AM_RESULT restart_streaming()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  int32_t stream_max = get_stream_number_max();
  for (int32_t i = 0; i < stream_max; i++) {
    if (restart_streaming_id & (1 << i)) {
      printf("restart stream %d\n",i);
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to pause stream%d!\n",i);
        continue;
      }
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to resume stream%d!\n",i);
        continue;
      }
    }
  }
  return ret;
}

static AM_RESULT set_force_idr()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;

  int32_t stream_max = get_stream_number_max();
  for (int32_t i = 0; i < stream_max; i++) {
    if (force_idr_id & (1 << i)) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("Failed to set stream%d to force idr!");
        break;
      }
    }
  }

  return ret;
}

static AM_RESULT set_stream_params()
{
  AM_RESULT ret = AM_RESULT_OK;

  for (auto &m : g_stream_params_setting) {
    if (!(m.second.is_set)) {
      continue;
    }

    bool has_setting = false;
    am_service_result_t service_result = {0};
    am_stream_parameter_t params = {0};
    params.stream_id = m.first;
    if (m.second.source.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SOURCE_EN_BIT);
      params.source = m.second.source.value.v_int;
      has_setting = true;
    }
    if (m.second.type.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_TYPE_EN_BIT);
      params.type = m.second.type.value.v_int;
      has_setting = true;
    }
    if (m.second.framerate.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_FRAMERATE_EN_BIT);
      params.fps = m.second.framerate.value.v_int;
      has_setting = true;
    }
    if (m.second.target_bitrate.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_BITRATE_EN_BIT);
      params.bitrate = m.second.target_bitrate.value.v_int;
      has_setting = true;
    }
    if (m.second.absolute_bitrate.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_ABS_BITRATE_EN_BIT);
      params.abs_bitrate = m.second.absolute_bitrate.value.v_uint;
      has_setting = true;
    }
    if (m.second.w.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SIZE_EN_BIT);
      params.width = m.second.w.value.v_int;
      has_setting = true;
    }
    if (m.second.h.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SIZE_EN_BIT);
      params.height = m.second.h.value.v_int;
      has_setting = true;
    }
    if (m.second.x.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
      params.offset_x = m.second.x.value.v_int;
      has_setting = true;
    }
    if (m.second.y.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
      params.offset_y = m.second.y.value.v_int;
      has_setting = true;
    }
    if (m.second.flip.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_FLIP_EN_BIT);
      params.flip = m.second.flip.value.v_int;
      has_setting = true;
    }
    if (m.second.rotate.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_ROTATE_EN_BIT);
      params.rotate = m.second.rotate.value.v_bool;
      has_setting = true;
    }
    if (m.second.profile.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_PROFILE_EN_BIT);
      params.profile = m.second.profile.value.v_int;
      has_setting = true;
    }
    if (m.second.gop_n.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_GOP_N_EN_BIT);
      params.gop_n = m.second.gop_n.value.v_int;
      has_setting = true;
    }
    if (m.second.gop_idr.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_GOP_IDR_EN_BIT);
      params.idr_interval = m.second.gop_idr.value.v_int;
      has_setting = true;
    }
    if (m.second.quality.is_set) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_ENC_QUALITY_EN_BIT);
      params.quality = m.second.quality.value.v_int;
      has_setting = true;
    }
    if (m.second.bandwidth.is_set && m.second.latency.is_set) {
      params.i_frame_max_size = int32_t((float(m.second.bandwidth.value.v_int *
                                               m.second.latency.value.v_int)
          / (8 * 1000)) * 0.8);
      if (params.i_frame_max_size > 8192) {
        ERROR("I frame max size:%d is more than 8192KB, ignore it!",params.i_frame_max_size);
      } else {
        SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_I_FRAME_SIZE_EN_BIT);
        has_setting = true;
      }
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,
                                &params, sizeof(params),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("failed to set stream%d parameters!\n", m.first);
        break;
      }
    }
  }
  return ret;
}

static AM_RESULT save_buffer_params()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_buffer_fmt_t params = {0};
  am_service_result_t service_result = {0};
  SET_BIT(params.enable_bits, AM_BUFFER_FMT_SAVE_CFG_EN_BIT);
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,
                            &params, sizeof(params),
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("Failed to save buffer config!");
  }
  return ret;

}
static AM_RESULT save_stream_params()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_stream_parameter_t params = {0};
  am_service_result_t service_result = {0};
  SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SAVE_CFG_EN_BIT);
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,
                            &params, sizeof(params),
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("failed to save stream config!");
  }
  return ret;

}

static AM_RESULT save_warp_params()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_warp_t warp_cfg = {0};
  SET_BIT(warp_cfg.enable_bits, AM_WARP_SAVE_CFG_EN_BIT);
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,
                            &warp_cfg, sizeof(warp_cfg),
                            &service_result, sizeof(service_result));
  ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    NOTICE("Warp plugin is not loaded!\n");
  } else if (ret == AM_RESULT_ERR_PLATFORM_SUPPORT) {
    ERROR("This platform not support warp mode%d", warp_cfg.warp_mode);
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to save warp config!\n");
  }
  return ret;
}

static AM_RESULT save_eis_params()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_encode_eis_ctrl_t eis_config = {0};
  SET_BIT(eis_config.enable_bits, AM_ENCODE_EIS_SAVE_CFG_EN_BIT);
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET,
                            &eis_config, sizeof(eis_config),
                            &service_result, sizeof(service_result));
  ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    NOTICE("EIS plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to save eis config!\n");
  }
  return ret;
}

static AM_RESULT save_lbr_params()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  am_encode_lbr_ctrl_t lbr_config = {0};
  SET_BIT(lbr_config.enable_bits, AM_ENCODE_LBR_SAVE_CURRENT_CONFIG_EN_BIT);
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET,
                            &lbr_config, sizeof(lbr_config),
                            &service_result, sizeof(service_result));
  ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    NOTICE("LBR plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to save lbr config!\n");
  }
  return ret;
}

static AM_RESULT show_stream_info()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    am_stream_parameter_t info = {0};
    am_service_result_t service_result;
    int32_t stream_max = get_stream_number_max();
    for (int32_t i = 0; i < stream_max; ++i) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET,
                                &i, sizeof(uint32_t),
                                &service_result, sizeof(service_result));
      if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
        ERROR("Failed to get stream%d parameters!", i);
        break;
      }
      memcpy(&info, service_result.data, sizeof(am_stream_parameter_t));

      PRINTF("[Stream%d parameters]", i);
      PRINTF("  type:\t\t\t%s", (info.type == 1) ? "H264" :
          ((info.type == 2) ? "H265" : ((info.type == 3) ? "Mjpeg" : "Null")));
      if ((info.type < 1) || (info.type > 3)) {
        PRINTF("\n");
        continue;
      }
      PRINTF("  source:\t\t\t%d", info.source);
      PRINTF("  framerate:\t\t%d", info.fps);
      PRINTF("  size:\t\t\t%d x %d", info.width, info.height);
      PRINTF("  offset:\t\t\t%d x %d", info.offset_x, info.offset_y);
      PRINTF("  flip:\t\t\t%d", info.flip);
      PRINTF("  roate:\t\t\t%d", info.rotate);
      if (info.type == 1 || info.type == 2) {
        PRINTF("  profile:\t\t\t%d", info.profile);
        PRINTF("  bitrate:\t\t\t%d", info.bitrate);
        PRINTF("  abs_bitrate:\t\t%d", info.abs_bitrate);
        PRINTF("  gop_n:\t\t\t%d", info.gop_n);
        PRINTF("  IDR interval:\t\t%d", info.idr_interval);
        PRINTF("  max_i_frame_size:\t\t%d KB", info.i_frame_max_size);
      } else if (info.type == 3) {
        PRINTF("  quality:\t\t%d", info.quality);
      }
      PRINTF("\n");
    }
  } while(0);

  return ret;
}

static AM_RESULT show_stream_status()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_stream_status_t *status = nullptr;
  am_service_result_t service_result;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
    ERROR("Failed to Get Stream format!");
    return ret;
  }
  status = (am_stream_status_t*)service_result.data;

  printf("\n");
  uint32_t encode_count = 0;
  PRINTF("[Stream State]");
  for (uint32_t i = 0; i < MAX_STREAM_NUM; ++i) {
    if (TEST_BIT(status->status, i)) {
      ++encode_count;
      PRINTF("  Stream[%d]: \t\tencoding", i);
    }
  }
  printf("\n");
  if (encode_count == 0) {
    PRINTF("No stream in encoding!");
  }

  return ret;
}

static AM_RESULT show_dptz_ratio()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};
  uint32_t *val_ptr = nullptr;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CHANNEL_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get channel number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t channel_num = *val_ptr;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get platform max buffer number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t buffer_num = *val_ptr;

    for (uint32_t channel_id = 0; channel_id < channel_num; ++channel_id) {
      for (uint32_t buffer_id = 0; buffer_id < buffer_num; ++buffer_id) {
        am_buffer_id_t buffer = {0};
        buffer.channel_id = channel_id;
        buffer.buffer_id = buffer_id;

        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET,
                                  &buffer, sizeof(buffer),
                                  &service_result, sizeof(service_result));
        ret = AM_RESULT(service_result.ret);
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          ERROR("DPTZ plugin is not loaded!\n");
          break;
        } else if (ret != AM_RESULT_OK) {
          ERROR("Failed to get dptz config!");
          break;
        }
        am_dptz_ratio_t *cfg = (am_dptz_ratio_t*)service_result.data;

        PRINTF("[Channel %d Buffer %d dptz config]", channel_id, buffer_id);
        PRINTF("  pan_ratio:\t\t%.02f", cfg->pan_ratio);
        PRINTF("  tilt_ratio:\t\t%.02f", cfg->tilt_ratio);
        PRINTF("  zoom_ratio:\t\t%.02f", cfg->zoom_ratio);
        PRINTF("\n");
      }
    }
  } while(0);

  return ret;
}

static AM_RESULT show_dptz_size()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};
  uint32_t *val_ptr = nullptr;

  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_CHANNEL_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get channel number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t channel_num = *val_ptr;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = AM_RESULT(service_result.ret)) != AM_RESULT_OK) {
      ERROR("Failed to get platform max buffer number!\n");
      break;
    }
    val_ptr = (uint32_t*)service_result.data;
    uint32_t buffer_num = *val_ptr;

    for (uint32_t channel_id = 0; channel_id < channel_num; ++channel_id) {
      for (uint32_t buffer_id = 0; buffer_id < buffer_num; ++buffer_id) {
        am_buffer_id_t buffer = {0};
        buffer.channel_id = channel_id;
        buffer.buffer_id = buffer_id;

        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET,
                                  &buffer, sizeof(buffer),
                                  &service_result, sizeof(service_result));
        ret = AM_RESULT(service_result.ret);
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          ERROR("DPTZ plugin is not loaded!\n");
          break;
        } else if (ret != AM_RESULT_OK) {
          ERROR("Failed to get dptz config!");
          break;
        }
        am_dptz_size_t *cfg = (am_dptz_size_t*)service_result.data;

        PRINTF("[Channel %d Buffer %d dptz size]", channel_id, buffer_id);
        PRINTF("  offset.x:\t\t\t%d", cfg->x);
        PRINTF("  offset.y:\t\t\t%d", cfg->y);
        PRINTF("  size.width:\t\t%d", cfg->w);
        PRINTF("  size.height:\t\t%d", cfg->h);
      }
    }
  } while(0);

  return ret;
}

static AM_RESULT show_warp_cfg()
{
  AM_RESULT ret = AM_RESULT_OK;
  int32_t region_id = -1;
  do {
    am_warp_t *cfg = nullptr;
    am_service_result_t service_result;
    if (g_warp_cfg.region_id.is_set) {
      region_id = g_warp_cfg.region_id.value.v_int;
    } else {
      region_id = 0;
      NOTICE("Not specify the region ID, use the default region %d", region_id);
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET,
                              &region_id, sizeof(region_id),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      WARN("Warp plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("Failed to get warp config!");
      break;
    }
    cfg = (am_warp_t*)service_result.data;

    PRINTF("[warp region %d config]", region_id);

    PRINTF("  ldc warp mode:\t\t%d", cfg->warp_mode);
    PRINTF("  max_radius:\t\t%d", cfg->max_radius);
    PRINTF("  ldc_strength:\t\t%.02f", cfg->ldc_strength);
    PRINTF("  pano_hfov_degree:\t\t%.02f", cfg->pano_hfov_degree);
    PRINTF("  ver_pano_fov_degree:\t%.02f", cfg->ver_pano_fov_degree);
    PRINTF("  warp_region_yaw:\t\t%d", cfg->warp_region_yaw);
    PRINTF("  warp_region_pitch:\t\t%d", cfg->warp_region_pitch);
    PRINTF("  warp_region_rotate:\t%d", cfg->warp_region_rotate);
    PRINTF("  warp_hor_zoom:\t\t%d/%d", cfg->hor_zoom >> 16, cfg->hor_zoom & 0xffff);
    PRINTF("  warp_ver_zoom:\t\t%d/%d", cfg->ver_zoom >> 16, cfg->ver_zoom & 0xffff);
    PRINTF("  sub_roi_pan_angle:\t\t%f", cfg->pan_angle);
    PRINTF("  sub_roi_tilt_angle:\t%f", cfg->tilt_angle);
    PRINTF("  sub_roi_offset:\t\t%fx%f", cfg->sub_roi_offset_x,
                                         cfg->sub_roi_offset_y);
  } while (0);

  return ret;
}

static AM_RESULT show_lbr_cfg()
{
  AM_RESULT ret = AM_RESULT_OK;
  uint32_t stream_id;
  am_encode_lbr_ctrl_t *cfg = nullptr;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < MAX_STREAM_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      WARN("LBR plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("Failed to get lbr config!");
      break;
    }

    cfg = (am_encode_lbr_ctrl_t*)service_result.data;

    PRINTF("\n");
    PRINTF("[Stream%d LBR Config]", stream_id);
    PRINTF("  enable_lbr:\t\t%d", cfg->enable_lbr);
    PRINTF("  auto_bitrate_ceiling:\t%d", cfg->auto_bitrate_ceiling);
    PRINTF("  bitrate_ceiling:\t\t%d", cfg->bitrate_ceiling);
    PRINTF("  drop_frame:\t\t%d", cfg->drop_frame);
  }
  return ret;
}

static AM_RESULT show_eis_cfg()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    am_encode_eis_ctrl_t *cfg = nullptr;
    am_service_result_t service_result;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
   ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      WARN("EIS plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("Failed to get eis config!");
      break;
    }

    cfg = (am_encode_eis_ctrl_t*)service_result.data;
    PRINTF("[EIS Config]");
    PRINTF("  Mode:\t\t\t%d",cfg->eis_mode);
  } while(0);

  return ret;
}

static AM_RESULT show_motion_detect_config()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result = {0};
  am_video_md_config_s *md_cfg = nullptr;
  uint32_t roi_id = 0;
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET,
                              &roi_id, sizeof(roi_id),
                              &service_result, sizeof(service_result));
    ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Motion detect plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("Failed to get motion detect config!");
      break;
    }

    md_cfg = (am_video_md_config_s*) service_result.data;
    string str_enable = "Enable";
    string str_disable = "Disable";
    printf("\n");
    PRINTF("[Motion Detect Plugin]");
    PRINTF("  motion detect plugin:\t%s", md_cfg->enable ? str_enable.c_str() : str_disable.c_str());
    PRINTF("  buffer ID:\t\t%d", md_cfg->buffer_id);
    PRINTF("  buffer type:\t\t%d", md_cfg->buffer_type);
    for (uint32_t i = 0; i < MD_MAX_ROI; i ++) {
      roi_id = i;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET,
                                &roi_id,
                                sizeof(roi_id),
                                &service_result,
                                sizeof(service_result));

      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("Motion detect plugin is not loaded!\n");
        break;
      } else if (ret != AM_RESULT_OK) {
        ERROR("Failed to get motion detect config!");
        break;
      }
      PRINTF("[ROI%d]",i);
      PRINTF("  threshold[0]:\t\t%d", md_cfg->threshold.threshold[0]);
      PRINTF("  threshold[1]:\t\t%d", md_cfg->threshold.threshold[1]);
      PRINTF("  level_change_delay[0]:\t%d",
             md_cfg->level_change_delay.lc_delay[0]);
      PRINTF("  level_change_delay[1]:\t%d",
             md_cfg->level_change_delay.lc_delay[1]);
      PRINTF("  valid:\t\t\t%d", md_cfg->roi.valid);
      PRINTF("  left:\t\t\t%d", md_cfg->roi.left);
      PRINTF("  right:\t\t\t%d", md_cfg->roi.right);
      PRINTF("  top:\t\t\t%d", md_cfg->roi.top);
      PRINTF("  bottom:\t\t\t%d", md_cfg->roi.bottom);
    }
  } while (0);

  return ret;
}

static void show_buffer_number_max()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if (service_result.ret != AM_RESULT_OK) {
    ERROR("failed to get platform max buffer number!\n");
    return;
  }
  uint32_t *val = (uint32_t*)service_result.data;
  PRINTF("The platform support max %d buffer!\n", *val);
}

static void show_stream_number_max()
{
  PRINTF("The platform support max %d streams!\n", get_stream_number_max());
}

static void show_overlay_max()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if (service_result.ret != AM_RESULT_OK) {
    ERROR("failed to get overlay max number!\n");
    return;
  }
  am_overlay_limit_val_t *val = (am_overlay_limit_val_t*)service_result.data;
  PRINTF("The platform support max %d streams for overlay!\n",
         val->platform_stream_num_max);
  PRINTF("The platform support max %d overlay areas for a stream!\n",
         val->platform_overlay_area_num_max);
  PRINTF("The user defined max %d streams for overlay!\n",
         val->user_def_stream_num_max);
  PRINTF("The user defined max %d overlay areas for a stream!\n",
         val->user_def_overlay_area_num_max);
}

static void get_overlay_parameter(uint32_t stream_id,
                                  const overlay_area_setting &setting)
{
  do {
    if (!setting.is_set) {
      break;
    }
    am_overlay_id_t input_para;
    am_service_result_t service_result;

    input_para.stream_id = stream_id;
    input_para.area_id = setting.area_id;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET,
                              &input_para, sizeof(input_para),
                              &service_result, sizeof(service_result));
    AM_RESULT ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Overlay plugin is not loaded!\n");
      break;
    } else if (ret != AM_RESULT_OK) {
      ERROR("failed to get overlay config!\n");
      break;
    }

    am_overlay_area_t *area = (am_overlay_area_t*)service_result.data;
    if ((area->enable !=0) && (area->enable != 1)) {
      PRINTF("[Stream %d overlay area%d: (not created)]\n",
             stream_id, input_para.area_id);
      break;
    }
    PRINTF("[Stream %d overlay area%d: (%s)]\n", stream_id, input_para.area_id,
           area->enable == 1 ? "enabled" : "disabled");
    PRINTF("    rotate:\t\t\t%d\n", area->rotate);
    PRINTF("    background color:\t0x%x\n", area->bg_color);
    PRINTF("    area size:\t\t%d x %d\n", area->width, area->height);
    PRINTF("    area offset:\t\t%d x %d\n", area->offset_x, area->offset_y);
    PRINTF("    area buffer number:\t%d\n", area->buf_num);
    PRINTF("    data block number:\t%d\n", area->data_num);

    int32_t n = area->data_num;
    for (int32_t i = 0; i < n; ++i) {
      input_para.data_index[0] = i;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET,
                                &input_para, sizeof(input_para),
                                &service_result, sizeof(service_result));
      ret = AM_RESULT(service_result.ret);
      if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
        ERROR("Overlay plugin is not loaded!\n");
        break;
      } else if (ret != AM_RESULT_OK) {
        ERROR("failed to get overlay data parameter!\n");
        break;
      }
      am_overlay_data_get_t *data = (am_overlay_data_get_t*)service_result.data;

      PRINTF("\n");
      if (data->type > 5) {
        PRINTF("    data block%d: (not created)", i);
        ++n;
        continue;
      }

      PRINTF("    data block%d: %s\n", i, data->type == 0 ? "string" :
          (data->type == 1 ? "picture" : (data->type == 2 ? "time" :
              (data->type == 3 ? "animation" : (data->type == 4 ? "line" :
                  (data->type == 5) ? "rectangle" : "none")))));
      PRINTF("    block index:\t%u\n", data->index);
      PRINTF("    block size:\t%d x %d\n", data->width, data->height);
      PRINTF("    block offset:\t%d x %d\n", data->offset_x, data->offset_y);
#if 0
      if (0 == data->type) {
        PRINTF("      block size:\t\t%d x %d\n", data->str[0].width, data->str[0].height);
        PRINTF("      block offset:\t\t%d x %d\n", data->str[0].offset_x, data->str[0].offset_y);
        PRINTF("      spacing:\t\t\t%d\n", data->str[0].spacing);
        PRINTF("      font size:\t\t%d\n", data->str[0].font_size);
        PRINTF("      background color:\t\t0x%x\n", data->str[0].bg_color);
        if (data->str[0].font_color < 8){
          PRINTF("      font color:\t\t%d\n", data->str[0].font_color);
        } else {
          PRINTF("      font color:\t\t0x%x\n", data->str[0].font_color);
        }
        PRINTF("      font outline_width:\t%d\n", data->str[0].font_outline_w);
        PRINTF("      font outline color:\t0x%x\n", data->str[0].font_outline_color);
        PRINTF("      font hor_bold:\t\t%d\n", data->str[0].font_hor_bold);
        PRINTF("      font ver_bold:\t\t%d\n", data->str[0].font_ver_bold);
        PRINTF("      font italic:\t\t%d\n", data->str[0].font_italic);
      }
      if (2 == data->type) {
        PRINTF("      block size:\t\t%d x %d\n", data->time[0].width, data->time[0].height);
        PRINTF("      block offset:\t\t%d x %d\n", data->time[0].offset_x, data->time[0].offset_y);
        PRINTF("      spacing:\t\t\t%d\n", data->time[0].spacing);
        PRINTF("      font size:\t\t%d\n", data->time[0].font_size);
        PRINTF("      background color:\t\t0x%x\n", data->time[0].bg_color);
        if (data->time[0].font_color < 8){
          PRINTF("      font color:\t\t%d\n", data->time[0].font_color);
        } else {
          PRINTF("      font color:\t\t0x%x\n", data->time[0].font_color);
        }
        PRINTF("      font outline_width:\t%d\n", data->time[0].font_outline_w);
        PRINTF("      font outline color:\t0x%x\n", data->time[0].font_outline_color);
        PRINTF("      font hor_bold:\t\t%d\n", data->time[0].font_hor_bold);
        PRINTF("      font ver_bold:\t\t%d\n", data->time[0].font_ver_bold);
        PRINTF("      font italic:\t\t%d\n", data->time[0].font_italic);
        PRINTF("      msec:\t\t\t%d\n", data->time[0].msec_en);
        PRINTF("      format:\t\t\t%d\n", data->time[0].time_format);
        PRINTF("      is_12h:\t\t\t%d\n", data->time[0].is_12h);
      }
      if (1 == data->type || 3 == data->type) {
        PRINTF("      color key:\t\t0x%x\n", data->pic[0].color_key);
        PRINTF("      color range:\t\t%d\n", data->pic[0].color_range);
        if (3 == data->type) {
          PRINTF("      bmp number:\t\t%d\n", data->pic[0].bmp_num);
          PRINTF("      frame interval:\t\t%d\n", data->pic[0].interval);
        }
      }
      if (4 == data->type) {
        if (data->line[0].line_color < 8){
          PRINTF("      line color:\t\t%d\n", data->line[0].line_color);
        } else {
          PRINTF("      line color:\t\t0x%x\n", data->line[0].line_color);
        }
        PRINTF("      line thickness:\t%d\n", data->line[0].line_tn);
        PRINTF("      point number:\t%d\n", data->line[0].p_n);
        for (uint32_t n = 0; n < data->line[0].p_n; ++n) {
          PRINTF("        p%d(%d,%d) ", n, data->line[0].p_x[n],
                                      data->line[0].p_y[n]);
        }
      }
      if (5 == data->type) {
        PRINTF("block size:\t\t%d x %d\n", data->rtg[0].width, data->rtg[0].height);
        PRINTF("block offset:\t%d x %d\n", data->rtg[0].offset_x, data->rtg[0].offset_y);
        if (data->rtg[0].rtg_type == 0) {
          if (data->rtg[0].rtg_color < 8){
            PRINTF("Hollow rectangle color:\t\t%d\n", data->rtg[0].rtg_color);
          } else {
            PRINTF("Hollow rectangle color:\t\t0x%x\n", data->rtg[0].rtg_color);
          }
          PRINTF("Hollow rectangle line thickness:\t%d\n", data->rtg[0].rtg_line_tn);
          PRINTF("Hollow point pair number:\t%d\n", data->rtg[0].point_pair_num);
          for (uint32_t n = 0; n < data->rtg[0].point_pair_num; ++n) {
            ERROR("  (%u x %u, %u x %u) ",
                   data->rtg[0].first_x[n],
                   data->rtg[0].first_y[n],
                   data->rtg[0].second_x[n],
                   data->rtg[0].second_y[n]);
          }
        } else if (data->rtg[0].rtg_type == 1) {
          if (data->rtg[0].rtg_color < 8){
            PRINTF("Solid rectangle color:\t\t%d\n", data->rtg[0].rtg_color);
          } else {
            PRINTF("Solid rectangle color:\t\t0x%x\n", data->rtg[0].rtg_color);
          }
          PRINTF("Solid rectangle line thickness:\t%d\n", data->rtg[0].rtg_line_tn);
          PRINTF("Solid point pair number:\t%d\n", data->rtg[0].point_pair_num);
          for (uint32_t n = 0; n < data->rtg[0].point_pair_num; ++n) {
            PRINTF("  (%u x %u) ",data->rtg[0].first_x[n], data->rtg[0].first_y[n]);
          }
        } else {
          ERROR("Rectangle type error.");
        }
      }
#endif
    }
  } while(0);

  return;
}

static void init_overlay_area(uint32_t stream_id,
                              const overlay_area_setting &setting)
{
  if (!setting.is_set) {
    return;
  }
  am_service_result_t service_result = {0};
  am_overlay_area_t area;

  SET_BIT(area.enable_bits, AM_OVERLAY_INIT_EN_BIT);
  area.stream_id = stream_id;
  if (setting.attr.x.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
    area.offset_x = setting.attr.x.value.v_int;
  }

  if (setting.attr.y.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
    area.offset_y = setting.attr.y.value.v_int;
  }
  if (setting.attr.w.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
    area.width = setting.attr.w.value.v_int;
  }
  if (setting.attr.h.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
    area.height = setting.attr.h.value.v_int;
  }
  if (setting.attr.rotate.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_ROTATE_EN_BIT);
    area.rotate = setting.attr.rotate.value.v_int;
  }
  if (setting.attr.bg_color.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
    area.bg_color = setting.attr.bg_color.value.v_uint;
  }
  if (setting.attr.buf_num.is_set) {
    SET_BIT(area.enable_bits, AM_OVERLAY_BUF_NUM_EN_BIT);
    area.buf_num = setting.attr.buf_num.value.v_int;
  }

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT,
                            &area, sizeof(area),
                            &service_result, sizeof(service_result));
  AM_RESULT ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    ERROR("Overlay plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to init overlay area!\n");
  } else {
    int32_t *area_id = (int32_t*)service_result.data;
    PRINTF("The overlay area id = %d\n", *area_id);
  }
}

static void add_data_to_area(uint32_t stream_id,
                             const overlay_area_setting &setting)
{
  am_service_result_t service_result = {0};
  am_overlay_data_t  data = {0};
  int32_t type = -1;
  int32_t count = 0;
  data.stream_id = stream_id;
  data.area_id = setting.area_id;
  bool ret = true;
  do {
    if (!setting.is_set) {
      break;
    }
    data.data_num = setting.data.size();
    for (auto &m : setting.data) {
      if (!m.is_set) {
        continue;
      }
      if ((type != -1) && (type != m.type.value.v_int)) {
        ERROR("Please add same data type for one time");
        ret = false;
        break;
      }
      if (type == -1) {
        type = m.type.value.v_int;
        data.type = type;
        if (type == 0) {
          memset(data.str, 0, OVERLAY_MAX_STR_NUM * sizeof(am_overlay_str_data_t));
        } else if (type == 2) {
          memset(data.time, 0, OVERLAY_MAX_TIME_NUM * sizeof(am_overlay_time_data_t));
        } else if (type == 1 || type == 3) {
          memset(data.pic, 0, OVERLAY_MAX_PIC_NUM * sizeof(am_overlay_pic_data_t));
        } else if (type == 4) {
          memset(data.line, 0, OVERLAY_MAX_LINE_NUM * sizeof(am_overlay_line_data_t));
        } else if ((type == 5) || (type == 6)) {
          memset(data.rtg, 0, OVERLAY_MAX_RTG_NUM * sizeof(am_overlay_rtg_data_t));
          data.type = 5;
        } else {
          ERROR("Overlay data type %d error!", type);
          continue;
        }
      }
      if (type == 0) {
        if (count >= OVERLAY_MAX_STR_NUM) {
          PRINTF("Don't add more than %d data block at one time for type:%d",
                 OVERLAY_MAX_STR_NUM, type);
          break;
        }
        am_overlay_str_data_t &str = data.str[count++];
        SET_BIT(str.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          str.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          str.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          str.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          str.height = m.h.value.v_int;
        }
        if (m.str.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_STRING_EN_BIT);
          snprintf(str.str, OVERLAY_MAX_STRING, "%s", m.str.str.c_str());
        }
        if (m.spacing.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_CHAR_SPACING_EN_BIT);
          str.spacing = m.spacing.value.v_int;
        }
        if (m.bg_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
          str.bg_color = m.bg_color.value.v_uint;
        }
        if (m.font_name.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT);
          snprintf(str.font_type, OVERLAY_MAX_FILENAME, "%s",
                   m.font_name.str.c_str());
        }
        if (m.font_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_COLOR_EN_BIT);
          str.font_color = m.font_color.value.v_uint;
        }
        if (m.font_size.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_SIZE_EN_BIT);
          str.font_size = m.font_size.value.v_int;
        }
        if (m.font_outline_w.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          str.font_outline_w = m.font_outline_w.value.v_int;
        }
        if (m.font_outline_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          str.font_outline_color = m.font_outline_color.value.v_uint;
        }
        if (m.font_hor_bold.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          str.font_hor_bold = m.font_hor_bold.value.v_int;
        }
        if (m.font_ver_bold.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          str.font_ver_bold = m.font_ver_bold.value.v_int;
        }
        if (m.font_italic.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_ITALIC_EN_BIT);
          str.font_italic = m.font_italic.value.v_int;
        }
      } else if (type == 2) { //time
        if (count >= OVERLAY_MAX_TIME_NUM) {
          PRINTF("Don't add more than %d data block at one time for type:%d",
                 OVERLAY_MAX_TIME_NUM, type);
          break;
        }
        am_overlay_time_data_t &time = data.time[count++];
        SET_BIT(time.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          time.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          time.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          time.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          time.height = m.h.value.v_int;
        }
        if (m.pre_str.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          snprintf(time.pre_str, OVERLAY_MAX_STRING, "%s", m.pre_str.str.c_str());
        }
        if (m.suf_str.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          snprintf(time.suf_str, OVERLAY_MAX_STRING, "%s", m.suf_str.str.c_str());
        }
        if (m.en_msec.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.msec_en = m.en_msec.value.v_int;
        }
        if (m.time_format.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.time_format = m.time_format.value.v_int;
        }
        if (m.is_12h.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.is_12h = m.is_12h.value.v_int;
        }
        if (m.spacing.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_CHAR_SPACING_EN_BIT);
          time.spacing = m.spacing.value.v_int;
        }
        if (m.bg_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
          time.bg_color = m.bg_color.value.v_uint;
        }
        if (m.font_name.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT);
          snprintf(time.font_type, OVERLAY_MAX_FILENAME, "%s",
                   m.font_name.str.c_str());
        }
        if (m.font_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_COLOR_EN_BIT);
          time.font_color = m.font_color.value.v_uint;
        }
        if (m.font_size.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_SIZE_EN_BIT);
          time.font_size = m.font_size.value.v_int;
        }
        if (m.font_outline_w.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          time.font_outline_w = m.font_outline_w.value.v_int;
        }
        if (m.font_outline_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          time.font_outline_color = m.font_outline_color.value.v_uint;
        }
        if (m.font_hor_bold.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          time.font_hor_bold = m.font_hor_bold.value.v_int;
        }
        if (m.font_ver_bold.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          time.font_ver_bold = m.font_ver_bold.value.v_int;
        }
        if (m.font_italic.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_ITALIC_EN_BIT);
          time.font_italic = m.font_italic.value.v_int;
        }
      } else if ((type == 1) || (type == 3)) {
        if (count >= OVERLAY_MAX_PIC_NUM) {
          PRINTF("Don't add more than %d data block at one time for type:%d",
                 OVERLAY_MAX_PIC_NUM, type);
          break;
        }
        am_overlay_pic_data_t &pic = data.pic[count++];

        SET_BIT(pic.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          pic.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          pic.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          pic.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          pic.height = m.h.value.v_int;
        }
        if (m.bmp.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_EN_BIT);
          snprintf(pic.bmp, OVERLAY_MAX_FILENAME, "%s", m.bmp.str.c_str());
        }
        if (m.bmp_num.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
          pic.bmp_num = m.bmp_num.value.v_int;
        }
        if (m.interval.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
          pic.interval = m.interval.value.v_int;
        }
        if (m.color_key.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
          pic.color_key = m.color_key.value.v_uint;
        }
        if (m.color_range.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
          pic.color_range = m.color_range.value.v_int;
        }
      } else if (type == 4) {
        if (count >= OVERLAY_MAX_LINE_NUM) {
          PRINTF("Don't add more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        am_overlay_line_data_t &line = data.line[count++];
        SET_BIT(line.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          line.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          line.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          line.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          line.height = m.h.value.v_int;
        }
        if (m.line_color.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_COLOR_EN_BIT);
          line.line_color = m.line_color.value.v_uint;
        }
        if (m.thickness.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_THICKNESS_EN_BIT);
          line.line_tn = m.thickness.value.v_int;
        }
        if (m.points_num.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_POINTS_EN_BIT);
          line.p_n = m.points_num.value.v_int;
          for (uint32_t i = 0; i < line.p_n; ++i) {
            line.p_x[i] = m.p[i].first;
            line.p_y[i] = m.p[i].second;
          }
        }
      } else if (type == 5) {
        if (count >= OVERLAY_MAX_RTG_NUM) {
          PRINTF("Don't add more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        am_overlay_rtg_data_t &rtg = data.rtg[count ++];
        rtg.rtg_type = 0;
        SET_BIT(rtg.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.height = m.h.value.v_int;
        }
        if (m.rtg_color.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_COLOR_EN_BIT);
          rtg.rtg_color = m.rtg_color.value.v_uint;
        }
        if (m.rtg_thickness.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_THICKNESS_EN_BIT);
          rtg.rtg_line_tn = m.rtg_thickness.value.v_int;
        }
        if (m.rtg_point_vector.size() > 0) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_POINTS_EN_BIT);
          rtg.point_pair_num = m.rtg_point_vector.size();
          for (uint32_t i = 0; i < rtg.point_pair_num; ++i) {
            rtg.first_x[i] = m.rtg_point_vector[i].first.first;
            rtg.first_y[i] = m.rtg_point_vector[i].first.second;
            rtg.second_x[i] = m.rtg_point_vector[i].second.first;
            rtg.second_y[i] = m.rtg_point_vector[i].second.second;
          }
        }
      } else if (type == 6) {
        if (count >= OVERLAY_MAX_RTG_NUM) {
          PRINTF("Don't add more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        am_overlay_rtg_data_t &rtg = data.rtg[count ++];
        rtg.rtg_type = 1;
        SET_BIT(rtg.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
        if (m.x.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.offset_x = m.x.value.v_int;
        }
        if (m.y.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.offset_y = m.y.value.v_int;
        }
        if (m.w.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.width = m.w.value.v_int;
        }
        if (m.h.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RECT_EN_BIT);
          rtg.height = m.h.value.v_int;
        }
        if (m.rtg_color.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_COLOR_EN_BIT);
          rtg.rtg_color = m.rtg_color.value.v_uint;
        }
        if (m.rtg_thickness.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_THICKNESS_EN_BIT);
          rtg.rtg_line_tn = m.rtg_thickness.value.v_int;
        }
        if (m.rtg_point_vector.size() > 0) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_POINTS_EN_BIT);
          rtg.point_pair_num = m.rtg_point_vector.size();
          for (uint32_t i = 0; i < rtg.point_pair_num; ++i) {
            rtg.first_x[i] = m.rtg_point_vector[i].first.first;
            rtg.first_y[i] = m.rtg_point_vector[i].first.second;
          }
        }
      } else {
        ERROR("Invalid type %d", type);
        break;
      }
    }
    if (!ret) {
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD,
                              &data, sizeof(data),
                              &service_result, sizeof(service_result));
    AM_RESULT call_ret = AM_RESULT(service_result.ret);
    if (call_ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Overlay plugin is not loaded!\n");
    } else if (call_ret != AM_RESULT_OK) {
      ERROR("failed to add data to area!\n");
    } else {
      am_overlay_id_t *data_index = (am_overlay_id_t*)service_result.data;
      for(uint8_t i = 0; i < data_index->index_num; ++ i) {
        PRINTF("The data block index = %d\n", data_index->data_index[i]);
      }
    }
  } while(0);
}

static void update_area_data(uint32_t stream_id,
                             const overlay_area_setting &setting)
{
  am_service_result_t service_result = {0};
  am_overlay_data_t  data = {0};
  int32_t count = 0;
  data.stream_id = stream_id;
  data.area_id = setting.area_id;
  int32_t type = -1;
  if (setting.data[0].type.is_set) {
    type = setting.data[0].type.value.v_int;
  }
  bool need_break = false;
  do {
    if (!setting.is_set) {
      break;
    }
    data.data_num = setting.data.size();
    for (auto &m : setting.data) {
      if (!m.is_set) {
        continue;
      }
      if (type < 0) {
        am_overlay_id_t input_para;
        input_para.stream_id = stream_id;
        input_para.area_id = setting.area_id;
        input_para.data_index[0] = m.data_idx;
        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET,
                                  &input_para, sizeof(input_para),
                                  &service_result, sizeof(service_result));
        if (service_result.ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          ERROR("Overlay plugin is not loaded!\n");
          return;
        } else if (service_result.ret != AM_RESULT_OK) {
          ERROR("failed to get stream%d area%d data block%d type!\n",stream_id, data.area_id, m.data_idx);
          return;
        }

        am_overlay_data_get_s *return_data = (am_overlay_data_get_s*)service_result.data;
        type = return_data->type;
      }
      if ((m.type.is_set) && (type != m.type.value.v_int)) {
        ERROR("Only the same type data block is supported to update per time");
        need_break = true;
        break;
      }
      data.type = type;
      if (type == 0) {
        if (count >= OVERLAY_MAX_STR_NUM) {
          PRINTF("Don't update more than %d data block at one time for type:%d",
                 OVERLAY_MAX_STR_NUM, type);
          need_break = true;
          break;
        }
        am_overlay_str_data_t &str = data.str[count ++];
        SET_BIT(str.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        str.data_index = m.data_idx;
        if (m.str.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_STRING_EN_BIT);
          snprintf(str.str, OVERLAY_MAX_STRING, "%s", m.str.str.c_str());
        }
        if (m.spacing.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_CHAR_SPACING_EN_BIT);
          str.spacing = m.spacing.value.v_int;
        }
        if (m.bg_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
          str.bg_color = m.bg_color.value.v_uint;
        }
        if (m.font_name.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT);
          snprintf(str.font_type, OVERLAY_MAX_FILENAME, "%s",
                   m.font_name.str.c_str());
        }
        if (m.font_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_COLOR_EN_BIT);
          str.font_color = m.font_color.value.v_uint;
        }
        if (m.font_size.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_SIZE_EN_BIT);
          str.font_size = m.font_size.value.v_int;
        }
        if (m.font_outline_w.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          str.font_outline_w = m.font_outline_w.value.v_int;
        }
        if (m.font_outline_color.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          str.font_outline_color = m.font_outline_color.value.v_uint;
        }
        if (m.font_hor_bold.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          str.font_hor_bold = m.font_hor_bold.value.v_int;
        }
        if (m.font_ver_bold.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          str.font_ver_bold = m.font_ver_bold.value.v_int;
        }
        if (m.font_italic.is_set) {
          SET_BIT(str.enable_bits, AM_OVERLAY_FONT_ITALIC_EN_BIT);
          str.font_italic = m.font_italic.value.v_int;
        }
      } else if (type == 2) {
        if (count >= OVERLAY_MAX_TIME_NUM) {
          PRINTF("Don't update more than %d data block at one time for type:%d",
                 OVERLAY_MAX_TIME_NUM, type);
          need_break = true;
          break;
        }
        am_overlay_time_data_t &time = data.time[count ++];
        SET_BIT(time.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        time.data_index = m.data_idx;
        if (m.pre_str.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          snprintf(time.pre_str, OVERLAY_MAX_STRING, "%s", m.pre_str.str.c_str());
        }
        if (m.suf_str.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          snprintf(time.suf_str, OVERLAY_MAX_STRING, "%s", m.suf_str.str.c_str());
        }
        if (m.en_msec.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.msec_en = m.en_msec.value.v_int;
        }
        if (m.time_format.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.time_format = m.time_format.value.v_int;
        }
        if (m.is_12h.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_TIME_EN_BIT);
          time.is_12h = m.is_12h.value.v_int;
        }
        if (m.spacing.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_CHAR_SPACING_EN_BIT);
          time.spacing = m.spacing.value.v_int;
        }
        if (m.bg_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
          time.bg_color = m.bg_color.value.v_uint;
        }
        if (m.font_name.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT);
          snprintf(time.font_type, OVERLAY_MAX_FILENAME, "%s",
                   m.font_name.str.c_str());
        }
        if (m.font_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_COLOR_EN_BIT);
          time.font_color = m.font_color.value.v_uint;
        }
        if (m.font_size.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_SIZE_EN_BIT);
          time.font_size = m.font_size.value.v_int;
        }
        if (m.font_outline_w.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          time.font_outline_w = m.font_outline_w.value.v_int;
        }
        if (m.font_outline_color.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
          time.font_outline_color = m.font_outline_color.value.v_uint;
        }
        if (m.font_hor_bold.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          time.font_hor_bold = m.font_hor_bold.value.v_int;
        }
        if (m.font_ver_bold.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
          time.font_ver_bold = m.font_ver_bold.value.v_int;
        }
        if (m.font_italic.is_set) {
          SET_BIT(time.enable_bits, AM_OVERLAY_FONT_ITALIC_EN_BIT);
          time.font_italic = m.font_italic.value.v_int;
        }
      } else if (type == 1 || type == 3) {
        if (count >= OVERLAY_MAX_PIC_NUM) {
          PRINTF("Don't update more than %d data block at every time for type:%d",
                 OVERLAY_MAX_PIC_NUM, type);
          break;
        }
        am_overlay_pic_data_t &pic = data.pic[count ++];
        SET_BIT(pic.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        pic.data_index = m.data_idx;
        if (m.bmp.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_EN_BIT);
          snprintf(pic.bmp, OVERLAY_MAX_FILENAME, "%s", m.bmp.str.c_str());
        }
        if (m.bmp_num.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
          pic.bmp_num = m.bmp_num.value.v_int;
        }
        if (m.interval.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
          pic.interval = m.interval.value.v_int;
        }
        if (m.color_key.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
          pic.color_key = m.color_key.value.v_uint;
        }
        if (m.color_range.is_set) {
          SET_BIT(pic.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
          pic.color_range = m.color_range.value.v_int;
        }
      } else if (type == 4) {
        if (count >= OVERLAY_MAX_LINE_NUM) {
          PRINTF("Don't update more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        am_overlay_line_data_t &line = data.line[count ++];
        SET_BIT(line.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        line.data_index = m.data_idx;
        if (m.line_color.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_COLOR_EN_BIT);
          line.line_color = m.line_color.value.v_uint;
        }
        if (m.thickness.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_THICKNESS_EN_BIT);
          line.line_tn = m.thickness.value.v_int;
        }
        if (m.points_num.is_set) {
          SET_BIT(line.enable_bits, AM_OVERLAY_LINE_POINTS_EN_BIT);
          line.p_n = m.points_num.value.v_int;
          for (uint32_t i = 0; i < line.p_n; ++i) {
            line.p_x[i] = m.p[i].first;
            line.p_y[i] = m.p[i].second;
          }
        }
      } else if (type == 5) {
        if (count >= OVERLAY_MAX_RTG_NUM) {
          PRINTF("Don't add more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        am_overlay_rtg_data_t &rtg = data.rtg[count ++];
        data.type = 5;
        rtg.rtg_type = 0;//hollow rectangle
        SET_BIT(rtg.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        rtg.data_index = m.data_idx;
        if (m.rtg_color.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_COLOR_EN_BIT);
          rtg.rtg_color = m.rtg_color.value.v_uint;
        }
        if (m.rtg_thickness.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_THICKNESS_EN_BIT);
          rtg.rtg_line_tn = m.rtg_thickness.value.v_int;
        }
        if (m.rtg_point_vector.size() > 0) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_POINTS_EN_BIT);
          rtg.point_pair_num = m.rtg_point_vector.size();
          for (uint32_t i = 0; i < rtg.point_pair_num; ++i) {
            rtg.first_x[i] = m.rtg_point_vector[i].first.first;
            rtg.first_y[i] = m.rtg_point_vector[i].first.second;
            rtg.second_x[i] = m.rtg_point_vector[i].second.first;
            rtg.second_y[i] = m.rtg_point_vector[i].second.second;
          }
        }
      } else if (type == 6) {
        if (count >= OVERLAY_MAX_RTG_NUM) {
          PRINTF("Don't add more than %d data block at every time for type:%d",
                 OVERLAY_MAX_LINE_NUM, type);
          break;
        }
        data.type = 5;
        am_overlay_rtg_data_t &rtg = data.rtg[count ++];
        rtg.rtg_type = 1;//solid rectangle
        rtg.data_index = m.data_idx;
        SET_BIT(rtg.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
        if (m.rtg_color.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_COLOR_EN_BIT);
          rtg.rtg_color = m.rtg_color.value.v_uint;
        }
        if (m.rtg_thickness.is_set) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_THICKNESS_EN_BIT);
          rtg.rtg_line_tn = m.rtg_thickness.value.v_int;
        }
        if (m.rtg_point_vector.size() > 0) {
          SET_BIT(rtg.enable_bits, AM_OVERLAY_RTG_POINTS_EN_BIT);
          rtg.point_pair_num = m.rtg_point_vector.size();
          for (uint32_t i = 0; i < rtg.point_pair_num; ++i) {
            rtg.first_x[i] = m.rtg_point_vector[i].first.first;
            rtg.first_y[i] = m.rtg_point_vector[i].first.second;
          }
        }
      } else {
        ERROR("Invalid type %d", type);
        need_break = true;
        break;
      }
    }
    if (need_break) {
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE,
                              &data, sizeof(data),
                              &service_result, sizeof(service_result));
    AM_RESULT ret = AM_RESULT(service_result.ret);
    if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
      ERROR("Overlay plugin is not loaded!\n");
    } else if (ret != AM_RESULT_OK) {
      am_overlay_id_t *opt_ret_value = (am_overlay_id_t*)service_result.data;
      ERROR("failed to update data to area, stream id : %d, area id %d",
            opt_ret_value->stream_id, opt_ret_value->area_id);
      for(uint16_t i = 0; i < opt_ret_value->index_num; ++ i) {
        if (opt_ret_value->index_ret[i] < 0) {
          ERROR("overlay data block[%u] update error.",
                opt_ret_value->data_index[i]);
        }
      }
    }
  } while(0);
}

static void set_overlay_state(uint32_t stream_id,
                              const overlay_setting &setting)
{
  if (!setting.is_set) {
    return;
  }
  am_service_result_t service_result = {0};
  am_overlay_id_s param;

  param.stream_id = stream_id;
  if (setting.remove_data.is_set) {
    SET_BIT(param.enable_bits, AM_OVERLAY_DATA_REMOVE_EN_BIT);
    param.area_id = setting.remove_data.area_id;
    param.index_num = setting.remove_data.data.size();
    uint8_t count = 0;
    if (param.index_num >= OVERLAY_MAX_INDEX_NUM) {
      ERROR("The max index number is %d for one time", OVERLAY_MAX_INDEX_NUM);
      return;
    }
    for (auto &m : setting.remove_data.data) {
      param.data_index[count] = m.data_idx;
      ++ count;
    }
  } else if (setting.remove.is_set) {
    SET_BIT(param.enable_bits, AM_OVERLAY_REMOVE_EN_BIT);
    param.area_id = setting.remove.area_id;
  } else if (setting.enable.is_set) {
    SET_BIT(param.enable_bits, AM_OVERLAY_ENABLE_EN_BIT);
    param.area_id = setting.enable.area_id;
  } else if (setting.disable.is_set) {
    SET_BIT(param.enable_bits, AM_OVERLAY_DISABLE_EN_BIT);
    param.area_id = setting.disable.area_id;
  } else {
    return;
  }

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET,
                            &param, sizeof(param),
                            &service_result, sizeof(service_result));
  AM_RESULT ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    ERROR("Overlay plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to set overlay!\n");
    if (setting.remove_data.is_set) {
      am_overlay_id_t *opt_ret_value = (am_overlay_id_t*)service_result.data;
      ERROR("Failed to remove data block, stream id %d, area id %d",
            opt_ret_value->stream_id, opt_ret_value->area_id);
      for (uint16_t i = 0; i< opt_ret_value->index_num; ++ i) {
        if (opt_ret_value->index_ret[i] < 0) {
          ERROR("Failed to remove overlay data block %d",
                opt_ret_value->data_index[i]);
        }
      }
    }
  }
}

static void set_overlay()
{
  for (auto &m : g_overlay_setting) {
    if (!(m.second.is_set)) {
      continue;
    }

    if (m.second.init.is_set) {
      init_overlay_area(m.first, m.second.init);
    } else if (m.second.add_data.is_set) {
      add_data_to_area(m.first, m.second.add_data);
    } else if (m.second.update_data.is_set) {
      update_area_data(m.first, m.second.update_data);
    } else if (m.second.show.is_set) {
      get_overlay_parameter(m.first, m.second.show);
    } else {
      set_overlay_state(m.first, m.second);
    }
  }
}

static AM_RESULT destroy_overlay()
{
  AM_RESULT ret = AM_RESULT_OK;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY, nullptr, 0,
                            &service_result, sizeof(service_result));
  ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    ERROR("Overlay plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to destroy overlay!\n");
  }
  return ret;
}

static AM_RESULT save_overlay_parameter()
{
  AM_RESULT ret = AM_RESULT_OK;
  INFO("save overlay configure!!!\n");
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE, nullptr, 0,
                            &service_result, sizeof(service_result));
  ret = AM_RESULT(service_result.ret);
  if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
    ERROR("Overlay plugin is not loaded!\n");
  } else if (ret != AM_RESULT_OK) {
    ERROR("failed to destroy overlay!\n");
  }
  return ret;
}

static AM_RESULT show_all_info()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((ret = show_working_mode()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = show_vin_info()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = show_buffer_info()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = show_canvas_info()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = show_stream_status()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = show_stream_info()) != AM_RESULT_OK) {
      break;
    }
    if (((ret = show_dptz_ratio()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
    if (((ret = show_dptz_size()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
    if (((ret = show_warp_cfg()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
    if (((ret = show_lbr_cfg()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
    if (((ret = show_eis_cfg()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
    if (((ret = show_motion_detect_config()) != AM_RESULT_OK) &&
        (ret != AM_RESULT_ERR_PLUGIN_LOAD)) {
      break;
    }
  } while (0);
  return ret;
}

int32_t main(int32_t argc, char **argv)
{
  if (argc < 2) {
    usage(argc, argv);
    return -1;
  }
  register_critical_error_signal_handler();
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (init_param(argc, argv) < 0) {
    return -1;
  }
  AM_RESULT ret = AM_RESULT_OK;
  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }

  do {
    if (show_info_flag) {
      show_all_info();
      break;
    } else {
      if (show_working_mode_flag) {
        show_working_mode();
      }
      if (show_lock_state) {
        show_stream_lock_state();
      }
      if (show_vin_info_flag) {
        show_vin_info();
      }
      if (show_buffer_flag) {
        show_buffer_info();
      }
      if (show_canvas_flag) {
        show_canvas_info();
      }
      if (show_status_flag) {
        show_stream_status();
      }
      if (show_stream_info_flag) {
        show_stream_info();
      }
      if (show_warp_flag) {
        show_warp_cfg();
      }
      if (show_dptz_ratio_flag) {
        show_dptz_ratio();
      }
      if (show_dptz_size_flag) {
        show_dptz_size();
      }
      if (show_lbr_flag) {
        show_lbr_cfg();
      }
      if (show_eis_flag) {
        show_eis_cfg();
      }
      if (show_motion_detect_flag) {
        show_motion_detect_config();
      }
    }
    if (show_avail_cpu_flag) {
      show_avail_cpu_clk();
    }
    if (show_cur_cpu_flag) {
      show_cur_cpu_clk();
    }
    if (set_cur_cpu_flag) {
      set_cur_cpu_clk();
    }
    if (get_platform_buffer_num_max_flag) {
      show_buffer_number_max();
      break;
    }
    if (get_platform_stream_num_max_flag) {
      show_stream_number_max();
      break;
    }

    if (start_flag) {
      if ((ret = start_encode()) != AM_RESULT_OK) {
        break;
      }
    } else if (stop_flag) {
      if ((ret = stop_encode()) != AM_RESULT_OK) {
        break;
      }
    }
    if (pause_streaming_id) {
      if ((ret = stop_streaming()) != AM_RESULT_OK) {
        break;
      }
    }
    if (resume_streaming_id) {
      if ((ret = start_streaming()) != AM_RESULT_OK) {
        break;
      }
    }

    if (set_dptz_ratio_flag) {
      if ((ret = set_dptz_ratio()) != AM_RESULT_OK) {
        break;
      }
    } else if (set_dptz_size_flag) {
      if ((ret = set_dptz_size()) != AM_RESULT_OK) {
        break;
      }
    }
    stream_lock_operation();
    if ((ret = set_buffer_fmt()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = set_warp_config()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = set_lbr_config()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = set_eis_config()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = set_motion_detect_config()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = set_stream_params()) != AM_RESULT_OK) {
      break;
    }
    if ((ret = stop_vout()) != AM_RESULT_OK) {
      break;
    }
    if(force_idr_id) {
      if ((ret = set_force_idr()) != AM_RESULT_OK) {
        break;
      }
    }

    if (fd_recog_enable) {
      if ((ret = enable_face_recognition()) != AM_RESULT_OK) {
        break;
      }
    }

    if (fd_recog_disable) {
      if ((ret = disable_face_recognition()) != AM_RESULT_OK) {
        break;
      }
    }

    if (destroy_overlay_flag) {
      if ((ret = destroy_overlay()) != AM_RESULT_OK) {
        break;
      }
    }
    if (get_overlay_max_flag) {
      show_overlay_max();
    }
    if (save_overlay_flag) {
      if ((ret = save_overlay_parameter()) != AM_RESULT_OK) {
        break;
      }
    }
    if (save_stream_cfg_flag) {
      if ((ret =save_stream_params()) != AM_RESULT_OK) {
        break;
      }
    }
    if (save_buffer_cfg_flag) {
      if ((ret =save_buffer_params()) != AM_RESULT_OK) {
        break;
      }
    }
    if (save_warp_cfg_flag) {
      if ((ret =save_warp_params()) != AM_RESULT_OK) {
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          PRINTF("WARP doen't load");
        } else {
          break;
        }
      }
    }
    if (save_eis_cfg_flag) {
      if ((ret =save_eis_params()) != AM_RESULT_OK) {
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          PRINTF("EIS doesn't load");
        } else {
          break;
        }
      }
    }
    if (save_md_cfg_flag) {
      if ((ret = save_current_md_config_to_file()) != AM_RESULT_OK) {
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          PRINTF("Motion-detect doesn't load");
        } else {
          break;
        }
      }
    }
    if (save_lbr_cfg_flag) {
      if ((ret = save_lbr_params()) != AM_RESULT_OK) {
        if (ret == AM_RESULT_ERR_PLUGIN_LOAD) {
          PRINTF("LBR doesnl't load");
        } else {
          break;
        }
      }
    }
    set_overlay();
    if (goto_vca_mode_flag) {
      goto_vca_mode();
    } else if (goto_iav_unloaded_mode_flag) {
      goto_iav_unloaded_mode();
    } else if (goto_iav_current_mode_flag) {
      goto_iav_current_mode();
    }

    if (restart_streaming_id) {
      if ((ret = restart_streaming()) != AM_RESULT_OK) {
        break;
      }
    }
  } while (0);

  return ret;
}
