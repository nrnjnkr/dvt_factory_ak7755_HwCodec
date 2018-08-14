
/**
 * bld/arm/_udivsi3.S
 *
 * History:
 *    2016/3/4 - [Jorney] porting from u-boot. Thanks Richard Earnshaw.
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
dividend	.req	r0
divisor		.req	r1
result		.req	r2
curbit		.req	r3
	.text
	.globl	 __udivsi3
	.type	__udivsi3 ,function
	.globl	__aeabi_uidiv
	.type	__aeabi_uidiv ,function
	.align	0
 __udivsi3:
 __aeabi_uidiv:
	cmp	divisor, #0
	beq	Ldiv0
	mov	curbit, #1
	mov	result, #0
	cmp	dividend, divisor
	bcc	Lgot_result
Loop1:
	@ Unless the divisor is very big, shift it up in multiples of
	@ four bits, since this is the amount of unwinding in the main
	@ division loop.  Continue shifting until the divisor is
	@ larger than the dividend.
	cmp	divisor, #0x10000000
	cmpcc	divisor, dividend
	movcc	divisor, divisor, lsl #4
	movcc	curbit, curbit, lsl #4
	bcc	Loop1
Lbignum:
	@ For very big divisors, we must shift it a bit at a time, or
	@ we will be in danger of overflowing.
	cmp	divisor, #0x80000000
	cmpcc	divisor, dividend
	movcc	divisor, divisor, lsl #1
	movcc	curbit, curbit, lsl #1
	bcc	Lbignum
Loop3:
	@ Test for possible subtractions, and note which bits
	@ are done in the result.  On the final pass, this may subtract
	@ too much from the dividend, but the result will be ok, since the
	@ "bit" will have been shifted out at the bottom.
	cmp	dividend, divisor
	subcs	dividend, dividend, divisor
	orrcs	result, result, curbit
	cmp	dividend, divisor, lsr #1
	subcs	dividend, dividend, divisor, lsr #1
	orrcs	result, result, curbit, lsr #1
	cmp	dividend, divisor, lsr #2
	subcs	dividend, dividend, divisor, lsr #2
	orrcs	result, result, curbit, lsr #2
	cmp	dividend, divisor, lsr #3
	subcs	dividend, dividend, divisor, lsr #3
	orrcs	result, result, curbit, lsr #3
	cmp	dividend, #0			@ Early termination?
	movnes	curbit, curbit, lsr #4		@ No, any more bits to do?
	movne	divisor, divisor, lsr #4
	bne	Loop3
Lgot_result:
	mov	r0, result
	mov	pc, lr
Ldiv0:
	str	lr, [sp, #-4]!
	bl	 __div0       (PLT)
	mov	r0, #0			@ about as wrong as it could be
	ldmia	sp!, {pc}
	.size  __udivsi3       , . -  __udivsi3

.globl __aeabi_uidivmod
__aeabi_uidivmod:

	stmfd	sp!, {r0, r1, ip, lr}
	bl	__aeabi_uidiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr

.globl __aeabi_idivmod
__aeabi_idivmod:

	stmfd	sp!, {r0, r1, ip, lr}
	bl	__aeabi_idiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr


.macro ARM_DIV_BODY dividend, divisor, result, curbit


	@ Initially shift the divisor left 3 bits if possible,
	@ set curbit accordingly.  This allows for curbit to be located
	@ at the left end of each 4 bit nibbles in the division loop
	@ to save one loop in most cases.
	tst	\divisor, #0xe0000000
	moveq	\divisor, \divisor, lsl #3
	moveq	\curbit, #8
	movne	\curbit, #1

	@ Unless the divisor is very big, shift it up in multiples of
	@ four bits, since this is the amount of unwinding in the main
	@ division loop.  Continue shifting until the divisor is
	@ larger than the dividend.
1:	cmp	\divisor, #0x10000000
	cmplo	\divisor, \dividend
	movlo	\divisor, \divisor, lsl #4
	movlo	\curbit, \curbit, lsl #4
	blo	1b

	@ For very big divisors, we must shift it a bit at a time, or
	@ we will be in danger of overflowing.
1:	cmp	\divisor, #0x80000000
	cmplo	\divisor, \dividend
	movlo	\divisor, \divisor, lsl #1
	movlo	\curbit, \curbit, lsl #1
	blo	1b

	mov	\result, #0


	@ Division loop
1:	cmp	\dividend, \divisor
	subhs	\dividend, \dividend, \divisor
	orrhs	\result,   \result,   \curbit
	cmp	\dividend, \divisor,  lsr #1
	subhs	\dividend, \dividend, \divisor, lsr #1
	orrhs	\result,   \result,   \curbit,  lsr #1
	cmp	\dividend, \divisor,  lsr #2
	subhs	\dividend, \dividend, \divisor, lsr #2
	orrhs	\result,   \result,   \curbit,  lsr #2
	cmp	\dividend, \divisor,  lsr #3
	subhs	\dividend, \dividend, \divisor, lsr #3
	orrhs	\result,   \result,   \curbit,  lsr #3
	cmp	\dividend, #0			@ Early termination?
	movnes	\curbit,   \curbit,  lsr #4	@ No, any more bits to do?
	movne	\divisor,  \divisor, lsr #4
	bne	1b

.endm

.macro ARM_DIV2_ORDER divisor, order


	cmp	\divisor, #(1 << 16)
	movhs	\divisor, \divisor, lsr #16
	movhs	\order, #16
	movlo	\order, #0

	cmp	\divisor, #(1 << 8)
	movhs	\divisor, \divisor, lsr #8
	addhs	\order, \order, #8

	cmp	\divisor, #(1 << 4)
	movhs	\divisor, \divisor, lsr #4
	addhs	\order, \order, #4

	cmp	\divisor, #(1 << 2)
	addhi	\order, \order, #3
	addls	\order, \order, \divisor, lsr #1


.endm

	.align	5
.globl __divsi3
.globl __aeabi_idiv
__divsi3:
__aeabi_idiv:
	cmp	r1, #0
	eor	ip, r0, r1			@ save the sign of the result.
	beq	_Ldiv0
	rsbmi	r1, r1, #0			@ loops below use unsigned.
	subs	r2, r1, #1			@ division by 1 or -1 ?
	beq	10f
	movs	r3, r0
	rsbmi	r3, r0, #0			@ positive dividend value
	cmp	r3, r1
	bls	11f
	tst	r1, r2				@ divisor is power of 2 ?
	beq	12f

	ARM_DIV_BODY r3, r1, r0, r2

	cmp	ip, #0
	rsbmi	r0, r0, #0
	mov	pc, lr

10:	teq	ip, r0				@ same sign ?
	rsbmi	r0, r0, #0
	mov	pc, lr

11:	movlo	r0, #0
	moveq	r0, ip, asr #31
	orreq	r0, r0, #1
	mov	pc, lr

12:	ARM_DIV2_ORDER r1, r2

	cmp	ip, #0
	mov	r0, r3, lsr r2
	rsbmi	r0, r0, #0
	mov	pc, lr

_Ldiv0:

	str	lr, [sp, #-4]!
	bl	__div0
	mov	r0, #0			@ About as wrong as it could be.
	ldr	pc, [sp], #4

__div0:
	b .
