/* test_fdet.cpp
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
#include <getopt.h>
#include <sys/time.h>

#include "fdet_lib.h"

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
	printf("%20s %10d ms\n", s, ms)

#define FDET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __FILE__, __LINE__);	\
		return ret;	\
	}

#include "../utils2/fb.cpp"
#include "../utils2/iav.cpp"

static char			a_pic[64] = {'\0'};
static int			a_video = 0;
static int			a_vw = 640;
static int			a_vh = 360;

static float			a_face_sl_step = 1.1;
static int			a_face_min_num = 2;

static int			a_gui = 0;

static fdet_rect_t		*v_rects;
static fdet_data_t		v_fdet;
static second_buf_t		v_buf;
static int hit_per_face = 2;
static int detected_threshold = 50;

struct option long_options[] = {
	{"pic",				1,	0,	'p'},
	{"video",			1,	0,	'v'},

	{"face-scale-step",		1,	0,	's'},
	{"face-min-num",		1,	0,	'n'},

	{"hit_per_face",		1,	0,	'h'},
	{"detected_threshold",		1,	0,	't'},

	{"gui",				0,	0,	'g'},
	{0,				0,	0,	 0 },
};

const char *short_options = "p:v:s:n:h:t:g";

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
			a_face_sl_step = atof(optarg);
			break;

		case 'n':
			a_face_min_num = atoi(optarg);
			break;

		case 'h':
			hit_per_face = atoi(optarg);
			break;

		case 't':
			detected_threshold = atoi(optarg);
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
	Mat				rgb, gray;
	int				w, h;
	int				i, num, ret;

	MEASURE_TIME_VARIABLES();

	ret = init_param(argc, argv);
	FDET_CHECK_ERROR();
	if (a_video) {
		w = a_vw;
		h = a_vh;
	} else {
		w = 0;
		h = 0;
	}
	if (a_gui) {
		ret = open_fb(&w, &h);
		FDET_CHECK_ERROR();

		ret = blank_fb();
		FDET_CHECK_ERROR();
	}

	if (a_pic[0] != '\0') {
		/* Load Image */
		rgb = imread(a_pic);
		if (rgb.empty()) {
			ret = -1;
			FDET_CHECK_ERROR();
		}

		ret = fdet_init(&v_fdet, rgb.cols, rgb.rows, rgb.cols, a_face_sl_step, a_face_min_num);
		FDET_CHECK_ERROR();

		cvtColor(rgb, gray, CV_BGR2GRAY);
		cvtColor(rgb, rgb, CV_BGR2RGB);
		if ((int)gray.step != rgb.cols) {
			ret = -1;
			FDET_CHECK_ERROR();
		}

		printf("%20s %10d x %4d\n", "Image size:", gray.cols, gray.rows);

		MEASURE_TIME_START();

		ret = fdet_do(&v_fdet, gray.data, hit_per_face);
		FDET_CHECK_ERROR();

		MEASURE_TIME_END("Detection:");
		printf("%20s %10d\n", "Detected faces:", ret);

		if (a_gui) {
			num	= ret;
			v_rects	= v_fdet.rects;
			for (i = 0; i < num; i++) {
				rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[0], 1);
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
			FDET_CHECK_ERROR();

			ret = refresh_fb();
			FDET_CHECK_ERROR();
		}
	}

	if (a_video) {
		unsigned char	*p, *q;

		ret = init_iav();
		FDET_CHECK_ERROR();

		ret = fdet_init(&v_fdet, w, h, w, a_face_sl_step, a_face_min_num);
		FDET_CHECK_ERROR();

		gray.create(h, w, CV_8U);
		rgb.create(h, w, CV_8UC3);

		if ((int)gray.step != w) {
			ret = -1;
			FDET_CHECK_ERROR();
		}

		while (1) {
			ret = get_second_buf(&v_buf);
			FDET_CHECK_ERROR();

			if (v_buf.yuv_w != w || v_buf.yuv_h != h) {
				ret = -1;
				FDET_CHECK_ERROR();
			}

			MEASURE_TIME_START();

			p = v_buf.yuv_y;
			q = gray.data;
			for (i = 0; i < h; i++) {
				memcpy(q, p, w);
				p += v_buf.yuv_p;
				q += gray.step;
			}

			ret = fdet_do(&v_fdet, gray.data, hit_per_face);
			FDET_CHECK_ERROR();

			MEASURE_TIME_END("Detection:");

			if (a_gui) {
				memset(rgb.data, 0, rgb.rows * rgb.step);

				num	= ret;
				v_rects	= v_fdet.rects;

				for (i = 0; i < num; i++) {
					if(v_rects[i].valid>1 && v_rects[i].score > detected_threshold){
						//printf("the score is %f\n",v_rects[i].score);
						//printf("threshold=%d,hit=%d\n",detected_threshold,hit_per_face);
						rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[0], 1);
					}
				}

				if (num) {
					ret = render_fb(rgb.data, rgb.cols, rgb.rows, rgb.step, 0, 0);
					FDET_CHECK_ERROR();

					ret = refresh_fb();
					FDET_CHECK_ERROR();
				} else {
					ret = blank_fb();
					FDET_CHECK_ERROR();
				}
			}
		}

		ret = exit_iav();
		FDET_CHECK_ERROR();
	}

	fdet_exit(&v_fdet);

	if (a_gui) {
		ret = close_fb();
		FDET_CHECK_ERROR();
	}

	return 0;
}

