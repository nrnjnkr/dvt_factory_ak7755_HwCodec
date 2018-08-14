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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <basetypes.h>
#include "iav_ioctl.h"

static int		v_iav = -1;
static unsigned char	*v_dspmem = NULL;
static int		v_dspsize = 0;

int init_iav(void)
{
	int			ret;
	struct iav_querybuf	qb;

	v_iav = open("/dev/iav", O_RDWR);
	if (v_iav <= 0) {
		return -1;
	}

	qb.buf	= IAV_BUFFER_DSP;
	ret	= ioctl(v_iav, IAV_IOC_QUERY_BUF, &qb);
	if (ret < 0) {
		return -1;
	}

	v_dspsize = qb.length;
	v_dspmem = (unsigned char *)mmap(NULL, v_dspsize, PROT_READ, MAP_SHARED, v_iav, qb.offset);
	if (v_dspmem == MAP_FAILED) {
		return -1;
	}

	return 0;
}

int get_main_buf(main_buf_t *buf)
{
	int				ret;
	struct iav_querydesc		qd;

	qd.qid = IAV_DESC_ME1;
	qd.arg.me1.buf_id = (enum iav_srcbuf_id)0;
	qd.arg.me1.flag &= ~IAV_BUFCAP_NONBLOCK;
	ret = ioctl(v_iav, IAV_IOC_QUERY_DESC, &qd);
	if (ret < 0) {
		return -1;
	} else {
		buf->me1_w = qd.arg.me1.width;
		buf->me1_h = qd.arg.me1.height;
		buf->me1_p = qd.arg.me1.pitch;
		buf->me1_y = v_dspmem + qd.arg.me1.data_addr_offset;
	}

	qd.qid = IAV_DESC_YUV;
	qd.arg.yuv.buf_id = (enum iav_srcbuf_id)0;
	qd.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
	ret = ioctl(v_iav, IAV_IOC_QUERY_DESC, &qd);
	if (ret < 0) {
		return -1;
	}
	buf->yuv_w = qd.arg.yuv.width;
	buf->yuv_h = qd.arg.yuv.height;
	buf->yuv_p = qd.arg.yuv.pitch;
	buf->yuv_y = v_dspmem + qd.arg.yuv.y_addr_offset;
	buf->yuv_uv = v_dspmem + qd.arg.yuv.uv_addr_offset;

	return 0;
}

int exit_iav(void)
{
	int		ret;

	if (v_dspmem && v_dspsize > 0) {
		ret = munmap(v_dspmem, v_dspsize);
		if (ret < 0) {
			return -1;
		}
		v_dspmem = NULL;
		v_dspsize = 0;
	}

	close(v_iav);

	return 0;
}
