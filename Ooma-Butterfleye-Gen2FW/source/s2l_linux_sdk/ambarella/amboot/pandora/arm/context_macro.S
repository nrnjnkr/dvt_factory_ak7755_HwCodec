/**
 * pandora/arm/context_macro.S

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


.macro	PANDORA_SAVE_CONTEXT
	stmdb	sp!, {r0}
	stmdb	sp, {sp}^
	sub	sp, sp, #4
	ldmia	sp!, {r0}
	stmdb	r0!, {lr}

	mov	lr, r0
	ldmia	sp!, {r0}
	stmdb	lr, {r0-r14}^
	sub	lr, lr, #60
	mrs	r0, spsr
	stmdb	lr!, {r0}
	ldr	r0, =current_task
	ldr	r0, [r0]
	str	lr, [r0]
.endm

.macro	PANDORA_RESTORE_CONTEXT
	ldr	r0, =current_task
	ldr	r0, [r0]
	ldr	lr, [r0]
	ldmfd	lr!, {r0}
	msr	spsr_cxsf, r0
	ldmfd	lr, {r0-r14}^
	ldr	lr, [lr, #60]
	subs	pc, lr, #0x4
.endm

#endif
