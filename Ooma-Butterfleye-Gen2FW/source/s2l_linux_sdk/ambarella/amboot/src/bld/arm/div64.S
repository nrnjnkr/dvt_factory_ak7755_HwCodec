 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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

.global __aeabi_ldivmod
__aeabi_ldivmod:
 	cmp		r3, #0
 	cmpeq	r2, #0
 	bne		L0
 	cmp		r1, #0
 	cmpeq	r0, #0
 	movlt	r1, #-2147483648
 	movlt	r0, #0
 	mvngt	r1, #-2147483648
 	mvngt	r0, #0
 	b		__aeabi_idiv0
L0:
 	sub		sp, sp, #8
 	push	{sp, lr}
 	bl		__gnu_ldivmod_helper
 	ldr		lr, [sp, #4]
 	add		sp, sp, #8
 	pop		{r2, r3}
 	bx		lr


.global __aeabi_uldivmod
__aeabi_uldivmod:
 	cmp		r3, #0
 	cmpeq	r2, #0
 	bne		L1
 	cmp		r1, #0
 	cmpeq	r0, #0
 	mvnne	r1, #0
 	mvnne	r0, #0
 	b		__aeabi_idiv0
L1:
 	sub		sp, sp, #8
 	push	{sp, lr}
 	bl		__gnu_uldivmod_helper
 	ldr		lr, [sp, #4]
 	add		sp, sp, #8
 	pop		{r2, r3}
 	bx		lr

__aeabi_idiv0:
 	push	{r1, lr}
 	mov		r0, #8
 	b		.
 	pop		{r1, pc}

__gnu_ldivmod_helper:
 	push	{r4, r5, r6, r7, r8, lr}
 	mov		r8, r2
 	mov		r4, r0
 	mov		r5, r1
 	mov		r6, r3
 	ldr		r7, [sp, #24]
 	bl		__divdi3
 	mul		lr, r8, r1
 	umull	r2, r3, r8, r0
 	mla		r6, r0, r6, lr
 	subs	r2, r4, r2
 	add		r3, r6, r3
 	sbc		r3, r5, r3
 	strd	r2, [r7]
 	pop		{r4, r5, r6, r7, r8, pc}

__gnu_uldivmod_helper:
 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
 	mov		r5, r3
 	mov		r6, r2
 	mov		r8, r0
 	mov		r9, r1
 	ldr		r7, [sp, #32]
 	bl		__udivdi3
 	mul		r3, r0, r5
 	umull	r4, r5, r0, r6
 	mla		r6, r6, r1, r3
 	subs	r4, r8, r4
 	add		r5, r6, r5
 	sbc		r5, r9, r5
 	strd	r4, [r7]
 	pop		{r4, r5, r6, r7, r8, r9, sl, pc}

__divdi3:
 	cmp		r1, #0
 	push	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
 	movge	ip, #0
 	blt		L10
L2:
 	cmp		r3, #0
 	movge	r8, r2
 	movge	r9, r3
 	blt		L9
L3:
 	cmp		r1, r9
 	mov		r4, r0
 	cmpeq	r0, r8
 	mov		r5, r1
 	movcc	r8, #0
 	movcc	r9, #0
 	bcc		L8
 	cmp		r9, #0
 	clzeq	r3, r8
 	clzne	r2, r9
 	addeq	r2, r3, #32
 	cmp		r1, #0
 	clzeq	r3, r0
 	clzne	r3, r1
 	addeq	r3, r3, #32
 	rsb		r3, r3, r2
 	lsl		r7, r9, r3
 	sub		sl, r3, #32
 	rsb		lr, r3, #32
 	lsl		r6, r8, r3
 	orr		r7, r7, r8, lsl sl
 	mov		r2, r3
 	orr		r7, r7, r8, lsr lr
 	cmp		r1, r7
 	cmpeq	r0, r6
 	movcc	r8, #0
 	movcc	r9, #0
 	bcc		L4
 	mov		fp, #1
 	subs	r0, r0, r6
 	lsl		r9, fp, sl
 	sbc		r1, r1, r7
 	lsl		r8, fp, r3
 	mov		r4, r0
 	orr		r9, r9, fp, lsr lr
 	mov		r5, r1
L4:
 	cmp		r3, #0
 	beq		L8
 	lsrs	r7, r7, #1
 	rrx		r6, r6
 	b		L6
L5:
 	subs	r4, r4, r6
 	sbc		r5, r5, r7
 	adds	r4, r4, r4
 	adc		r5, r5, r5
 	adds	r4, r4, #1
 	adc		r5, r5, #0
 	subs	r3, r3, #1
 	beq		L7
L6:
 	cmp		r7, r5
 	cmpeq	r6, r4
 	bls		L5
 	adds	r4, r4, r4
 	adc		r5, r5, r5
 	subs	r3, r3, #1
 	bne		L6
L7:
 	lsr		r3, r4, r2
 	rsb		r1, r2, #32
 	lsr		lr, r5, r2
 	sub		r0, r2, #32
 	orr		r3, r3, r5, lsl r1
 	adds	r8, r8, r4
 	lsl		r7, lr, r2
 	adc		r9, r9, r5
 	orr		r3, r3, r5, lsr r0
 	orr		r7, r7, r3, lsl r0
 	lsl		r6, r3, r2
 	orr		r7, r7, r3, lsr r1
 	subs	r8, r8, r6
 	sbc		r9, r9, r7
L8:
 	adds	r0, ip, #0
 	mov		r1, #0
 	movne	r0, #1
 	rsbs	r2, r0, #0
 	rsc		r3, r1, #0
 	eor		r8, r8, r2
 	eor		r9, r9, r3
 	adds	r0, r0, r8
 	adc		r1, r1, r9
 	pop		{r4, r5, r6, r7, r8, r9, sl, fp, pc}
L9:
 	rsbs	r8, r2, #0
 	mvn		ip, ip
 	rsc		r9, r3, #0
 	b		L3
L10:
 	rsbs	r0, r0, #0
 	mvn		ip, #0
 	rsc		r1, r1, #0
 	b		L2

__udivdi3:
 	push	{r4, r5, r6, r7, r8, lr}
 	mov		r5, r1
 	mov		r4, r0
 	cmp		r5, r3
 	cmpeq	r4, r2
 	mov		r0, r2
 	mov		r1, r3
 	bcc		L15
 	cmp		r3, #0
 	clzeq	r3, r0
 	clzne	r2, r3
 	addeq	r2, r3, #32
 	cmp		r5, #0
 	clzeq	r3, r4
 	clzne	r3, r5
 	addeq	r3, r3, #32
 	rsb		r3, r3, r2
 	lsl		r7, r1, r3
 	sub		lr, r3, #32
 	rsb		r2, r3, #32
 	lsl		r6, r0, r3
 	orr		r7, r7, r0, lsl lr
 	mov		ip, r3
 	orr		r7, r7, r0, lsr r2
 	cmp		r5, r7
 	cmpeq	r4, r6
 	movcc	r0, #0
 	movcc	r1, #0
 	bcc		L11
 	mov		r8, #1
 	subs	r4, r4, r6
 	lsl		r1, r8, lr
 	lsl		r0, r8, r3
 	orr		r1, r1, r8, lsr r2
 	sbc		r5, r5, r7
L11:
 	cmp		r3, #0
 	popeq	{r4, r5, r6, r7, r8, pc}
 	lsrs	r7, r7, #1
 	rrx		r6, r6
 	b		L13
L12:
 	subs	r4, r4, r6
 	sbc		r5, r5, r7
 	adds	r4, r4, r4
 	adc		r5, r5, r5
 	adds	r4, r4, #1
 	adc		r5, r5, #0
 	subs	r3, r3, #1
 	beq		L14
L13:
 	cmp		r7, r5
 	cmpeq	r6, r4
 	bls		L12
 	adds	r4, r4, r4
 	adc		r5, r5, r5
 	subs	r3, r3, #1
 	bne		L13
L14:
 	lsr		lr, r4, ip
 	rsb		r6, ip, #32
 	lsr		r8, r5, ip
 	sub		r7, ip, #32
 	orr		lr, lr, r5, lsl r6
 	adds	r0, r0, r4
 	lsl		r3, r8, ip
 	adc		r1, r1, r5
 	orr		lr, lr, r5, lsr r7
 	orr		r3, r3, lr, lsl r7
 	lsl		r2, lr, ip
 	orr		r3, r3, lr, lsr r6
 	subs	r0, r0, r2
 	sbc		r1, r1, r3
 	pop		{r4, r5, r6, r7, r8, pc}
L15:
 	mov		r0, #0
 	mov		r1, #0
 	pop		{r4, r5, r6, r7, r8, pc}

