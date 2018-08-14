/**
 * bld/vic.c
 *
 * Vector interrupt controller related utilities.
 *
 * History:
 *    2005/07/26 - [Charles Chiou] created file
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

#define IRQ_TO_BANK(irq)	((irq) >> 5)
#define IRQ_TO_OFFSET(irq)	((irq) & 0x1f)

/* ==========================================================================*/

void irq_set_type(u32 irq, u32 type)
{
	u32 bank = IRQ_TO_BANK(irq);
	u32 offset = IRQ_TO_OFFSET(irq);

	switch (type) {
	case IRQ_RISING_EDGE:
		writel(VIC_REG(bank, VIC_INT_SENSE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_BOTHEDGE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_EVT_INT_OFFSET), offset);
		break;
	case IRQ_FALLING_EDGE:
		writel(VIC_REG(bank, VIC_INT_SENSE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_BOTHEDGE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_EVT_CLR_INT_OFFSET), offset);
		break;
	case IRQ_BOTH_EDGES:
		writel(VIC_REG(bank, VIC_INT_SENSE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_BOTHEDGE_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_EVT_CLR_INT_OFFSET), offset);
		break;
	case IRQ_LEVEL_HIGH:
		writel(VIC_REG(bank, VIC_INT_SENSE_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_BOTHEDGE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_EVT_INT_OFFSET), offset);
		break;
	case IRQ_LEVEL_LOW:
		writel(VIC_REG(bank, VIC_INT_SENSE_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_BOTHEDGE_CLR_INT_OFFSET), offset);
		writel(VIC_REG(bank, VIC_INT_EVT_CLR_INT_OFFSET), offset);
		break;
	default:
		BUG_ON(1);
		break;
	}
}

void irq_enable(u32 irq)
{
	u32 bank = IRQ_TO_BANK(irq);
	u32 offset = IRQ_TO_OFFSET(irq);

	writel(VIC_REG(bank, VIC_INT_EN_INT_OFFSET), offset);
}

void irq_disable(u32 irq)
{
	u32 bank = IRQ_TO_BANK(irq);
	u32 offset = IRQ_TO_OFFSET(irq);

	/* clear obsolete edge irq */
#if (VIC_SUPPORT_CPU_OFFLOAD == 1)
	writel(VIC_REG(bank, VIC_EDGE_CLR_OFFSET), 0x1 << offset);
#else
	writel(VIC_REG(bank, VIC_INT_EDGE_CLR_OFFSET), offset);
#endif

	writel(VIC_REG(bank, VIC_INT_EN_CLR_INT_OFFSET), offset);
}

void irq_init(void)
{
	int i;

	disable_interrupts();

	/*
	 * Set VIC sense and event type for each entry
	 */
	for (i = 0; i < VIC_INSTANCES; i++) {
		writel(VIC_REG(i, VIC_SENSE_OFFSET), 0x00000000);
		writel(VIC_REG(i, VIC_BOTHEDGE_OFFSET), 0x00000000);
		writel(VIC_REG(i, VIC_EVENT_OFFSET), 0x00000000);
		/* Disable all IRQ */
		writel(VIC_REG(i, VIC_INT_SEL_OFFSET), 0x00000000);
		writel(VIC_REG(i, VIC_INTEN_OFFSET), 0x00000000);
		writel(VIC_REG(i, VIC_INTEN_CLR_OFFSET), 0xffffffff);
		writel(VIC_REG(i, VIC_EDGE_CLR_OFFSET), 0xffffffff);
		writel(VIC_REG(i, VIC_INT_PTR0_OFFSET), 0xffffffff);
		writel(VIC_REG(i, VIC_INT_PTR1_OFFSET), 0x00000000);
	}

	enable_interrupts();
}

void irq_handler(void)
{
	u32 irq, irq_sta;
#if (VIC_SUPPORT_CPU_OFFLOAD == 1)
	u32 i;

	for (i = 0; i < VIC_INSTANCES; i++) {
		do {
			irq = readl(VIC_REG(i, VIC_INT_PENDING_OFFSET));
			if (irq == 0) {
				irq_sta = readl(VIC_REG(i, VIC_IRQ_STA_OFFSET));
				if ((irq_sta & 0x1) == 0)
					break;
			}

			irq += i * NR_VIC_IRQ_SIZE;

			if (irq >= NR_IRQS)
				break;

			/* ack and disable the irq */
			irq_disable(irq);

			ASSERT(!irq_manager[irq].handler, "irq = %d\n", irq);
			irq_manager[irq].handler(irq_manager[irq].data);

			irq_enable(irq);

		} while (1);
	}
#else
	do {
		irq = readl(AHBSP_PRI_IRQ_C0_REG);
		if (irq == VIC_NULL_PRI_IRQ_VAL)
			break;
		else if (irq == 0) {
			irq_sta = readl(VIC_REG(0, VIC_IRQ_STA_OFFSET));
			if ((irq_sta & 0x1) == 0)
				break;
		} else if (irq >= NR_IRQS)
			break;

		/* ack and disable the irq */
		irq_disable(irq);

		ASSERT(!irq_manager[irq].handler, "irq = %d\n", irq);
		irq_manager[irq].handler(irq_manager[irq].data);

		irq_enable(irq);

	} while (1);
#endif
}

