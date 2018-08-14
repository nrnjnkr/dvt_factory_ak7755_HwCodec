/*
 * ambhw/dma.h
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

#ifndef __AMBHW__DMA_H__
#define __AMBHW__DMA_H__

#include <ambhw/chip.h>

/* ==========================================================================*/
#if (CHIP_REV == CV1)
#define DMA_INSTANCES			2
#define NUM_DMA_CHANNELS 		16
#else
#define DMA_INSTANCES			1
#define NUM_DMA_CHANNELS 		8
#endif

#if (CHIP_REV == CV1)
#define DMA0_OFFSET			0xA000
#else
#define DMA0_OFFSET			0x5000
#endif
#define DMA0_BASE			(AHB_N_BASE + DMA0_OFFSET)
#define DMA0_REG(x)			(DMA0_BASE + (x))

#define DMA1_OFFSET			0xB000
#define DMA1_BASE			(AHB_N_BASE + DMA1_OFFSET)
#define DMA1_REG(x)			(DMA1_BASE + (x))

/* ==========================================================================*/

#define INVALID_DMA_CHAN		0xFF

#define NOR_SPI_TX_DMA_CHAN		0
#define NOR_SPI_RX_DMA_CHAN		1
#define SSI1_TX_DMA_CHAN		2
#define SSI1_RX_DMA_CHAN		3
#define UART_TX_DMA_CHAN		4
#define UART_RX_DMA_CHAN		5
#define I2S_RX_DMA_CHAN			6
#define I2S_TX_DMA_CHAN			7

/* ==========================================================================*/
#define DMA0_CHAN_CTR_REG(x)		DMA0_REG((0x300 + ((x) << 4)))
#define DMA0_CHAN_SRC_REG(x)		DMA0_REG((0x304 + ((x) << 4)))
#define DMA0_CHAN_DST_REG(x)		DMA0_REG((0x308 + ((x) << 4)))
#define DMA0_CHAN_STA_REG(x)		DMA0_REG((0x30c + ((x) << 4)))
#define DMA0_CHAN_DA_REG(x)		DMA0_REG((0x380 + ((x) << 2)))

#define DMA1_CHAN_CTR_REG(x)		DMA1_REG((0x300 + ((x) << 4)))
#define DMA1_CHAN_SRC_REG(x)		DMA1_REG((0x304 + ((x) << 4)))
#define DMA1_CHAN_DST_REG(x)		DMA1_REG((0x308 + ((x) << 4)))
#define DMA1_CHAN_STA_REG(x)		DMA1_REG((0x30c + ((x) << 4)))
#define DMA1_CHAN_DA_REG(x)		DMA1_REG((0x380 + ((x) << 2)))

/* ==========================================================================*/

#define DMA_INT_OFFSET			0x3f0
#define DMA_PAUSE_SET_OFFSET		0x3f4
#define DMA_PAUSE_CLR_OFFSET		0x3f8
#define DMA_EARLY_END_OFFSET		0x3fc

/* ==========================================================================*/

/* DMA_CHANX_CTR_REG */
#define DMA_CHANX_CTR_EN		0x80000000
#define DMA_CHANX_CTR_D			0x40000000
#define DMA_CHANX_CTR_WM		0x20000000
#define DMA_CHANX_CTR_RM		0x10000000
#define DMA_CHANX_CTR_NI		0x08000000
#define DMA_CHANX_CTR_BLK_1024B		0x07000000
#define DMA_CHANX_CTR_BLK_512B		0x06000000
#define DMA_CHANX_CTR_BLK_256B		0x05000000
#define DMA_CHANX_CTR_BLK_128B		0x04000000
#define DMA_CHANX_CTR_BLK_64B		0x03000000
#define DMA_CHANX_CTR_BLK_32B		0x02000000
#define DMA_CHANX_CTR_BLK_16B		0x01000000
#define DMA_CHANX_CTR_BLK_8B		0x00000000
#define DMA_CHANX_CTR_TS_8B		0x00C00000
#define DMA_CHANX_CTR_TS_4B		0x00800000
#define DMA_CHANX_CTR_TS_2B		0x00400000
#define DMA_CHANX_CTR_TS_1B		0x00000000

/* DMA descriptor bit fields */
#define DMA_DESC_EOC			0x01000000
#define DMA_DESC_WM			0x00800000
#define DMA_DESC_RM			0x00400000
#define DMA_DESC_NI			0x00200000
#define DMA_DESC_TS_8B			0x00180000
#define DMA_DESC_TS_4B			0x00100000
#define DMA_DESC_TS_2B			0x00080000
#define DMA_DESC_TS_1B			0x00000000
#define DMA_DESC_BLK_1024B		0x00070000
#define DMA_DESC_BLK_512B		0x00060000
#define DMA_DESC_BLK_256B		0x00050000
#define DMA_DESC_BLK_128B		0x00040000
#define DMA_DESC_BLK_64B		0x00030000
#define DMA_DESC_BLK_32B		0x00020000
#define DMA_DESC_BLK_16B		0x00010000
#define DMA_DESC_BLK_8B			0x00000000
#define DMA_DESC_ID			0x00000004
#define DMA_DESC_IE			0x00000002
#define DMA_DESC_ST			0x00000001

/* DMA_CHANX_STA_REG */
#define DMA_CHANX_STA_DM		0x80000000
#define DMA_CHANX_STA_OE		0x40000000
#define DMA_CHANX_STA_DA		0x20000000
#define DMA_CHANX_STA_DD		0x10000000
#define DMA_CHANX_STA_OD		0x08000000
#define DMA_CHANX_STA_ME		0x04000000
#define DMA_CHANX_STA_BE		0x02000000
#define DMA_CHANX_STA_RWE		0x01000000
#define DMA_CHANX_STA_AE		0x00800000
#define DMA_CHANX_STA_DN		0x00400000

/* DMA_INT_REG */
#define DMA_INT_CHAN(x)			(0x1 << (x))

/* DMA_DUAL_SPACE_MODE_REG */
#define DMA_DSM_EN                      0x80000000
#define DMA_DSM_MAJP_2KB                0x00000090
#define DMA_DSM_SPJP_64B                0x00000004
#define DMA_DSM_SPJP_128B               0x00000005

/* ==========================================================================*/


struct dma_desc {
	u32 src;
	u32 dst;
	u32 next_desc;
	u32 rpt_addr;
	u32 xfr_count;
	u32 attr;
	u32 rsvd;
	u32 rpt;
};

#endif /* __AMBHW__DMA_H__ */

