/**
 * pca953x.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 * Modify: XianqingZheng <xqzheng@ambarella.com>
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

#include <bldfunc.h>
#include <ambhw/idc.h>
#include <peripheral.h>

void pca9539_set_gpio(int i2c_id, u32 id, u32 set)
{
	u8 pca9539_adds = 0xE8;
	u8 pca9539_cfg_adds;
	u8 pca9539_out_adds;
	u8 pca9539_shift;
	u8 reg_val;

	if (id < 8) {
		pca9539_cfg_adds = 0x06;
		pca9539_out_adds = 0x02;
		pca9539_shift = id;
	} else if (id < 16) {
		pca9539_cfg_adds = 0x07;
		pca9539_out_adds = 0x03;
		pca9539_shift = (id - 8);
	} else {
		printf("pca953x: Invalid gpio id %d\n", id);
		return;
	}

	reg_val = idc_bld_read_8_8(i2c_id,
		pca9539_adds, pca9539_cfg_adds);
	reg_val &= ~(0x1 << pca9539_shift);
	idc_bld_write_8_8(i2c_id,
		pca9539_adds, pca9539_cfg_adds, reg_val);

	reg_val = idc_bld_read_8_8(i2c_id,
		pca9539_adds, pca9539_out_adds);
	if (set) {
		reg_val |= (0x1 << pca9539_shift);
	} else {
		reg_val &= ~(0x1 << pca9539_shift);
	}
	idc_bld_write_8_8(i2c_id,
		pca9539_adds, pca9539_out_adds, reg_val);
}

void pca9539_direction_input(int i2c_id, u32 id)
{
	u8 pca9539_adds = 0xE8;
	u8 pca9539_cfg_adds;
	u8 pca9539_shift;
	u8 reg_val;

	if (id < 8) {
		pca9539_cfg_adds = 0x06;
		pca9539_shift = id;
	} else if (id < 16) {
		pca9539_cfg_adds = 0x07;
		pca9539_shift = (id - 8);
	} else {
		printf("pca953x: Invalid gpio id %d\n", id);
		return;
	}

	reg_val = idc_bld_read_8_8(i2c_id,
		pca9539_adds, pca9539_cfg_adds);
	reg_val |= (0x1 << pca9539_shift);
	idc_bld_write_8_8(i2c_id,
		pca9539_adds, pca9539_cfg_adds, reg_val);
}

