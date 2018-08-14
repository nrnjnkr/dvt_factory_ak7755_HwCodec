/**
 * include/macro.S
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * History:
 *    2015/11/26 - [Cao Rongrong] Add ARMv8 supported
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

#if defined(__CORTEXA53__)

/*
 * CLIDR_EL1: Identifies the type of caches, Read Only
 *
 * 31 30   27  24 23 21 20  18 17  15 14  12 11   9 8    6 5    3 2    0
 * ^----^----^---^-----^------^------^------^------^------^------^------^
 * |RES0|LoUU|LoC|LoUIS|Ctype7|Ctype6|Ctype5|Ctype4|Ctype3|Ctype2|Ctype1|
 *  ---------------------------------------------------------------------
 *
 *
 *
 * CSSELR_EL1: Select the current Cache Size ID Register
 *
 * 31                                                    4 3     1    0
 * ^------------------------------------------------------^--------^-----^
 * |                              RES0                    |  Level | InD |
 *  ---------------------------------------------------------------------
 *
 *
 * CCSIDR_EL1: Provide information of currently selected cache, Read Only
 *
 * 31   30   29   28	27            13 12                   3 2         0
 * ^----^----^----^----^----------------^----------------------^----------^
 * | WT | WB | RA | WA |     NumSets    |     Associativity    | Linesize |
 *  ---- ---- ---- ---- ---------------- ---------------------- ----------
 *
 * Linesize = log2(line size in bytes) - 4
 * Associativity = ways - 1
 * NumSets = sets - 1
 *
 *
 * DC ISW or DC CISW:
 *
 *  63         32 31   32-A  32-A-1    B B-1    L L-1     4 3    1   0
 * ^-------------^----------^-----------^--------^---------^------^-----^
 * |     RES0    |    Way   |    RES0   |   Set  |   RES0  | Level| RES0|
 *  --------------------------------------------------------------------
 *
 * A = log2(Associativity)
 * L = log2(Linesize)
 * S = log2(Set)
 * B = L + S
 *
 */

.macro	operate_all_cache, op, lvl, type, line, way, set, \
				wayp, wayv, cp_val, dc_val, tmp
	dsb	sy

	mov	\lvl, #0		/* lvl = level variable */

/* loop_level */
1:
	mrs	\cp_val, clidr_el1	/* read clidr_el1 to check Ctype */
	lsl	\tmp, \lvl, #1
	add	\tmp, \tmp, \lvl	/* tmp = lvl << 1 + lvl = lvl * 3 */
	lsr	\type, \cp_val, \tmp
	and	\type, \type, #7	/* Cache type */
	cbz	\type, 5f		/* return if no cache, no need to check upper layer */
	cmp	\type, #2
	b.lt	4f			/* skip if icache */

	/*---------- START Specified Level Operation -----------*/

	lsl	\tmp, \lvl, #1
	msr	csselr_el1, \tmp	/* Selects the current Cache level */
	isb				/* make sure csselr_el1 has taken effect */

	mrs	\cp_val, ccsidr_el1
	and	\line, \cp_val, #7
	add	\line, \line, #4	/* line = log2(cache line size) */
	lsr	\way, \cp_val, #3
	and	\way, \way, #0x3ff	/* way = ways - 1 */
	clz	\wayp, \way		/* wayp = way position in DC ISW/CSW/CISW */
	sub	\wayp, \wayp, #32	/* wayp = 64 - 32 - A = 32 - log2(ways) */
	lsr	\set, \cp_val, #13
	and	\set, \set, #0x7fff	/* set = sets - 1 */

/* loop_set */
2:
	mov	\wayv, \way		/* wayv = way variable */

/* loop_way */
3:
	lsl	\dc_val, \wayv, \wayp
	orr	\dc_val, \dc_val, \lvl, lsl #1
	lsl	\tmp, \set, \line
	orr	\dc_val, \dc_val, \tmp
	dc	\op, \dc_val

	subs	\wayv, \wayv, #1
	b.ge	3b			/* back to loop_way */
	subs	\set, \set, #1
	b.ge	2b			/* back to loop_set */

	/*---------- END Specified Level Operation -----------*/
4:
	add	\lvl, \lvl, #1		/* increment cache level */
	b	1b			/* back to loop_level */

5:
	mov	\tmp, #0
	msr	csselr_el1, \tmp	/* restore csselr_el1 */
	dsb	sy
	isb
.endm

.macro	invalidate_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, \
				tmp7, tmp8, tmp9
	operate_all_cache isw, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4, \
				 \tmp5, \tmp6, \tmp7, \tmp8, \tmp9
.endm

.macro	clean_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, \
				tmp7, tmp8, tmp9
	operate_all_cache csw, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4, \
				\tmp5, \tmp6, \tmp7, \tmp8, \tmp9
.endm

.macro	invcln_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, \
				tmp7, tmp8, tmp9
	operate_all_cache cisw, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4, \
				\tmp5, \tmp6, \tmp7, \tmp8, \tmp9
.endm

.macro	invalidate_all_tlb
	tlbi	alle3
	dsb	sy
.endm

.macro  clean_invalidate_dcache, ops, addr, end, tmp0, tmp1

	mrs	\tmp1, ctr_el0
	lsr	\tmp1, \tmp1, #16
	and	\tmp1, \tmp1, #0xf
	mov	\tmp0, #4
	lsl	\tmp0, \tmp0, \tmp1		/* cache line size */

	sub	\tmp1, \tmp0, #1
	bic	\addr, \addr, \tmp1
1: 	dc	\ops, \addr
	add	\addr, \addr, \tmp0
	cmp	\addr, \end
	b.lo	1b
	dsb	sy
.endm

.macro armv8_switch_el, reg, el1_label, el2_label, el3_label
	mrs		\reg, CurrentEL
	cmp		\reg, #0x4
	b.eq	\el1_label
	cmp		\reg, #0x8
	b.eq	\el2_label
	cmp		\reg, #0xc
	b.eq	\el3_label
	b		.
.endm

#else

.macro	invalidate_all_tlb, tmp0
	mov		\tmp0, #0x00
	mcr		p15, 0, \tmp0, c8, c7, 0		/* Invalidate entire unified TLB */
	mcr		p15, 0, \tmp0, c8, c6, 0		/* Invalidate entire data TLB */
	mcr		p15, 0, \tmp0, c8, c5, 0		/* Invalidate entire instruction TLB */
.endm

.macro	operate_all_cache, cr, tmp0, tmp1, tmp2, tmp3, tmp4
	mov		\tmp0, #0
	mcr		p15, 0, \tmp0, c7, c5, 6		/* Invalidate entire branch prediction array */
	mcr		p15, 0, \tmp0, c7, c5, 0		/* Invalidate entire icache */
	mcr		p15, 2, \tmp0, c0, c0, 0		/* cache size selection register, select dcache */
	mrc		p15, 1, \tmp0, c0, c0, 0		/* cache size id register */
	mov		\tmp0, \tmp0, asr #13
	movw		\tmp2, #0x0fff
	and		\tmp0, \tmp0, \tmp2
	cmp		\tmp0, #0x7f
	moveq		\tmp0, #0x1000
	beq		1f
	cmp		\tmp0, #0xff
	moveq		\tmp0, #0x2000
	movne		\tmp0, #0x4000
1:
	mov		\tmp1, #0
	mov		\tmp2, #0x40000000
	mov		\tmp3, #0x80000000
	mov		\tmp4, #0xc0000000
2:
	mcr		p15, 0, \tmp1, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp2, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp3, c7, \cr, 2		/* invalidate dcache by set / way */
	mcr		p15, 0, \tmp4, c7, \cr, 2		/* invalidate dcache by set / way */
	add		\tmp1, \tmp1, #0x20
	add		\tmp2, \tmp2, #0x20
	add		\tmp3, \tmp3, #0x20
	add		\tmp4, \tmp4, #0x20
	cmp		\tmp1, \tmp0
	bne		2b
.endm

.macro	invalidate_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c6, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
.endm

.macro	clean_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c10, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
.endm

.macro	invcln_all_cache, tmp0, tmp1, tmp2, tmp3, tmp4
	operate_all_cache c14, \tmp0, \tmp1, \tmp2, \tmp3, \tmp4
.endm

#endif

