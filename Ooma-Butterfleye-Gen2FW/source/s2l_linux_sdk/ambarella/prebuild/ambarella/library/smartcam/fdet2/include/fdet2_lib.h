/*******************************************************************************
 *  fdet2_lib.h
 *
 * History:
 *    2017/02/2 - [Richard Ren] Create
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
#ifndef	_AMBA_FDET_H
#define  _AMBA_FDET_H


#include<stdio.h>

typedef struct _ROI_t{
	int x;
	int y;
	int w;
	int h;
}t_roi;

typedef struct winsz_t{
	int w;
	int h;
}t_winsz;



typedef struct _fdet_rect_t{
	int y;
	int x;
	int h;
	int w;
	int v;
	double sc;
	double sl;
}t_fdet_rect;

#ifdef __cplusplus
extern "C" {
#endif

int ini_amba_fdet(int w, int  h,int  p,double s );
int do_amba_fdet(unsigned char* d, t_fdet_rect* frr,int mxn, int mrh, double ratt,double mslo);

#ifdef __cplusplus
}
#endif

#endif
