/**
 * ambhw/nand.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __AMBHW_NAND_H__
#define __AMBHW_NAND_H__

#include <ambhw/chip.h>

#ifdef CONFIG_NAND_COMBO_CONTROLLER
#include <ambhw/nand_combo.h>
#else
#include <ambhw/nand_legacy.h>
#endif

struct nand_ns_info {
	u32	nand_id;
	u8	id5;
};

static const struct nand_ns_info ns_nand_ids[] = {
	{0xC2F18095, 0x02},
	{0xC2DA9095, 0x06},
	{0xC2DA9095, 0x56},
	{0xC2D3D195, 0x5A},
	{0xECDC1095, 0x56},
};

#define FLASH_TIMING_MIN(x, offs) flash_timing(0, x, offs)
#define FLASH_TIMING_MAX(x, offs) flash_timing(1, x, offs)

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align)	((size) & ~((align) - 1))
#endif

extern u32 get_nand_freq_hz(void);

static inline int flash_timing(int minmax, int val, int offs)
{
	u32 clk, x;
	int n, r;

	val = (val >> offs) & 0xff;

	/* to avoid overflow, divid clk by 1000000 first */
	clk = get_nand_freq_hz() / 1000000;

	x = val * clk;
	n = x / 1000;
	r = x % 1000;

	if (r != 0)
		n++;

	if (minmax)
		n--;

	return (n < 1 ? 1 : n) << offs;
}

#endif

