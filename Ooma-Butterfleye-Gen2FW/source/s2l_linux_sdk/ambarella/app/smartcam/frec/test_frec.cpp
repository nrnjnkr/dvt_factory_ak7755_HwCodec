/*
 *
 * History:
 *    2015/12/04 - [Zhenwu Xue] Create
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/time.h>

#include "fdet_lib.h"
#include "frec.h"

#include <opencv2/opencv.hpp>

#define FREC_WARNING()	printf("%s %d: Warning!\n", __func__, __LINE__)

#define FREC_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}

#define	MEASURE_TIME_VARIABLES()	\
	struct timeval			tv1, tv2;	\
	int				ms

#define MEASURE_TIME_START() gettimeofday(&tv1, NULL)
#define MEASURE_TIME_END(s)	\
	gettimeofday(&tv2, NULL);	\
	ms = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;	\
	printf("%16s %10d ms\n", s, ms)

using namespace cv;

typedef struct {
	float		rep[128];
	char		name[64];
	float		dist;
	int		flag;
} rep_t;

typedef struct {
	int		me1_w;
	int		me1_h;
	int		me1_p;
	unsigned char	*me1_y;

	int		yuv_w;
	int		yuv_h;
	int		yuv_p;
	unsigned char	*yuv_y;
	unsigned char	*yuv_uv;
} main_buf_t;

#define MAX_PERSON	1000

#include "fb.cpp"
#include "iav.cpp"

static char		a_landmark[64] = "/sdcard/landmark.b";
static char		a_model[64] = "/sdcard/model.b";
static char		a_library[64] = "/sdcard/Face_Library";
static int		a_list = 0;
static int		a_generate = 0; /* 1: All 2: Incremental */
static int		a_add = 0;
static char		a_pic[64] = {'\0'};
static int		a_video = 0;
static int		a_distance = 0;
static float		a_threshold = 0.1;
static char		a_compare[64] = {'\0'};
static int		a_show = 0;

static fdet_data_t	v_fdet;
static void		*v_frec;
static frec_face_t	v_face;
static rep_t		*v_rep = NULL;
static int		v_num = 0;

struct option long_options[] = {
	{"landmark",	1,	0,	'k'},
	{"model",	1,	0,	'm'},
	{"library",	1,	0,	'l'},
	{"list",	0,	0,	'L'},
	{"generate",	1,	0,	'g'},
	{"add",		0,	0,	'a'},
	{"pic",		1,	0,	'p'},
	{"video",	0,	0,	'v'},
	{"distance",	0,	0,	'd'},
	{"threshold",	1,	0,	't'},
	{"compare",	1,	0,	'c'},
	{"show",	0,	0,	's'},
	{0,		0,	0,	 0 },
};

const char *short_options = "k:m:l:g:ap:vLdt:c:s";

int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'k':
			strcpy(a_landmark, optarg);
			break;

		case 'm':
			strcpy(a_model, optarg);
			break;

		case 'l':
			strcpy(a_library, optarg);
			break;

		case 'L':
			a_list = 1;
			break;

		case 'g':
			a_generate = atoi(optarg);
			break;

		case 'a':
			a_add = 1;
			break;

		case 'p':
			strcpy(a_pic, optarg);
			break;

		case 'v':
			a_video = 1;
			break;

		case 'd':
			a_distance = 1;
			break;

		case 't':
			a_threshold = atof(optarg);
			break;

		case 'c':
			strcpy(a_compare, optarg);
			break;

		case 's':
			a_show = 1;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			return -1;
		}
	}

	return 0;
}

int generate()
{
	int			ret;
	char			string[64];
	char			name[64];
	char			*file;
	DIR			*dir;
	struct dirent		*ptr;
	int			i, len;
	Mat			rgb, gray, stf;
	fdet_rect_t		*r;
	FILE			*B;
	unsigned char		*p, *q;

	sprintf(string, "%s/origin", a_library);
	dir = opendir(string);
	while (1) {
		ptr = readdir(dir);
		if (!ptr) {
			break;
		}
		if (ptr->d_type != 8) {
			continue;
		}
		file = ptr->d_name;
		len = strlen(file);
		for (i = len - 1; i >= 0; i--) {
			if (file[i] == '.') {
				break;
			}
		}
		if (i < 1 || i >= len - 1) {
			continue;
		}

		strncpy(name, file, i);
		name[i] = '\0';
		if (a_generate == 2) {
			sprintf(string, "%s/rep/%s.b", a_library, name);
			if (!access(string, 0)) {
				continue;
			}
		}

		sprintf(string, "%s/origin/%s", a_library, file);
		gray = imread(string, 0);
		if (gray.empty()) {
			FREC_WARNING();
			continue;
		}

		ret = fdet_init(&v_fdet, gray.cols, gray.rows, gray.step, 1.1, 1);
		FREC_CHECK_ERROR();

		ret = fdet_do(&v_fdet, gray.data);
		FREC_CHECK_ERROR();

		if (ret == 1) {
			r = v_fdet.rects;

			printf("Generating rep for %s ...\n", string);

			rgb = imread(string);
			if (rgb.empty()) {
				FREC_WARNING();
				fdet_exit(&v_fdet);
				continue;
			}
			cvtColor(rgb, rgb, CV_BGR2RGB);

			v_face.d = rgb.data;
			v_face.w = rgb.cols;
			v_face.h = rgb.rows;
			v_face.p = rgb.step;

			v_face.left  = r->left;
			v_face.right = r->right;
			v_face.low   = r->low;
			v_face.high  = r->high;

			ret = frec_do(v_frec, &v_face);
			FREC_CHECK_ERROR();

			sprintf(string, "%s/rep/%s.b", a_library, name);
			B = fopen(string, "wb");
			ret = B ? 0 : -1;
			FREC_CHECK_ERROR();

			fwrite(v_face.rep, sizeof(float), 128, B);
			fclose(B);

			sprintf(string, "%s/standard/%s.jpg", a_library, name);
			stf.create(96, 96, CV_8UC3);
			p = v_face.face;
			q = stf.data;
			for (i = 0; i < 96; i++) {
				memcpy(q, p, 3 * 96);
				p += 3 * 96;
				q += stf.step;
			}
			cvtColor(stf, stf, CV_RGB2BGR);
			imwrite(string, stf);

			printf("Done.\n");
		} else {
			FREC_WARNING();
		}

		fdet_exit(&v_fdet);
	}
	closedir(dir);

	return 0;
}

int read_library()
{
	int			ret;
	char			string[64];
	char			*file;
	char			*name;
	DIR			*dir;
	struct dirent		*ptr;
	int			len;
	FILE			*B;

	sprintf(string, "%s/rep", a_library);
	dir = opendir(string);
	while (1) {
		ptr = readdir(dir);
		if (!ptr) {
			break;
		}
		if (ptr->d_type != 8) {
			continue;
		}
		file = ptr->d_name;
		len = strlen(file);
		if (len < 3 || file[len - 2] != '.' || file[len - 1] != 'b') {
			continue;
		}

		name = v_rep[v_num].name;
		strncpy(name, file, len - 2);
		name[len - 2] = '\0';

		sprintf(string, "%s/rep/%s", a_library, file);
		B = fopen(string, "rb");
		ret = B ? 0 : -1;
		FREC_CHECK_ERROR();

		fread(v_rep[v_num].rep, sizeof(float), 128, B);

		fclose(B);

		if (a_list) {
			printf("Subject %05d:\t%-20s\n", v_num, name);
		}

		v_num++;

		if (v_num >= MAX_PERSON) {
			FREC_WARNING();
			break;
		}
	}
	closedir(dir);

	return 0;
}

int compare()
{
	int			ret, i;
	float			ref[128], diff, dist;
	FILE			*B;

	B = fopen(a_compare, "rb");
	ret = B ? 0 : -1;
	FREC_CHECK_ERROR();
	fread(ref, sizeof(float), 128, B);
	fclose(B);

	dist = 0;
	for (i = 0; i < 128; i++) {
		diff = v_face.rep[i] - ref[i];
		dist = dist + diff * diff;
	}

	printf("\nL2 Distance from %s: %1.e\n", a_compare, dist);

	return 0;
}

int recog_pic(int *id)
{
	int			ret;
	Mat			gray, rgb;
	fdet_rect_t		*r;
	int			i, j, idx;
	float			diff, dist;
	float			min;

	*id = -1;

	gray = imread(a_pic, 0);
	ret = gray.empty() ? -1 : 0;
	FREC_CHECK_ERROR();

	ret = fdet_init(&v_fdet, gray.cols, gray.rows, gray.step, 1.1, 1);
	FREC_CHECK_ERROR();

	ret = fdet_do(&v_fdet, gray.data);
	FREC_CHECK_ERROR();

	if (ret > 1) {
		printf("Warning: %d faces detected, recognize the first one only.\n", ret);
	}

	if (ret >= 1) {
		r = v_fdet.rects;

		rgb = imread(a_pic);
		if (rgb.empty()) {
			FREC_WARNING();
			fdet_exit(&v_fdet);
			return -1;
		}
		cvtColor(rgb, rgb, CV_BGR2RGB);

		if (a_show) {
			Mat		fc;

			fc = rgb.clone();
			rectangle(fc, Point(r->left, r->low), Point(r->right, r->high), CV_RGB(255, 0, 0), 3);
			if (fc.cols > 320 || fc.rows > 180) {
				float		mw, mh, mm;

				mw = (float)320 / fc.cols;
				mh = (float)180 / fc.rows;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}

				resize(fc, fc, cvSize(fc.cols * mm, fc.rows * mm), CV_INTER_AREA);
			}
			ret = render_fb(fc.data, fc.cols, fc.rows, fc.step, (320 - fc.cols) / 2, (180 - fc.rows) / 2);
			FREC_CHECK_ERROR();
		}

		v_face.d = rgb.data;
		v_face.w = rgb.cols;
		v_face.h = rgb.rows;
		v_face.p = rgb.step;

		v_face.left  = r->left;
		v_face.right = r->right;
		v_face.low   = r->low;
		v_face.high  = r->high;

		ret = frec_do(v_frec, &v_face);
		FREC_CHECK_ERROR();

		if (a_show) {
			Mat		lm1, lm2, lm3, lm4;
			int		i, w, h, left, right, low, high;
			unsigned char	*p, *q;

			/* Landmarks before warping */
			lm1 = rgb.clone();

			w = r->right - r->left + 1;
			h = r->high  - r->low  + 1;
			left  = r->left  - w / 5;
			right = r->right + w / 5;
			low   = r->low   - h / 5;
			high  = r->high  + h / 5;
			if (left < 0) {
				left = 0;
			}
			if (right >= lm1.cols) {
				right = lm1.cols - 1;
			}
			if (low < 0) {
				low = 0;
			}
			if (high >= lm1.rows) {
				high = lm1.rows;
			}
			w = right - left + 1;
			h = high  - low  + 1;

			for (i = 0; i < 68; i++) {
				circle(lm1, Point(v_face.pt1[2 * i], v_face.pt1[2 * i + 1]), 4, CV_RGB(255, 0, 0), 2);
			}

			lm2.create(h, w, CV_8UC3);
			p = lm1.data + low * lm1.step + left * 3;
			q = lm2.data;
			for (i = 0; i < h; i++) {
				memcpy(q, p, 3 * w);
				p += lm1.step;
				q += lm2.step;
			}
			if (w > 320 || h > 180) {
				float		mw, mh, mm;

				mw = (float)320 / w;
				mh = (float)180 / h;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}
				resize(lm2, lm2, cvSize(w * mm, h * mm), CV_INTER_AREA);
			}

			w = lm2.cols;
			h = lm2.rows;
			ret = render_fb(lm2.data, w, h, lm2.step, 320 + (320 - w) / 2, (180 - h) / 2);
			FREC_CHECK_ERROR();

			/* Landmarks after warping */
			lm3 = rgb.clone();
			p = v_face.warp;
			q = lm3.data;
			for (i = 0; i < lm3.rows; i++) {
				memcpy(q, p, 3 * lm3.cols);
				p += (3 * lm3.cols);
				q += lm3.step;
			}

			w = v_face.right2 - v_face.left2 + 1;
			h = v_face.high2  - v_face.low2  + 1;
			left  = v_face.left2  - w / 10;
			right = v_face.right2 + w / 10;
			low   = v_face.low2   - h / 10;
			high  = v_face.high2  + h / 10;
			if (left < 0) {
				left = 0;
			}
			if (right >= lm3.cols) {
				right = lm3.cols - 1;
			}
			if (low < 0) {
				low = 0;
			}
			if (high >= lm3.rows) {
				high = lm3.rows;
			}
			w = right - left + 1;
			h = high  - low  + 1;

			for (i = 0; i < 68; i++) {
				circle(lm3, Point(v_face.pt2[2 * i], v_face.pt2[2 * i + 1]), 4, CV_RGB(255, 0, 0), 2);
			}

			lm4.create(h, w, CV_8UC3);
			p = lm3.data + low * lm3.step + left * 3;
			q = lm4.data;
			for (i = 0; i < h; i++) {
				memcpy(q, p, 3 * w);
				p += lm3.step;
				q += lm4.step;
			}
			if (w > 320 || h > 180) {
				float		mw, mh, mm;

				mw = (float)320 / w;
				mh = (float)180 / h;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}
				resize(lm4, lm4, cvSize(w * mm, h * mm), CV_INTER_AREA);
			}

			w = lm4.cols;
			h = lm4.rows;
			ret = render_fb(lm4.data, w, h, lm4.step, (320 - w) / 2, 180 + (180 - h) / 2);
			FREC_CHECK_ERROR();

			/* Normalized Face */
			ret = render_fb(v_face.face, 96, 96, 3 * 96, 352, 222);
			FREC_CHECK_ERROR();
		}

		min = 4;
		idx = 0;
		for (i = 0; i < v_num; i++) {
			dist = 0;
			for (j = 0; j < 128; j++) {
				diff = v_rep[i].rep[j] - v_face.rep[j];
				dist = dist + diff * diff;
			}
			v_rep[i].dist	= dist;
			v_rep[idx].flag	= 0;
			if (dist < min) {
				min = dist;
				idx = i;
			}

		}
		*id = idx;

		if (a_show && min <= a_threshold) {
			char		string[64];
			Mat		fc;

			sprintf(string, "%s/standard/%s.jpg", a_library, v_rep[idx].name);
			fc = imread(string);
			ret = fc.empty() ? -1 : 0;
			FREC_CHECK_ERROR();
			cvtColor(fc, fc, CV_BGR2RGB);
			ret = render_fb(fc.data, fc.cols, fc.rows, fc.step, 512, 222);
			FREC_CHECK_ERROR();
		}

		if (a_distance) {
			printf("\n");
			for (i = 0; i < v_num; i++) {
				min = 4;
				idx = 0;
				for (j = 0; j < v_num; j++) {
					if (v_rep[j].flag) {
						continue;
					}
					if (v_rep[j].dist < min) {
						min = v_rep[j].dist;
						idx = j;
					}
				}
				printf("Subject %05d:\t%-20s\t%5.2f\n", idx, v_rep[idx].name, min);
				v_rep[idx].flag = 1;
			}
		}

		if (a_show) {
			ret = refresh_fb();
			FREC_CHECK_ERROR();
		}
	}

	fdet_exit(&v_fdet);

	return 0;
}

int recog_video(int *id)
{
	main_buf_t		mb;
	int			ret;
	Mat			rgb;
	fdet_rect_t		*r;
	int			i, j, k, idx;
	float			diff, dist;
	float			min;

	*id = -1;

	ret = get_main_buf(&mb);
	FREC_CHECK_ERROR();

	ret = fdet_do(&v_fdet, mb.me1_y);
	FREC_CHECK_ERROR();

	if (ret > 1) {
		printf("Warning: %d faces detected, recognize the first one only.\n", ret);
	}

	if (ret >= 1) {
		int		w, h, left, right, low, high;
		int		_r, _g, _b, y, u, v;
		unsigned char	*o, *p, *q;

		ret = blank_fb();
		FREC_CHECK_ERROR();

		r = v_fdet.rects;

		if (a_show) {
			Mat		fc;

			fc.create(mb.me1_h, mb.me1_w, CV_8UC3);
			rectangle(fc, Point(r->left, r->low), Point(r->right, r->high), CV_RGB(255, 0, 0), 3);
			ret = render_fb(fc.data, fc.cols, fc.rows, fc.step, 0, 0);
			FREC_CHECK_ERROR();
		}

		r->left  *= 4;
		r->right *= 4;
		r->low   *= 4;
		r->high  *= 4;
		w = r->right - r->left + 1;
		h = r->high  - r->low  + 1;
		left  = r->left  - 3 * w / 10;
		right = r->right + 3 * w / 10;
		low   = r->low   - 3 * h / 10;
		high  = r->high  + 3 * h / 10;
		if (left < 0) {
			left = 0;
		}
		if (right >= mb.yuv_w) {
			right = mb.yuv_w - 1;
		}
		if (low < 0) {
			low = 0;
		}
		if (high >= mb.yuv_h) {
			high = mb.yuv_h - 1;
		}
		r->left  -= left;
		r->right -= left;
		r->low   -= low;
		r->high  -= low;
		w = right - left + 1;
		h = high  - low  + 1;
		rgb.create(h, w, CV_8UC3);

		p = mb.yuv_y + low * mb.yuv_p + left;
		q = mb.yuv_uv + (low / 2) * mb.yuv_p;
		o = rgb.data;
		for (i = 0; i < h; i++) {
			for (j = 0; j < w; j++) {
				y = p[j];
				k = left + j;
				if (k & 0x1) {
					u = q[k - 1] - 128;
					v = q[k - 0] - 128;
				} else {
					u = q[k + 0] - 128;
					v = q[k + 1] - 128;
				}
				_r = y + (183763 * v >> 17);
				_g = y - (45107 * u >> 17) - (46802 * v >> 16);
				_b = y + (116130 * u >> 16);

				if (_r < 0) {
					_r = 0;
				}
				if (_r > 255) {
					_r = 255;
				}
				if (_g < 0) {
					_g = 0;
				}
				if (_g > 255) {
					_g = 255;
				}
				if (_b < 0) {
					_b = 0;
				}
				if (_b > 255) {
					_b = 255;
				}

				o[3 * j + 0] = _r;
				o[3 * j + 1] = _g;
				o[3 * j + 2] = _b;
			}

			p += mb.yuv_p;
			if ((low + i) & 0x1) {
				q += mb.yuv_p;
			}
			o += rgb.step;
		}

		v_face.d = rgb.data;
		v_face.w = rgb.cols;
		v_face.h = rgb.rows;
		v_face.p = rgb.step;

		v_face.left  = r->left;
		v_face.right = r->right;
		v_face.low   = r->low;
		v_face.high  = r->high;

		ret = frec_do(v_frec, &v_face);
		FREC_CHECK_ERROR();

		if (a_show) {
			Mat		lm1, lm2, lm3, lm4;

			/* Landmarks before warping */
			lm1 = rgb.clone();

			w = r->right - r->left + 1;
			h = r->high  - r->low  + 1;
			left  = r->left  - w / 5;
			right = r->right + w / 5;
			low   = r->low   - h / 5;
			high  = r->high  + h / 5;
			if (left < 0) {
				left = 0;
			}
			if (right >= lm1.cols) {
				right = lm1.cols - 1;
			}
			if (low < 0) {
				low = 0;
			}
			if (high >= lm1.rows) {
				high = lm1.rows - 1;
			}
			w = right - left + 1;
			h = high  - low  + 1;

			for (i = 0; i < 68; i++) {
				circle(lm1, Point(v_face.pt1[2 * i], v_face.pt1[2 * i + 1]), 4, CV_RGB(255, 0, 0), 2);
			}

			lm2.create(h, w, CV_8UC3);
			p = lm1.data + low * lm1.step + left * 3;
			q = lm2.data;
			for (i = 0; i < h; i++) {
				memcpy(q, p, 3 * w);
				p += lm1.step;
				q += lm2.step;
			}
			if (w > 320 || h > 180) {
				float		mw, mh, mm;

				mw = (float)320 / w;
				mh = (float)180 / h;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}
				resize(lm2, lm2, cvSize(w * mm, h * mm), CV_INTER_AREA);
			}

			w = lm2.cols;
			h = lm2.rows;
			ret = render_fb(lm2.data, w, h, lm2.step, 320 + (320 - w) / 2, (180 - h) / 2);
			FREC_CHECK_ERROR();

			/* Landmarks after warping */
			lm3 = rgb.clone();
			p = v_face.warp;
			q = lm3.data;
			for (i = 0; i < lm3.rows; i++) {
				memcpy(q, p, 3 * lm3.cols);
				p += (3 * lm3.cols);
				q += lm3.step;
			}

			w = v_face.right2 - v_face.left2 + 1;
			h = v_face.high2  - v_face.low2  + 1;
			left  = v_face.left2  - w / 10;
			right = v_face.right2 + w / 10;
			low   = v_face.low2   - h / 10;
			high  = v_face.high2  + h / 10;
			if (left < 0) {
				left = 0;
			}
			if (right >= lm3.cols) {
				right = lm3.cols - 1;
			}
			if (low < 0) {
				low = 0;
			}
			if (high >= lm3.rows) {
				high = lm3.rows - 1;
			}
			w = right - left + 1;
			h = high  - low  + 1;

			for (i = 0; i < 68; i++) {
				circle(lm3, Point(v_face.pt2[2 * i], v_face.pt2[2 * i + 1]), 4, CV_RGB(255, 0, 0), 2);
			}

			lm4.create(h, w, CV_8UC3);
			p = lm3.data + low * lm3.step + left * 3;
			q = lm4.data;
			for (i = 0; i < h; i++) {
				memcpy(q, p, 3 * w);
				p += lm3.step;
				q += lm4.step;
			}
			if (w > 320 || h > 180) {
				float		mw, mh, mm;

				mw = (float)320 / w;
				mh = (float)180 / h;
				if (mw < mh) {
					mm = mw;
				} else {
					mm = mh;
				}
				resize(lm4, lm4, cvSize(w * mm, h * mm), CV_INTER_AREA);
			}

			w = lm4.cols;
			h = lm4.rows;
			ret = render_fb(lm4.data, w, h, lm4.step, (320 - w) / 2, 180 + (180 - h) / 2);
			FREC_CHECK_ERROR();

			/* Normalized Face */
			ret = render_fb(v_face.face, 96, 96, 3 * 96, 352, 222);
			FREC_CHECK_ERROR();
		}

		min = 4;
		idx = 0;
		for (i = 0; i < v_num; i++) {
			dist = 0;
			for (j = 0; j < 128; j++) {
				diff = v_rep[i].rep[j] - v_face.rep[j];
				dist = dist + diff * diff;
			}
			v_rep[i].dist	= dist;
			v_rep[idx].flag	= 0;
			if (dist < min) {
				min = dist;
				idx = i;
			}

		}
		*id = idx;

		if (a_show && min <= a_threshold) {
			char		string[64];
			Mat		fc;

			sprintf(string, "%s/standard/%s.jpg", a_library, v_rep[idx].name);
			fc = imread(string);
			ret = fc.empty() ? -1 : 0;
			FREC_CHECK_ERROR();
			cvtColor(fc, fc, CV_BGR2RGB);
			ret = render_fb(fc.data, fc.cols, fc.rows, fc.step, 512, 222);
			FREC_CHECK_ERROR();
		}

		if (a_distance) {
			printf("\n");
			for (i = 0; i < v_num; i++) {
				min = 4;
				idx = 0;
				for (j = 0; j < v_num; j++) {
					if (v_rep[j].flag) {
						continue;
					}
					if (v_rep[j].dist < min) {
						min = v_rep[j].dist;
						idx = j;
					}
				}
				printf("Subject %05d:\t%-20s\t%5.2f\n", idx, v_rep[idx].name, min);
				v_rep[idx].flag = 1;
			}
		}

		if (a_show) {
			ret = refresh_fb();
			FREC_CHECK_ERROR();

			exit(0);
		}
	}



	return 0;
}

int main(int argc, char **argv)
{
	int				ret;
	int				id;

	MEASURE_TIME_VARIABLES();

	ret = init_param(argc, argv);
	FREC_CHECK_ERROR();

	if (a_show) {
		ret = open_fb();
		FREC_CHECK_ERROR();

		ret = blank_fb();
		FREC_CHECK_ERROR();
	}

	gettimeofday(&tv1, NULL);
	v_frec = frec_init(a_landmark, a_model);
	ret  = v_frec ? 0 : -1;
	FREC_CHECK_ERROR();
	gettimeofday(&tv2, NULL);
	ms = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;
	printf("%16s %10d ms\n", "frec init:", ms);

	if (a_generate == 1 || a_generate == 2) {
		MEASURE_TIME_START();

		ret = generate();
		FREC_CHECK_ERROR();

		MEASURE_TIME_END("generate:");
	}

	{
		MEASURE_TIME_START();

		v_rep = (rep_t *)malloc(MAX_PERSON * sizeof(rep_t));
		ret = v_rep ? 0 : -1;
		FREC_CHECK_ERROR();

		ret = read_library();
		FREC_CHECK_ERROR();

		MEASURE_TIME_END("read library:");
	}

	if (a_pic[0] != '\0') {
		MEASURE_TIME_START();

		ret = recog_pic(&id);
		FREC_CHECK_ERROR();

		MEASURE_TIME_END("recog pic:");

		printf("\nResult:\n");
		if (id >= 0 && id < v_num) {
			if (v_rep[id].dist <= a_threshold) {
				printf("Most match: %-20s\t%5.2f\n", v_rep[id].name, v_rep[id].dist);
			} else {
				printf("No match, the minimum distance is: %5.2f\n", v_rep[id].dist);
			}
		} else {
			printf("No face found.\n");
		}
	}

	if (a_video) {
		main_buf_t		mb;

		ret = init_iav();
		FREC_CHECK_ERROR();

		ret = get_main_buf(&mb);
		FREC_CHECK_ERROR();

		ret = fdet_init(&v_fdet, mb.me1_w, mb.me1_h, mb.me1_p, 1.1, 1);
		FREC_CHECK_ERROR();

		while (1) {
			MEASURE_TIME_START();

			ret = recog_video(&id);
			FREC_CHECK_ERROR();

			MEASURE_TIME_END("recog video:");

			printf("\nResult:\n");
			if (id >= 0 && id < v_num) {
				if (v_rep[id].dist <= a_threshold) {
					printf("Most match: %-20s\t%5.2f\n", v_rep[id].name, v_rep[id].dist);
				} else {
					printf("No match, the minimum distance is: %5.2f\n", v_rep[id].dist);
				}
			} else {
				printf("No face found.\n");
			}
		}

		ret = exit_iav();
		FREC_CHECK_ERROR();

		fdet_exit(&v_fdet);
	}

	if (a_compare[0] != '\0') {
		ret = compare();
		FREC_CHECK_ERROR();
	}

	frec_exit(v_frec);
	if (v_rep) {
		free(v_rep);
	}

	if (a_show) {
		close_fb();
	}

	return 0;
}
