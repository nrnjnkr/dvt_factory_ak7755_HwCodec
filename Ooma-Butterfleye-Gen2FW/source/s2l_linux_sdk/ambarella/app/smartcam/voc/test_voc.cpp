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
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sys/time.h>
#include <getopt.h>

#include "voc.h"

#define	MEASURE_TIME_VARIABLES()	\
	struct timeval			tv1, tv2;	\
	int						ms

#define MEASURE_TIME_START()		gettimeofday(&tv1, NULL)

#define MEASURE_TIME_END(s)	gettimeofday(&tv2, NULL);	\
	ms = 10 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 100000;	\
	printf("%-16s: %.1f s\n", s, 0.1 * ms)

#define	VOC_ERROR()	printf("ERROR: %s %d!\n", __FILE__, __LINE__);	\
	exit(-1)

using namespace cv;

#include "../utils2/fb.cpp"
#include "../utils2/iav.cpp"
#include "../utils2/csc.cpp"

struct option long_options[] = {
	{"debug",		0,	0,	'd'},
	{"threshold",	1,	0,	't'},
	{"picture",		1,	0,	'p'},
	{"weights",		1,	0,	'w'},
	{"video",		1,	0,	'v'},
	{0,				0,	0,	 0 },
};

const char *short_options = "dt:p:w:v:";

static int				a_debug = 0;
static float			a_threshold = 0.2;
static char				a_picture[64] = "/sdcard/person.jpg";
static char				a_weights[64] = "/sdcard/voc.w";
static int				a_vw = 640;
static int				a_vh = 360;

static voc_config_t		v_cfg;
static void				*v_vs;
static voc_input_t		v_input;
static voc_output_t		v_output;
static int				v_pic = 0;
static int				v_video = 0;
static second_buf_t		v_buf;

int parse_parameters(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'd':
			a_debug = 1;
			break;

		case 't':
			a_threshold = atof(optarg);
			break;

		case 'p':
			strcpy(a_picture, optarg);
			v_pic = 1;
			break;

		case 'w':
			strcpy(a_weights, optarg);
			break;

		case 'v':
			sscanf(optarg, "%dx%d", &a_vw, &a_vh);
			v_video = 1;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			break;

		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	Mat				rgb, img;
	int				i, nb, ret, fid;
	voc_object_t	*ob;
	int				w, h;
	Scalar			color;

	MEASURE_TIME_VARIABLES();

	ret = parse_parameters(argc, argv);
	if (ret < 0) {
		VOC_ERROR();
	}

	w = 0;
	h = 0;
	ret = open_fb(&w, &h);
	if (ret < 0) {
		VOC_ERROR();
	}

	ret = blank_fb();
	if (ret < 0) {
		VOC_ERROR();
	}

	if (v_video) {
		ret = init_iav();
		if (ret < 0) {
			VOC_ERROR();
		}

		ret = get_second_buf(&v_buf);
		if (ret < 0) {
			VOC_ERROR();
		}

		if (v_buf.yuv_w != a_vw || v_buf.yuv_h != a_vh) {
			VOC_ERROR();
		}

		rgb.create(a_vh, a_vw, CV_8UC3);
	}

	if (v_pic) {
		rgb = imread(a_picture);
		if (rgb.empty()) {
			VOC_ERROR();
		}

		cvtColor(rgb, rgb, CV_BGR2RGB);
	}

	img.create(448, 448, CV_8UC3);

	v_cfg.threshold = a_threshold;
	strcpy(v_cfg.weights, a_weights);
	MEASURE_TIME_START();
	v_vs = voc_init(&v_cfg);
	if (!v_vs) {
		VOC_ERROR();
	}
	MEASURE_TIME_END("Init");

	fid = 1;
	while (1) {
		if (v_video) {
			ret = get_second_buf(&v_buf);
			if (ret < 0) {
				VOC_ERROR();
			}

			printf("\n====== Frame %d =========\n", fid);
			__yuv_2_rgb__(a_vw, a_vh, v_buf.yuv_p, rgb.step, v_buf.yuv_y, v_buf.yuv_uv, rgb.data);
		}

		resize(rgb, img, Size(448, 448), 0, 0, INTER_AREA);

		v_input.original_width	= rgb.cols;
		v_input.original_height	= rgb.rows;
		v_input.data			= img.data;
		MEASURE_TIME_START();
		nb = voc_do(v_vs, &v_input, &v_output);
		MEASURE_TIME_END("Detect");
		ob = v_output.objects;
		for (i = 0; i < nb; i++, ob++) {
			color = COLORS[ob->type % NUM_COLORS];
			rectangle(rgb, Point(ob->left, ob->low), Point(ob->right, ob->high), color, 2);
			putText(rgb, voc_object_names[ob->type], Point((ob->left + ob->right) / 2, (ob->low + ob->high) / 2), FONT_HERSHEY_SIMPLEX, 0.5, color);
		}

		if (rgb.cols > w || rgb.rows > h) {
			float		r1, r2, r;

			r1 = (float)rgb.cols / w;
			r2 = (float)rgb.rows / h;
			if (r1 > r2) {
				r = r1;
			} else {
				r = r2;
			}
			resize(rgb, rgb, Size(rgb.cols / r, rgb.rows / r), 0, 0, INTER_AREA);
		}

		ret = render_fb(rgb.data, rgb.cols, rgb.rows, rgb.step, (w - rgb.cols) / 2 , (rgb.rows - h) / 2);
		if (ret < 0) {
			VOC_ERROR();
		}

		ret = refresh_fb();
		if (ret < 0) {
			VOC_ERROR();
		}

		if (v_pic) {
			break;
		}

		fid++;
	}

	close_fb();

	if (v_video) {
		exit_iav();
	}

	voc_exit(v_vs);

	return 0;
}
