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
#include <bldfunc.h>
#include <irq.h>
#include <ambhw/timer.h>
#include <pandora.h>

unsigned long pandora_initialise_stack(struct pandora_task *t)
{
	int i;
	unsigned long *stack = (unsigned long *)t->stack;
	unsigned long bayonetta = 0x5a5a5a5a5a5a5a00;

	for (i = 30; i >= 0; i--){
		if (i == 0) {
			*stack = (unsigned long)t->parameter;	/* X0 */
		} else {
			*stack = bayonetta | i;			/* X1 ~ X30 */
		}
		stack--;
	}
	*stack = (unsigned long)t->entry;			/* ELR	*/
	stack --;
#ifdef CONFIG_AARCH64_TRUSTZONE
	*stack = 0x20000304;					/* SPSR	*/
#else
	*stack = 0x2000030C;					/* SPSR	*/
#endif
	stack--;
	*stack = (unsigned long)t->stack;			/* SP_EL0 */

	return (unsigned long)stack;
}

void arm_cpu_relax(void)
{
	__asm__ volatile("svc 0":::);
}


