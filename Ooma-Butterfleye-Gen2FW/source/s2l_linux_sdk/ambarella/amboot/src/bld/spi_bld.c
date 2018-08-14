/**
 * bld/spi_bld.c
 *
 * Author: Long Zhao <longzhao@ambarella.com>
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
#include <ambhw/spi.h>
#include <ambhw/gpio.h>

typedef union {
	struct {
		u32		dfs	: 4;	/* [3:0] */
		u32		frf	: 2;	/* [5:4] */
		u32		scph	: 1;	/* [6] */
		u32		scpol	: 1;	/* [7] */
		u32		tmod	: 2;	/* [9:8] */
		u32		slv_oe	: 1;	/* [10] */
		u32		srl	: 1;	/* [11] */
		u32		reserv1	: 5;	/* [16:12] */
		u32		residue	: 1;	/* [17] */
		u32		tx_lsb	: 1;	/* [18] */
		u32		rx_lsb	: 1;	/* [19] */
		u32		reserv2	: 1;	/* [20] */
		u32		fc_en	: 1;	/* [21] */
		u32		rxd_mg	: 4;	/* [25:22] */
		u32		byte_ws	: 1;	/* [26] */
		u32		hold	: 1;	/* [27] */
		u32		reserv3	: 4;	/* [31:28] */
	} s;
	u32	w;
} spi_ctrl0_reg_t;

typedef struct {
	u8 cs_gpio;
	u8 act_level;
	u8 cs_change;
} spi_cs_info_t;

static spi_cs_info_t spi_cs;
static u8 cfs_dfs;
/*===========================================================================*/
static u32 spi_bld_get_addr(u8 spi_id, u32 reg_offset)
{
	u32 reg_adds = 0xFFFFFFFF;

	switch (spi_id) {
	case SPI_MASTER1:
		reg_adds = SPI0_REG(reg_offset);
		break;
	case SPI_MASTER2:
		reg_adds = SPI1_REG(reg_offset);
		break;
	default:
		printf("Invalid SPI id: %d\n", spi_id);
	}

	return reg_adds;
}

/*===========================================================================*/
static u32 spi_bld_readl(u8 spi_id, u32 reg_offset)
{
	u32 reg_adds;
	u32 reg_val = 0;

	reg_adds = spi_bld_get_addr(spi_id, reg_offset);
	if (reg_adds != 0xFFFFFFFF) {
		reg_val = readl(reg_adds);
	}

	return reg_val;
}

static inline void spi_bld_writel(u8 spi_id, u32 reg_offset, u32 reg_val)
{
	u32 reg_add;

	reg_add = spi_bld_get_addr(spi_id, reg_offset);
	writel(reg_add, reg_val);
}

/*===========================================================================*/
static inline void spi_bld_enable(u8 spi_id)
{
	spi_bld_writel(spi_id, SPI_SSIENR_OFFSET, 1);
	spi_bld_writel(spi_id, SPI_SER_OFFSET, 1);
}

static inline void spi_bld_disable(u8 spi_id)
{
	spi_bld_writel(spi_id, SPI_SSIENR_OFFSET, 0);
	spi_bld_writel(spi_id, SPI_IMR_OFFSET, 0);
	spi_bld_writel(spi_id, SPI_SER_OFFSET, 0);
}

/*===========================================================================*/
static void spi_bld_cs_active(void)
{
	if (spi_cs.act_level)
		gpio_set(spi_cs.cs_gpio);
	else
		gpio_clr(spi_cs.cs_gpio);
}

static void spi_bld_cs_deactive(void)
{
	if (spi_cs.act_level)
		gpio_clr(spi_cs.cs_gpio);
	else
		gpio_set(spi_cs.cs_gpio);
}

/*===========================================================================*/
static void spi_bld_set_baudrate(u8 spi_id, u32 freq_hz)
{
	unsigned int ssi_clk;
	u32 sckdv;

	if (!freq_hz) {
		return;
	}
	ssi_clk = get_ssi_freq_hz();
	sckdv = ((ssi_clk / freq_hz) + 0x01) & 0xFFFE;

	spi_bld_writel(spi_id, SPI_BAUDR_OFFSET, sckdv);
}

/*===========================================================================*/
void spi_bld_init(u8 spi_id, amba_spi_cfg_t *spi_cfg)
{
	spi_ctrl0_reg_t reg_cr0;

	spi_bld_disable(spi_id);

	spi_bld_set_baudrate(spi_id, spi_cfg->baud_rate);

	reg_cr0.w = 0;
	reg_cr0.s.dfs	= spi_cfg->cfs_dfs - 1;
	reg_cr0.s.scph = (spi_cfg->spi_mode & SPI_CPHA) ? 1 : 0;
	reg_cr0.s.scpol = (spi_cfg->spi_mode & SPI_CPOL) ? 1 : 0;
	reg_cr0.s.tx_lsb = (spi_cfg->spi_mode & SPI_LSB_FIRST) ? 1 : 0;
	reg_cr0.s.rx_lsb = (spi_cfg->spi_mode & SPI_LSB_FIRST) ? 1 : 0;
	reg_cr0.s.residue = 1;
	spi_bld_writel(spi_id, SPI_CTRLR0_OFFSET, reg_cr0.w);

	spi_bld_writel(spi_id, SPI_IMR_OFFSET, SPI_TXEIS_MASK);
	spi_bld_writel(spi_id, SPI_TXFTLR_OFFSET, 0);
	spi_bld_writel(spi_id, SPI_RXFTLR_OFFSET, 1);

	cfs_dfs = spi_cfg->cfs_dfs;

	spi_cs.cs_gpio = spi_cfg->cs_gpio;
	spi_cs.act_level = (spi_cfg->spi_mode & SPI_CS_HIGH) ? 1 : 0;
	spi_cs.cs_change = spi_cfg->cs_change;

	gpio_config_sw_out(spi_cfg->cs_gpio);
	spi_bld_cs_deactive();
}

/*===========================================================================*/
static void spi_bld_polling_status(u8 spi_id)
{
	u32 reg_ctrl;
	int wait_loop = SPI_POLLING_MAX_WAIT_LOOP;

	do {
		reg_ctrl = spi_bld_readl(spi_id, SPI_SR_OFFSET);
		wait_loop--;
	} while (((reg_ctrl & 0x05) != 0x04) && (wait_loop > 0));
}

/*===========================================================================*/
static int spi_bld_write_fifo(u8 spi_id, u8 *buf, int len)
{
	int i = 0;
	u16 data;
	void *wbuf;
	u32 widx, total_len, xfer_len;

	wbuf = (void *)buf;
	widx = 0;
	total_len = (cfs_dfs > 8) ? (len >> 1) : (len);

	while(total_len > widx) {
		xfer_len = ((total_len - widx) < SPI_DATA_FIFO_SIZE_16) ? (total_len - widx) : SPI_DATA_FIFO_SIZE_16;

		for (i = 0; i < xfer_len; i++) {
			if (cfs_dfs <= 8)
				data = ((u8 *)wbuf)[widx++];
			else
				data = ((u16 *)wbuf)[widx++];
			spi_bld_writel(spi_id, SPI_DR_OFFSET, data);
		}
		spi_bld_polling_status(spi_id);
	}

	return len;
}

/*===========================================================================*/
static int spi_bld_read_fifo(u8 spi_id, u8 *buf, int len)
{
	int i = 0;
	u16 data;
	void *rbuf;
	u32 ridx, total_len, xfer_len, rxflr;

	rbuf = (void *)buf;
	ridx = 0;
	total_len = (cfs_dfs > 8) ? (len >> 1) : (len);

	while(total_len > ridx) {
		/* dummy write */
		xfer_len = ((total_len - ridx) < SPI_DATA_FIFO_SIZE_16) ? (total_len - ridx) : SPI_DATA_FIFO_SIZE_16;

		for (i = 0; i < xfer_len; i++)
			spi_bld_writel(spi_id, SPI_DR_OFFSET, SPI_DUMMY_DATA);

		spi_bld_polling_status(spi_id);

		/* read */
		rxflr = spi_bld_readl(spi_id, SPI_RXFLR_OFFSET);
		if (xfer_len > rxflr)
			xfer_len = rxflr;
		for(i = 0; i < xfer_len; i++) {
			data = spi_bld_readl(spi_id, SPI_DR_OFFSET);
			if (cfs_dfs <= 8)
				((u8 *)rbuf)[ridx++] = data & 0xff;
			else
				((u16 *)rbuf)[ridx++] = data;
		}
	}

	return len;
}

/*===========================================================================*/
int spi_bld_write(u8 spi_id, u8 *tx_buf, int tx_len)
{
	spi_bld_enable(spi_id);
	spi_bld_cs_active();
	spi_bld_write_fifo(spi_id, tx_buf, tx_len);
	spi_bld_cs_deactive();
	spi_bld_disable(spi_id);

	return tx_len;
}

int spi_bld_read(u8 spi_id, u8 *rx_buf, int rx_len)
{
	spi_bld_enable(spi_id);
	spi_bld_cs_active();
	spi_bld_read_fifo(spi_id, rx_buf, rx_len);
	spi_bld_cs_deactive();
	spi_bld_disable(spi_id);

	return rx_len;
}

int spi_bld_write_then_read(u8 spi_id, u8 *tx_buf, int tx_len, u8 *rx_buf, int rx_len)
{
	/* write */
	spi_bld_enable(spi_id);
	spi_bld_cs_active();
	spi_bld_write_fifo(spi_id, tx_buf, tx_len);
	if (spi_cs.cs_change)
		spi_bld_cs_deactive();
	spi_bld_disable(spi_id);

	/* then read */
	spi_bld_enable(spi_id);
	if (spi_cs.cs_change)
		spi_bld_cs_active();
	spi_bld_read_fifo(spi_id, rx_buf, rx_len);
	spi_bld_cs_deactive();
	spi_bld_disable(spi_id);

	return rx_len;
}

