/*
 *
 * History:
 *    2015/03/26 - [QI ZHU] Create
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

#include <stdio.h>
#include <unistd.h>
#include <basetypes.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <math.h>
#include <fcntl.h>
#if defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3)
#include "iav_ioctl.h"
#else
#include "iav_drv.h"
#endif

#include "ambas_common.h"
#include "iav.h"
#include "defog_lib.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "enc_defog.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

efm_defog_param efm = {
		.frame_factor = 1,
};
struct option long_options[] = {
	{"hps",			1,	0,	'p'},
	{"t0",			1,	0,	't'},
	{"debug",		0,	0,	'd'},
	{"fb",			0,	0,	'f'},
	{0,			0,	0,	 0 },
};

const char *short_options = "W:H:w:h:m:s:o:p:t:df";
defog_config_t g_cfg;
defog_buf_addr_t	addr;
int	fd_iav;
u8 *bsb_mem;
u32 bsb_size;
static struct iav_buf_cap_s cap;

static int config_efm_params(void)
{
	u32 	fps, fps_q9;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &fps_q9) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}

	fps = DIV_ROUND(512000000, fps_q9);
	if (fps < 1) {
		printf("error:vin fps[%d] < 1\n ", fps);
		return -1;
	}

	if (efm.frame_factor > fps || (efm.frame_factor < 1)) {
		printf("error:the factor must be in 1~%d\n ", fps);
		return -1;
	}

	// both 90000 and 512000000 divide 10000.
	efm.yuv_pts_distance = (u64)fps_q9 * 9 / 51200 * efm.frame_factor;

	return 0;
}
int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'p':
			g_cfg.hps = atoi(optarg);
			break;

		case 't':
			g_cfg.T0 = atoi(optarg);
			break;

		case 'd':
			g_cfg.debug = 1;
			break;

		case 'f':
			g_cfg.fb = 1;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			return -1;
		}
	}
	if (config_efm_params() < 0) {
		printf("config_efm_params error\n");
		return -1;
	}
	return 0;
}

int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB2, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}
	bsb_mem = info.addr;
	bsb_size = info.length;

	/* Map for EFM feature */
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	mem_mapped = 1;


	return 0;
}

int get_yuv_data(void)
{
	int ret = -1;
	cap.flag = 1;
	ret = ioctl(fd_iav, IAV_IOC_READ_BUF_CAP_EX, &cap);
	if (ret < 0) {
		printf("DEFOG ERROR: Unable to read CAP_buffer!\n");
		return ret;
	}
	addr.main_y_in = cap.yuv[IAV_ENCODE_SOURCE_MAIN_BUFFER].y_addr;
	addr.main_uv_in = cap.yuv[IAV_ENCODE_SOURCE_MAIN_BUFFER].uv_addr;
	g_cfg.bs.main_width = cap.yuv[IAV_ENCODE_SOURCE_MAIN_BUFFER].width;
	g_cfg.bs.main_height = cap.yuv[IAV_ENCODE_SOURCE_MAIN_BUFFER].height;
	g_cfg.bs.main_pitch_in = cap.yuv[IAV_ENCODE_SOURCE_MAIN_BUFFER].pitch;
	g_cfg.bs.main_pitch_out = g_cfg.bs.main_width;
	efm.main_width = g_cfg.bs.main_width;
	efm.main_height= g_cfg.bs.main_height;

	addr.me1_in = cap.me1[IAV_ENCODE_SOURCE_MAIN_BUFFER].addr;
	g_cfg.bs.me1_width = cap.me1[IAV_ENCODE_SOURCE_MAIN_BUFFER].width;
	g_cfg.bs.me1_height = cap.me1[IAV_ENCODE_SOURCE_MAIN_BUFFER].height;
	g_cfg.bs.me1_pitch_in = cap.me1[IAV_ENCODE_SOURCE_MAIN_BUFFER].pitch;
	g_cfg.bs.me1_pitch_out = g_cfg.bs.me1_width;

	addr.second_y = cap.yuv[IAV_ENCODE_SOURCE_SECOND_BUFFER].y_addr;
	addr.second_uv = cap.yuv[IAV_ENCODE_SOURCE_SECOND_BUFFER].uv_addr;
	g_cfg.bs.second_width = cap.yuv[IAV_ENCODE_SOURCE_SECOND_BUFFER].width;
	g_cfg.bs.second_height = cap.yuv[IAV_ENCODE_SOURCE_SECOND_BUFFER].height;
	g_cfg.bs.second_pitch = cap.yuv[IAV_ENCODE_SOURCE_SECOND_BUFFER].pitch;

	return ret;
}
int main(int argc, char **argv)
{
	int			ret;
	u8			*p;
	int vin_tick = -1;
	char vin_array[8];
	char *vsync_proc = "/proc/ambarella/vin0_vsync";
	iav_mmap_info_t		minfo;


	vin_tick = open(vsync_proc, O_RDONLY);
	if (vin_tick < 0) {
		printf("Cannot open [%s].\n", vsync_proc);
		return -1 ;
	}

	fd_iav = open("/dev/iav", O_RDWR);
	if (!fd_iav) {
		printf("DEFOG ERROR: Unable to open /dev/fd_iav!\n");
		return -1;
	}
	ioctl(fd_iav, IAV_IOC_MAP_DSP, &minfo);



	memset(&g_cfg, 0, sizeof(g_cfg));
	ret = init_param(argc, argv);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	if (get_yuv_data() < 0) {
		printf("DEFOG ERROR: Unable to read CAP_buffer!\n");
		return -1;
	}

	ret = defog_start(&g_cfg);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	while (1) {
		read(vin_tick, vin_array, 8);
		feed_data_defog();
	}

	if (vin_tick >= 0) {
		close(vin_tick);
		vin_tick = -1;
	}
	defog_stop(&g_cfg);
	free(p);

	return 0;
}
