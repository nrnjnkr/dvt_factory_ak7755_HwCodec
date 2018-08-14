/**
 * pandora/pandora.h
 *
 * 2017-03-06	Create file by Jorney
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

#ifndef __OSPLUS__
#define __OSPLUS__

#ifdef CONFIG_PANDORA_RTOS

#define	HZ							1000
#define OS_ERR						-1
#define TSKIDLE_PRIORITY			0
#define DEFAULT_THREAD_STACK_SIZE	0x8000

static inline void local_irq_disable(void)
{
#if defined(__aarch64__)
	__asm__ volatile("msr daifset, #3");
#else
	__asm__ volatile("cpsid i");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
#endif
}

static inline void local_irq_enable(void)
{
#if defined(__aarch64__)
	__asm__ volatile("msr daifclr, #3");
#else
	__asm__ volatile("cpsie i");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
#endif
}

struct pandora_task {
	unsigned long offset;
	unsigned long stack;
	int prior;
	int block;
	char freeze;
	char name[32];
	void *parameter;
	void *entry;
	struct pandora_task *next;
	unsigned long timeout;
};

struct pandora_completion {
	unsigned long done;
	void *owner;
};

typedef	unsigned long pandora_mutex_t;
typedef	struct pandora_completion completion_t;


void timer_init(void);
void schedule_disable(void);

void pandora_schedule_start(void);
int pandora_thread_init(void);
int pandora_thread_create(const char *name, int prior,
		void *call, void *arg);
void local_irq_disable(void);
void local_irq_enable(void);
void cpu_relax(void);
void yield(void);
void mutex_init(pandora_mutex_t *mutex);
void mutex_lock(pandora_mutex_t *mutex);
void mutex_unlock(pandora_mutex_t *mutex);

void __pandora_sche_start(void);
void __mutex_atomic_clr(void *);
void __mutex_atomic_set(void *);

void init_completion(completion_t *event);
void complete(completion_t *event);
void complete_isr(completion_t *event);
int wait_for_completion_timeout(completion_t *event, unsigned int msc);
void wait_for_completion(completion_t *x);
int pandora_thread_freeze(const char *name);
void pandora_fingerprint(void);

unsigned long pandora_initialise_stack(struct pandora_task *t);
void arm_cpu_relax(void);
void timer_irq_disable(void);

extern volatile struct pandora_task *current_task;
#endif
#endif
