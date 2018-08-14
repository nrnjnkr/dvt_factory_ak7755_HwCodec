 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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
#include <getopt.h>
#include <sys/time.h>

#include "mhdet_lib.h"

#include <opencv2/opencv.hpp>
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;

#define	MEASURE_TIME_VARIABLES()	\
	struct timeval			tv1, tv2;	\
	int				ms

#define MEASURE_TIME_START() gettimeofday(&tv1, NULL)
#define MEASURE_TIME_END(s)	\
	gettimeofday(&tv2, NULL);	\
	ms = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;	\
	printf("%16s %10d ms\n", s, ms)

#define MHDET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __FILE__, __LINE__);	\
		return ret;	\
	}

#include "../utils2/fb.cpp"
#include "../utils2/iav.cpp"
#include "../utils2/csc.cpp"

static int			a_vw = 1280;
static int			a_vh = 720;
static float			a_scale = 1.1;
static int			a_step = 4;
static int			a_neighbors = 2;
static int			a_mdet_only = 0;
static int			a_heads_only = 0;
static int			a_all_first = 0;
static int			a_wait = 1000000;
static int			a_debug = 0;
static char			a_log[128] = "/sdcard/mhdet/log";
static int			a_frames = 1000000;
static int			a_histogram = 0;
static int			a_gui = 0;

static void			*v_mhdet;
static mhdet_rect_t		*v_rects;
static mhdet_config_t		v_cfg;
static mhdet_input_t		v_in;
static mhdet_output_t		*v_out;
static second_buf_t		v_buf;
static int			v_log = 0;

struct option long_options[] = {
	{"video",	1,	0,	'v'},
	{"scale",	1,	0,	's'},
	{"step",	1,	0,	't'},
	{"neighbors",	1,	0,	'n'},
	{"mdet_only",	0,	0,	'm'},
	{"heads_only",	0,	0,	'h'},
	{"all_first",	0,	0,	'a'},
	{"debug",	0,	0,	'd'},
	{"wait",	1,	0,	'w'},
	{"log",		1,	0,	'l'},
	{"frames",	1,	0,	'f'},
	{"histogram",	1,	0,	'H'},
	{"gui",		0,	0,	'g'},
	{0,		0,	0,	 0 },
};

const char *short_options = "v:s:t:n:mhadw:l:f:H:g";

int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'v':
			sscanf(optarg, "%dx%d", &a_vw, &a_vh);
			break;

		case 's':
			a_scale = atof(optarg);
			break;

		case 't':
			a_step = atoi(optarg);
			break;

		case 'n':
			a_neighbors = atoi(optarg);
			break;

		case 'm':
			a_mdet_only = 1;
			break;

		case 'h':
			a_heads_only = 1;
			break;

		case 'a':
			a_all_first = 1;
			break;

		case 'd':
			a_debug = 1;
			break;

		case 'w':
			a_wait = atoi(optarg);
			break;

		case 'l':
			strcpy(a_log, optarg);
			v_log = 1;
			break;

		case 'f':
			a_frames = atoi(optarg);
			break;

		case 'H':
			a_histogram = atoi(optarg);
			break;

		case 'g':
			a_gui = 1;
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
	Mat				yuv, rgb, bgr, osd;
	char				text[128];
	int				w, h;
	int				i, j, hist = 0, fid, num, ret;
	int				max, sum;
	FILE				*LOG = NULL;

	MEASURE_TIME_VARIABLES();

	ret = init_param(argc, argv);
	MHDET_CHECK_ERROR();

	v_cfg.scale	= a_scale;
	v_cfg.step	= a_step;
	v_cfg.neighbors	= a_neighbors;
	v_cfg.all_first	= a_all_first;
	v_cfg.mdet_only	= a_mdet_only;
	v_cfg.debug	= a_debug;

	v_mhdet = mhdet_init(&v_cfg);
	if (!v_mhdet) {
		ret = -1;
		MHDET_CHECK_ERROR();
	}

	if (a_gui) {
		w = a_vw / 2;
		h = a_vh / 2;

		ret = open_fb(&w, &h);
		MHDET_CHECK_ERROR();

		ret = blank_fb();
		MHDET_CHECK_ERROR();
	}

	ret = init_iav();
	MHDET_CHECK_ERROR();

	yuv.create(a_vh, a_vw, CV_8UC3);
	rgb.create(a_vh, a_vw, CV_8UC3);
	bgr.create(a_vh, a_vw, CV_8UC3);
	osd.create(h, w, CV_8UC3);

	if (v_log) {
		sprintf(text, "%s/time.txt", a_log);
		LOG = fopen(text, "w");
		if (!LOG) {
			ret = -1;
			MHDET_CHECK_ERROR();
		}
	}

	fid = 1;
	max = 0;
	sum = 0;
	while (fid <= a_frames) {
		ret = get_main_buf(&v_buf);
		MHDET_CHECK_ERROR();

		if (v_buf.yuv_p != a_vw) {
			ret = -1;
			MHDET_CHECK_ERROR();
		}

		memcpy(yuv.data, v_buf.yuv_y, a_vw * a_vh);
		memcpy(yuv.data + a_vw * a_vh, v_buf.yuv_uv, a_vw * a_vh / 2);

		MEASURE_TIME_START();
		v_in.y	 = yuv.data;
		v_in.uv  = yuv.data + a_vw * a_vh;
		v_in.w	 = yuv.cols;
		v_in.h	 = yuv.rows;
		v_out	 = mhdet_do(v_mhdet, &v_in);
		MEASURE_TIME_END("Detection:");

		if (ms > max) {
			max = ms;
		}
		sum += ms;
		if (a_histogram && ms > a_histogram) {
			hist++;
		}

		printf("%16s %10d ms\n", "Max Time:", max);
		printf("%16s %10d ms\n", "Avg Time:", sum / fid);
		if (a_histogram) {
			printf("%16s %10d / %d\n", "Over Time:", hist, fid);
		}
		printf("%16s %10d\n\n", "Detected:", v_out->num_heads);

		if (v_log) {
			fprintf(LOG, "============== Frame %4d =================\n", fid);
			fprintf(LOG, "%16s %10d ms\n", "Detection Time:", ms);
			fprintf(LOG, "%16s %10d ms\n", "Max Time:", max);
			fprintf(LOG, "%16s %10d ms\n", "Avg Time:", sum / fid);
			if (a_histogram) {
				fprintf(LOG, "%16s %10d / %d\n", "Over Time:", hist, fid);
			}
			fprintf(LOG, "%16s %10d\n\n", "Detected:", v_out->num_heads);
		}

		if (a_gui) {
			__yuv_2_rgb__(a_vw, a_vh, a_vw, rgb.step, yuv.data, yuv.data + a_vw * a_vh, rgb.data);

			if (v_log) {
				sprintf(text, "%s/rgb/%04d.jpg", a_log, fid);
				cvtColor(rgb, bgr, CV_RGB2BGR);
				imwrite(text, bgr);
			}

			num	= v_out->num_rois;
			v_rects	= v_out->rois;
			for (j = 0; j < num; j++) {
				if (!a_heads_only) {
					if (v_rects[j].valid) {
						rectangle(rgb, Point(v_rects[j].left, v_rects[j].low), Point(v_rects[j].right, v_rects[j].high), COLORS[1], 2);
					}

					if (a_debug && !v_rects[j].valid) {
						rectangle(rgb, Point(v_rects[j].left, v_rects[j].low), Point(v_rects[j].right, v_rects[j].high), COLORS[2], 2);
					}
				}
			}

			num	= v_out->num_heads;
			v_rects	= v_out->heads;
			for (i = 0; i < num; i++) {
				rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[0], 2);
			}


			resize(rgb, osd, cvSize(osd.cols, osd.rows), CV_INTER_AREA);
			ret = render_fb(osd.data, osd.cols, osd.rows, osd.step, 0, 0);
			MHDET_CHECK_ERROR();

			ret = refresh_fb();
			MHDET_CHECK_ERROR();

			if (v_log) {
				sprintf(text, "%s/osd/%04d.jpg", a_log, fid);
				cvtColor(rgb, bgr, CV_RGB2BGR);
				imwrite(text, bgr);
			}
		}

		if (ms > a_wait) {
			getchar();
		}

		fid++;
	}

	ret = blank_fb();
	MHDET_CHECK_ERROR();

	ret = exit_iav();
	MHDET_CHECK_ERROR();

	mhdet_exit(v_mhdet);

	if (v_log) {
		fclose(LOG);
	}

	return 0;
}
