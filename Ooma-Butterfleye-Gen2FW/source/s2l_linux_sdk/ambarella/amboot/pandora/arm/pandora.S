/**
 * pandora/arm/pandora.S
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

#include <arm/context_macro.S>

.global __pandora_sche_start
__pandora_sche_start:
	PANDORA_RESTORE_CONTEXT

.global __pandora_yield
__pandora_yield:
	mov	r1, #0
	str	r1, [r0]
	ldmia	sp, {r0 - lr}^
	nop
	ldr	lr, [sp, #60]
	add	sp, sp, #72

	PANDORA_SAVE_CONTEXT
	bl pandora_task_switch
	PANDORA_RESTORE_CONTEXT
	b	.

.global __pandora_swi
__pandora_swi:
	/*
	 * FIXME: when 'PC = SWI 0', it will trigger an Software exception,
	 * and the 'LR = PC + 4'. So the return addr should be LR. However,
	 * it's not the same to irq exception which 'LR = PC + 4', and the
	 * PC is not finish being excuted, so the return address is the LR - 4,
	 * points to not excuted PC.
	 *
	 * 'add lr, lr, #4' is a workround over here, just for compatibility
	 * with 'subs pc, lr, #4'.
	 *
	 */
	add	lr, lr, #4

	PANDORA_SAVE_CONTEXT
	bl pandora_task_switch
	PANDORA_RESTORE_CONTEXT

.global __mutex_atomic_set
__mutex_atomic_set:
	push	{r0-r2}
	mov	r1, #1
1:	ldr	r2, [r0]
	cmp	r2, r1
	bne	__lock
	swi	#0
	b	1b
__lock:
	cpsid	i
	str	r1, [r0]
	cpsie	i
	pop	{r0-r2}
	mov	pc, lr

.global __mutex_atomic_clr
__mutex_atomic_clr:
	push	{r0-r1}
	mov	r1, #0
	cpsid	i
	str	r1, [r0]
	cpsie	i
	pop	{r0-r1}
	mov	pc, lr

