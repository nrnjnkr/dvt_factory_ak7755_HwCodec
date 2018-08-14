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

#include <stdio.h>
#include <unistd.h>
#include <basetypes.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <math.h>
#include <fcntl.h>
#include <ctype.h>
#include <iav_encode_drv.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "defog_lib.h"
#include "enc_defog.h"

#define MAX_YUV_FRAME_NUM (120)
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

struct option long_options[] = {
	{"hps",			1,	0,	'p'},
	{"t0",			1,	0,	't'},
	{"debug",		0,	0,	'd'},
	{"efm-main-yuv-file",	1,	0,	'f'},
	{"efm-main-fsize",	1,	0,	'z'},
	{"efm-second-yuv-file",	1,	0,	'F'},
	{"efm-second-fsize",	1,	0,	'Z'},
	{"efm-total-fnum",	1,	0,	'n'},
	{0,			0,	0,	 0 },
};
struct hint_s {
	const char *arg;
	const char *str;
};
static const struct hint_s hint[] = {
		{"1~7, default value: 1", "\t\t\tThe parameter of HPS"},
		{"10~50, default value: 25", "\t\t\tThe parameter of t0"},
		{"", "\t\t\t\t\t\tThe flag for debug info printing"},
		{"filename(main EFM buffer)", "\tSpecify YUV input file of main EFM buffer for EFM feature."},
		{"W x H(main EFM buffer)", "\t\tSpecify frame size of main EFM buffer for EFM feature"},
		{"filename(second EFM buffer)", "\tSpecify YUV input file of second EFM buffer for EFM feature."},
		{"W x H (second EFM buffer)", "\tSpecify frame size of second EFM buffer for EFM feature"},
		{"1~120", "\t\t\t\tSpecify the total frame number from YUV file"},
};
const char *short_options = "p:t:f:n:z:F:N:Z:d";


efm_defog_param efm = {
		.frame_factor = 1,
};

static int	g_hps = DEFOG_DEFAULT_HPS;
static int	g_t0 = DEFOG_DEFAULT_T0;
static int	g_debug = 0;
int fd_iav;
defog_buf_addr_t	addr;
defog_config_t		g_cfg;

static u8 * main_yuv_addr = NULL;
static u8 * main_yuv_end = NULL;
static u8 * main_frame_start = NULL;

static u8 * second_yuv_addr = NULL;
static u8 * second_yuv_end = NULL;
static u8 * second_frame_start = NULL;

static u8 * me1_addr = NULL;

u32 main_frame_size, second_frame_size, main_total_size, second_total_size, me1_total_size;

void usage(void)
{
	int i;
	printf("test_feed_yuv usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
	return ;
}

int config_efm_params(void)
{
	u32 	fps, fps_q9;
	iav_source_buffer_setup_ex_t	buffer_setup;
	iav_reso_ex_t *yuv_buf_size = NULL;

	memset(&buffer_setup, 0, sizeof(buffer_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}

	if (buffer_setup.type[IAV_ENCODE_SOURCE_DRAM_FIRST] !=
		IAV_SOURCE_BUFFER_TYPE_ENCODE) {
		printf("yuv buf[%d] error, the buffer is not encode type\n",
			IAV_ENCODE_SOURCE_DRAM_FIRST);
		return -1;
	}

	yuv_buf_size = &buffer_setup.size[IAV_ENCODE_SOURCE_DRAM_FIRST];
	if ((yuv_buf_size->width < efm.main_width) || (yuv_buf_size->height < efm.main_height)) {
		printf("error:YUV buf[%d] size is smaller than the setting,"
			 "YUV buf:width[%d], height[%d], setting:width[%d], height[%d]\n",
			IAV_ENCODE_SOURCE_DRAM_FIRST,
			yuv_buf_size->width, yuv_buf_size->height, efm.main_width, efm.main_height);
		return -1;
	}

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

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

int init_param(int argc, char **argv)
{
	int		c, i;
	int width, height;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'p':
			g_hps = atoi(optarg);
			break;

		case 't':
			g_t0 = atoi(optarg);
			break;

		case 'd':
			g_debug = 1;
			break;

		case 'f':
			if (strlen(optarg) >= FILENAME_LENGTH) {
				printf("File name of main EFM buffer [%s] is too long. It cannot be longer than "
						"%d characters.\n", optarg, (FILENAME_LENGTH - 1));
				return -1;
			}
			strcpy(efm.main_yuv, optarg);
			printf("YUV input file of main EFM buffer [%s].\n", efm.main_yuv);
			efm.main_file_flag = 1;
			break;

		case 'n':
			i = atoi(optarg);
			if (i < 1 || i > MAX_YUV_FRAME_NUM) {
				printf("Total frame number of main EFM buffer [%d] must be in the range of [1~120].\n", i);
				return -1;
			}
			efm.frame_num = i;
			efm.frame_num_flag = 1;
			break;

		case 'z':
			if (get_arbitrary_resolution(optarg, &width, &height) < 0) {
				printf("Failed to get resolution of main EFM buffer from [%s].\n", optarg);
				return -1;
			}
			efm.main_width = width;
			efm.main_height = height;
			efm.main_size_flag = 1;
			break;

		case 'F':
			if (strlen(optarg) >= FILENAME_LENGTH) {
				printf("File name of second EFM buffer [%s] is too long. It cannot be longer than "
						"%d characters.\n", optarg, (FILENAME_LENGTH - 1));
				return -1;
			}
			strcpy(efm.second_yuv, optarg);
			printf("YUV input file of second EFM buffer[%s].\n", efm.second_yuv);
			efm.second_file_flag = 1;
			break;

		case 'Z':
			if (get_arbitrary_resolution(optarg, &width, &height) < 0) {
				printf("Failed to get resolution of second EFM buffer from [%s].\n", optarg);
				return -1;
			}
			efm.second_width = width;
			efm.second_height = height;
			efm.second_size_flag = 1;
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

	if (mem_mapped) {
		return 0;
	}
	/* Map for EFM feature */
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}
	mem_mapped = 1;
	return 0;
}
static int prepare_main_efm_mem(u32 total_size)
{
	main_yuv_addr = (u8*)malloc(total_size);
	if (!main_yuv_addr) {
		printf("Failed to allocate main EFM buffers.\n");
		return -1;
	}
	return 0;
}
static int prepare_me1_efm_mem(u32 total_size)
{
	me1_addr = (u8*)malloc(total_size);
	if (!me1_addr) {
		printf("Failed to allocate me1 EFM buffers.\n");
		return -1;
	}
	return 0;
}
static int prepare_second_efm_mem(u32 total_size)
{
	second_yuv_addr = (u8*)malloc(total_size);
	if (!second_yuv_addr) {
		printf("Failed to allocate second EFM buffers.\n");
		return -1;
	}
	return 0;
}

static int prepare_main_efm_files(u32 total_size)
{
	static int init_efm_file = 0;
	int yuv_fd = -1;
	char file_name[256] = {0};

	if (init_efm_file == 0) {
		// read YV12 planar format
		sprintf(file_name, "%s", efm.main_yuv);

		if ((yuv_fd = open(file_name, O_RDONLY)) < 0) {
			printf("Failed to open YUV file [%s].\n", file_name);
			goto ERROR_EXIT;
		}
		read(yuv_fd, main_yuv_addr, total_size);
		if (yuv_fd >= 0) {
			close(yuv_fd);
			yuv_fd = -1;
		}
		main_frame_start = main_yuv_addr;
		main_yuv_end = main_yuv_addr + total_size;
		init_efm_file = 1;
	}
	return 0;

ERROR_EXIT:
	if (main_yuv_addr) {
		free(main_yuv_addr);
		main_yuv_addr = NULL;
	}
	if (yuv_fd >= 0) {
		close(yuv_fd);
		yuv_fd = -1;
	}
	return -1;
}

static int prepare_second_efm_files(u32 total_size)
{
	static int init_efm_file = 0;
	int yuv_fd = -1;
	char file_name[256] = {0};

	if (init_efm_file == 0) {
		// read YV12 planar format
		sprintf(file_name, "%s", efm.second_yuv);

		if ((yuv_fd = open(file_name, O_RDONLY)) < 0) {
			printf("Failed to open YUV file [%s].\n", file_name);
			goto ERROR_EXIT;
		}
		read(yuv_fd, second_yuv_addr, total_size);
		if (yuv_fd >= 0) {
			close(yuv_fd);
			yuv_fd = -1;
		}
		second_frame_start = second_yuv_addr;
		second_yuv_end = second_yuv_addr + total_size;
		init_efm_file = 1;
	}
	return 0;

ERROR_EXIT:
	if (second_yuv_addr) {
		free(second_yuv_addr);
		second_yuv_addr = NULL;
	}
	if (yuv_fd >= 0) {
		close(yuv_fd);
		yuv_fd = -1;
	}
	return -1;
}
int get_yuv_data(void)
{
	#define ROW_MAX	(4)
	#define COL_MAX	(4)
	u16 yuv_pitch;
	u16 i, j, row, col;
	u32 me1_data;
	u8 * src = NULL, * dst = NULL;

	addr.main_y_in		= main_frame_start;
	addr.main_uv_in		= main_frame_start + main_frame_size;
	addr.second_y		= second_frame_start;
	addr.second_uv		= second_frame_start + second_frame_size;
	addr.me1_in         = me1_addr;


	// Feed YUV data
	yuv_pitch = g_cfg.bs.main_width;
	for (i = 0; i < (efm.main_height / ROW_MAX); ++i) {

		// Read ME1 data
		src = main_frame_start + i * ROW_MAX * efm.main_width;
		dst = addr.me1_in + i * yuv_pitch;
		for (col = 0; col < (efm.main_width / COL_MAX); ++col) {
			for (row = 0, j = col * COL_MAX, me1_data = 0;
					row < efm.main_width * ROW_MAX; row += efm.main_width) {
				me1_data += (src[row + j] + src[row + j + 1] +
					src[row + j + 2] + src[row + j + 3]);
			}
			dst[col] = me1_data >> 4;
		}
	}

	// Update frame start address
	if (main_frame_start + main_frame_size*3/2 < main_yuv_end) {
		main_frame_start += main_frame_size*3/2;
		second_frame_start += second_frame_size*3/2;
	} else {
		main_frame_start = main_yuv_addr;
		second_frame_start = second_yuv_addr;
	}

	return 0;
}
int main(int argc, char **argv)
{
	int			ret;
	defog_buf_size_t	*bs;
	int vin_tick = -1;
	char vin_array[8];
	char *vsync_proc = "/proc/ambarella/vin0_vsync";

	if (argc < 2) {
		usage();
		return -1;
	}

	vin_tick = open(vsync_proc, O_RDONLY);
	if (vin_tick < 0) {
		printf("Cannot open [%s].\n", vsync_proc);
		return -1 ;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	ret = init_param(argc, argv);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	memset(&g_cfg, 0, sizeof(g_cfg));
	g_cfg.hps			= g_hps;
	g_cfg.T0			= g_t0;
	g_cfg.debug		= g_debug;
	bs			= &g_cfg.bs;
	bs->main_width		= efm.main_width;
	bs->main_height		= efm.main_height;
	bs->main_pitch_in	= efm.main_width;
	bs->main_pitch_out	= efm.main_width;
	bs->second_width	= efm.second_width;
	bs->second_height	= efm.second_height;
	bs->second_pitch	= efm.second_width;
	bs->me1_width = bs->main_width/4;
	bs->me1_height = bs->main_height/4;
	bs->me1_pitch_in = bs->main_width/4;
	bs->me1_pitch_out = bs->main_width/4;

	ret = defog_start(&g_cfg);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	main_frame_size = efm.main_width * efm.main_height;
	main_total_size = main_frame_size * 3 / 2 * efm.frame_num;
	second_frame_size = efm.second_width * efm.second_height;
	second_total_size = second_frame_size * 3 / 2 * efm.frame_num;
	me1_total_size = main_frame_size /4;

    if (prepare_main_efm_mem(main_total_size) < 0) {
		printf("prepare_main_efm_mem error!\n");
		return -1;
	}
	 if (prepare_me1_efm_mem(me1_total_size) < 0) {
		printf("prepare_me1_efm_mem error!\n");
		return -1;
	}
    if (prepare_second_efm_mem(second_total_size) < 0) {
		printf("prepare_second_efm_mem error!\n");
		return -1;
	}
	if (prepare_main_efm_files(main_total_size) < 0) {
		printf("prepare_second_efm_mem error!\n");
		return -1;
	}
	if (prepare_second_efm_files(second_total_size) < 0) {
		printf("prepare_second_efm_mem error!\n");
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

	ret = defog_stop(&g_cfg);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	free(main_yuv_addr);
	free(second_yuv_addr);
	free(me1_addr);

	return 0;
}
