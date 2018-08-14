/*
 *
 * History:
 *    2015/06/18 - [Zhenwu Xue] Create
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
#include <sys/time.h>
#include <getopt.h>
#include "cv.h"
#include "opencv2/opencv.hpp"
#include "pdet_lib.h"

using namespace cv;

struct option long_options[] = {
	{"image",	1,	0,	'i'},
	{0,			0,	0,	 0 },
};

const char *short_options = "i:";

static char	g_img[256];

int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'i':
			sprintf(g_img, "%s", optarg);
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
	Pdet					pdet;
	Plate					*p;
	int						ret, i;
	Mat						img;
	struct timeval			tv_begin, tv_end;

	ret = init_param(argc, argv);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	img = imread(g_img, 0);
	if (img.empty()) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	pdet.d = img.data;
	pdet.w = img.cols;
	pdet.h = img.rows;
	pdet.p = img.step;
	ret = pdet_init(&pdet);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	}

	gettimeofday(&tv_begin, NULL);
	ret = pdet_once(&pdet);
	if (ret < 0) {
		printf("ERROR: %d!\n", __LINE__);
		return -1;
	} else {
		gettimeofday(&tv_end, NULL);
		printf("Detected %d plates in %ld ms.\n", ret, 1000 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000);
		for (i = 0, p = pdet.plates; i < ret; i++, p++) {
			printf("\t%d %d\t%d %d\n", p->left, p->right, p->low, p->high);
		}
	}

	pdet_exit(&pdet);

	return 0;
}
