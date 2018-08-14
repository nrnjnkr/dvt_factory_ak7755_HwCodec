/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
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
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

static int			v_fb;
static struct fb_fix_screeninfo	v_finfo;
static struct fb_var_screeninfo	v_vinfo;
static unsigned short		*v_mem = NULL;

int open_fb(void)
{
	int		ret;

	v_fb = open("/dev/fb0", O_RDWR);
	if (v_fb < 0) {
		ret = -1;
		goto open_fb_exit;
	}

	ret = ioctl(v_fb, FBIOGET_FSCREENINFO, &v_finfo);
	    if (ret < 0) {
			ret = -1;
			goto open_fb_exit;
	    }

	ret = ioctl(v_fb, FBIOGET_VSCREENINFO, &v_vinfo);
	if (ret < 0) {
		ret = -1;
		goto open_fb_exit;
	}

	if (v_vinfo.yres > v_vinfo.yres_virtual / 2) {
		ret = -1;
		goto open_fb_exit;
	}

	v_mem = (unsigned short *)mmap(0, v_finfo.smem_len, PROT_WRITE, MAP_SHARED, v_fb, 0);
	if (!v_mem) {
		ret = -1;
		goto open_fb_exit;
	}

	ret = ioctl(v_fb, FBIOBLANK, FB_BLANK_NORMAL);
	if (ret < 0) {
		ret = -1;
		munmap(v_mem, v_finfo.smem_len);
		goto open_fb_exit;
	}

open_fb_exit:
	if (ret && v_fb > 0) {
		close(v_fb);
	}
	return ret;
}

int blank_fb(void)
{
	if (v_fb < 0) {
		return -1;
	}

	ioctl(v_fb, FBIOBLANK, FB_BLANK_NORMAL);
	return ioctl(v_fb, FBIOPAN_DISPLAY, &v_vinfo);
}

int render_fb(unsigned char *d, unsigned int w, unsigned int h, unsigned int sp, unsigned int sx, unsigned int sy)
{
	unsigned int		i, j, k, sq;
	unsigned char		*p;
	unsigned short		r, g, b;
	unsigned short		*q;

	if (v_fb < 0 || !v_mem) {
		return -1;
	}

	if (!d || sx >= v_vinfo.xres || sy >= v_vinfo.yres) {
		return -1;
	}

	if (sx + w > v_vinfo.xres || sy + h > v_vinfo.yres) {
		return -1;
	}

	sq = v_finfo.line_length / 2;

	if (v_vinfo.yoffset) {
		q = v_mem;
	} else {
		q = v_mem + v_vinfo.yres * sq;
	}

	p = d;	
	q = q + sy * sq + sx;
	for (i = 0; i < h; i++) {
		for (j = 0, k = 0; j < w; j++, k += 3) {
			r	= p[k + 0] >> 3;
			g	= p[k + 1] >> 2;
			b	= p[k + 2] >> 3;
			
			q[j]	= (r << 11) | (g << 5) | (b << 0);
		}

		p += sp;
		q += sq;
	}

	return 0;
}

int refresh_fb(void)
{
	int			ret;
	
	if (v_vinfo.yoffset) {
		v_vinfo.yoffset	= 0;
	} else {
		v_vinfo.yoffset	= v_vinfo.yres;
	}

	ret = ioctl(v_fb, FBIOPAN_DISPLAY, &v_vinfo);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int close_fb(void)
{
	if (v_fb > 0 && v_mem) {
		munmap(v_mem, v_finfo.smem_len);
		close(v_fb);
		return 0;
	} else {
		return -1;
	}
}
