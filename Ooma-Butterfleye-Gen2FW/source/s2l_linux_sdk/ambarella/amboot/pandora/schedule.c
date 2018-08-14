/**
 * pandora/schedule.c
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
#include <bldfunc.h>
#include <ambhw/gpio.h>
#include <ambhw/timer.h>
#include <pandora.h>

unsigned long long jiffies = 0;

volatile struct pandora_task *current_task = NULL;
volatile struct pandora_task *highest_priority_task = NULL;
volatile int pandorayield = 0;

void mutex_init(pandora_mutex_t *mutex)
{
	__mutex_atomic_clr(mutex);
}
void mutex_lock(pandora_mutex_t *mutex)
{
	__mutex_atomic_set(mutex);
}
void mutex_unlock(pandora_mutex_t *mutex)
{
	__mutex_atomic_clr(mutex);
	cpu_relax();
}
void init_completion(completion_t *x)
{
	x->owner = NULL;
	x->done = 0;
}
void complete_isr(completion_t *x)
{
	highest_priority_task = (struct pandora_task *)x->owner;
	x->owner = NULL;
	x->done = 1;
	yield();
}
void complete(completion_t *x)
{
	highest_priority_task = (struct pandora_task *)x->owner;
	x->owner = NULL;
	x->done = 1;
}

void wait_for_completion(completion_t *x)
{
	struct pandora_task *task;

	local_irq_disable();

	task = (struct pandora_task *)current_task;
	x->owner = (void *)task;
	task->timeout = ~0;
	task->block = 1;

	local_irq_enable();

	if (!x->done) {
		do {
			/* generate a software interrupt to relax cpu */
			cpu_relax();
		} while(!x->done);
	}

	local_irq_disable();

	x->done = 0;
	task->block = 0;

	local_irq_enable();
}
/**
 * wait_for_completion_timeout - wait event with timeout.
 * @ x: wait event.
 * @ msec: timeout to return after waitting envent.
 * @ return: 0 - timeout return, > 0 - woke up by event.
 *
 * Setting timeout as 0 will block the task without timeout.
 */
int wait_for_completion_timeout(completion_t *x, unsigned int msec)
{

	unsigned int tmo;
	struct pandora_task *task;

	local_irq_disable();

	task = (struct pandora_task *)current_task;
	x->owner = (void *)task;
	if (msec) {
		task->timeout = jiffies + msec;
		tmo = 0;
	} else {
		task->timeout = ~0;
		tmo = 1;
	}

	task->block = 1;

	local_irq_enable();

	if (!x->done) {
		do {
			/* generate a software interrupt to relax cpu */
			cpu_relax();
			tmo = (task->timeout > jiffies ? 1 : 0);
		} while((!x->done) && tmo);
	} else {
		tmo = 1;
	}

	local_irq_disable();

	x->done = 0;
	task->block = 0;

	local_irq_enable();

	return tmo;
}
static struct pandora_task *pandora_task_alloc(const char *name, int prior, void *func, void *arg)
{
	struct pandora_task *new;
	void *stack_top;

	new = (struct pandora_task *)malloc(sizeof(struct pandora_task));
	if (!new) {
		printf("%s %d\n", __func__,__LINE__);
		goto pandora_task_alloc_exit;
	}

	memset(new, 0, sizeof(struct pandora_task));

	new->prior = prior;
	new->parameter = arg;
	new->entry = func;
	new->next = new;
	new->freeze = 0;
	new->block = 0;
	new->timeout = ~0;
	strcpy(new->name, name);

	stack_top = malloc(DEFAULT_THREAD_STACK_SIZE);
	if (!stack_top) {
		printf("%s %d\n", __func__,__LINE__);
		goto pandora_task_alloc_stack_err;
	}
	/* Stack end address being 8 alignment */
	new->stack = ((unsigned long)stack_top +
				DEFAULT_THREAD_STACK_SIZE - 1) & (~0xf);

	new->offset = pandora_initialise_stack(new);
#if 0
	printf("[%08x]task: %s prior: %d stack: %08x entry: %08x next: %08x\n",
			new, new->name, new->prior, new->stack, new->entry, new->next);
#endif

	return new;

pandora_task_alloc_stack_err:
	free(new);
pandora_task_alloc_exit:
	return NULL;

}
static int task_insert(struct pandora_task *new)
{
	struct pandora_task *tmp = current_task->next;
	current_task->next = new;
	new->next = tmp;

	return 0;
}
int pandora_thread_freeze(const char *name)
{
	struct pandora_task *t;

	local_irq_disable();
	if (!name) {
		current_task->freeze = 1;
	} else {
		t = current_task->next;
		while (t != current_task) {
			if (!memcmp(name, t->name, strlen(name))) {
				t->freeze = 1;
				break;
			}
		}

		if (t == current_task) {
			printf("%s %d NOT FOUND!\n", __func__,__LINE__);
		}
	}
	local_irq_enable();
	return 0;
}

int pandora_thread_create(const char *name, int prior, void *call, void *arg)
{
	struct pandora_task *task;
	int ret = 0;

	if (prior <= 0) {
		printf("%s %d\n", __func__,__LINE__);
		return OS_ERR;
	}

	local_irq_disable();

	task = pandora_task_alloc(name, prior, call, arg);
	if (!task) {
		printf("%s %d\n", __func__,__LINE__);
		ret = OS_ERR;
		goto pandora_thread_create_err;
	}

	task_insert(task);

pandora_thread_create_err:
	local_irq_enable();
	return ret;
}
static void pandora_idle(void *arg)
{
	/* FIXME: all other tasks can be scheduled after idle task is
	 * in the process. Or the timer tick is ignored */
	for (;;) {
	}
}

int pandora_thread_init(void)
{
	struct pandora_task *task;
	int ret = 0;

	local_irq_disable();

	task = pandora_task_alloc("idle", TSKIDLE_PRIORITY , pandora_idle, NULL);
	if (!task) {
		printf("%s %d\n", __func__,__LINE__);
		ret = OS_ERR;
		goto pandora_thread_init_err;
	}

	current_task = task;

pandora_thread_init_err:
	local_irq_enable();
	return ret;
}
void pandora_schedule_start(void)
{
	/* FIXME: the first task is idle, which has the lowest priority.
	 * Switching the higher task at the next timer tick, if the higher
	 * task is ready */

	BUG_ON(!current_task);

	/* the first task should be scheduled in 1HZ. */
	timer_init();
	__pandora_sche_start();

}
static struct pandora_task *pandora_task_pick(int prior)
{
	struct pandora_task *p = current_task->next;

	while (p != current_task) {

		if ((p->prior < prior) || (p->freeze)) {
			p = p->next;
			continue;
		}
		current_task = p;
		return p;
	}
	return NULL;
}

static struct pandora_task *pandora_task_pick_lower(int prior)
{
	int i;
	struct pandora_task *p = current_task->next;

	for (i = prior - 1; i > TSKIDLE_PRIORITY; i--)
	{
		while (p != current_task) {

			if ((p->prior < i)
					|| (p->freeze)
					|| (p->prior == TSKIDLE_PRIORITY)) {
				p = p->next;
				continue;
			}
			current_task = p;
			return p;
		}
	}
	return NULL;
}
void pandora_task_switch(void)
{
	struct pandora_task *p = NULL;
	int prior = current_task->prior;

	if (!current_task)
		return ;

	/* Wake up the blocked task . highest_priority_task is token when wake_up
	 * or wake_up_from_isr */
	if (highest_priority_task){
		current_task = highest_priority_task;
		highest_priority_task = NULL;
		goto __switch_return;
	}
	p = pandora_task_pick(prior);

	/* if it can't pick a task which has higher or equal priority,
	 * and the current task is blocked, then the lower priority tasks
	 * have the chance to be scheduled */
	if (!p && (current_task->block || current_task->freeze)) {
		pandora_task_pick_lower(prior);
	}

__switch_return:
	return ;
}

void cpu_relax(void)
{
	arm_cpu_relax();
}
void schedule_disable(void)
{
	timer_irq_disable();
}
void yield(void)
{
	pandorayield = 1;
}

static void timer8_handler(void *data)
{
	jiffies++;
	yield();
}

void timer_irq_disable(void)
{
	irq_disable(TIMER8_IRQ);
}

void timer_init(void)
{
	int tick = get_apb_bus_freq_hz() / HZ;

	writel(TIMER8_STATUS_REG, tick);
	writel(TIMER8_RELOAD_REG, tick);
	writel(TIMER8_MATCH1_REG, 0x00000000);
	writel(TIMER8_MATCH2_REG, 0x00000000);
	setbitsl(TIMER_CTR_REG, 0x5 << 28);

	request_irq(TIMER8_IRQ, IRQ_RISING_EDGE, timer8_handler, NULL);
}

