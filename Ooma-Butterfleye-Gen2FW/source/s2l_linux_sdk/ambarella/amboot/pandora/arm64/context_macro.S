/**
 * pandora/arm64/context_macro.S

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

#ifndef __CONTEXT_ASM__
#define __CONTEXT_ASM__

#include <macro.S>

/*
 *	^ ------ ^
 *	| sp_el0 |
 *	* ------ *
 *	|  spsr  |
 *	* ------ *
 *	| elr_elx|
 *	* ------ *
 *	|   x0   |
 *	* ------ *
 *	|   x1   |
 *	* ------ *
 *	|  ....  |
 *	* ------ *
 *	|   x30  |
 *	* ------ *
 */

.macro	PANDORA_SAVE_CONTEXT

	stp	x27, x28, [sp, #-16]!
	mrs	x28, sp_el0
	stp	x29, x30, [x28, #-16]!
	mov	x30, x28
	ldp	x27, x28, [sp], #16

	stp	x27, x28, [x30, #-16]!
	stp	x25, x26, [x30, #-16]!
	stp	x23, x24, [x30, #-16]!
	stp	x21, x22, [x30, #-16]!
	stp	x19, x20, [x30, #-16]!
	stp	x17, x18, [x30, #-16]!
	stp	x15, x16, [x30, #-16]!
	stp	x13, x14, [x30, #-16]!
	stp	x11, x12, [x30, #-16]!
	stp	x9, x10, [x30, #-16]!
	stp	x7, x8, [x30, #-16]!
	stp	x5, x6, [x30, #-16]!
	stp	x3, x4, [x30, #-16]!
	stp	x1, x2, [x30, #-16]!

	armv8_switch_el x1, 1f, 2f, 3f
1:	mrs	x1, elr_el1
	mrs	x2, spsr_el1
	mrs	x3, sp_el0
	b	0f
2:	b	.
3:	mrs	x1, elr_el3
	mrs	x2, spsr_el3
	mrs	x3, sp_el0
0:	stp	x1, x0, [x30, #-16]!
	stp	x3, x2, [x30, #-16]!

	ldr	x0, current_task
	str	x30, [x0]
.endm

.macro	PANDORA_RESTORE_CONTEXT
	ldr	x0, current_task
	ldr	x30, [x0]

	ldp	x3, x2, [x30], #16
	ldp	x4, x0, [x30], #16
	armv8_switch_el x1, 1f, 2f, 3f
1:	msr	elr_el1, x4
	msr	spsr_el1, x2
	msr	sp_el0, x3
	b	0f
2:	b	.
3:	msr	elr_el3, x4
	msr	spsr_el3, x2
	msr	sp_el0, x3
0:	ldp	x1, x2, [x30], #16
	ldp	x3, x4, [x30], #16
	ldp	x5, x6, [x30], #16
	ldp	x7, x8, [x30], #16
	ldp	x9, x10, [x30], #16
	ldp	x11, x12, [x30], #16
	ldp	x13, x14, [x30], #16
	ldp	x15, x16, [x30], #16
	ldp	x17, x18, [x30], #16
	ldp	x19, x20, [x30], #16
	ldp	x21, x22, [x30], #16
	ldp	x23, x24, [x30], #16
	ldp	x25, x26, [x30], #16
	ldp	x27, x28, [x30], #16

	stp	x27, x28, [sp, #-16]!
	mov	x28, x30
	ldp	x29, x30, [x28], #16
	ldp	x27, x28, [sp], #16

.endm

#endif
