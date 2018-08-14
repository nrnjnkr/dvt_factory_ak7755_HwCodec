/**
 * pandora/arm64/pandora.S
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

#include <arm64/context_macro.S>

.global __pandora_sche_start
__pandora_sche_start:
	PANDORA_RESTORE_CONTEXT
	eret

.global __pandora_yield
__pandora_yield:
	PANDORA_SAVE_CONTEXT
	bl pandora_task_switch
	PANDORA_RESTORE_CONTEXT
	eret

.global __pandora_svc
__pandora_svc:
	PANDORA_SAVE_CONTEXT
	bl pandora_task_switch
	PANDORA_RESTORE_CONTEXT
	eret

.global __mutex_atomic_set
__mutex_atomic_set:
1:	ldr	w1, [x0]
	cbz	w1, __lock
	svc	#0
	b	1b
__lock:
	msr	daifset, #3
	mov	w1, #1
	str	w1, [x0]
	msr	daifclr, #3
	ret

.global __mutex_atomic_clr
__mutex_atomic_clr:
	msr	daifset, #3
	mov	w1, #0
	str	w1, [x0]
	msr	daifclr, #3
	ret

