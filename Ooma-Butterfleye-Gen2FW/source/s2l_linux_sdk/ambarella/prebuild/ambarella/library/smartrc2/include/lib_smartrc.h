/*******************************************************************************
 * lib_smartrc.h
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
#ifndef _LIB_SMARTRC_H_
#define _LIB_SMARTRC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetypes.h"
#include "lib_smartrc_common.h"

#ifndef AMBA_API
#define AMBA_API __attribute__((visibility("default")))
#endif

AMBA_API int smartrc_init_lib(u32 qp_matrix_single_size);
AMBA_API int smartrc_deinit_lib(void);

AMBA_API int smartrc_cfg_param(smartrc_param_t *smartrc_params);

AMBA_API int smartrc_thread_start(void);
AMBA_API int smartrc_thread_stop(void);

AMBA_API int smartrc_cfg_debug_param(smartrc_debug_info_t *debug_info);
AMBA_API int smartrc_get_debug_map(void);
AMBA_API int smartrc_set_log_level(smartrc_log_level_t log_level);
AMBA_API int smartrc_get_log_level(void);
AMBA_API int smartrc_get_version(version_t *version);
AMBA_API int smartrc_show_cfg_param(void);

#ifdef __cplusplus
}
#endif

#endif /* _LIB_SMARTRC_H_ */
