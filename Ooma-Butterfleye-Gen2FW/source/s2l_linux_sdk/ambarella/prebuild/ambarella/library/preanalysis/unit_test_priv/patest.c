/*******************************************************************************
 * \file patest.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
#include "getopt.h"
#else
#include <unistd.h>
#include <getopt.h>
#endif //_WIN32

#include "preanalysis.h"
#include "bufpool.h"
#include "utils.h"

#ifdef _WIN32
#define f_seek _fseeki64
#define f_tell _ftelli64
#else
#define f_seek fseeko
#define f_tell ftello
#endif

#define BUFFER_ALIGNMENT_SHIFT 4

typedef struct amba_preanalysis_static_area_config_s
{
  uint8_t  str_distance[2];                   // short-term reference (STR) distance from current picture,
                                              // str_distance[0]>str_distance[1], str_distance[1]=0 if there is only one STR
  uint8_t  adaptive_threshold;                // 0: fixed threshold, 1: adaptive threshold based on noise-level
  uint8_t  motion_threshold;                  // motion detected when short-term difference>=motion_threshold
                                              // larger value leads to less motion area
} amba_preanalysis_static_area_config_t;

typedef struct amba_preanalysis_uncovered_area_config_s
{
  uint8_t  str_distance[2];                   // short-term reference (STR) distance from current picture,
                                              // str_distance[0]>str_distance[1], str_distance[1]=0 if there is only one STR
  uint8_t  adaptive_threshold;                // 0: fixed threshold, 1: adaptive threshold based on noise-level
  uint8_t  motion_threshold;                  // motion detected when short-term difference>=motion_threshold
                                              // larger value leads to less motion area
  uint8_t  static_threshold;                  // static area detected when short-term difference<static_threshold after motion is detected
                                              // larger value leads to more uncovered area
  uint8_t  ltr_similarity_threshold;          // LTR considered as good reference when long-term difference<ltr_similarity_threshold
                                              // larger value leads to less uncovered area
  uint8_t  ltr_difference_threshold;          // LTR considered as bad reference when long-term difference>=ltr_difference_threshold
                                              // larger value leads to less uncovered area
  uint8_t  counter_threshold;                 // update uncovered area after motion is detected and then being static for counter_threshold frames
                                              // larger value leads to less uncovered area
} amba_preanalysis_uncovered_area_config_t;

typedef struct amba_preanalysis_weighted_prediction_config_s
{
  amba_preanalysis_uncovered_area_config_t ua;
  uint8_t  p_slice_mode;                      // P slice analysis mode, 0: LMS model, 1: DC model
  uint8_t  b_slice_mode;                      // B slice analysis mode, 0: simple distance calculation, 1: DC model with some candidates and bi-weighted prediction
  uint8_t  distortion_shift;                  // additional downscaling shift for distortion evaluation
  uint8_t  add_offset_candidate;              // 0: disable, 1: add an offset-based candidate, 2: substitute a repeated candidate with an offset-based candidate
  uint8_t  use_motion_threshold;              // the threshold (in static area percentage) to include motion area information in brightness calculation
  uint8_t  num_region_horizontal;             // number of horizontal regions
  uint8_t  num_region_vertical;               // number of horizontal regions
  uint8_t  reserved0;
} amba_preanalysis_weighted_prediction_config_t;

typedef struct amba_preanalysis_perfect_background_config_s
{
  uint8_t  str_distance[2];                   // short-term reference (STR) distance from current picture,
                                              // str_distance[0]>str_distance[1], str_distance[1]=0 if there is only one STR
  uint8_t  adaptive_threshold;                // 0: fixed threshold, 1: adaptive threshold based on noise-level
  uint8_t  motion_threshold;                  // motion detected when short-term difference>=motion_threshold
                                              // larger value leads to less motion area
  uint8_t  max_static_counter;                // max_static_counter>0, when updating a static pixel in perfect-background image
                                              // using the current image, the alpha value in the blending (portion of pixel value in the current
                                              // image) is around (256/max_static_counter). smaller value leads to faster update and less robust result
  uint8_t  num_motion_accumulation_frames;    // number of past frames to consider when identifying a moving pixel
  uint8_t  alpha_output_shift;                // map_width=image_width<<alpha_output_shift, map_height=image_height<<alpha_output_shift
  uint8_t  reserved0;
} amba_preanalysis_perfect_background_config_t;

typedef struct amba_preanalysis_config_s
{
  uint32_t enable_flags;                      // bit-wise enable flag for different modules
  uint16_t image_width;                       // source image width, same as the encoder
  uint16_t image_height;                      // source image height, same as the encoder

  uint8_t  input_bufpool_size[AMBA_PREANALYSIS_NUM_DSP_ENGINES]; // the minimum number of buffers needed for VDSP input buffer pool, set by the library
  uint8_t  output_bufpool_size;               // the output buffer pool size, set by the caller
  uint8_t  dummy_latency;                     // the encoder dummy latency in number of frames set by the caller

  uint8_t  enable_ltr;                        // enable long-term reference (LTR)
  uint8_t  ltr_option;                        // 0: use the closest LTR against current picture for processing
                                              // 1: use the LTR before the closest LTR against current picture for processing
                                              // set to 1 if Q pictures are used as LTRs, otherwise set to 0
  uint8_t  gop_m;                             // P picture period, same as the encoder
  uint8_t  reserved1;
  amba_preanalysis_perfect_background_config_t pb;
  amba_preanalysis_static_area_config_t sa;
  union
  {
    amba_preanalysis_uncovered_area_config_t ua;
    amba_preanalysis_weighted_prediction_config_t wp;
  };
} amba_preanalysis_config_t;

int num_frames = 0;
uint16_t image_width = 1920;
uint16_t image_height = 1080;
uint8_t downscaling_shift = 4;

char *yuv_in = NULL;
char *yuv_out = NULL;
char *motion_out = NULL;
char *uncovered_out = NULL;
char *pbg_out = NULL;
char *alpha_out = NULL;
char *static_out = NULL;
char *wp_out = NULL;

void print_help(void)
{
  printf("\nAMBA preanalysis library unit-test utility:\n");
  printf("Usage: patest [options] input\n");
  printf("  -h,  --help                     this help text\n");
  printf("  -o,  --output                   downscaled YUV input [optional]\n");
  printf("  -m,  --motion                   motion map [optional]\n");
  printf("  -u,  --uncovered                uncovered area map [optional]\n");
  printf("  -s,  --static-area              static area map [optional]\n");
  printf("  -p,  --perfect-background       perfect-background output [optional]\n");
  printf("  -a,  --alpha-map                alpha map [optional]\n");
  printf("  -w,  --weighted-prediction      weighted prediction [optional]\n");
  printf("  -n,  --num-frames               number of frames to process [optional,default=0]\n");
  printf("                                  0 means all frames in sequence\n");
  printf("       --dummy-latency            encoder dummy latency [optional,default=5]\n");
  printf("       --width                    YUV input width [optional,default=1920]\n");
  printf("       --height                   YUV input height [optional,default=1080]\n");
  printf("       --downscaling-shift        downscaling right-shift ratio [optional,default=4]\n");
  printf("       --gop-n                    GOP_N [optional,default=960]\n");
  printf("       --gop-m                    GOP_M [optional,default=3]\n");
  printf("       --gop-q                    GOP_Q [optional,default=60]\n");
  printf("                                  0 means Q frames unused\n");
  printf("       --ua-str0-distance         uncovered area 1st short-term reference distance [optional,default=2]\n");
  printf("       --ua-str1-distance         uncovered area 2nd short-term reference distance [optional,default=4]\n");
  printf("                                  0 means 2nd short-term reference is unused\n");
  printf("       --ltr-period               long-term reference period [optional,default=960]\n");
  printf("       --ltr-option               [optional,default=0]\n");
  printf("                                  0 means nearest long-term reference is used\n");
  printf("                                  1 means 2nd-nearest long-term reference is used\n");
  printf("                                  if long-term reference is from Q frames, set to 1, else 0\n");
  printf("       --ua-motion-t              uncovered area motion threshold [optional,default=8]\n");
  printf("       --static-t                 static threshold [optional,default=2]\n");
  printf("       --ltr-similarity-t         long-term reference similarity threshold [optional,default=2]\n");
  printf("       --ltr-difference-t         long-term reference difference threshold [optional,default=16]\n");
  printf("       --uncover-counter-t        uncover update threshold [optional,default=3]\n");
  printf("       --ua-adaptive              0: fixed, 1: adaptive based on noise level [optional,default=1]\n");
  printf("       --max-static-counter       maximum static counter for perfect background [optional,default=4]\n");
  printf("       --num-motion-acc           number of motion accumulated frames [optional,default=4]\n");
  printf("       --alpha-shift              alpha output left-shift [optional,default=0]\n");
  printf("       --pb-str0-distance         perfect background 1st short-term reference distance [optional,default=8]\n");
  printf("       --pb-str1-distance         perfect background 2nd short-term reference distance [optional,default=15]\n");
  printf("                                  0 means 2nd short-term reference is unused\n");
  printf("       --pb-motion-t              perfect background motion threshold [optional,default=4]\n");
  printf("       --pb-adaptive              0: fixed, 1: adaptive based on noise level [optional,default=1]\n");
  printf("       --sa-str0-distance         static area 1st short-term reference distance [optional,default=2]\n");
  printf("       --sa-str1-distance         static area 2nd short-term reference distance [optional,default=4]\n");
  printf("                                  0 means 2nd short-term reference is unused\n");
  printf("       --sa-motion-t              static area motion threshold [optional,default=4]\n");
  printf("       --sa-adaptive              0: fixed, 1: adaptive based on noise level [optional,default=1]\n");
  printf("       --b-slice-mode             weighted prediction B slice analysis mode [optional,default=1]\n");
  printf("                                  0: simple distance calculation, 1: DC model with some candidates and bi-weighted prediction\n");
  printf("       --distortion-shift         weighted prediction additional downscaling shift for distortion evaluation [optional,default=3]\n");
  printf("       --add-offset-candidate     0: disable, 1: add an offset-based candidate, 2: substitute a repeated candidate with an offset-based candidate [optional,default=2]\n");
  printf("       --use-motion-t             the threshold (in static area percentage) to include motion area information in brightness calculation [optional,default=60]\n");
  printf("       --num-region-horizontal    number of horizontal regions in brightness calculation [optional,default=1]\n");
  printf("       --num-region-vertical      number of vertical regions in brightness calculation [optional,default=1]\n");
  printf("       --use-lms-model            computation model for weighted prediction candidates generation [optional,default=1]\n");
  printf("                                  0: use DC model, 1: use LMS model\n");
}

static void open_file(FILE **f, const char *path, const char *attr)
{
  if (NULL == path) return;

  assert(f);
  assert(attr);

  *f = fopen(path, attr);
  if (NULL == *f)
  {
    fprintf(stderr, "%s(): cannot open file %s\n", __func__, path);
    exit(-1);
  }
}

static void write_file(FILE *f, amba_preanalysis_buf_t *buf)
{
  uint8_t *base;
  amba_preanalysis_buf_info_t info;

  if (NULL == f)
  {
    return;
  }

  assert(buf);

  if (0 != get_buf_info(buf->pool, &info))
  {
    fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
    exit(-1);
  }

  base = (uint8_t *)buf->base;

  if (info.pitch == info.width)
  {
    int len = info.pitch * info.height;

    if (len != fwrite(base, sizeof(uint8_t), len, f))
    {
      fprintf(stderr, "%s(): error calling fwrite()\n", __func__);
      exit(-1);
    }
  }
  else
  {
    int i;
    for (i = 0; i < info.height; ++i)
    {
      if (info.width != fwrite(&base[i * info.pitch], sizeof(uint8_t), info.width, f))
      {
        fprintf(stderr, "%s(): error calling fwrite()\n", __func__);
        exit(-1);
      }
    }
  }
}

static void get_num_frames(FILE *f, int frame_size)
{
  int64_t f_len = 0;
  int frames_in_file = 0;

  assert(f);

  f_seek(f, 0L, SEEK_END);
  f_len = f_tell(f);
  rewind(f);

  frames_in_file = (int)(f_len / frame_size);

  if (0 == num_frames || frames_in_file < num_frames)
  {
    num_frames = frames_in_file;
    fprintf(stdout, "%s(): %d frames will be processed\n", __func__, num_frames);
  }
}

static void derive_input_metadata(amba_preanalysis_config_t *cfg, amba_preanalysis_input_t *input, uint16_t gop_n, uint16_t ltr_period)
{
  assert(cfg && input);

  if (input->frame_no % gop_n)
  {
    input->frame_type = input->frame_no % cfg->gop_m ? (uint8_t)AMBA_PREANALYSIS_B_PICT : (uint8_t)AMBA_PREANALYSIS_P_PICT;

    input->flag_long_term = cfg->enable_ltr && (0 == input->frame_no % ltr_period) ? 1 : 0;

    input->flag_output = 1;
  }
  else
  {
    input->frame_type = (uint8_t)AMBA_PREANALYSIS_IDR_PICT;

    input->flag_long_term = cfg->enable_ltr;

    input->flag_output = cfg->enable_flags & AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND ? 0 : 1;
  }
}

int main(int argc, char *argv[])
{
  FILE *f_in = NULL;
  FILE *f_out = NULL;
  FILE *f_motion = NULL;
  FILE *f_uncovered = NULL;
  FILE *f_pbg = NULL;
  FILE *f_alpha = NULL;
  FILE *f_static = NULL;
  FILE *f_wp = NULL;
  bufpool_t *input_pool[AMBA_PREANALYSIS_NUM_DSP_ENGINES] = { NULL };
  amba_preanalysis_buf_info_t buf_info;
  amba_preanalysis_context_t *ctx = NULL;
  amba_preanalysis_status_t status;

  amba_preanalysis_config_t config;
  amba_preanalysis_ret_t ret_value;
  amba_preanalysis_buf_func_t fp;

  amba_preanalysis_idsp_output_t output_idsp;
  amba_preanalysis_vdsp_output_t output_vdsp;

  uint16_t ltr_period = 960;
  uint16_t gop_n = 960;
  uint16_t gop_q = 60;

  int frame_size = 0;
  int frame_no = 0;
  uint8_t *src_image = NULL;
  int i;

  fprintf(stdout, "preanalysis library version: %d\n", amba_preanalysis_get_version());

  assert(sizeof(amba_preanalysis_configuration_t) == sizeof(amba_preanalysis_config_t));

  ret_value = amba_preanalysis_create_default_config((amba_preanalysis_configuration_t *)&config);
  if (AMBA_PREANALYSIS_OK != ret_value)
  {
    fprintf(stderr, "%s(): error calling amba_preanalysis_create_default_config(): %d\n", __func__, ret_value);
    exit(-1);
  }

  while (1)
  {
    int c;
    int option_index = 0;
    static struct option long_options[] =
    {
      {"width",                 required_argument,  0,    0},
      {"height",                required_argument,  0,    0},
      {"downscaling-shift",     required_argument,  0,    0},
      {"dummy-latency",         required_argument,  0,    0},
      {"gop-n",                 required_argument,  0,    0},
      {"gop-m",                 required_argument,  0,    0},
      {"gop-q",                 required_argument,  0,    0},
      {"ua-str0-distance",      required_argument,  0,    0},
      {"ua-str1-distance",      required_argument,  0,    0},
      {"ltr-period",            required_argument,  0,    0},
      {"ltr-option",            required_argument,  0,    0},
      {"ua-motion-t",           required_argument,  0,    0},
      {"static-t",              required_argument,  0,    0},
      {"ltr-similarity-t",      required_argument,  0,    0},
      {"ltr-difference-t",      required_argument,  0,    0},
      {"uncover-counter-t",     required_argument,  0,    0},
      {"ua-adaptive",           required_argument,  0,    0},
      {"max-static-counter",    required_argument,  0,    0},
      {"num-motion-acc",        required_argument,  0,    0},
      {"alpha-shift",           required_argument,  0,    0},
      {"pb-str0-distance",      required_argument,  0,    0},
      {"pb-str1-distance",      required_argument,  0,    0},
      {"pb-motion-t",           required_argument,  0,    0},
      {"pb-adaptive",           required_argument,  0,    0},
      {"sa-str0-distance",      required_argument,  0,    0},
      {"sa-str1-distance",      required_argument,  0,    0},
      {"sa-motion-t",           required_argument,  0,    0},
      {"sa-adaptive",           required_argument,  0,    0},
      {"p-slice-mode",          required_argument,  0,    0},
      {"b-slice-mode",          required_argument,  0,    0},
      {"distortion-shift",      required_argument,  0,    0},
      {"add-offset-candidate",  required_argument,  0,    0},
      {"use-motion-t",          required_argument,  0,    0},
      {"num-region-horizontal", required_argument,  0,    0},
      {"num-region-vertical",   required_argument,  0,    0},
      {"help",                  no_argument,        0,  'h'},
      {"num-frames",            required_argument,  0,  'n'},
      {"output",                required_argument,  0,  'o'},
      {"motion",                required_argument,  0,  'm'},
      {"uncovered",             required_argument,  0,  'u'},
      {"perfect-background",    required_argument,  0,  'p'},
      {"alpha-map",             required_argument,  0,  'a'},
      {"static-area",           required_argument,  0,  's'},
      {"weighted-prediction",   required_argument,  0,  'w'},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "hn:o:m:u:p:a:s:w:", long_options, &option_index);
    if (-1 == c) break;

    switch (c)
    {
    case 0:
      switch (option_index)
      {
      case 0:
        image_width = atoi(optarg);
        break;
      case 1:
        image_height = atoi(optarg);
        break;
      case 2:
        downscaling_shift = atoi(optarg);
        break;
      case 3:
        config.dummy_latency = atoi(optarg);
        break;
      case 4:
        gop_n = atoi(optarg);
        break;
      case 5:
        config.gop_m = atoi(optarg);
        break;
      case 6:
        gop_q = atoi(optarg);
        break;
      case 7:
        config.ua.str_distance[0] = atoi(optarg);
        break;
      case 8:
        config.ua.str_distance[1] = atoi(optarg);
        break;
      case 9:
        ltr_period = atoi(optarg);
        config.enable_ltr = !!ltr_period;
        break;
      case 10:
        config.ltr_option = atoi(optarg);
        break;
      case 11:
        config.ua.motion_threshold = atoi(optarg);
        break;
      case 12:
        config.ua.static_threshold = atoi(optarg);
        break;
      case 13:
        config.ua.ltr_similarity_threshold = atoi(optarg);
        break;
      case 14:
        config.ua.ltr_difference_threshold = atoi(optarg);
        break;
      case 15:
        config.ua.counter_threshold = atoi(optarg);
        break;
      case 16:
        config.ua.adaptive_threshold = atoi(optarg);
        break;
      case 17:
        config.pb.max_static_counter = atoi(optarg);
        break;
      case 18:
        config.pb.num_motion_accumulation_frames = atoi(optarg);
        break;
      case 19:
        config.pb.alpha_output_shift = atoi(optarg);
        break;
      case 20:
        config.pb.str_distance[0] = atoi(optarg);
        break;
      case 21:
        config.pb.str_distance[1] = atoi(optarg);
        break;
      case 22:
        config.pb.motion_threshold = atoi(optarg);
        break;
      case 23:
        config.pb.adaptive_threshold = atoi(optarg);
        break;
      case 24:
        config.sa.str_distance[0] = atoi(optarg);
        break;
      case 25:
        config.sa.str_distance[1] = atoi(optarg);
        break;
      case 26:
        config.sa.motion_threshold = atoi(optarg);
        break;
      case 27:
        config.sa.adaptive_threshold = atoi(optarg);
        break;
      case 28:
        config.wp.p_slice_mode = atoi(optarg);
        break;
      case 29:
        config.wp.b_slice_mode = atoi(optarg);
        break;
      case 30:
        config.wp.distortion_shift = atoi(optarg);
        break;
      case 31:
        config.wp.add_offset_candidate = atoi(optarg);
        break;
      case 32:
        config.wp.use_motion_threshold = atoi(optarg);
        break;
      case 33:
        config.wp.num_region_horizontal = atoi(optarg);
        break;
      case 34:
        config.wp.num_region_vertical = atoi(optarg);
        break;
      }
      break;
    case 'h':
      print_help();
      exit(0);
      break;
    case 'n':
      num_frames = atoi(optarg);
      break;
    case 'o':
      yuv_out = optarg;
      break;
    case 'm':
      motion_out = optarg;
      break;
    case 'u':
      uncovered_out = optarg;
      break;
    case 'p':
      pbg_out = optarg;
      break;
    case 'a':
      alpha_out = optarg;
      break;
    case 's':
      static_out = optarg;
      break;
    case 'w':
      wp_out = optarg;
      break;
    case '?':
      break;
    default:
      fprintf(stderr, "?? getopt returned char code 0%o ??\n", c);
    }
  }

  if (optind < argc)
  {
    yuv_in = argv[optind];
  }

  if (NULL == yuv_in)
  {
    fprintf(stderr, "%s(): no input file\n", __func__);
    exit(-1);
  }
  else
  {
    open_file(&f_in, yuv_in, "rb");
  }

  open_file(&f_out, yuv_out, "wb");
  open_file(&f_motion, motion_out, "wb");

  if (uncovered_out)
  {
    open_file(&f_uncovered, uncovered_out, "wb");
    config.enable_flags |= AMBA_PREANALYSIS_FLAG_UNCOVERED_AREA;
  }

  if (alpha_out)
  {
    open_file(&f_alpha, alpha_out, "wb");
    open_file(&f_pbg, pbg_out, "wb");
    config.enable_flags |= AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND;
  }

  if (static_out)
  {
    open_file(&f_static, static_out, "wb");
    config.enable_flags |= AMBA_PREANALYSIS_FLAG_STATIC_AREA;
  }

  if (wp_out)
  {
    open_file(&f_wp, wp_out, "wb");
    config.enable_flags |= AMBA_PREANALYSIS_FLAG_WEIGHTED_PREDICTION;
  }

  /* calculate the input image size */
  config.image_width = (image_width + (1 << downscaling_shift) - 1) >> downscaling_shift;
  config.image_height = (image_height + (1 << downscaling_shift) - 1) >> downscaling_shift;
  config.output_bufpool_size = 2;

  if(config.ltr_option)
  {
    assert(gop_q);
  }

  ctx = amba_preanalysis_open((amba_preanalysis_configuration_t *)&config);
  if (AMBA_PREANALYSIS_OK != ret_value)
  {
    fprintf(stderr, "%s(): error calling amba_preanalysis_open(): %d\n", __func__, ret_value);
    exit(-1);
  }

  buf_info.width = config.image_width;
  buf_info.pitch = ((config.image_width + (1 << BUFFER_ALIGNMENT_SHIFT) - 1) >> BUFFER_ALIGNMENT_SHIFT) << BUFFER_ALIGNMENT_SHIFT;
  buf_info.height = config.image_height;
  for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
  {
    if (0 == config.input_bufpool_size[i])
    {
      continue;
    }

    input_pool[i] = (bufpool_t *)create_bufpool(&buf_info, config.input_bufpool_size[i]);
    if (NULL == input_pool)
    {
      fprintf(stderr, "%s(): error calling create_bufpool()\n", __func__);
      exit(-1);
    }
  }

  ret_value = amba_preanalysis_set_debug_level(ctx, AMBA_PREANALYSIS_DEBUG_LEVEL_INFO);
  if (AMBA_PREANALYSIS_OK != ret_value)
  {
    fprintf(stderr, "%s(): error calling amba_preanalysis_set_debug_level(): %d\n", __func__, ret_value);
    exit(-1);
  }

  fp.create_bufpool = create_bufpool;
  fp.destroy_bufpool = destroy_bufpool;
  fp.get_buf_info = get_buf_info;
  fp.request_buf = request_buf;
  fp.release_buf = release_buf;
  ret_value = amba_preanalysis_register_buf_callback(ctx, &fp);
  if (AMBA_PREANALYSIS_OK != ret_value)
  {
    fprintf(stderr, "%s(): error calling amba_preanalysis_register_buf_callback(): %d\n", __func__, ret_value);
    exit(-1);
  }

  frame_size = (image_width * image_height * 3) >> 1; // assume YUV 4:2:0 format
  get_num_frames(f_in, frame_size);

  src_image = malloc(frame_size * sizeof(uint8_t));
  if (!src_image)
  {
    fprintf(stderr, "%s(): error calling malloc()\n", __func__);
    exit(-1);
  }

  memset(&output_idsp, 0, sizeof(output_idsp));
  memset(&output_vdsp, 0, sizeof(output_vdsp));

  for (frame_no = 0; frame_no < num_frames; ++frame_no)
  {
    amba_preanalysis_input_t input[AMBA_PREANALYSIS_NUM_DSP_ENGINES];

    for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
    {
      memset(&input[i], 0, sizeof(amba_preanalysis_input_t));
      input[i].frame_no = frame_no;
      input[i].buf.pool = (void *)input_pool[i];

      if (NULL == input_pool[i])
      {
        continue;
      }

      if (AMBA_PREANALYSIS_VDSP_ENGINE == i)
      {
        derive_input_metadata(&config, &input[i], gop_n, ltr_period);
      }

      if (0 != request_buf(&input[i].buf))
      {
        fprintf(stderr, "%s(): error calling request_buf()\n", __func__);
        exit(-1);
      }
    }

    fprintf(stdout, "%s(): read frame %d\n", __func__, frame_no);

    if (frame_size != fread(src_image, sizeof(uint8_t), frame_size, f_in))
    {
      fprintf(stderr, "%s(): error calling fread()\n", __func__);
      exit(-1);
    }

    if (input[AMBA_PREANALYSIS_VDSP_ENGINE].buf.base && input[AMBA_PREANALYSIS_IDSP_ENGINE].buf.base)
    {
      if (0 != get_buf_info(input[AMBA_PREANALYSIS_VDSP_ENGINE].buf.pool, &buf_info))
      {
        fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
        exit(-1);
      }

      down_sample(src_image, input[AMBA_PREANALYSIS_VDSP_ENGINE].buf.base, image_width, image_height, buf_info.pitch, downscaling_shift);
      memcpy(input[AMBA_PREANALYSIS_IDSP_ENGINE].buf.base, input[AMBA_PREANALYSIS_VDSP_ENGINE].buf.base, sizeof(uint8_t) * buf_info.pitch * buf_info.height);
      write_file(f_out, &input[AMBA_PREANALYSIS_VDSP_ENGINE].buf);
    }
    else
    {
      for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
      {
        if (NULL == input[i].buf.base)
        {
          continue;
        }

        if (0 != get_buf_info(input[i].buf.pool, &buf_info))
        {
          fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
          exit(-1);
        }

        down_sample(src_image, input[i].buf.base, image_width, image_height, buf_info.pitch, downscaling_shift);
        write_file(f_out, &input[i].buf);
      }
    }

    if (config.enable_flags)
    {
      if (config.enable_flags & AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND)
      {
        ret_value = amba_preanalysis_process_idsp(ctx, &input[AMBA_PREANALYSIS_IDSP_ENGINE], &output_idsp);
        if (AMBA_PREANALYSIS_OK != ret_value)
        {
          fprintf(stderr, "%s(): error calling amba_preanalysis_process_idsp(): %d\n", __func__, ret_value);
          exit(-1);
        }
      }

      if (config.enable_flags & ~AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND)
      {
        ret_value = amba_preanalysis_process_vdsp(ctx, &input[AMBA_PREANALYSIS_VDSP_ENGINE], &output_vdsp);
        if (AMBA_PREANALYSIS_OK != ret_value)
        {
          fprintf(stderr, "%s(): error calling amba_preanalysis_process_vdsp(): %d\n", __func__, ret_value);
          exit(-1);
        }
      }
    }
    else
    {
      for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
      {
        if (NULL == input[i].buf.base)
        {
          continue;
        }

        if (0 != release_buf(&input[i].buf))
        {
          fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
          exit(-1);
        }
      }
    }

    if (output_vdsp.output_flags & AMBA_PREANALYSIS_FLAG_UNCOVERED_AREA)
    {
      if (output_vdsp.motion_map.base && f_motion)
      {
        if (0 != get_buf_info(output_vdsp.motion_map.pool, &buf_info))
        {
          fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
          exit(-1);
        }
        assert(buf_info.pitch == buf_info.width);

        f_seek(f_motion, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

        write_file(f_motion, &output_vdsp.motion_map);
        if (0 != release_buf(&output_vdsp.motion_map))
        {
          fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
          exit(-1);
        }
      }

      if (output_vdsp.uncovered_area.base)
      {
        if (0 != get_buf_info(output_vdsp.uncovered_area.pool, &buf_info))
        {
          fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
          exit(-1);
        }
        assert(buf_info.pitch == buf_info.width);

        scale_result(output_vdsp.uncovered_area.base, buf_info.width, buf_info.height, buf_info.pitch);

        f_seek(f_uncovered, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

        fprintf(stdout, "%s(): output frame %d\n", __func__, (uint32_t)output_vdsp.frame_no);

        write_file(f_uncovered, &output_vdsp.uncovered_area);
        if (0 != release_buf(&output_vdsp.uncovered_area))
        {
          fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
          exit(-1);
        }
      }
    }

    if (output_vdsp.output_flags & AMBA_PREANALYSIS_FLAG_STATIC_AREA)
    {
      if (output_vdsp.static_area.base)
      {
        if (0 != get_buf_info(output_vdsp.static_area.pool, &buf_info))
        {
          fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
          exit(-1);
        }
        assert(buf_info.pitch == buf_info.width);

        scale_result(output_vdsp.static_area.base, buf_info.width, buf_info.height, buf_info.pitch);

        f_seek(f_static, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

        write_file(f_static, &output_vdsp.static_area);
        if (0 != release_buf(&output_vdsp.static_area))
        {
          fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
          exit(-1);
        }
      }
    }

    if (output_idsp.output_flags & AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND)
    {
      if (output_idsp.alpha_index.base)
      {
        if (0 != get_buf_info(output_idsp.alpha_index.pool, &buf_info))
        {
          fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
          exit(-1);
        }
        assert(buf_info.pitch == buf_info.width);

        create_alpha(output_idsp.alpha_index.base, buf_info.width, buf_info.height);

        write_file(f_alpha, &output_idsp.alpha_index);
        if (0 != release_buf(&output_idsp.alpha_index))
        {
          fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
          exit(-1);
        }
      }
    }

    if (output_vdsp.output_flags & AMBA_PREANALYSIS_FLAG_WEIGHTED_PREDICTION)
    {
      f_seek(f_wp, output_vdsp.frame_no * sizeof(output_vdsp.weighted_prediction), SEEK_SET);

      if (sizeof(output_vdsp.weighted_prediction) != fwrite(&output_vdsp.weighted_prediction, sizeof(uint8_t), sizeof(output_vdsp.weighted_prediction), f_wp))
      {
        fprintf(stderr, "%s(): error calling fwrite()\n", __func__);
        exit(-1);
      }
    }
  }

  /* no more input, process until no more output */
  do
  {
    if (config.enable_flags & ~AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND)
    {
      /*
       * use the NULL pointer for input to indicate we are in the flushing stage to push out results
       * because of internally delayed processing pipeline
       */
      ret_value = amba_preanalysis_process_vdsp(ctx, NULL, &output_vdsp);
      if (AMBA_PREANALYSIS_OK != ret_value)
      {
        fprintf(stderr, "%s(): error calling amba_preanalysis_process_picture(): %d\n", __func__, ret_value);
        exit(-1);
      }

      if (output_vdsp.output_flags & AMBA_PREANALYSIS_FLAG_UNCOVERED_AREA)
      {
        if (output_vdsp.motion_map.base && f_motion)
        {
          if (0 != get_buf_info(output_vdsp.motion_map.pool, &buf_info))
          {
            fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
            exit(-1);
          }
          assert(buf_info.pitch == buf_info.width);

          f_seek(f_motion, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

          write_file(f_motion, &output_vdsp.motion_map);
          if (0 != release_buf(&output_vdsp.motion_map))
          {
            fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
            exit(-1);
          }
        }

        if (output_vdsp.uncovered_area.base)
        {
          if (0 != get_buf_info(output_vdsp.uncovered_area.pool, &buf_info))
          {
            fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
            exit(-1);
          }
          assert(buf_info.pitch == buf_info.width);

          scale_result(output_vdsp.uncovered_area.base, buf_info.width, buf_info.height, buf_info.pitch);

          fprintf(stdout, "%s(): output frame %d\n", __func__, (uint32_t)output_vdsp.frame_no);
          f_seek(f_uncovered, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

          write_file(f_uncovered, &output_vdsp.uncovered_area);
          if (0 != release_buf(&output_vdsp.uncovered_area))
          {
            fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
            exit(-1);
          }
        }
      }

      if (output_vdsp.output_flags & AMBA_PREANALYSIS_FLAG_STATIC_AREA)
      {
        if (output_vdsp.static_area.base)
        {
          if (0 != get_buf_info(output_vdsp.static_area.pool, &buf_info))
          {
            fprintf(stderr, "%s(): error calling get_buf_info()\n", __func__);
            exit(-1);
          }
          assert(buf_info.pitch == buf_info.width);

          scale_result(output_vdsp.static_area.base, buf_info.width, buf_info.height, buf_info.pitch);

          f_seek(f_static, output_vdsp.frame_no * buf_info.width * buf_info.height, SEEK_SET);

          write_file(f_static, &output_vdsp.static_area);
          if (0 != release_buf(&output_vdsp.static_area))
          {
            fprintf(stderr, "%s(): error calling release_buf()\n", __func__);
            exit(-1);
          }
        }
      }
    }
  } while(output_vdsp.motion_map.base || output_vdsp.uncovered_area.base || output_vdsp.static_area.base);

  for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
  {
    ret_value = amba_preanalysis_get_status(ctx, i, &status);
    if (AMBA_PREANALYSIS_OK != ret_value)
    {
      fprintf(stderr, "%s(): error calling amba_preanalysis_get_status(): %d\n", __func__, ret_value);
      exit(-1);
    }
    fprintf(stdout, "%s(): %d buffers still held by engine context %d\n", __func__, status.num_buffers_used, i);
  }

  ret_value = amba_preanalysis_close(ctx);
  if (AMBA_PREANALYSIS_OK != ret_value)
  {
    fprintf(stderr, "%s(): error calling amba_preanalysis_close(): %d\n", __func__, ret_value);
    exit(-1);
  }

  for (i = 0; i < AMBA_PREANALYSIS_NUM_DSP_ENGINES; ++i)
  {
    if (input_pool[i])
    {
      destroy_bufpool(input_pool[i]);
    }
  }

  if (f_in) fclose(f_in);
  if (f_out) fclose(f_out);
  if (f_motion) fclose(f_motion);
  if (f_uncovered) fclose(f_uncovered);
  if (f_pbg) fclose(f_pbg);
  if (f_alpha) fclose(f_alpha);
  if (f_static) fclose(f_static);
  if (f_wp) fclose(f_wp);

  return 0;
}
