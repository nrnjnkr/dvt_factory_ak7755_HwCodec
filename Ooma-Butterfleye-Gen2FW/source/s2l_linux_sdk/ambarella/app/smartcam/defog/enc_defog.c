/*
 *
 * History:
 *    2015/03/26 - [Qi ZHU] Create
 *
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
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <signal.h>
#include <basetypes.h>
#include <pthread.h>
#include "defog_lib.h"
#include "enc_defog.h"

#if defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3)
#include "iav_ioctl.h"
#include "iav_ucode_ioctl.h"
#else
#include "iav_drv.h"
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif


//verbose
static int verbose_mode = 0;
// the device file handle
extern int fd_iav;
extern defog_config_t g_cfg;
extern defog_buf_addr_t	addr;
extern efm_defog_param efm;
int get_yuv_data(void);

int feed_data_defog(void)
{
	static u32 pts = 0;
	iav_enc_dram_buf_frame_ex_t buf_frame;
	iav_enc_dram_buf_update_ex_t buf_update;
	struct timeval curr_time, curr_time_end;

	int ret=-1;

	if (g_cfg.debug == 1) {
		verbose_mode=1;
	}
	memset(&buf_frame, 0, sizeof(buf_frame));
	buf_frame.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;

	if (verbose_mode) {
		gettimeofday(&curr_time, NULL);
	}
	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX, &buf_frame);

	if (verbose_mode) {
		gettimeofday(&curr_time_end, NULL);
		printf("Defog_debug:------time diff:%ld ms\n", (curr_time_end.tv_usec - curr_time.tv_usec)/1000
			+ (curr_time_end.tv_sec - curr_time.tv_sec)* 1000);
		curr_time = curr_time_end;
	}

	if (get_yuv_data() < 0) {
		printf("DEFOG ERROR: Unable to read CAP_buffer!\n");
	    return -1;
	}

	addr.main_y_out	= buf_frame.y_addr;
	addr.main_uv_out = buf_frame.uv_addr;
	addr.me1_out = buf_frame.me1_addr;

	ret = defog_frame(&g_cfg, &addr);
	if (ret < 0) {
		printf("ERROR: defog_frame %d!\n", __LINE__);
		return -1;
	}

	if (verbose_mode) {
		gettimeofday(&curr_time_end, NULL);
		printf("Defog_debug:++++++time diff:%ld ms\n\n",  (curr_time_end.tv_usec - curr_time.tv_usec)/1000
			+ (curr_time_end.tv_sec - curr_time.tv_sec)* 1000);
	}

	// Update ME1 & YUV data
	memset(&buf_update, 0, sizeof(buf_update));
	buf_update.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
	buf_update.frame_id = buf_frame.frame_id;
	buf_update.frame_pts = pts;
	buf_update.size.width = efm.main_width;
	buf_update.size.height = efm.main_height;

	pts += efm.yuv_pts_distance;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX, &buf_update);

	return 0;
}
