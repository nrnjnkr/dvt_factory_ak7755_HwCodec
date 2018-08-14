/**
 * pandora/arm/processor.c
 *
 * Copyright (c) 2020 Ambarella, Inc.
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

#include <amboot.h>
#include <irq.h>
#include <ambhw/timer.h>
#include <pandora.h>

unsigned long pandora_initialise_stack(struct pandora_task *t)
{
	unsigned long *stack = (unsigned long *)t->stack;

	*stack = (unsigned long)t->entry + 4;			/* R15 / PC */
	stack--;
	*stack = 0xeeeeeeee;						/* R14 / LR */
	stack--;
	*stack = (unsigned long)t->stack;			/* R13 / SP */
	stack--;
	*stack = 0xcccccccc;			/* R12 */
	stack--;
	*stack = 0xbbbbbbbb;			/* R11 */
	stack--;
	*stack = 0xaaaaaaaa;			/* R10 */
	stack--;
	*stack = 0x99999999;			/* R9 */
	stack--;
	*stack = 0x88888888;			/* R8 */
	stack--;
	*stack = 0x77777777;			/* R7 */
	stack--;
	*stack = 0x66666666;			/* R6 */
	stack--;
	*stack = 0x55555555;			/* R5 */
	stack--;
	*stack = 0x44444444;			/* R4 */
	stack--;
	*stack = 0x33333333;			/* R3 */
	stack--;
	*stack = 0x22222222;			/* R2 */
	stack--;
	*stack = 0x11111111;			/* R1 */
	stack--;
	*stack = (unsigned long)t->parameter;			/* R0 */
	stack--;

	*stack = 0x1f;					/* SPSR */

	return (unsigned long)stack;
}

void arm_cpu_relax(void)
{
	__asm__ volatile("swi 0":::);
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
}


