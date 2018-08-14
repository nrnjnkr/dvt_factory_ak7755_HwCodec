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

#include "pedestrian_lib.h"

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

#define PEDESTRIAN_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __FILE__, __LINE__);	\
		return ret;	\
	}

#include "../utils2/fb.cpp"
#include "../utils2/iav.cpp"
#include "../utils2/csc.cpp"

static char			a_pic[64] = {'\0'};
static int			a_video = 0;
static int			a_vw = 640;
static int			a_vh = 360;
static float			a_scale = 1.08;
static int			a_step = 2;
static int			a_neighbors = 2;
static int			a_gui = 0;

static void			*v_pd;
static pedestrian_rect_t	*v_rects;
static pedestrian_config_t	v_cfg;
static pedestrian_input_t	v_in;
static pedestrian_output_t	*v_out;
static second_buf_t		v_buf;

struct option long_options[] = {
	{"pic",		1,	0,	'p'},
	{"video",	1,	0,	'v'},
	{"scale",	1,	0,	's'},
	{"step",	1,	0,	't'},
	{"neighbors",	1,	0,	'n'},
	{"gui",		0,	0,	'g'},
	{0,		0,	0,	 0 },
};

const char *short_options = "p:v:s:t:n:g";

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
			strcpy(a_pic, optarg);
			break;

		case 'v':
			sscanf(optarg, "%dx%d", &a_vw, &a_vh);
			if (a_vw >= 0 && a_vh >= 0) {
				a_video = 1;
			}
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
	Mat				rgb;
	int				w, h;
	int				i, num, ret;

	MEASURE_TIME_VARIABLES();

	ret = init_param(argc, argv);
	PEDESTRIAN_CHECK_ERROR();

	v_cfg.scale	= a_scale;
	v_cfg.step	= a_step;
	v_cfg.neighbors	= a_neighbors;

	v_pd = pedestrian_init(&v_cfg);
	if (!v_pd) {
		ret = -1;
		PEDESTRIAN_CHECK_ERROR();
	}

	if (a_gui) {
		if (a_video) {
			w = a_vw;
			h = a_vh;
		} else {
			w = 0;
			h = 0;
		}

		ret = open_fb(&w, &h);
		PEDESTRIAN_CHECK_ERROR();

		ret = blank_fb();
		PEDESTRIAN_CHECK_ERROR();
	}


	if (a_pic[0] != '\0') {
		/* Load Image */
		MEASURE_TIME_START();

		rgb = imread(a_pic);
		if (rgb.empty()) {
			ret = -1;
			PEDESTRIAN_CHECK_ERROR();
		}
		cvtColor(rgb, rgb, CV_BGR2RGB);

		MEASURE_TIME_END("Load image:");
		printf("%16s %10d x %4d\n", "Image size:", rgb.cols, rgb.rows);

		MEASURE_TIME_START();
		v_in.d	= rgb.data;
		v_in.w	= rgb.cols;
		v_in.h	= rgb.rows;
		v_in.p	= rgb.step;
		v_out	= pedestrian_do(v_pd, &v_in);
		MEASURE_TIME_END("Detection:");
		printf("%16s %10d\n", "Detected faces:", v_out->num);

		if (a_gui) {
			MEASURE_TIME_START();

			num	= v_out->num;
			v_rects	= v_out->rects;
			for (i = 0; i < num; i++) {
				rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 2);
			}
			if (rgb.cols > w || rgb.rows > h) {
				float		mw, mh, mm;

				mw = (float)w / rgb.cols;
				mh = (float)h / rgb.rows;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}

				resize(rgb, rgb, cvSize(rgb.cols * mm, rgb.rows * mm), CV_INTER_AREA);
			}

			ret = render_fb(rgb.data, rgb.cols, rgb.rows, rgb.step, (w - rgb.cols) / 2, (h - rgb.rows) / 2);
			PEDESTRIAN_CHECK_ERROR();

			ret = refresh_fb();
			PEDESTRIAN_CHECK_ERROR();

			MEASURE_TIME_END("Draw:");
		}
	}

	if (a_video) {

		ret = init_iav();
		PEDESTRIAN_CHECK_ERROR();

		rgb.create(h, w, CV_8UC3);

		while (1) {
			ret = get_second_buf(&v_buf);
			PEDESTRIAN_CHECK_ERROR();

			MEASURE_TIME_START();

			__yuv_2_rgb__(w, h, v_buf.yuv_p, rgb.step, v_buf.yuv_y, v_buf.yuv_uv, rgb.data);

			v_in.d	= rgb.data;
			v_in.w	= rgb.cols;
			v_in.h	= rgb.rows;
			v_in.p	= rgb.step;
			v_out	= pedestrian_do(v_pd, &v_in);
			MEASURE_TIME_END("Detection:");
			printf("%16s %10d\n", "Detected:", v_out->num);

			if (a_gui) {

				num	= v_out->num;
				v_rects	= v_out->rects;

				if (num > 0) {
					memset(rgb.data, 0, rgb.rows * rgb.step);

					for (i = 0; i < num; i++) {
						rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 2);
					}

					ret = render_fb(rgb.data, rgb.cols, rgb.rows, rgb.step, 0, 0);
					PEDESTRIAN_CHECK_ERROR();

					ret = refresh_fb();
					PEDESTRIAN_CHECK_ERROR();
				} else {
					ret = blank_fb();
					PEDESTRIAN_CHECK_ERROR();
				}
			}
		}

		ret = exit_iav();
		PEDESTRIAN_CHECK_ERROR();
	}

	pedestrian_exit(v_pd);

	return 0;
}
