/*******************************************************************************
 *  fdet_lib.h
 *
 * History:
 *    2015/03/26 - [Zhenwu Xue] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 ******************************************************************************/

#ifndef __FDET_LIB_H__
#define __FDET_LIB_H__

typedef struct {
	int		left;
	int		right;
	int		low;
	int		high;
	double  score;
	int		valid;
} fdet_rect_t;

typedef struct {
	/* Input Argument */
	int		w;
	int		h;
	int		p;

	double		s;
	int		n;

	/* Output Argument */
	fdet_rect_t	*rects;

	/* Internal Buffer */
	unsigned int	*sum0;
	unsigned int	*sum1;
	double		*square;
	unsigned char	*frame;
} fdet_data_t;

typedef struct {
	unsigned short major;
	unsigned char minor;
	unsigned char patch;
	unsigned short year;
	unsigned char month;
	unsigned char day;
} fdet_version;


typedef struct {
	unsigned int* tsum;
	int nw;
	int nh;
	fdet_data_t *d;
	int begin_row;
	int end_row;
	int step;
} fdet_context_t;


#ifdef __cplusplus
extern "C" {
#endif

int fdet_init(fdet_data_t *d, int w, int h, int p, double _scale, int min_num);
int fdet_do(fdet_data_t *d, unsigned char *in, int min_rects_per_face);

int S5L_fdet_do(fdet_data_t *d, unsigned char *in);

int fdet_exit(fdet_data_t *d);

void fdet_get_version(fdet_version *version);

#ifdef __cplusplus
}
#endif

#endif
