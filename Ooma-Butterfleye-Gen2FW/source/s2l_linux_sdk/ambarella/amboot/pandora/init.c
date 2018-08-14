/**
 * pandora/init.c
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

#include <amboot.h>
#include <irq.h>
#include <bldfunc.h>
#include <ambhw/gpio.h>
#include <ambhw/timer.h>
#include <pandora.h>

void pandora(void)
{
	malloc_init();
	rct_pll_init();
	enable_fio_dma();
	rct_reset_fio();
	dma_channel_select();

#if defined(CONFIG_AMBOOT_ENABLE_GPIO)
	gpio_init();
#else
	gpio_mini_init(main_boot_from);
#endif
	/* Initialize various peripherals used in AMBoot */
	if (amboot_bsp_early_init != NULL) {
		amboot_bsp_early_init();
	}

	uart_init();
	putstr("\x1b[4l");	/* Set terminal to replacement mode */
	putstr("\r\n");		/* First, output a blank line to UART */
	irq_init();

	pandora_thread_init();
	pandora_thread_create("main", TSKIDLE_PRIORITY + 2, main, NULL);
	pandora_schedule_start();

	for(;;);
}
void pandora_fingerprint(void)
{
	pr_color(RED, "Pandora: multithreading built-in\n");
}
