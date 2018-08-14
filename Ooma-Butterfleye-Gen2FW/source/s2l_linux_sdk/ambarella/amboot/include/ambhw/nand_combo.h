/**
 * ambhw/nand_combo.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __AMBHW_NAND_COMBO_H__
#define __AMBHW_NAND_COMBO_H__

#include <ambhw/chip.h>

/* ==========================================================================*/

#define FIO_OFFSET			0x2000
#define FIO_4K_OFFSET			0x3000

#define FIO_BASE			(AHB_N_BASE + FIO_OFFSET)
#define FIO_4K_BASE			(AHB_N_BASE + FIO_4K_OFFSET)

#define FIO_REG(x)			(FIO_BASE + (x))
#define FIO_4K_REG(x)			(FIO_4K_BASE + (x))

/* Flash I/O Subsystem */
#define FIO_CTRL_OFFSET			0x00
#define FIO_RAW_INT_STATUS_OFFSET	0x04
#define FIO_CTRL2_OFFSET		0x08
#define FIO_INT_ENABLE_OFFSET		0x0C
#define FIO_INT_STATUS_OFFSET		0x10
#define FIO_ECC_RPT_CFG_OFFSET		0xA0
#define FIO_ECC_RPT_STATUS_OFFSET	0xA4
#define FIO_ECC_RPT_STATUS2_OFFSET	0xA8

#define FIO_CTRL_REG			FIO_REG(0x00)
#define FIO_RAW_INT_STATUS_REG		FIO_REG(0x04)
#define FIO_CTRL2_REG			FIO_REG(0x08)
#define FIO_INT_ENABLE_REG		FIO_REG(0x0C)
#define FIO_INT_STATUS_REG		FIO_REG(0x10)
#define FIO_ECC_RPT_CFG_REG		FIO_REG(0xA0)
#define FIO_ECC_RPT_STATUS_REG		FIO_REG(0xA4)
#define FIO_ECC_RPT_STATUS2_REG		FIO_REG(0xA8)

/* NAND Flash Controller */
#define NAND_CTRL_OFFSET		0x120
#define NAND_CMD_OFFSET			0x124
#define NAND_TIMING0_OFFSET		0x128
#define NAND_TIMING1_OFFSET		0x12c
#define NAND_TIMING2_OFFSET		0x130
#define NAND_TIMING3_OFFSET		0x134
#define NAND_TIMING4_OFFSET		0x138
#define NAND_TIMING5_OFFSET		0x13c
#define NAND_STATUS_OFFSET		0x140
#define NAND_ID_OFFSET			0x144
#define NAND_COPY_ADDR_OFFSET		0x148
#define NAND_EXT_CTRL_OFFSET		0x15c
#define NAND_EXT_ID_OFFSET		0x160
#define NAND_TIMING6_OFFSET		0x164

#define NAND_CC_OFFSET			0x170
#define NAND_CC_WORD_OFFSET		0x174
#define NAND_CP_ADDR_H_OFFSET		0x17C
#define NAND_CC_DAT0_OFFSET		0x180
#define NAND_CC_DAT1_OFFSET		0x184
#define NAND_CC_DAT2_OFFSET		0x188
#define NAND_CC_DAT3_OFFSET		0x18C
#define NAND_CC_DAT4_OFFSET		0x190
#define NAND_CC_DAT5_OFFSET		0x194
#define NAND_CC_DAT6_OFFSET		0x198
#define NAND_CC_DAT7_OFFSET		0x19C

#define NAND_CTRL_REG			FIO_REG(0x120)
#define NAND_CMD_REG			FIO_REG(0x124)
#define NAND_TIMING0_REG		FIO_REG(0x128)
#define NAND_TIMING1_REG		FIO_REG(0x12c)
#define NAND_TIMING2_REG		FIO_REG(0x130)
#define NAND_TIMING3_REG		FIO_REG(0x134)
#define NAND_TIMING4_REG		FIO_REG(0x138)
#define NAND_TIMING5_REG		FIO_REG(0x13c)
#define NAND_STATUS_REG			FIO_REG(0x140)
#define NAND_ID_REG			FIO_REG(0x144)
#define NAND_COPY_ADDR_REG		FIO_REG(0x148)
#define NAND_EXT_CTRL_REG		FIO_REG(0x15c)
#define NAND_EXT_ID_REG			FIO_REG(0x160)
#define NAND_TIMING6_REG		FIO_REG(0x164)

#define NAND_CC_REG			FIO_REG(0x170)
#define NAND_CC_WORD_REG		FIO_REG(0x174)
#define NAND_CP_ADDR_H_REG		FIO_REG(0x17C)
#define NAND_CC_DAT0_REG		FIO_REG(0x180)
#define NAND_CC_DAT1_REG		FIO_REG(0x184)
#define NAND_CC_DAT2_REG		FIO_REG(0x188)
#define NAND_CC_DAT3_REG		FIO_REG(0x18C)
#define NAND_CC_DAT4_REG		FIO_REG(0x190)
#define NAND_CC_DAT5_REG		FIO_REG(0x194)
#define NAND_CC_DAT6_REG		FIO_REG(0x198)
#define NAND_CC_DAT7_REG		FIO_REG(0x19C)

/* FDMA Engine */
#define FDMA_SP_MEM_ADDR_OFFSET		0x204
#define FDMA_MN_CTRL_OFFSET		0x300
#define FDMA_MN_MEM_ADDR_OFFSET		0x304
#define FDMA_MN_STATUS_OFFSET		0x30c
#define FDMA_MN_DESC_ADDR_OFFSET	0x380
#define FDMA_DSM_CTRL_OFFSET		0x3a0

#define FDMA_SP_MEM_ADDR_REG		FIO_REG(0x204)
#define FDMA_MN_CTRL_REG		FIO_REG(0x300)
#define FDMA_MN_MEM_ADDR_REG		FIO_REG(0x304)
#define FDMA_MN_STATUS_REG		FIO_REG(0x30c)
#define FDMA_MN_DESC_ADDR_REG		FIO_REG(0x380)
#define FDMA_DSM_CTRL_REG		FIO_REG(0x3a0)

/*
 * Bitwise Definition for Flash I/O Subsystem
 */

/* FIO_CTRL_REG */
#define FIO_CTRL_SKIP_BLANK_ECC		0x00000080
#define FIO_CTRL_ECC_BCH_ENABLE		0x00000040
#define FIO_CTRL_ECC_BCH8		0x00000020 /* this bit is RO */
#define FIO_CTRL_RDERR_STOP		0x00000008
#define FIO_CTRL_RANDOM_READ		0x00000002

/* FIO_RAW_INT_STATUS_REG */
/* FIO_INT_ENABLE_REG */
/* FIO_INT_STATUS_REG */
#define FIO_INT_ECC_RPT_UNCORR		0x00000020
#define FIO_INT_ECC_RPT_THRESH		0x00000010
#define FIO_INT_AXI_BUS_ERR		0x00000004
#define FIO_INT_SND_LOOP_TIMEOUT	0x00000002
#define FIO_INT_OPERATION_DONE		0x00000001

/* FIO_ECC_RPT_CFG_REG */
#define FIO_ECC_RPT_ERR_NUM_TH(x)	((x) << 16)

/* FIO_ECC_RPT_STATUS_REG */
#define FIO_ECC_RPT_ERR_DET		0x80000000
#define FIO_ECC_RPT_CORR_FAIL		0x40000000
#define FIO_ECC_RPT_CORR_FAIL_OV	0x20000000
#define FIO_ECC_RPT_MAX_ERR_NUM(v)	(((v) >> 20) & 0xf)
#define FIO_ECC_RPT_BLK_ADDR(v)		((v) & 0xffff)

/* FIO_ECC_RPT_STATUS2_REG */
#define FIO_ECC_RPT_UNCORR_BLK_ADDR(v)	((v) & 0xffff)


/*
 * Bitwise Definition for NAND Flash Controller
 */

/* NAND_CTRL_REG */
#define NAND_CTRL_A33_32(x)		((x) << 28)
#define NAND_CTRL_SPARE_ADDR		0x08000000
#define NAND_CTRL_P3			0x01000000
#define NAND_CTRL_I4			0x00800000
#define NAND_CTRL_WAS			0x00000400
#define NAND_CTRL_WP			0x00000200
#define NAND_CTRL_SIZE_8G		0x00000070
#define NAND_CTRL_SIZE_4G		0x00000060
#define NAND_CTRL_SIZE_2G		0x00000050
#define NAND_CTRL_SIZE_1G		0x00000040
#define NAND_CTRL_SIZE_512M		0x00000030
#define NAND_CTRL_SIZE_256M		0x00000020
#define NAND_CTRL_SIZE_128M		0x00000010
#define NAND_CTRL_SIZE_64M		0x00000000

/* NAND_CMD_REG */
#define NAND_CMD_A31_4(x)		((x) & 0xfffffff0)
#define NAND_CMD_CMD(c)			((c) & 0xf)

/* NAND_EXT_CTRL_REG */
#define NAND_EXT_CTRL_4K_PAGE		0x02000000
#define NAND_EXT_CTRL_DEV_SZ3		0x01000000
#define NAND_EXT_CTRL_I5		0x00800000
#define NAND_EXT_CTRL_SPARE_2X		0x00000001

/* NAND_CC_REG */
#define NAND_CC_TERMINATE_CE		0x80000000
#define NAND_CC_DATA_SRC_REGISTER	0x00000000
#define NAND_CC_DATA_SRC_DMA		0x00400000
#define NAND_CC_ADDR2_NI		0x00080000
#define NAND_CC_ADDR1_NI		0x00040000
#define NAND_CC_ADDR_SRC(x)		((x) << 16)
#define NAND_CC_CMD1(x)			((x) << 14)
#define NAND_CC_ADDR_CYCLE(x)		((x) << 11)
#define NAND_CC_CMD2(x)			((x) << 9)
#define NAND_CC_RW_READ			0x00000100
#define NAND_CC_RW_WRITE		0x00000080
#define NAND_CC_RW_NODATA		0x00000000
#define NAND_CC_WAIT_RB			0x00000020
#define NAND_CC_WAIT_TWHR		0x00000040
#define NAND_CC_DATA_CYCLE(x)		((x) << 0)

/* NAND_CC_WORD_REG */
#define NAND_CC_WORD_CMD1VAL0(c)	((c) & 0xff)
#define NAND_CC_WORD_CMD1VAL1(c)	(((c) & 0xff) << 8)
#define NAND_CC_WORD_CMD2VAL0(c)	(((c) & 0xff) << 16)
#define NAND_CC_WORD_CMD2VAL1(c)	(((c) & 0xff) << 24)


/*
 * Bitwise Definition for FDMA Engine
 */

/* FDMA_MN_CTRL_REG */
#define FDMA_CTRL_ENABLE		0x80000000
#define FDMA_CTRL_DESC_MODE		0x40000000
#define FDMA_CTRL_WRITE_MEM		0x20000000
#define FDMA_CTRL_READ_MEM		0x00000000
#define FDMA_CTRL_BLK_SIZE_32B		0x02000000
#define FDMA_CTRL_BLK_SIZE_64B		0x03000000
#define FDMA_CTRL_BLK_SIZE_128B		0x04000000
#define FDMA_CTRL_BLK_SIZE_256B		0x05000000
#define FDMA_CTRL_BLK_SIZE_512B		0x06000000
#define FDMA_CTRL_BLK_SIZE_1024B	0x07000000

/* FDMA_MN_STATUS_REG */
#define FDMA_STATUS_DESC_BUS_ERR	0x40000000
#define FDMA_STATUS_DESC_ADDR_ERR	0x20000000
#define FDMA_STATUS_DESC_DMA_DONE	0x10000000
#define FDMA_STATUS_DESC_1ST_DONE	0x08000000
#define FDMA_STATUS_DMA_BUS_ERR		0x02000000
#define FDMA_STATUS_DMA_ADDR_ERR	0x00800000
#define FDMA_STATUS_DMA_DONE		0x00400000
#define FDMA_STATUS_XFER_COUNT(n)	((n) & 0x003fffff)

/* FDMA_DSM_CTRL_REG */
#define FDMA_DSM_SPARE_JP_SIZE_16B	0x04
#define FDMA_DSM_SPARE_JP_SIZE_32B	0x05
#define FDMA_DSM_MAIN_JP_SIZE_512B	0x90

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

#if (CHIP_REV == S2L)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{54, 55, 56, 61, 62, 63, 64, 65, \
					 66, 67, 68, 69, 70, 71, 72}
#elif (CHIP_REV == S3)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{115, 116, 117, 122, 123, 124, 125, \
					 126, 127, 128, 129, 130, 131, 132, 133}
#elif (CHIP_REV == S3L)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{55, 56, 57, 62, 63, 64, 65, 66, \
					 67, 68, 69, 70, 71, 72, 73}
#elif (CHIP_REV == S5)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{56, 57, 58, 63, 64, 65, 66, 67, \
					 68, 69, 70, 71, 72, 73, 74}
#elif (CHIP_REV == S5L)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{72, 73, 74, 79, 80, 81, 82, 83, \
					 84, 85, 86, 87, 88, 89, 90}
#elif (CHIP_REV == CV1)
#define MINIPIN_NAND_ALTFUNC		2
#define MINIPIN_NAND_PIN		{86, 87, 88, 93, 94, 95, 96, 97, \
					 98, 99, 100, 101, 102, 103, 104}
#else
#error "MINIPIN nand: not defined!"
#endif

extern int nand_init(void);
extern int nand_mark_bad_block(u32 block);
extern int nand_is_bad_block(u32 block);
extern int nand_correct_offset(u32 start_blk, u32 offset, u32 *c_offset);
extern void nand_output_bad_block(u32 block, int bb_type);
extern int nand_read_data(u8 *dst, u8 *src, int len);
extern int nand_read_pages(u32 block, u32 page, u32 pages, u8 *main_buf,
	u8 *spare_buf, u32 enable_ecc);
extern int nand_prog_pages(u32 block, u32 page, u32 pages, u8 *main_buf,
	u8 *spare_buf);
extern int nand_prog_pages_noecc(u32 block, u32 page, u32 pages, u8 *buf);
extern int nand_read_spare(u32 block, u32 page, u32 pages, u8 *spare_buf);
extern int nand_prog_spare(u32 block, u32 page, u32 pages, u8 *spare_buf);
extern int nand_erase_block(u32 block);
#if defined(CONFIG_NAND_USE_FLASH_BBT)
extern int nand_scan_bbt(int verbose);
extern int nand_update_bbt(u32 bblock, u32 gblock);
extern int nand_erase_bbt(void);
extern int nand_isbad_bbt(u32 block);
extern int nand_show_bbt(void);
extern int nand_has_bbt(void);
#endif

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

