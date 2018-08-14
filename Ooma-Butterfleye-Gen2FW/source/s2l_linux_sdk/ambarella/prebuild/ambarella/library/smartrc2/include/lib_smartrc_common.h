/*******************************************************************************
 * lib_smartrc_common.h
 *
 * History:
 *   2016/09/08 - [Hao Qian] created file
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

#ifndef _LIB_SMARTRC_COMMON_H_
#define _LIB_SMARTRC_COMMON_H_

#include <basetypes.h>
#include <config.h>
#include <iav_ioctl.h>

#define SMARTRC_MAX_STREAM_NUM		(IAV_STREAM_MAX_NUM_ALL)
#define SMARTRC_ENCODE_WIDTH_MIN	(320)
#define SMARTRC_ENCODE_WIDTH_MAX	(3840)
#define SMARTRC_ENCODE_HEIGHT_MIN	(240)
#define SMARTRC_ENCODE_HEIGHT_MAX	(2160)
#define SMARTRC_ENCODE_DUMMY_MAX	(5)
#define SMARTRC_ENCODE_DUMMY_MIN	(1)
#define SMARTRC_MAX_FILE_PATH_LENGTH	(128)
#define SMARTRC_MAX_FILE_NAME_LENGTH	(128)
#define SMARTRC_MAX_GOP_Q		(63)
#define SMARTRC_MAX_UC_COUNT_THRESHOLD	(10)
#define SMARTRC_MAX_DIRECT_BIAS		(65535)
#define SMARTRC_MAX_INTRA_BIAS		(65535)
#define SMARTRC_MAX_ZMV_THRESHOLD	(255)

/* FixMe: To be supported in frame level control */
typedef enum profile_e {
	PROFILE_STATIC = 0,
	PROFILE_NUM,
	PROFILE_FIRST = PROFILE_STATIC,
	PROFILE_LAST = PROFILE_NUM,
} profile_t;

typedef enum smartrc_cfg_flag_e {
	SPECIFY_REF_MAP_FLAG = (1 << 0),
	SPECIFY_LTR_OPTION_FLAG = (1 << 1),
	SPECIFY_UA_CNT_THRESHOLD_FLAG = (1 << 2),
	SPECIFY_DUMMY_LATENCY_FLAG = (1 << 3),
	SPECIFY_ZMV_THRESHOLD_FLAG = (1 << 4),
	SPECIFY_INTRA_BIAS_FLAG = (1 << 5),
	SPECIFY_DIRECT_BIAS_FLAG = (1 << 6),
	SPECIFY_FORCE_ZMV_FLAG = (1 << 7),
	SPECIFY_WP_ENABLE_FLAG = (1 << 8),
} smartrc_cfg_flag_t;

typedef enum smartrc_ref_map_e {
	SPECIFY_REF_MAP_NONE = 0,
	SPECIFY_UC_REF,
	SPECIFY_MOTION_REF,
	SPECIFY_STATIC_REF,
	SPECIFY_UC_STATIC_REF,
	SPECIFY_UC_MOTION_REF,
	SPECIFY_MOTION_STATIC_REF, /* This one is NOT supported in current version. */
	SPECIFY_ALL_REF_MAP,
	SPECIFY_REF_MAP_MAX_NUM,
	SPECIFY_REF_MAP_FIRST = SPECIFY_REF_MAP_NONE,
	SPECIFY_REF_MAP_LAST = SPECIFY_REF_MAP_MAX_NUM,
} smartrc_ref_map_t;

typedef enum smartrc_debug_flag_e {
	SPECIFY_PERFORMANCE_FLAG = (1 << 0),
	SPECIFY_ME_FLAG = (1 << 1),
	SPECIFY_UC_FLAG = (1 << 2),
	SPECIFY_MOTION_FLAG = (1 << 3),
	SPECIFY_STATIC_FLAG = (1 << 4),
	SPECIFY_MARK_FLAG = (1 << 5),
	SPECIFY_FILE_PATH_FLAG = (1 << 6),
} smartrc_debug_flag_t;

typedef struct smartrc_debug_info_s {
	u32 debug_flag_map;
	char file_path[SMARTRC_MAX_FILE_PATH_LENGTH];
} smartrc_debug_info_t;

typedef enum smartrc_log_level_e
{
	LOG_ERR = 0,
	LOG_MSG = 1,
	LOG_INFO = 2,
	LOG_DBG = 3,
	LOG_NUM,
	LOG_LEVEL_FIRST = LOG_ERR,
	LOG_LEVEL_LAST = LOG_NUM,
} smartrc_log_level_t;

typedef struct version_s {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

typedef struct smartrc_h264_param_s {
	u8 zmv_threshold;
	u8 force_zmv_enable;
	u8 reserved1[2];
	u16 user1_intra_bias;
	u16 user1_direct_bias;
	u32 reserved2[2];
} smartrc_h264_param_t;

typedef struct smartrc_h265_param_s {
	u32 reserved[4];
} smartrc_h265_param_t;

typedef struct smartrc_stream_param_s {
	u32 cfg_flag_map;
	u8 ref_map;
	u8 ltr_option;				// 0: use the closest LTR against current picture for processing
								// 1: use the LTR before the closest LTR against current picture for processing
								// set to 1 if Q pictures are used as LTRs, otherwise set to 0
	u8 uc_counter_threshold;	// update uncovered area after motion is detected and then being static for counter_threshold frames
								// larger value leads to less uncovered area
	u8 dummy_latency;			// encoder dummy latency in number of frames set by the caller
	smartrc_h264_param_t h264;
	smartrc_h265_param_t h265;
} smartrc_stream_param_t;

typedef struct smartrc_param_s {
	u32 stream_map;
	smartrc_stream_param_t stream[SMARTRC_MAX_STREAM_NUM];
} smartrc_param_t;

#endif /* _LIB_SMARTRC_COMMON_H_ */
