/**
 * ambhw/nand_legacy.h
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

#ifndef __AMBHW_NAND_LEGACY_H__
#define __AMBHW_NAND_LEGACY_H__

#include <ambhw/chip.h>

/* ==========================================================================*/

#define FIO_FIFO_OFFSET			0x0000
#define FIO_OFFSET			0x1000
#define FIO_4K_OFFSET			0x30000

#define FIO_FIFO_BASE			(AHB_N_BASE + FIO_FIFO_OFFSET)
#define FIO_BASE			(AHB_N_BASE + FIO_OFFSET)
#define FIO_4K_BASE			(AHB_N_BASE + FIO_4K_OFFSET)

#define FIO_REG(x)			(FIO_BASE + (x))
#define FIO_4K_REG(x)			(FIO_4K_BASE + (x))

/* ==========================================================================*/

#if (CHIP_REV == S2L)  || (CHIP_REV == S3)
#define FIO_SUPPORT_SKIP_BLANK_ECC	0
#else
#define FIO_SUPPORT_SKIP_BLANK_ECC	1
#endif

#if (CHIP_REV == S2L) || (CHIP_REV == S3)
#define NAND_ECC_RPT_NUM_SUPPORT	0
#else
#define NAND_ECC_RPT_NUM_SUPPORT	1
#endif

#if (CHIP_REV == S2L) || (CHIP_REV == S3)
#define NAND_CUSTOM_CMD_SUPPORT	0
#else
#define NAND_CUSTOM_CMD_SUPPORT	1
#endif

/* ==========================================================================*/

#define FIO_CTR_OFFSET			0x000
#define FIO_STA_OFFSET			0x004
#define FIO_DMACTR_OFFSET		0x080
#define FIO_DMAADR_OFFSET		0x084
#define FIO_DMASTA_OFFSET		0x08c
#define FIO_DSM_CTR_OFFSET              0x0a0
#define FIO_ECC_RPT_STA_OFFSET          0x0a4

#define FLASH_CTR_OFFSET		0x120
#define FLASH_CMD_OFFSET		0x124
#define FLASH_TIM0_OFFSET		0x128
#define FLASH_TIM1_OFFSET		0x12c
#define FLASH_TIM2_OFFSET		0x130
#define FLASH_TIM3_OFFSET		0x134
#define FLASH_TIM4_OFFSET		0x138
#define FLASH_TIM5_OFFSET		0x13c
#define FLASH_STA_OFFSET		0x140
#define FLASH_ID_OFFSET			0x144
#define FLASH_CFI_OFFSET		0x148
#define FLASH_LEN_OFFSET		0x14c
#define FLASH_INT_OFFSET		0x150
#define FLASH_EX_CTR_OFFSET		0x15c
#define FLASH_EX_ID_OFFSET		0x160

/* followings are for customer command, start from S3L */
#define FLASH_TIM6_OFFSET		0x164
#define FLASH_CC_OFFSET			0x170
#define FLASH_CC_WORD_OFFSET		0x174
#define FLASH_CC_DATA0_OFFSET		0x180
#define FLASH_CC_DATA1_OFFSET		0x184
#define FLASH_CC_DATA2_OFFSET		0x188
#define FLASH_CC_DATA3_OFFSET		0x18c
#define FLASH_CC_DATA4_OFFSET		0x190
#define FLASH_CC_DATA5_OFFSET		0x194
#define FLASH_CC_DATA6_OFFSET		0x198
#define FLASH_CC_DATA7_OFFSET		0x19c

#define FIO_CTR_REG			FIO_REG(0x000)
#define FIO_STA_REG			FIO_REG(0x004)
#define FIO_DMACTR_REG			FIO_REG(0x080)
#define FIO_DMAADR_REG			FIO_REG(0x084)
#define FIO_DMASTA_REG			FIO_REG(0x08c)
#define FIO_DSM_CTR_REG			FIO_REG(0x0a0)
#define FIO_ECC_RPT_STA_REG		FIO_REG(0x0a4)

#define NAND_CTR_REG			FIO_REG(0x120)
#define NAND_CMD_REG			FIO_REG(0x124)
#define NAND_TIMING0_REG		FIO_REG(0x128)
#define NAND_TIMING1_REG		FIO_REG(0x12c)
#define NAND_TIMING2_REG		FIO_REG(0x130)
#define NAND_TIMING3_REG		FIO_REG(0x134)
#define NAND_TIMING4_REG		FIO_REG(0x138)
#define NAND_TIMING5_REG		FIO_REG(0x13c)
#define NAND_STA_REG			FIO_REG(0x140)
#define NAND_ID_REG			FIO_REG(0x144)
#define NAND_COPY_ADDR_REG		FIO_REG(0x148)
#define NAND_LEN_REG			FIO_REG(0x14c)
#define NAND_INT_REG			FIO_REG(0x150)
#define NAND_EXT_CTR_REG		FIO_REG(0x15c)
#define NAND_EXT_ID5_REG		FIO_REG(0x160)

/* followings are for customer command, start from S3L */
#define NAND_TIMING6_REG		FIO_REG(0x164)
#define NAND_CC_REG			FIO_REG(0x170)
#define NAND_CC_WORD_REG		FIO_REG(0x174)
#define NAND_CC_DATA0_REG		FIO_REG(0x180)
#define NAND_CC_DATA1_REG		FIO_REG(0x184)
#define NAND_CC_DATA2_REG		FIO_REG(0x188)
#define NAND_CC_DATA3_REG		FIO_REG(0x18c)
#define NAND_CC_DATA4_REG		FIO_REG(0x190)
#define NAND_CC_DATA5_REG		FIO_REG(0x194)
#define NAND_CC_DATA6_REG		FIO_REG(0x198)
#define NAND_CC_DATA7_REG		FIO_REG(0x19c)

#define NAND_READ_CMDWORD_REG		FIO_REG(0x154)
#define NAND_PROG_CMDWORD_REG		FIO_REG(0x158)
#define NAND_CMDWORD1(x)		((x) << 16)
#define NAND_CMDWORD2(x)		((x) & 0xff)

#define NAND_CPS1_ADDR_REG		FIO_REG(0x154)
#define NAND_CPD1_ADDR_REG		FIO_REG(0x158)
#define NAND_CPS2_ADDR_REG		FIO_REG(0x15c)
#define NAND_CPD2_ADDR_REG		FIO_REG(0x160)
#define NAND_CPS3_ADDR_REG		FIO_REG(0x164)
#define NAND_CPD3_ADDR_REG		FIO_REG(0x168)
#define NAND_NBR_SPA_REG		FIO_REG(0x16c)


/* ==========================================================================*/

/* FIO_CTR_REG */
#define FIO_CTR_DA			0x00020000
#define FIO_CTR_DR			0x00010000
#define FIO_CTR_SX			0x00000100
#if (FIO_SUPPORT_SKIP_BLANK_ECC == 1)
#define FIO_CTR_SKIP_BLANK		0x00000080
#else
#define FIO_CTR_SKIP_BLANK		0x00000000
#endif
#define FIO_CTR_ECC_8BIT		0x00000060
#define FIO_CTR_ECC_6BIT		0x00000040
#define FIO_CTR_RS			0x00000010
#define FIO_CTR_SE			0x00000008
#define FIO_CTR_CO			0x00000004
#define FIO_CTR_RR			0x00000002
#define FIO_CTR_XD			0x00000001

/* FIO_STA_REG */
#define FIO_STA_SI			0x00000008
#define FIO_STA_CI			0x00000004
#define FIO_STA_XI			0x00000002
#define FIO_STA_FI			0x00000001

/* FIO_DMACTR_REG */
#define FIO_DMACTR_EN			0x80000000
#define FIO_DMACTR_RM			0x40000000
#define FIO_DMACTR_SD			0x30000000
#define FIO_DMACTR_CF			0x20000000
#define FIO_DMACTR_XD			0x10000000
#define FIO_DMACTR_FL			0x00000000
#define FIO_DMACTR_BLK_32768B		0x0c000000
#define FIO_DMACTR_BLK_16384B		0x0b000000
#define FIO_DMACTR_BLK_8192B		0x0a000000
#define FIO_DMACTR_BLK_4096B		0x09000000
#define FIO_DMACTR_BLK_2048B		0x08000000
#define FIO_DMACTR_BLK_1024B		0x07000000
#define FIO_DMACTR_BLK_256B		0x05000000
#define FIO_DMACTR_BLK_512B		0x06000000
#define FIO_DMACTR_BLK_128B		0x04000000
#define FIO_DMACTR_BLK_64B		0x03000000
#define FIO_DMACTR_BLK_32B		0x02000000
#define FIO_DMACTR_BLK_16B		0x01000000
#define FIO_DMACTR_BLK_8B		0x00000000
#define FIO_DMACTR_TS8B			0x00c00000
#define FIO_DMACTR_TS4B			0x00800000
#define FIO_DMACTR_TS2B			0x00400000
#define FIO_DMACTR_TS1B			0x00000000

/* FIO_DMASTA_REG */
#define FIO_DMASTA_RE			0x04000000
#define FIO_DMASTA_AE			0x02000000
#define FIO_DMASTA_DN			0x01000000

/* FIO_DSM_CTR_REG */
#define FIO_DSM_EN			0x80000000
#define FIO_DSM_MAJP_2KB		0x00000090
#define FIO_DSM_SPJP_64B		0x00000004
#define FIO_DSM_SPJP_128B		0x00000005

/* FIO_ECC_RPT_REG */
#define FIO_ECC_RPT_ERR			0x80000000
#define FIO_ECC_RPT_FAIL		0x40000000

/* NAND_CTR_REG */
#define NAND_CTR_A(x)			((x) << 28)
#define NAND_CTR_SA			0x08000000
#define NAND_CTR_SE			0x04000000
#define NAND_CTR_C2			0x02000000
#define NAND_CTR_P3			0x01000000
#define NAND_CTR_I4			0x00800000
#define NAND_CTR_RC			0x00400000
#define NAND_CTR_CC			0x00200000
#define NAND_CTR_CE			0x00100000
#define NAND_CTR_EC_MAIN		0x00080000
#define NAND_CTR_EC_SPARE		0x00040000
#define NAND_CTR_EG_MAIN		0x00020000
#define NAND_CTR_EG_SPARE		0x00010000
#define NAND_CTR_WP			0x00000200
#define NAND_CTR_IE			0x00000100
#define NAND_CTR_XS			0x00000080
#define NAND_CTR_SZ_8G			0x00000070
#define NAND_CTR_SZ_4G			0x00000060
#define NAND_CTR_SZ_2G			0x00000050
#define NAND_CTR_SZ_1G			0x00000040
#define NAND_CTR_SZ_512M		0x00000030
#define NAND_CTR_SZ_256M		0x00000020
#define NAND_CTR_SZ_128M		0x00000010
#define NAND_CTR_SZ_64M			0x00000000
#define NAND_CTR_4BANK			0x00000008
#define NAND_CTR_2BANK			0x00000004
#define NAND_CTR_1BANK			0x00000000
#define NAND_CTR_WD_64BIT		0x00000003
#define NAND_CTR_WD_32BIT		0x00000002
#define NAND_CTR_WD_16BIT		0x00000001
#define NAND_CTR_WD_8BIT		0x00000000

#define NAND_CTR_INTLVE			0x80000000
#define NAND_CTR_SPRBURST		0x40000000
#define NAND_CFG_STAT_ENB		0x00002000
#define NAND_CTR_2INTL			0x00001000
#define NAND_CTR_K9			0x00000800

#define NAND_CTR_WAS			0x00000400

/* NAND_CMD_REG */
#define NAND_AMB_CMD_ADDR(x)		((x) << 4)
#define NAND_AMB_CMD_NOP		0x0
#define NAND_AMB_CMD_DMA		0x1
#define NAND_AMB_CMD_RESET		0x2
#define NAND_AMB_CMD_NOP2		0x3
#define NAND_AMB_CMD_NOP3		0x4
#define NAND_AMB_CMD_NOP4		0x5
#define NAND_AMB_CMD_NOP5		0x6
#define NAND_AMB_CMD_COPYBACK		0x7
#define NAND_AMB_CMD_NOP6		0x8
#define NAND_AMB_CMD_ERASE		0x9
#define NAND_AMB_CMD_READID		0xa
#define NAND_AMB_CMD_NOP7		0xb
#define NAND_AMB_CMD_READSTATUS		0xc
#define NAND_AMB_CMD_NOP8		0xd
#define NAND_AMB_CMD_READ		0xe
#define NAND_AMB_CMD_PROGRAM		0xf

/* NAND_CC_REG */
#define NAND_CC_TERM_CE_HIGH	0x80000000
#define NAND_CC_CMD1_VAL0		0x00004000
#define NAND_CC_CMD1_VAL1		0x00008000
#define NAND_CC_CMD2_VAL0		0x00000200
#define NAND_CC_CMD2_VAL1		0x00000400
#define NAND_CC_RW_WE			0x00000080
#define NAND_CC_RW_RE			0x00000100
#define NAND_CC_WAIT_RB			0x00000020
#define NAND_CC_WAIT_TWHR		0x00000040

/* NAND_INT_REG (NAND mode) */
#define NAND_INT_DI			0x1

/* NAND_EXT_CTR_REG */
#define NAND_EXT_CTR_I5			0x00800000
#define NAND_EXT_CTR_SP_2X		0x00000001

/* ==========================================================================*/

#define FDMA_OFFSET			0x12000
#define FDMA_BASE			(AHB_N_BASE + FDMA_OFFSET)
#define FDMA_REG(x)			(FDMA_BASE + (x))

#define FDMA_CTR_OFFSET			0x300
#define FDMA_SRC_OFFSET			0x304
#define FDMA_DST_OFFSET			0x308
#define FDMA_STA_OFFSET			0x30c
#define FDMA_DA_OFFSET			0x380
#define FDMA_DSM_CTR_OFFSET		0x3a0
#define FDMA_INT_OFFSET			0x3f0

#define FDMA_SPR_CNT_OFFSET		0x200
#define FDMA_SPR_SRC_OFFSET		0x204
#define FDMA_SPR_DST_OFFSET		0x208
#define FDMA_SPR_STA_OFFSET		0x20c
#define FDMA_SPR_DA_OFFSET		0x280

#define FDMA_CHAN_CTR_REG		FDMA_REG(FDMA_CTR_OFFSET)
#define FDMA_CHAN_SRC_REG		FDMA_REG(FDMA_SRC_OFFSET)
#define FDMA_CHAN_DST_REG		FDMA_REG(FDMA_DST_OFFSET)
#define FDMA_CHAN_STA_REG		FDMA_REG(FDMA_STA_OFFSET)
#define FDMA_CHAN_DA_REG		FDMA_REG(FDMA_DA_OFFSET)
#define FDMA_CHAN_DSM_CTR_REG		FDMA_REG(FDMA_DSM_CTR_OFFSET)

#define FDMA_CHAN_SPR_CNT_REG		FDMA_REG(FDMA_SPR_CNT_OFFSET)
#define FDMA_CHAN_SPR_SRC_REG		FDMA_REG(FDMA_SPR_SRC_OFFSET)
#define FDMA_CHAN_SPR_DST_REG		FDMA_REG(FDMA_SPR_DST_OFFSET)
#define FDMA_CHAN_SPR_STA_REG		FDMA_REG(FDMA_SPR_STA_OFFSET)
#define FDMA_CHAN_SPR_DA_REG		FDMA_REG(FDMA_SPR_DA_OFFSET)


/* FDMA_CTR_REG */
#define FDMA_CHAN_CTR_EN		0x80000000
#define FDMA_CHAN_CTR_D			0x40000000
#define FDMA_CHAN_CTR_WM		0x20000000
#define FDMA_CHAN_CTR_RM		0x10000000
#define FDMA_CHAN_CTR_NI		0x08000000
#define FDMA_CHAN_CTR_BLK_1024B		0x07000000
#define FDMA_CHAN_CTR_BLK_512B		0x06000000
#define FDMA_CHAN_CTR_BLK_256B		0x05000000
#define FDMA_CHAN_CTR_BLK_128B		0x04000000
#define FDMA_CHAN_CTR_BLK_64B		0x03000000
#define FDMA_CHAN_CTR_BLK_32B		0x02000000
#define FDMA_CHAN_CTR_BLK_16B		0x01000000
#define FDMA_CHAN_CTR_BLK_8B		0x00000000
#define FDMA_CHAN_CTR_TS_8B		0x00C00000
#define FDMA_CHAN_CTR_TS_4B		0x00800000
#define FDMA_CHAN_CTR_TS_2B		0x00400000
#define FDMA_CHAN_CTR_TS_1B		0x00000000

/* FDMA descriptor bit fields */
#define FDMA_DESC_EOC			0x01000000
#define FDMA_DESC_WM			0x00800000
#define FDMA_DESC_RM			0x00400000
#define FDMA_DESC_NI			0x00200000
#define FDMA_DESC_TS_8B			0x00180000
#define FDMA_DESC_TS_4B			0x00100000
#define FDMA_DESC_TS_2B			0x00080000
#define FDMA_DESC_TS_1B			0x00000000
#define FDMA_DESC_BLK_1024B		0x00070000
#define FDMA_DESC_BLK_512B		0x00060000
#define FDMA_DESC_BLK_256B		0x00050000
#define FDMA_DESC_BLK_128B		0x00040000
#define FDMA_DESC_BLK_64B		0x00030000
#define FDMA_DESC_BLK_32B		0x00020000
#define FDMA_DESC_BLK_16B		0x00010000
#define FDMA_DESC_BLK_8B		0x00000000
#define FDMA_DESC_ID			0x00000004
#define FDMA_DESC_IE			0x00000002
#define FDMA_DESC_ST			0x00000001

/* FDMA_STA_REG */
#define FDMA_CHAN_STA_DM		0x80000000
#define FDMA_CHAN_STA_OE		0x40000000
#define FDMA_CHAN_STA_DA		0x20000000
#define FDMA_CHAN_STA_DD		0x10000000
#define FDMA_CHAN_STA_OD		0x08000000
#define FDMA_CHAN_STA_ME		0x04000000
#define FDMA_CHAN_STA_BE		0x02000000
#define FDMA_CHAN_STA_RWE		0x01000000
#define FDMA_CHAN_STA_AE		0x00800000
#define FDMA_CHAN_STA_DN		0x00400000

/* ==========================================================================*/

#define EC_MDSD			0 /* check main disable and spare disable */
#define EC_MDSE			1 /* check main disable and spare enable */
#define EC_MESD			2 /* check main enable and spare disable */
#define EC_MESE			3 /* check main enable and spare enable */

#define EG_MDSD			0 /* generate main disable and spare disable */
#define EG_MDSE			1 /* generate main disable and spare enable */
#define EG_MESD			2 /* generate main enable and spare disable */
#define EG_MESE			3 /* generate main enable and spare enable */

#define MAIN_ONLY		0
#define SPARE_ONLY		1
#define MAIN_ECC		2
#define SPARE_ECC		3

#if (CHIP_REV == S2L)
#define FDMA_NODC_MN_BURST_SIZE	(FDMA_CHAN_CTR_BLK_512B | FDMA_CHAN_CTR_TS_4B)
#define FDMA_NODC_SP_BURST_SIZE	(FDMA_CHAN_CTR_BLK_16B | FDMA_CHAN_CTR_TS_4B)
#define FDMA_DESC_MN_BURST_SIZE	(FDMA_DESC_BLK_512B | FDMA_DESC_TS_4B)
#define FDMA_DESC_SP_BURST_SIZE	(FDMA_DESC_BLK_16B | FDMA_DESC_TS_4B)
#else
#define FDMA_NODC_MN_BURST_SIZE	(FDMA_CHAN_CTR_BLK_512B | FDMA_CHAN_CTR_TS_8B)
#define FDMA_NODC_SP_BURST_SIZE	(FDMA_CHAN_CTR_BLK_16B | FDMA_CHAN_CTR_TS_8B)
#define FDMA_DESC_MN_BURST_SIZE	(FDMA_DESC_BLK_512B | FDMA_DESC_TS_8B)
#define FDMA_DESC_SP_BURST_SIZE	(FDMA_DESC_BLK_16B | FDMA_DESC_TS_8B)
#endif

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
extern int nand_read_spare(u32 block, u32 page, u32 pages, u8 *buf);
extern int nand_prog_spare(u32 block, u32 page, u32 pages, u8 *buf);
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

