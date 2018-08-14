/*******************************************************************************
 *  enc_defog.h
 *
 * History:
 *    2015/03/26 - [Qi ZHU] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
#ifndef __ENC_DEFOG_H__
#define __ENC_DEFOG_H__

#include "basetypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILENAME_LENGTH (32)
typedef struct efm_defog_param {
	int main_width;
	int main_height;
	int main_size_flag;

	int second_width;
	int second_height;
	int second_size_flag;

	char main_yuv[FILENAME_LENGTH];
	int main_file_flag;
	char second_yuv[FILENAME_LENGTH];
	int second_file_flag;

	int frame_num;
	int frame_num_flag;

	int frame_factor;
	u32 yuv_pts_distance;

} efm_defog_param;

int feed_data_defog(void);
int get_yuv_data(void);
#endif
