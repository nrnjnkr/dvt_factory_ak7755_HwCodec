
/**
 * irq.h
 *
 * History:
 *    2015/12/1 - Jorney(qtu@ambarella.com) created file
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
#ifndef _AMBOOT_IRQ_H_
#define _AMBOOT_IRQ_H_

#if defined(__aarch64__)
#include <ambhw/gic.h>
#else
#include <ambhw/vic.h>
#endif

/* ==========================================================================*/
/* irq type */
#define IRQ_RISING_EDGE			0
#define IRQ_FALLING_EDGE		1
#define IRQ_BOTH_EDGES			2
#define IRQ_LEVEL_LOW			3
#define IRQ_LEVEL_HIGH			4

/* ==========================================================================*/
#ifndef __ASM__

typedef void (*irq_handler_t)(void *);

typedef struct irq_manager_s
{
	u32 irq;
	irq_handler_t handler;
	void *data;
} irq_manager_t;

extern void request_irq(u32 irq, u32 type, irq_handler_t handler, void *data);
extern void irq_init(void);
extern void irq_enable(u32 line);
extern void irq_disable(u32 line);
extern void irq_set_type(u32 irq, u32 type);
extern void irq_handler(void);

#if defined(__aarch64__)

static inline void enable_interrupts(void)
{
	__asm__ __volatile__("msr daifclr, #0x3":::"memory");
}

static inline void disable_interrupts(void)
{
	__asm__ __volatile__("msr daifset, #0x3":::"memory");
}

#else

static inline void enable_interrupts(void)
{
	__asm__ volatile("cpsie i");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
}

static inline void disable_interrupts(void)
{
	__asm__ volatile("cpsid i");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
}

#endif

/* ==========================================================================*/
#endif

#endif
