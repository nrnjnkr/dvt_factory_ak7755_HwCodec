/**
 * bld/gic.c
 *
 * History:
 *    2015/12/1 - Jorney(qtu@ambarella.com) created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

void irq_set_type(u32 irq, u32 type)
{
	u32 value, typemask;

	switch (type) {
	case IRQ_LEVEL_HIGH:
		type = 0;
		break;
	case IRQ_RISING_EDGE:
		type = 1;
		break;
	default:
		ASSERT(1, "Invalid gic irq type: %d\n", type);
		break;
	}

	value = readl(GICD_REG(GICD_ICFGR) + (irq / 16 ) * 4);
	typemask = 0x2 << ((irq % 16) * 2);

	if (type)
		value |= typemask;
	else
		value &= ~typemask;

	writel(GICD_REG(GICD_ICFGR) + (irq / 16 ) * 4, value);
}

void irq_disable(u32 irq)
{
	writel(GICD_REG(GICD_ICENABLER) + (irq / 32) * 4 , 1 << (irq % 32));
}

/* type switch: 0 - level_sensitive; 1 - edge_triggered */
void irq_enable(u32 irq)
{
	writel(GICD_REG(GICD_ISENABLER) + (irq / 32) * 4 , 1 << (irq % 32));
}

static int gic_dist_init(int irq_nr)
{
	int i;

	writel(GICD_REG(GICD_CTLR), 0);

	/* set type: 0: level-sensitive; 1: edge triggered */
	for (i = 32; i < irq_nr; i += 16)
		writel(GICD_REG(GICD_ICFGR) + i / 4, 0x0);

	/* set priority, default 0xa0 higher than cpu priority 0xf0 */
	for (i = 32; i < irq_nr; i += 4)
		writel(GICD_REG(GICD_IPRIORITYR) + i, 0xa0a0a0a0);

	/* disable all spi */
	for (i = 32; i < irq_nr; i += 32)
		writel(GICD_REG(GICD_ICENABLER) + i / 8, ~0);

	/* all interrupts only send to processor 0 as default */
	for (i = 32; i < irq_nr; i += 4)
		writel(GICD_REG(GICD_ITARGETSR) + i , 0x01010101);


	/* set group */
	for (i = 32; i < irq_nr; i += 32) {
		writel(GICD_REG(GICD_IGROUPR) + i / 8, 0);
	}

	/*
	 * Deal with the banked PPI and SGI interrupts:
	 * - disable all PPI interrupts
	 * - enable all SGI interrupts.
	 */
	writel(GICD_REG(GICD_ICENABLER), 0xffff0000);
	writel(GICD_REG(GICD_ISENABLER), 0x0000ffff);

	/* Set priority for PPI and SGI interrupts */
	for (i = 0; i < 32; i += 4)
		writel(GICD_REG(GICD_IPRIORITYR) + i, 0xa0a0a0a0);

	/* Enable Grp0 Grp1 interrupts forward */
	writel(GICD_REG(GICD_CTLR), 3);

	return 0;

}

static int gic_cpu_init(void)
{
	u32 bypass = 0;

	/* 16 supported priority levels, only interrupts with higher priority
	 * than the value in this register are signaled to the processor */
	writel(GICC_REG(GICC_PMR), 0xf0);

	/* TODO When the signaling of FIQs or IRQs by the CPU interface is disabled,
	 * Bypass signal is not signaled to the processor */
	bypass = readl(GICC_REG(GICC_CTRL));
	bypass &= 0x1e0;
	bypass |= 0x3;
	writel(GICC_REG(GICC_CTRL), bypass);

	return 0;
}

void irq_init(void)
{
	int irq_nr;

	disable_interrupts();

	/* Get interrupts nr supported */
	irq_nr = readl(GICD_REG(GICD_TYPER)) & 0x1f;
	irq_nr = ((irq_nr + 1) * 32);
	if (irq_nr > 1020)
		irq_nr = 1020;


	/* Distributor init (SPI)*/
	gic_dist_init(irq_nr);

	/* CPU interface init (SGI & PPI) */
	gic_cpu_init();

	enable_interrupts();
}

void irq_handler(void)
{
	u32 intack = readl(GICC_REG(GICC_IAR));
	u32 irq = intack & 0x3ff;

	writel(GICC_REG(GICC_EOIR), intack);

	if (irq < 16)
		printf("\n Warning: Unexpected SGI interrupt(%d) ...\n", irq);
	else if (irq < 31)
		printf("\n Warning: Unexpected PPI interrupt(%d) ...\n", irq);
	else if (irq < NR_IRQS && irq_manager[irq].handler)
		irq_manager[irq].handler(irq_manager[irq].data);
	else if (irq < 1020)
		printf("\n Warning: Unexpected SPI interrupt(%d) ...\n", irq);
}

void master_cpu_gic_setup(void)
{
#ifdef CONFIG_ARCH_CV1
	int irq_nr, i;

	irq_nr = readl(GICD_REG(GICD_TYPER)) & 0x1f;
	irq_nr = ((irq_nr + 1) * 32);
	if (irq_nr > 1020)
		irq_nr = 1020;

	/* Configuring all interrupts as Group 1*/
	for (i = 0; i < irq_nr; i += 32)
		writel(GICD_REG(GICD_IGROUPR) + i / 8, ~0);

	get_permission();
#endif
}
