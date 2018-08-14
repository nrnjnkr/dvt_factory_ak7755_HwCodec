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

#include "human_lib.h"

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
	printf("%32s %10d ms\n", s, ms)

#define HUMAN_CHECK_ERROR()	\
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

static int			a_face_disable = 0;
static float			a_face_sl_step = 1.1;
static int			a_face_xy_step = 2;
static int			a_face_min_num = 2;
static int			a_face_biggest = 0;

static int			a_pedestrian_disable = 0;
static float			a_pedestrian_sl_step = 1.08;
static int			a_pedestrian_xy_step = 2;
static int			a_pedestrian_min_num = 2;
static int			a_pedestrian_biggest = 0;

static int			a_return = 0;

static int			a_gui = 0;

static void			*v_hm;
static human_rect_t		*v_rects;
static human_config_t		v_cfg;
static human_input_t		v_in;
static human_output_t		v_out;
static second_buf_t		v_buf;

struct option long_options[] = {
	{"pic",				1,	0,	'p'},
	{"video",			1,	0,	'v'},

	{"face-disable",		0,	0,	'd'},
	{"face-scale-step",		1,	0,	's'},
	{"face-xy-step",		1,	0,	'x'},
	{"face-min-num",		1,	0,	'm'},
	{"face-biggest",		0,	0,	'b'},

	{"pedestrian-disable",		0,	0,	'D'},
	{"pedestrian-scale-step",	1,	0,	'S'},
	{"pedestrian-xy-step",		1,	0,	'X'},
	{"pedestrian-min-num",		1,	0,	'M'},
	{"pedestrian-biggest",		0,	0,	'B'},

	{"return-if-found",		0,	0,	'r'},

	{"gui",				0,	0,	'g'},
	{0,				0,	0,	 0 },
};

const char *short_options = "p:v:ds:x:m:bDS:X:M:Brg";

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

		case 'd':
			a_face_disable = 1;
			break;

		case 's':
			a_face_sl_step = atof(optarg);
			break;

		case 'x':
			a_face_xy_step = atoi(optarg);
			break;

		case 'm':
			a_face_min_num = atoi(optarg);
			break;

		case 'b':
			a_face_biggest = 1;
			break;

		case 'D':
			a_pedestrian_disable = 1;
			break;

		case 'S':
			a_pedestrian_sl_step = atof(optarg);
			break;

		case 'X':
			a_pedestrian_xy_step = atoi(optarg);
			break;

		case 'M':
			a_pedestrian_min_num = atoi(optarg);
			break;

		case 'B':
			a_pedestrian_biggest = 1;
			break;

		case 'r':
			a_return = 1;
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
	HUMAN_CHECK_ERROR();

	v_cfg.return_if_found		= a_return;

	v_cfg.face_disable		= a_face_disable;
	v_cfg.face_scale_step		= a_face_sl_step;
	v_cfg.face_xy_step		= a_face_xy_step;
	v_cfg.face_min_num		= a_face_min_num;
	v_cfg.face_biggest		= a_face_biggest;

	v_cfg.pedestrian_disable	= a_pedestrian_disable;
	v_cfg.pedestrian_scale_step	= a_pedestrian_sl_step;
	v_cfg.pedestrian_xy_step	= a_pedestrian_xy_step;
	v_cfg.pedestrian_min_num	= a_pedestrian_min_num;
	v_cfg.pedestrian_biggest	= a_pedestrian_biggest;

	v_hm = human_init(&v_cfg);
	if (!v_hm) {
		ret = -1;
		HUMAN_CHECK_ERROR();
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
		HUMAN_CHECK_ERROR();

		ret = blank_fb();
		HUMAN_CHECK_ERROR();
	}


	if (a_pic[0] != '\0') {
		/* Load Image */
		rgb = imread(a_pic);
		if (rgb.empty()) {
			ret = -1;
			HUMAN_CHECK_ERROR();
		}
		cvtColor(rgb, gray, CV_BGR2GRAY);
		cvtColor(rgb, rgb, CV_BGR2RGB);

		printf("%32s %10d x %4d\n", "Image size:", gray.cols, gray.rows);

		MEASURE_TIME_START();
		if (!a_face_disable) {
			v_in.y_addr	= gray.data;
			v_in.y_width	= gray.cols;
			v_in.y_height	= gray.rows;
			v_in.y_pitch	= gray.step;
		}
		if (!a_pedestrian_disable) {
			v_in.rgb_addr	= rgb.data;
			v_in.rgb_width	= rgb.cols;
			v_in.rgb_height	= rgb.rows;
			v_in.rgb_pitch	= rgb.step;
		}
		human_do(v_hm, &v_in, &v_out);
		MEASURE_TIME_END("Detection:");
		if (!a_face_disable) {
			printf("%32s %10d\n", "Detected faces:", v_out.num_face);
		}
		if (!a_pedestrian_disable) {
			printf("%32s %10d\n", "Detected pedestrians:", v_out.num_pedestrian);
		}

		if (a_gui) {
			if (!a_face_disable) {
				num	= v_out.num_face;
				v_rects	= v_out.faces;
				for (i = 0; i < num; i++) {
					rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 1);
				}
			}

			if (!a_pedestrian_disable) {
				num	= v_out.num_pedestrian;
				v_rects	= v_out.pedestrians;
				for (i = 0; i < num; i++) {
					rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 1);
				}
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
			HUMAN_CHECK_ERROR();

			ret = refresh_fb();
			HUMAN_CHECK_ERROR();
		}
	}

	if (a_video) {
		unsigned char	*p, *q;

		ret = init_iav();
		HUMAN_CHECK_ERROR();

		gray.create(h, w, CV_8UC3);
		rgb.create(h, w, CV_8UC3);

		while (1) {
			ret = get_second_buf(&v_buf);
			HUMAN_CHECK_ERROR();

			if (v_buf.yuv_w != w || v_buf.yuv_h != h) {
				ret = -1;
				HUMAN_CHECK_ERROR();
			}

			MEASURE_TIME_START();

			if (!a_face_disable) {
				p = v_buf.yuv_y;
				q = gray.data;
				for (i = 0; i < h; i++) {
					memcpy(q, p, w);
					p += v_buf.yuv_p;
					q += gray.step;
				}

				v_in.y_addr	= gray.data;
				v_in.y_width	= w;
				v_in.y_height	= h;
				v_in.y_pitch	= gray.step;
			}

			if (!a_pedestrian_disable) {
				__yuv_2_rgb__(w, h, v_buf.yuv_p, rgb.step, v_buf.yuv_y, v_buf.yuv_uv, rgb.data);

				v_in.rgb_addr	= rgb.data;
				v_in.rgb_width	= w;
				v_in.rgb_height	= h;
				v_in.rgb_pitch	= rgb.step;
			}

			ret = human_do(v_hm, &v_in, &v_out);
			HUMAN_CHECK_ERROR();

			MEASURE_TIME_END("Detection:");
			if (!a_face_disable) {
				printf("%32s %10d\n", "Detected faces:", v_out.num_face);
			}
			if (!a_pedestrian_disable) {
				printf("%32s %10d\n", "Detected pedestrians:", v_out.num_pedestrian);
			}

			if (a_gui && (!a_face_disable || !a_pedestrian_disable)) {
				memset(rgb.data, 0, rgb.rows * rgb.step);

				if (!a_face_disable) {
					num	= v_out.num_face;
					v_rects	= v_out.faces;

					for (i = 0; i < num; i++) {
						rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 1);
					}
				}

				if (!a_pedestrian_disable) {
					num	= v_out.num_pedestrian;
					v_rects	= v_out.pedestrians;

					for (i = 0; i < num; i++) {
						rectangle(rgb, Point(v_rects[i].left, v_rects[i].low), Point(v_rects[i].right, v_rects[i].high), COLORS[i % NUM_COLORS], 1);
					}
				}

				if (v_out.num_face || v_out.num_pedestrian) {
					ret = render_fb(rgb.data, rgb.cols, rgb.rows, rgb.step, (w - rgb.cols) / 2, (h - rgb.rows) / 2);
					HUMAN_CHECK_ERROR();

					ret = refresh_fb();
					HUMAN_CHECK_ERROR();
				} else {
					ret = blank_fb();
					HUMAN_CHECK_ERROR();
				}
			}
		}

		ret = exit_iav();
		HUMAN_CHECK_ERROR();
	}

	human_exit(v_hm);

	return 0;
}
