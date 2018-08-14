/**
 * bld/dsp/dsp_pandora.c
 *
 * History:
 *    2017/03/27 - [Tao Wu] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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

#include <irq.h>
#include <pandora.h>
#include <dsp/dsp.h>

#if (CHIP_REV == S5L)
#include "s5l/dsp_aaa.c"
#elif (CHIP_REV == S3L)
#include "s3l/dsp_aaa.c"
#elif (CHIP_REV == S2L)
#include "s2l/dsp_aaa.c"
#else
#error "Not implemented yet"
#endif

static dsp_context_t dsp_cnxt;

static void dsp_vcap_handler(void *data)
{
	/* TODO: wake up events */
	complete_isr(&dsp_cnxt.vcap_comp);
}

static void dsp_pandora_task(void *arg)
{
	request_irq(CODE_VCAP_IRQ, IRQ_RISING_EDGE, dsp_vcap_handler, NULL);

	dsp_pandora_aaa();

	complete(&dsp_cnxt.dsp_comp);
	pandora_thread_freeze(NULL);
	for(;;);
}

static unsigned int is_dsp_work(void)
{
	if ((dsp_cnxt.dsp_state == IAV_STATE_PREVIEW) ||
		(dsp_cnxt.dsp_state == IAV_STATE_ENCODING)) {
		return 1;
	}

	return 0;
}

void set_dsp_state(unsigned int dsp_state)
{
	dsp_cnxt.dsp_state = dsp_state;
}

int dsp_pandora_init(flpart_table_t *ptb)
{
	int ret = 0;
	set_dsp_state(IAV_STATE_INIT);

	if (amboot_bsp_entry != NULL) {
		ret = amboot_bsp_entry(ptb);
	}

	init_completion(&dsp_cnxt.dsp_comp);
	init_completion(&dsp_cnxt.vcap_comp);

	if (is_dsp_work()) {
		pandora_thread_create("dsp", TSKIDLE_PRIORITY + 1, dsp_pandora_task, ptb);
	} else {
		/* Other place wait_dsp_done */
		complete(&dsp_cnxt.dsp_comp);
	}
	return ret;
}

void wait_dsp_vcap(void)
{
	if (!wait_for_completion_timeout(&dsp_cnxt.vcap_comp, 1000)) {
		printf("pandora: wait_dsp_vcap timeout\n");
	}
}

void wait_dsp_done(void)
{
	if (!wait_for_completion_timeout(&dsp_cnxt.dsp_comp, 2000)) {
		printf("pandora: wait_dsp_done timeout\n");
	}
}
