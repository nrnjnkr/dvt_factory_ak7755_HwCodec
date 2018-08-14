/*
 * ambhw/spi.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
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

#ifndef __AMBHW__SPI_H__
#define __AMBHW__SPI_H__

#include <ambhw/chip.h>

/* ==========================================================================*/
#if (CHIP_REV == S5L)
#define SPI_AHB_INSTANCES			3
#define SPI_AHB_SLAVE_INSTANCES			1
#elif (CHIP_REV == CV1)
#define SPI_AHB_INSTANCES			6
#define SPI_AHB_SLAVE_INSTANCES			1
#else
#define SPI_AHB_INSTANCES			2
#define SPI_AHB_SLAVE_INSTANCES			1
#endif

#if (CHIP_REV == S2L) || (CHIP_REV == S3L) || (CHIP_REV == S3) || \
	(CHIP_REV == S5) || (CHIP_REV == S5L)
#define SPI0_OFFSET			0x20000
#define SPI1_OFFSET			0x21000
#define SPI2_OFFSET			0x22000
#define SPI_SLAVE_OFFSET		0x26000
#else
#define SPI0_OFFSET			0x11000
#define SPI1_OFFSET			0x12000
#define SPI2_OFFSET			0x13000
#define SPI3_OFFSET			0x14000
#define SPI4_OFFSET			0x15000
#define SPI5_OFFSET			0x1C000
#define SPI_SLAVE_OFFSET		0x10000
#endif

#define SPI0_BASE			(AHB_N_BASE + SPI0_OFFSET)
#define SPI1_BASE			(AHB_N_BASE + SPI1_OFFSET)
#define SPI2_BASE			(AHB_N_BASE + SPI2_OFFSET)
#define SPI3_BASE			(AHB_N_BASE + SPI3_OFFSET)
#define SPI4_BASE			(AHB_N_BASE + SPI4_OFFSET)
#define SPI5_BASE			(AHB_N_BASE + SPI5_OFFSET)
#define SPI_SLAVE_BASE			(AHB_N_BASE + SPI_SLAVE_OFFSET)
#define SPI0_REG(x)			(SPI0_BASE + (x))
#define SPI1_REG(x)			(SPI1_BASE + (x))
#define SPI2_REG(x)			(SPI2_BASE + (x))
#define SPI3_REG(x)			(SPI3_BASE + (x))
#define SPI4_REG(x)			(SPI4_BASE + (x))
#define SPI5_REG(x)			(SPI5_BASE + (x))
#define SPI_SLAVE_REG(x)		(SPI_SLAVE_BASE + (x))

/* ==========================================================================*/
#define SPI_MASTER1			0
#define SPI_MASTER2			1
#define SPI_MASTER3			2

/* ==========================================================================*/
/* SPI_FIFO_SIZE */
#define SPI_DATA_FIFO_SIZE_16		0x10
#define SPI_DATA_FIFO_SIZE_32		0x20
#define SPI_DATA_FIFO_SIZE_64		0x40
#define SPI_DATA_FIFO_SIZE_128		0x80

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define SPI_CTRLR0_OFFSET		0x00
#define SPI_CTRLR1_OFFSET		0x04
#define SPI_SSIENR_OFFSET		0x08
#define SPI_MWCR_OFFSET			0x0c // no PRM explain it and no code use it. should be commented after check
#define SPI_SER_OFFSET			0x10
#define SPI_BAUDR_OFFSET		0x14
#define SPI_TXFTLR_OFFSET		0x18
#define SPI_RXFTLR_OFFSET		0x1c
#define SPI_TXFLR_OFFSET		0x20
#define SPI_RXFLR_OFFSET		0x24
#define SPI_SR_OFFSET			0x28
#define SPI_IMR_OFFSET			0x2c
#define SPI_ISR_OFFSET			0x30
#define SPI_RISR_OFFSET			0x34
#define SPI_TXOICR_OFFSET		0x38
#define SPI_RXOICR_OFFSET		0x3c
#define SPI_RXUICR_OFFSET		0x40
#define SPI_MSTICR_OFFSET		0x44
#define SPI_ICR_OFFSET			0x48
#define SPI_DMAC_OFFSET			0x4c
#define SPI_IDR_OFFSET			0x58
#define SPI_VERSION_ID_OFFSET		0x5c
#define SPI_DR_OFFSET			0x60

#define SPI_SSIENPOLR_OFFSET		0x260
#define SPI_SCLK_OUT_DLY_OFFSET		0x264
#define SPI_START_BIT_OFFSET		0x268

/* ==========================================================================*/
/* SPI rw mode */
#define SPI_WRITE_READ		0
#define SPI_WRITE_ONLY		1
#define SPI_READ_ONLY		2

/* Tx FIFO empty interrupt mask */
#define SPI_TXEIS_MASK		0x00000001
#define SPI_TXOIS_MASK 		0x00000002
#define SPI_RXFIS_MASK 		0x00000010
#define SPI_FCRIS_MASK 		0x00000100

/* SPI Parameters */
#define SPI_DUMMY_DATA		0xffff
#define MAX_QUERY_TIMES		10
#define SPI_POLLING_MAX_WAIT_LOOP 5000000

/* Default SPI settings */
#define SPI_SCPOL		0
#define SPI_SCPH		0
#define SPI_FRF			0
#define SPI_CFS			0x0
#define SPI_DFS			0xf
#define SPI_BAUD_RATE		200000

/* ==========================================================================*/
typedef struct ambarella_spi_cfg_info {
	u8	spi_mode;
	u8	cfs_dfs;
	u8	cs_gpio;
	u8	cs_change;
	u32	baud_rate;
} amba_spi_cfg_t;

#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST	0x08
/* ==========================================================================*/

#ifndef __ASM__
/* ==========================================================================*/
extern void spi_bld_init(u8 spi_id, amba_spi_cfg_t *spi_cfg);
extern int spi_bld_write(u8 spi_id, u8 *tx_buf, int tx_len);
extern int spi_bld_read(u8 spi_id, u8 *rx_buf, int rx_len);
extern int spi_bld_write_then_read(u8 spi_id, u8 *tx_buf, int tx_len, u8 *rx_buf, int rx_len);

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

