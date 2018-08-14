/**
 * boards/s2lm_elektra/bsp/iav/sensor_light.c
 *
 * History:
 *    2015/10/08 -
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

#ifndef __SENSOR_LIGHT_C__
#define __SENSOR_LIGHT_C__

#define LIGHT_THRESHOLD                       (500)

#ifdef CONFIG_S2LMELEKTRA_CHECK_LIGHT
#define PWM_BL_SETPROP

#ifdef PWM_BL_SETPROP
static int pwm_bl_setprop(u32 brightness_level)
{
	void* fdt;
	char data[4]; //sizeof(u32)
	int rval, offset, len = 0;
	fdt = (void *)bld_hugebuf_addr;

	/*read DTB from the address to fdt*/
	rval = flprog_get_dtb((void *)fdt);
	if (rval < 0) {
		putstr("Get dtb failed\r\n");
		return rval;
	}

	offset = fdt_path_offset(fdt, "/bogus_bus/pwm_bl@0");
	if (offset < 0) {
		putstr("libfdt fdt_path_offset() error: ");
		putstr(fdt_strerror(offset));
		putstr("\r\n");
		return -1;
	}
	u32* tmp = (u32*)data;
	*tmp = cpu_to_fdt32(brightness_level);
	len += 4;
	rval = fdt_setprop(fdt, offset, "default-brightness-level", data, len);
	if (rval < 0) {
		putstr("libfdt fdt_setprop() error: ");
		putstr(fdt_strerror(rval));
		putstr("\r\n");
		return -1;
	}

	rval = flprog_set_dtb(fdt, fdt_totalsize(fdt), 0);
	if (rval < 0) {
		putstr("libfdt flprog_set_dtb() error: ");
		putstr(fdt_strerror(rval));
		putstr("\r\n");
	}
	return 0;
}
#endif

static void diag_adc_set_config(void)
{
	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_ENABLE));
	rct_timer_dly_ms(10);

	writel(ADC_SLOT_NUM_REG, 0);//set slot number=1
	writel(ADC_SLOT_PERIOD_REG, 60);//set slot period 60
	writel(ADC_SLOT_CTRL_0_REG, 0xfff);//set slot 0 ctrl 0xfff
}

static unsigned int read_light_sensor(void)
{
	unsigned int data = 0;

	writel(ADC16_CTRL_REG, 0x0);

	writel(SCALER_ADC_REG, ADC_SOFT_RESET | 0x2);
	writel(SCALER_ADC_REG, 0x2);
	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_CLEAR));
	diag_adc_set_config();

	writel(ADC_DATA2_REG, 0x0);

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) & (~ADC_CONTROL_MODE)));
	/* ADC control mode, single, start conversion */
	writel(ADC_CONTROL_REG,
		(readl(ADC_CONTROL_REG) | ADC_CONTROL_START));
	while ((readl(ADC_STATUS_REG) & ADC_STATUS_STATUS) == 0x0);
	rct_timer_dly_ms(10);
	/* ADC interface Read from Channel 0, 1, 2, 3 */
	data = readl(ADC_DATA2_REG);
	putstr("ADC: [");
	putdec(data);
	putstr("]\r\n");
	writel(ADC_CONTROL_REG, 0x0);
	writel(ADC_CONTROL_REG,
		(readl(ADC_CONTROL_REG) & (~ADC_CONTROL_ENABLE)));

	writel(ADC16_CTRL_REG, 0x2);
	return data;
}

static unsigned int check_light(void)
{
	unsigned int data = read_light_sensor();
#ifdef PWM_BL_SETPROP
	unsigned int brightness_level = 0;
	if (data < LIGHT_THRESHOLD) {
		gpio_config_sw_out(113);
		gpio_set(113);
		brightness_level = 50;
		pwm_bl_setprop(brightness_level);
	}
#endif
	return data;
}
#endif

#endif
