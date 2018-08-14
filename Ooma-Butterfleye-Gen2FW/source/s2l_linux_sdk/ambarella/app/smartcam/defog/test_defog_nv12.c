/*
 *
 * History:
 *    2015/03/26 - [Zhenwu Xue] Create
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

#include "defog_lib.h"

struct option long_options[] = {
	{"main_width",		1,	0,	'W'},
	{"main_height",		1,	0,	'H'},
	{"second_width",	1,	0,	'w'},
	{"second_height",	1,	0,	'h'},
	{"main",		1,	0,	'm'},
	{"second",		1,	0,	's'},
	{"out",			1,	0,	'o'},
	{"hps",			1,	0,	'p'},
	{"t0",			1,	0,	't'},
	{"debug",		0,	0,	'd'},
	{0,			0,	0,	 0 },
};

const char *short_options = "W:H:w:h:m:s:o:p:t:d";

static int	g_main_width = 1920;
static int	g_main_height = 1080;
static int	g_second_width = 240;
static int	g_second_height = 136;
static char	g_main_nv12[64] = "/sdcard/1080p.nv12";
static char	g_second_nv12[64] = "/sdcard/240_136.nv12";
static char	g_out_nv12[64] = "/sdcard/out.nv12";
static int	g_hps = DEFOG_DEFAULT_HPS;
static int	g_t0 = DEFOG_DEFAULT_T0;
static int	g_debug = 0;

int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'W':
			g_main_width = atoi(optarg);
			break;

		case 'H':
			g_main_height = atoi(optarg);
			break;

		case 'w':
			g_second_width = atoi(optarg);
			break;

		case 'h':
			g_second_height = atoi(optarg);
			break;

		case 'm':
			strcpy(g_main_nv12, optarg);
			break;

		case 's':
			strcpy(g_second_nv12, optarg);
			break;

		case 'o':
			strcpy(g_out_nv12, optarg);
			break;

		case 'p':
			g_hps = atoi(optarg);
			break;

		case 't':
			g_t0 = atoi(optarg);
			break;

		case 'd':
			g_debug = 1;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int			ret;
	FILE			*MAIN, *SECOND, *OUT;
	struct stat		finfo;
	unsigned char		*main_nv12, *second_nv12, *out_nv12;
	defog_config_t		cfg;
	defog_buf_addr_t	addr;
	defog_buf_size_t	*bs;

	ret = init_param(argc, argv);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	stat(g_main_nv12, &finfo);
	if (finfo.st_size != g_main_width * g_main_height * 3 / 2) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	stat(g_second_nv12, &finfo);
	if (finfo.st_size != g_second_width * g_second_height * 3 / 2) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.hps			= g_hps;
	cfg.T0			= g_t0;
	cfg.debug		= g_debug;
	bs			= &cfg.bs;
	bs->main_width		= g_main_width;
	bs->main_height		= g_main_height;
	bs->main_pitch_in	= g_main_width;
	bs->main_pitch_out	= g_main_width;
	bs->second_width	= g_second_width;
	bs->second_height	= g_second_height;
	bs->second_pitch	= g_second_width;
	ret = defog_start(&cfg);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	main_nv12 = malloc(g_main_width * g_main_height * 3 + g_second_width * g_second_height * 3 / 2);
	if (!main_nv12) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	second_nv12 = main_nv12 + g_main_width * g_main_height * 3 / 2;
	out_nv12 = second_nv12 + g_second_width * g_second_height * 3 / 2;

	MAIN = fopen(g_main_nv12, "rb");
	if (!MAIN) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	ret = fread(main_nv12, 1, g_main_width * g_main_height * 3 / 2, MAIN);
	if (ret != g_main_width * g_main_height * 3 / 2) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	fclose(MAIN);

	SECOND = fopen(g_second_nv12, "rb");
	if (!SECOND) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	ret = fread(second_nv12, 1, g_second_width * g_second_height * 3 / 2, SECOND);
	if (ret != g_second_width * g_second_height * 3 / 2) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	fclose(SECOND);

	addr.main_y_in		= main_nv12;
	addr.main_uv_in		= main_nv12 + g_main_width * g_main_height;
	addr.second_y		= second_nv12;
	addr.second_uv		= second_nv12 + g_second_width * g_second_height;
	addr.main_y_out		= out_nv12;
	addr.main_uv_out	= out_nv12 + g_main_width * g_main_height;
	ret = defog_frame(&cfg, &addr);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	OUT = fopen(g_out_nv12, "wb");
	if (!OUT) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	ret = fwrite(out_nv12, 1, g_main_width * g_main_height * 3 / 2, OUT);
	if (ret != g_main_width * g_main_height * 3 / 2) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}
	fclose(OUT);

	ret = defog_stop(&cfg);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	free(main_nv12);
	return 0;
}
