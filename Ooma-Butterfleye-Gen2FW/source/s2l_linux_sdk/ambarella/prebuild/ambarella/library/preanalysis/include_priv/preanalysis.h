/*******************************************************************************
 * \file preanalysis.h
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

#ifndef _PREANALYSIS_H_
#define _PREANALYSIS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef AMBA_API
#ifdef __GNUC__
#define AMBA_API __attribute__((visibility("default")))
#else
#define AMBA_API
#endif
#endif

#define AMBA_PREANALYSIS_MAX_NUM_LTRS           2   // maximum number of long-term references
#define AMBA_PREANALYSIS_MAX_NUM_BUFFERS        32  // maximum number of buffers in the input buffer pool

/**
 * this definition MUST match ucode header file
 */
typedef enum amba_preanalysis_frame_type_s
{
  AMBA_PREANALYSIS_INVALID_PICT   = 0,
  AMBA_PREANALYSIS_IDR_PICT       = 1,
  AMBA_PREANALYSIS_I_PICT         = 2,
  AMBA_PREANALYSIS_P_PICT         = 3,
  AMBA_PREANALYSIS_B_PICT         = 4,
  AMBA_PREANALYSIS_JPG_PICT       = 5,
  AMBA_PREANALYSIS_FS_P_PICT      = 6       // fast-seek P type
} amba_preanalysis_frame_type_t;

typedef enum amba_preanalysis_engine_s
{
  AMBA_PREANALYSIS_VDSP_ENGINE    = 0,
  AMBA_PREANALYSIS_IDSP_ENGINE    = 1,
  AMBA_PREANALYSIS_NUM_DSP_ENGINES
} amba_preanalysis_engine_t;

typedef enum amba_preanalysis_debug_level_s
{
  AMBA_PREANALYSIS_DEBUG_LEVEL_ERROR          = 0,
  AMBA_PREANALYSIS_DEBUG_LEVEL_WARNING        = 1,
  AMBA_PREANALYSIS_DEBUG_LEVEL_INFO           = 2,
  AMBA_PREANALYSIS_DEBUG_LEVEL_DEBUG          = 3,
} amba_preanalysis_debug_level_t;

typedef enum amba_preanalysis_ret_s
{
  AMBA_PREANALYSIS_OK             = 0,
  AMBA_PREANALYSIS_INVALID_ARG    = -1,
  AMBA_PREANALYSIS_NO_BUF         = -2,
  AMBA_PREANALYSIS_NONEXISTENT    = -3,
  AMBA_PREANALYSIS_UNKNOWN        = -4,
} amba_preanalysis_ret_t;

typedef enum amba_preanalysis_enable_flags_s
{
  AMBA_PREANALYSIS_FLAG_UNCOVERED_AREA        = (1 << 0),
  AMBA_PREANALYSIS_FLAG_PERFECT_BACKGROUND    = (1 << 1),
  AMBA_PREANALYSIS_FLAG_STATIC_AREA           = (1 << 2),
  AMBA_PREANALYSIS_FLAG_WEIGHTED_PREDICTION   = (1 << 3),
} amba_preanalysis_enable_flags_t;

typedef struct amba_preanalysis_buf_info_s
{
  uint16_t width;
  uint16_t height;
  uint16_t pitch;
  uint16_t reserved0;
} amba_preanalysis_buf_info_t;

typedef struct amba_preanalysis_buf_s
{
  void     *base;
  void     *pool;
} amba_preanalysis_buf_t;

typedef struct amba_preanalysis_weighted_prediction_out_s
{
  uint8_t frame_avg_brightness;
  int8_t  luma_log2_weight_denom;
  uint8_t reserved0[2];

  uint8_t wp_enable[2][4]; //[list][ref]
  int8_t  wp_weight[2][4]; //[list][ref] valid value -128 to 127
  int8_t  wp_offset[2][4]; //[list][ref] valid value -128 to 127
} amba_preanalysis_weighted_prediction_out_t;

typedef struct amba_preanalysis_buf_func_s
{
  void *(*create_bufpool)(amba_preanalysis_buf_info_t *info, int size);
  int (*request_buf)(amba_preanalysis_buf_t *buf);
  int (*release_buf)(amba_preanalysis_buf_t *buf);
  int (*get_buf_info)(void *buf_pool, amba_preanalysis_buf_info_t *info);
  void (*destroy_bufpool)(void *buf_pool);
} amba_preanalysis_buf_func_t;

typedef struct amba_preanalysis_input_s
{
  uint64_t frame_no;                          // frame identifier

  uint8_t  frame_type;                        // frame type, must be the same as defined in ucode
  uint8_t  flag_long_term;                    // frame is a long-term reference flag
  uint8_t  flag_output;                       // frame is output flag (perfect-background frame is not output)
  uint8_t  reserved0;

  amba_preanalysis_buf_t buf;                 // the input buffer, contains downscaled monochrome picture
} amba_preanalysis_input_t;

typedef struct amba_preanalysis_idsp_output_s
{
  uint64_t frame_no;                          // frame identifier carried over from the corresponding input buffer
  uint32_t output_flags;                      // bit-wise output flag for different modules

  amba_preanalysis_buf_t alpha_index;         // alpha index map, NULL if unset
} amba_preanalysis_idsp_output_t;

typedef struct amba_preanalysis_vdsp_output_s
{
  uint64_t frame_no;                          // frame identifier carried over from the corresponding input buffer
  uint32_t output_flags;                      // bit-wise output flag for different modules

  uint8_t  noise_level;                       // the output noise level
  uint8_t  frame_type;                        // frame type carried over from the corresponding output buffer
  uint8_t  reserved0[2];

  amba_preanalysis_buf_t uncovered_area;      // uncovered area map, NULL if unset
  amba_preanalysis_buf_t motion_map;          // motion map, NULL if unset
  amba_preanalysis_buf_t static_area;         // static area map, NULL if unset
  amba_preanalysis_weighted_prediction_out_t weighted_prediction; // weighted prediction output
} amba_preanalysis_vdsp_output_t;

typedef struct amba_preanalysis_status_s
{
  uint64_t frame_no;                          // frame identifier the last input/output buffer, -1 upon initialization
  uint8_t  num_buffers_used;                  // number of internally used input buffers
  uint8_t  reserved0[3];

  amba_preanalysis_input_t ltr[AMBA_PREANALYSIS_MAX_NUM_LTRS]; // the internally used long-term reference (LTR) buffers
  amba_preanalysis_input_t str[AMBA_PREANALYSIS_MAX_NUM_BUFFERS]; // the internally used short-term reference buffers
} amba_preanalysis_status_t;

typedef struct amba_preanalysis_configuration_s
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
  uint32_t reserved2[7];
} amba_preanalysis_configuration_t;

/**
 * \struct typedef struct amba_preanalysis_context_s amba_preanalysis_context_t
   \brief the opaque context returned by calling amba_preanalysis_open()
 */
typedef struct amba_preanalysis_context_s amba_preanalysis_context_t;

/**
 * \fn int amba_preanalysis_get_version(void)
 * \brief get the SVN revision number of the release.
 *
 * \return return SVN revision number, -1 for unofficial release.
 */
AMBA_API int amba_preanalysis_get_version(void);

/**
 * \fn int amba_preanalysis_create_default_config(amba_preanalysis_configuration_t *cfg)
 * \brief create default configuration to be used to initialize the library
 *
 * this function must be called prior to calling amba_preanalysis_open(). the returned configuration
 * is meant to be used in that call.
 *
 * \param cfg       the generated output configuration
 * \return return code
 */
AMBA_API int amba_preanalysis_create_default_config(amba_preanalysis_configuration_t *cfg);

/**
 * \fn int amba_preanalysis_get_config(amba_preanalysis_context_t *ctx, amba_preanalysis_configuration_t *cfg)
 * \brief get the current configuration of an instance of the library
 *
 * \param ctx       the handle to the instance
 * \param cfg       the received output configuration
 * \return return code
 */
AMBA_API int amba_preanalysis_get_config(amba_preanalysis_context_t *ctx, amba_preanalysis_configuration_t *cfg);

/**
 * \fn int amba_preanalysis_set_config(amba_preanalysis_context_t *ctx, amba_preanalysis_configuration_t *cfg)
 * \brief get the current configuration of an instance of the library
 *
 * \param ctx       the handle to the instance
 * \param cfg       the input configuration
 * \return return code
 */
AMBA_API int amba_preanalysis_set_config(amba_preanalysis_context_t *ctx, amba_preanalysis_configuration_t *cfg);

/**
 * \fn int amba_preanalysis_get_debug_level(amba_preanalysis_context_t *ctx)
 * \brief get the current debug level of an instance of the library
 *
 * \param ctx       the handle to the instance
 * \return debug level
 */
AMBA_API int amba_preanalysis_get_debug_level(amba_preanalysis_context_t *ctx);

/**
 * \fn int amba_preanalysis_set_debug_level(amba_preanalysis_context_t *ctx, int dbg_level)
 * \brief set the debug level of an instance of the library
 *
 * \param ctx       the handle to the instance
 * \param dbg_level the debug level
 * \return return code
 */
AMBA_API int amba_preanalysis_set_debug_level(amba_preanalysis_context_t *ctx, int dbg_level);

/**
 * \fn int amba_preanalysis_get_status(amba_preanalysis_context_t *ctx, amba_preanalysis_engine_t di, amba_preanalysis_status_t *status)
 * \brief get the current status of an instance of the library
 *
 * \param ctx       the handle to the instance
 * \param di        the DSP index
 * \param status    the received output configuration
 * \return return code
 */
AMBA_API int amba_preanalysis_get_status(amba_preanalysis_context_t *ctx, amba_preanalysis_engine_t di, amba_preanalysis_status_t *status);

/**
 * \fn amba_preanalysis_context_t *amba_preanalysis_open(amba_preanalysis_configuration_t *cfg)
 * \brief initialize the library and create one instance, returns a handle to the caller
 *
 * \param cfg       the input configuration
 * \return the context handle, NULL if failure
 */
AMBA_API amba_preanalysis_context_t *amba_preanalysis_open(amba_preanalysis_configuration_t *cfg);

/**
 * \fn int amba_preanalysis_register_buf_callback(amba_preanalysis_context_t *ctx, amba_preanalysis_buf_func_t *cb)
 * \brief register buffer management callback functions provided by the caller
 *
 * this function must be called prior to processing pictures, otherwise, the error is returned upon calling
 * amba_preanalysis_process_picture(). when this function is called, the library will allocate private and output
 * buffer pools using the callback functions.
 *
 * \param ctx       the handle to the instance
 * \param cb        the buffer management callback functions
 * \return return code
 */
AMBA_API int amba_preanalysis_register_buf_callback(amba_preanalysis_context_t *ctx, amba_preanalysis_buf_func_t *cb);

/**
 * \fn int amba_preanalysis_process_idsp(amba_preanalysis_context_t *ctx, amba_preanalysis_input_t *in, amba_preanalysis_idsp_output_t *out)
 * \brief accept input buffer, and produce output buffers
 *
 * the caller cannot assume the input buffer is unused when the function returns. the function is responsible for releasing the
 * input buffer. the caller does not need to allocate output buffers, the function is responsible for allocating the output
 * buffers. the caller is responsible for releasing the output buffers.
 *
 * \param ctx       the handle to the instance
 * \param in        the input
 * \param out       the output
 * \return return code
 */
AMBA_API int amba_preanalysis_process_idsp(amba_preanalysis_context_t *ctx, amba_preanalysis_input_t *in, amba_preanalysis_idsp_output_t *out);

/**
 * \fn int amba_preanalysis_process_vdsp(amba_preanalysis_context_t *ctx, amba_preanalysis_input_t *in, amba_preanalysis_vdsp_output_t *out)
 * \brief accept input buffer, and produce output buffers
 *
 * the caller cannot assume the input buffer is unused when the function returns. the function is responsible for releasing the
 * input buffer. the caller does not need to allocate output buffers, the function is responsible for allocating the output
 * buffers. the caller is responsible for releasing the output buffers.
 *
 * \param ctx       the handle to the instance
 * \param in        the input
 * \param out       the output
 * \return return code
 */
AMBA_API int amba_preanalysis_process_vdsp(amba_preanalysis_context_t *ctx, amba_preanalysis_input_t *in, amba_preanalysis_vdsp_output_t *out);

/**
 * \fn int amba_preanalysis_close(amba_preanalysis_context_t *ctx)
 * \brief clean up internal resources and close the instance.
 *
 * \param ctx       the handle to the instance
 * \return return code
 */
AMBA_API int amba_preanalysis_close(amba_preanalysis_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _PREANALYSIS_H_ */
