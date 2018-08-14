/*
 * kernel/private/drivers/ambarella/vout/vout_pll.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
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


#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <plat/clk.h>

#include "vout_pri.h"

/* ==========================================================================*/
typedef struct {
        u32     rate;
        u32     intp;
        u32     frac;
        u32     sdiv;
        u32     sout;
        u32     pre_scaler;
        u32     post_scaler;
        u32     pll_ctrl2;
        u32     pll_ctrl3;
} vout_pll_s;

const static vout_pll_s ambarella_digital_vout_pll_table[] = {
        //rate                  intp    frac    sdiv+1  sdout+1 pre     post    ctrl2           ctrl3
        {PLL_CLK_148_5MHZ,      0x63,   0,      1,      1,      4,      4,      0x3f770000,     0x68300},
        {PLL_CLK_74_25MHZ,      0x63,   0,      1,      4,      4,      2,      0x3f770000,     0x68300},
};
#define	VOUT_DIGITAL_PLL_NUM	ARRAY_SIZE(ambarella_digital_vout_pll_table)

static struct clk gclk_vo = {
	.parent		= NULL,
	.name		= "gclk_vo",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_HDMI_CTRL_REG,
	.pres_reg	= SCALER_HDMI_PRE_REG,
	.pres_val	= 4,
	.post_reg	= -1,
	.frac_reg	= PLL_HDMI_FRAC_REG,
	.ctrl2_reg	= PLL_HDMI_CTRL2_REG,
	.ctrl3_reg	= PLL_HDMI_CTRL3_REG,
	.ctrl3_val	= 0x00068306,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 8,
	.divider	= 10,
	.max_divider	= 0,
	.extra_scaler	= 1,
	.table		= ambarella_pll_vout_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_vout_table),
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk *rct_register_clk_vout(void)
{
	struct clk *pgclk_vo = NULL;

	pgclk_vo = clk_get(NULL, "gclk_vo");
	if (IS_ERR(pgclk_vo)) {
		ambarella_clk_add(&gclk_vo);
		pgclk_vo = &gclk_vo;
		pr_info("SYSCLK:VOUT[%lu]\n", clk_get_rate(pgclk_vo));
	}

	return pgclk_vo;
}

static struct clk gclk_vo2 = {
	.parent		= NULL,
	.name		= "gclk_vo2",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_VIDEO2_CTRL_REG,
	.pres_reg	= SCALER_VIDEO2_PRE_REG,
	.pres_val	= 4,
	.post_reg	= SCALER_VIDEO2_POST_REG,
	.post_val	= 10,
	.frac_reg	= PLL_VIDEO2_FRAC_REG,
	.ctrl2_reg	= PLL_VIDEO2_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO2_CTRL3_REG,
	.ctrl3_val	= 0x00068306,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
	.table		= ambarella_pll_vout_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_vout_table),
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk *rct_register_clk_vout2(void)
{
	struct clk *pgclk_vo2 = NULL;

	pgclk_vo2 = clk_get(NULL, "gclk_vo2");
	if (IS_ERR(pgclk_vo2)) {
		ambarella_clk_add(&gclk_vo2);
		pgclk_vo2 = &gclk_vo2;
		pr_info("SYSCLK:VOUT[%lu]\n", clk_get_rate(pgclk_vo2));
	}

	return pgclk_vo2;
}

/* ========================================================================== */
static int find_clock_in_table(u32 rate, const vout_pll_s *pll_table, u32 table_size){
        int     i = 0;
        for(i = 0; i < table_size; i++){
                if(rate == pll_table[i].rate)
                        return (i+1);
        }
        return 0;
}


int ambarella_vout_set_rate(struct clk *c, u32 table_index)
{
	union ctrl_reg_u ctrl_reg;

        vout_pll_s clk_pll = ambarella_digital_vout_pll_table[table_index-1];

	if (table_index <= 0)
		return -1;

        amba_rct_writel_en(c->pres_reg, (clk_pll.pre_scaler - 1) << 4);

	ctrl_reg.w = amba_rct_readl(c->ctrl_reg);
	ctrl_reg.s.intp = clk_pll.intp - 1;
	ctrl_reg.s.sdiv = clk_pll.sdiv - 1;
	ctrl_reg.s.sout = clk_pll.sout - 1;
	ctrl_reg.s.bypass = 0;
	ctrl_reg.s.frac_mode = 0;
	ctrl_reg.s.force_reset = 0;
	ctrl_reg.s.power_down = 0;
	ctrl_reg.s.halt_vco = 0;
	ctrl_reg.s.tristate = 0;
	ctrl_reg.s.force_lock = 1;
	ctrl_reg.s.force_bypass = 0;
	ctrl_reg.s.write_enable = 0;
	amba_rct_writel_en(c->ctrl_reg, ctrl_reg.w);

        amba_rct_writel_en(c->post_reg, (clk_pll.post_scaler - 1) << 4);

	amba_rct_writel(c->ctrl2_reg, clk_pll.pll_ctrl2);
	amba_rct_writel(c->ctrl3_reg, clk_pll.pll_ctrl3);
	return 0;
}


void rct_set_vout_clk_src(u32 mode)
{
}

void rct_set_vout_freq_hz(u32 freq_hz)
{
	clk_set_rate(rct_register_clk_vout(), freq_hz);
}

u32 rct_get_vout_freq_hz(void)
{
	return clk_get_rate(rct_register_clk_vout());
}

void rct_set_vout2_clk_src(u32 mode)
{
}

void rct_set_vout2_freq_hz(u32 freq_hz)
{
        int table_index = find_clock_in_table(freq_hz,
                                        ambarella_digital_vout_pll_table,
                                        VOUT_DIGITAL_PLL_NUM);
        if(table_index){
                ambarella_vout_set_rate(rct_register_clk_vout2(), table_index);
        }else{
	        clk_set_rate(rct_register_clk_vout2(), freq_hz);
        }
}

u32 rct_get_vout2_freq_hz(void)
{
	return clk_get_rate(rct_register_clk_vout2());
}

