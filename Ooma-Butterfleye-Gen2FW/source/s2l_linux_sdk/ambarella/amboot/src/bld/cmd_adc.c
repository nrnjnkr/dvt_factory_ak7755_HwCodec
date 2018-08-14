/**
 * bld/cmd_adc.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#include <bldfunc.h>
#include <ambhw/adc.h>
#include <ambhw/uart.h>

/*===========================================================================*/
static void diag_adc_set_slot_ctrl(u8 slot_id, u32 slot_value)
{
	switch (slot_id) {
	case 0:
		writel(ADC_SLOT_CTRL_0_REG, slot_value);
		break;
	case 1:
		writel(ADC_SLOT_CTRL_1_REG, slot_value);
		break;
	case 2:
		writel(ADC_SLOT_CTRL_2_REG, slot_value);
		break;
	case 3:
		writel(ADC_SLOT_CTRL_3_REG, slot_value);
		break;
	case 4:
		writel(ADC_SLOT_CTRL_4_REG, slot_value);
		break;
	case 5:
		writel(ADC_SLOT_CTRL_5_REG, slot_value);
		break;
	case 6:
		writel(ADC_SLOT_CTRL_6_REG, slot_value);
		break;
	case 7:
		writel(ADC_SLOT_CTRL_7_REG, slot_value);
		break;
	}
}

static void diag_adc_set_config(void)
{
	int i = 0;
	int slot_num_reg = 0;

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) |(ADC_CONTROL_MODE | ADC_CONTROL_ENABLE)));
	rct_timer_dly_ms(200);

	writel(ADC_SLOT_NUM_REG, 0);//set slot number=1
	for (i = 0; i <= slot_num_reg; i++) {
		diag_adc_set_slot_ctrl(i, (1 << ADC_NUM_CHANNELS) - 1);//set slot 0 ctrl 0xfff
	}
	writel(ADC_SLOT_PERIOD_REG, ADC_NUM_CHANNELS * ADC_PERIOD_CYCLE - 1);//set slot period
}

static void diag_adc(void)
{
	u32 data[ADC_NUM_CHANNELS] = {0};
	u32 old[ADC_NUM_CHANNELS]  = {0};
	u32 i, equal;
	int c = 0;

	writel(ADC16_CTRL_REG, 0x0);

	putstr("running ADC diagnostics...\r\n");
	putstr("press space key to quit!\r\n");

	writel(SCALER_ADC_REG, ADC_SOFT_RESET | 0x2);
	writel(SCALER_ADC_REG, 0x2);

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_CLEAR));
	diag_adc_set_config();

	writel(ADC_DATA0_REG, 0x0);
	writel(ADC_DATA1_REG, 0x0);
	writel(ADC_DATA2_REG, 0x0);
#if (ADC_NUM_CHANNELS > 3)
	writel(ADC_DATA3_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 4)
	writel(ADC_DATA4_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 5)
	writel(ADC_DATA5_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 6)
	writel(ADC_DATA6_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 7)
	writel(ADC_DATA7_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 8)
	writel(ADC_DATA8_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 9)
	writel(ADC_DATA9_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 10)
	writel(ADC_DATA10_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 11)
	writel(ADC_DATA11_REG, 0x0);
#endif

	for (;;) {
		if (uart_poll()) {
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
				break;
			}
		}
		/* ADC control mode, single, start conversion */
		writel(ADC_CONTROL_REG,
			(readl(ADC_CONTROL_REG) | ADC_CONTROL_START));

		while ((readl(ADC_STATUS_REG) & ADC_STATUS_STATUS) == 0x0);

		rct_timer_dly_ms(200);
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			old[i] = data[i];
		}

		/* ADC interface Read from Channel 0, 1, 2, 3 */
		data[0] = readl(ADC_DATA0_REG);
		data[1] = readl(ADC_DATA1_REG);
		data[2] = readl(ADC_DATA2_REG);
#if (ADC_NUM_CHANNELS > 3)
		data[3] = readl(ADC_DATA3_REG);
#endif
#if (ADC_NUM_CHANNELS > 4)
		data[4] = readl(ADC_DATA4_REG);
#endif
#if (ADC_NUM_CHANNELS > 5)
		data[5] = readl(ADC_DATA5_REG);
#endif
#if (ADC_NUM_CHANNELS > 6)
		data[6] = readl(ADC_DATA6_REG);
#endif
#if (ADC_NUM_CHANNELS > 7)
		data[7] = readl(ADC_DATA7_REG);
#endif
#if (ADC_NUM_CHANNELS > 8)
		data[8] = readl(ADC_DATA8_REG);
#endif
#if (ADC_NUM_CHANNELS > 9)
		data[9] = readl(ADC_DATA9_REG);
#endif
#if (ADC_NUM_CHANNELS >= 10)
		data[10] = readl(ADC_DATA10_REG);
#endif
#if (ADC_NUM_CHANNELS > 11)
		data[11] = readl(ADC_DATA11_REG);
#endif
		equal = 1;
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			if (data[i] != old[i]) {
				equal = 0;
				break;
			}
		}

		if (equal) {
			continue;
		}

		for (i = 0; i < ADC_NUM_CHANNELS; i++)
			printf("ADC[%d] = 0x%x \n", i, data[i]);
		printf("---ADC sampling done---\n");
	}

	writel(ADC_CONTROL_REG, 0x0);
	writel(ADC_CONTROL_REG,
		(readl(ADC_CONTROL_REG) & (~ADC_CONTROL_ENABLE)));
	writel(ADC16_CTRL_REG, 0x2);

	putstr("\r\ndone!\r\n");
}

/*===========================================================================*/
static int cmd_adc(int argc, char *argv[])
{
	diag_adc();

	return 0;
}

/*===========================================================================*/
static char help_adc[] =
	"adc - Diag ADC\r\n"
	;
__CMDLIST(cmd_adc, "adc", help_adc);

