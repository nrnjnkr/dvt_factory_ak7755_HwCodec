/**
 * bld/cortex.c
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

#include <bldfunc.h>
#include <irq.h>
#include <ambhw/cache.h>
#include <ambhw/cortex.h>
#include <ambhw/nand.h>
#include <eth/network.h>

#if defined(__aarch64__)
void get_permission(void)
{
    writel(0xf000000c, 1);
    writel(0xf1000008, 3);
    writel(0xf100000c, 3);
    writel(0xf1000010, 3);
    writel(0xf1000014, 3);
}
int bld_boot_secondary_cortex(void)
{
#ifndef CONFIG_AARCH64_TRUSTZONE
	int cpuboot = 0;

	cpuboot = readl(CORTEX_RESET_REG);

	/* core 1: clear bit 6 */
	cpuboot &= (~(1 << 6));

	/* core 2: clear bit 18 */
	cpuboot &= (~(1 << 18));

	/* core 3: clear bit 19 */
	cpuboot &= (~(1 << 19));

	writel(CORTEX_RESET_REG, cpuboot);
#endif
	return 0;
}

#else

int bld_boot_secondary_cortex(void)
{
	u32 cortex_ctl;
#if defined(CHIP_FIX_2NDCORTEX_BOOT)
	u32 fio_ctr;
	u32 sysconfig;
	u32 nand_boot;

	/* Enter random read mode */
	fio_ctr = readl(FIO_CTR_REG);
	writel(FIO_CTR_REG, (fio_ctr | FIO_CTR_RR));

	sysconfig = readl(SYS_CONFIG_REG);
	nand_boot = ((sysconfig & ~POC_BOOT_FROM_MASK) | 0x00000010);
	writel(SYS_CONFIG_REG, nand_boot);

	writel(FIO_4K_REG(0x00), 0xE59F0000);	//ldr	r0, [pc, #0]
	writel(FIO_4K_REG(0x04), 0xE12FFF10);	//bx	r0
	writel(FIO_4K_REG(0x08), 0x00000000);	//secondary cortex start from 0x00000000
	writel(FIO_4K_REG(0x0C), 0xEAFFFFFB);	//b	start
	rct_timer_dly_ms(1);
#endif
	/* FIXME: maybe the delay functions can be removed. */

	/* reset secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_RESET(0x2);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate on clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate off clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_CLOCK(0x2);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* secondary cortex core exit reset */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_RESET(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate on clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

#if defined(CHIP_FIX_2NDCORTEX_BOOT)
	writel(SYS_CONFIG_REG, sysconfig);
	/* Exit random read mode */
	writel(FIO_CTR_REG, fio_ctr);
	rct_timer_dly_ms(1);
#endif

	return 0;
}

#endif /* __aarch64__ */

