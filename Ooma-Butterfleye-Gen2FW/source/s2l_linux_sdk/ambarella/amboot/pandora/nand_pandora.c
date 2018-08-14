/**
 * bld/nand.c
 *
 * Flash controller functions with NAND chips.
 *
 * History:
 *    2005/02/15 - [Charles Chiou] created file
 *    2006/07/26 - [Charles Chiou] converted to DMA descriptor-mode
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
#include <ambhw/nand.h>
#include <ambhw/dma.h>
#include <ambhw/cache.h>
#include <fio/ftl_const.h>
#include <flash/nanddb.h>
#include <pandora.h>
#include <irq.h>

/* ==========================================================================*/
#define NAND_CMD_NOP			0x0
#define NAND_CMD_DMA			0x1
#define NAND_CMD_RESET			0x2
#define NAND_CMD_NOP2			0x3
#define NAND_CMD_NOP3			0x4
#define NAND_CMD_NOP4			0x5
#define NAND_CMD_NOP5			0x6
#define NAND_CMD_COPYBACK		0x7
#define NAND_CMD_NOP6			0x8
#define NAND_CMD_ERASE			0x9
#define NAND_CMD_READID			0xa
#define NAND_CMD_NOP7			0xb
#define NAND_CMD_READSTATUS		0xc
#define NAND_CMD_NOP8			0xd
#define NAND_CMD_READ			0xe
#define NAND_CMD_PROGRAM		0xf

#define NAND_CMD_TIMEOUT		1000
#define NAND_DMA_TIMEOUT		1000

/* ==========================================================================*/
extern flnand_t flnand;

/**
 * DMA descriptor.
 */
struct fio_dmadesc_s
{
	u32	src_addr;	/**< Source address */
	u32	dst_addr;	/**< Destination address */
	u32	next;		/**< Next descriptor */
	u32	rpt_addr;	/**< Report address */
	u32	xfrcnt;		/**< Transfer count */
	u32	ctrl;		/**< Control */
	u32	rsv0;		/**< Reserved */
	u32	rsv1;		/**< Reserved */
	u32	rpt;		/**< Report */
	u32	rsv2;		/**< Reserved */
	u32	rsv3;		/**< Reserved */
	u32	rsv4;		/**< Reserved */
} __attribute__((packed));

static struct fio_dmadesc_s G_fio_dmadesc __attribute__((aligned(32)));
static struct fio_dmadesc_s G_fio_dma_spr_desc __attribute__((aligned(32)));

#define PAGE_SIZE_512		512
#define PAGE_SIZE_2K		2048
#define MAX_SPARE_SIZE_BLK	8192

#define ECC_STEPS			4
static u8 buffer[PAGE_SIZE_2K] __attribute__ ((aligned(32)));
static u8 dummy_buffer_bch[MAX_SPARE_SIZE_BLK] __attribute__ ((aligned(32)));

static pandora_mutex_t nand_read_mutex;
static completion_t dma_completion;

/* ==========================================================================*/

static void nand_wait_cmd_done(u32 cmd)
{
	writel(NAND_CMD_REG, cmd);

	rct_timer2_reset_count();

	while(1) {
		if (readl(NAND_INT_REG) & NAND_INT_DI)
			break;

		if (rct_timer2_get_count() >= NAND_CMD_TIMEOUT) {
			putstr("nand cmd timeout: ");
			puthex(cmd);
			putstr("\r\n");
			while(1);
		}
	}

	writel(NAND_INT_REG, 0x0);
}

#if 0
static void nand_wait_dma_done(void)
{
	rct_timer2_reset_count();

	while(1) {
		if ((readl(NAND_INT_REG) & NAND_INT_DI) &&
			(readl(FDMA_REG(DMA_INT_OFFSET)) & 0x1) &&
			(readl(FIO_DMASTA_REG) & FIO_DMASTA_DN) &&
			!(readl(FDMA_CHAN_CTR_REG) & FDMA_CHAN_CTR_EN) &&
			!(readl(FIO_DMACTR_REG) & FIO_DMACTR_EN))
			break;

		if (rct_timer2_get_count() >= NAND_DMA_TIMEOUT) {
			putstr("nand dma timeout\r\n");
			while(1);
		}
	}
}
#else
static void nand_wait_dma_done(void)
{
	wait_for_completion_timeout(&dma_completion, 1000);
}
#endif

static void fio_dma_handler(void *data)
{
	writel(FIO_DMASTA_REG, 0x0);
	writel(FIO_ECC_RPT_STA_REG, 0x0);
	complete_isr(&dma_completion);
}

#if 0
static void nand_wait_desc_dma_done(void)
{
	rct_timer2_reset_count();

	while(1) {
		_clean_flush_d_cache();

		if ((readl(NAND_INT_REG) & NAND_INT_DI) &&
			(readl(FDMA_REG(DMA_INT_OFFSET)) & 0x1) &&
			(readl(FIO_DMASTA_REG) & FIO_DMASTA_DN) &&
			(G_fio_dmadesc.rpt & FDMA_CHAN_STA_DN))
			break;

		if (rct_timer2_get_count() >= NAND_DMA_TIMEOUT) {
			putstr("nand desc dma timeout\r\n");
			while(1);
		}
	}
}
#else

static void nand_wait_desc_dma_done(void)
{
	wait_for_completion_timeout(&dma_completion, 1000);
}
#endif

/* ==========================================================================*/

/**
 * Calculate address from the (block, page) pair.
 */
u32 addr_from_block_page(u32 block, u32 page)
{
	u32 rval = ((block * flnand.pages_per_block) + page);

	if (flnand.main_size == PAGE_SIZE_512)
		return (rval << 9);
	else if (flnand.main_size == PAGE_SIZE_2K)
		return (rval << 11);

	return -1;
}

#define hweight8(w)		\
      (	(!!((w) & (0x01 << 0))) +	\
	(!!((w) & (0x01 << 1))) +	\
	(!!((w) & (0x01 << 2))) +	\
	(!!((w) & (0x01 << 3))) +	\
	(!!((w) & (0x01 << 4))) +	\
	(!!((w) & (0x01 << 5))) +	\
	(!!((w) & (0x01 << 6))) +	\
	(!!((w) & (0x01 << 7)))	)

static int count_zero_bits(u8 *buf, int size, int max_bits)
{
	int i, zero_bits = 0;

	for (i = 0; i < size; i++) {
		zero_bits += hweight8(~buf[i]);
		if (zero_bits > max_bits)
			break;
	}
	return zero_bits;
}


/*
	This func maybe not have effect when read (>1)pages data
	and we do not memset the erased page to 0xFF, if error bit < ecc_bits
*/
static int nand_bch_check_blank_pages(u32 pages, u8 *main, u8 *spare)
{
	u32 i, j;
	int zeroflip = 0;
	int oob_subset, main_subset;
	int zero_bits = 0;
	u8 *bsp;
	u8 *bufpos;

	bsp = spare;
	bufpos = main;
	main_subset = flnand.main_size / ECC_STEPS;
	oob_subset  = flnand.spare_size / ECC_STEPS;
	if (flnand.ecc_bits > 0x1) {
		for (i = 0; i < pages; i++) {
			zeroflip = 0;
			for (j = 0; j < ECC_STEPS; j++) {
				zero_bits = count_zero_bits(bufpos, main_subset,
								flnand.ecc_bits);
				if (zero_bits > flnand.ecc_bits)
					return -1;

				if (zero_bits)
					zeroflip = 1;

				zero_bits += count_zero_bits(bsp, oob_subset,
								flnand.ecc_bits);
				if (zero_bits > flnand.ecc_bits)
					return -1;

				bufpos += main_subset;
				bsp += oob_subset;
			}
			/* use zeroflip for declaring blank page status */
			if (zeroflip)
				printf("Erased blank page has bitflip \n");
		}
	}
	return 0;
}

static void nand_corrected_recovery(void)
{
	u32 fio_ctr_reg;

	/* FIO reset will just reset FIO registers, but will not affect
	 * Nand controller. */
	fio_ctr_reg = readl(FIO_CTR_REG);

	writel(FIO_RESET_REG, FIO_RESET_FIO_RST);
	rct_timer_dly_ms(1);
	writel(FIO_RESET_REG, 0x0);
	rct_timer_dly_ms(1);

	writel(FIO_CTR_REG, fio_ctr_reg);
}

/*
 * Set Flash_IO_dsm_control Register
 */
static void nand_en_bch()
{
	u32 fio_dsm_ctr = 0, fio_ctr_reg = 0, dma_dsm_ctr = 0;

	fio_ctr_reg = readl(FIO_CTR_REG);
	/* Setup FIO Dual Space Mode Control Register */
	if (flnand.ecc_bits > 0x1) {
		/* Using BCH */
		fio_dsm_ctr |= (FIO_DSM_EN | FIO_DSM_MAJP_2KB);
		dma_dsm_ctr |= (DMA_DSM_EN | DMA_DSM_MAJP_2KB);
		fio_ctr_reg |= (FIO_CTR_RS | FIO_CTR_CO);

		if (flnand.ecc_bits == 0x6) {
			fio_dsm_ctr |= FIO_DSM_SPJP_64B;
			dma_dsm_ctr |= DMA_DSM_SPJP_64B;
			fio_ctr_reg |=	FIO_CTR_ECC_6BIT;
		} else {
			fio_dsm_ctr |= FIO_DSM_SPJP_128B;
			dma_dsm_ctr |= DMA_DSM_SPJP_128B;
			fio_ctr_reg |=	FIO_CTR_ECC_8BIT;
			writel(NAND_EXT_CTR_REG, readl(NAND_EXT_CTR_REG) | NAND_EXT_CTR_SP_2X);
		}
	} else {
		/* Should not be here! */
		putstr("ECC bit is 0 or 1,Do not need to enable BCH,so it can not be here!\n\r");
	}

	if (FIO_SUPPORT_SKIP_BLANK_ECC)
		fio_ctr_reg |= FIO_CTR_SKIP_BLANK;

	writel(FIO_DSM_CTR_REG, fio_dsm_ctr);
	writel(FIO_CTR_REG, fio_ctr_reg);
	writel(FDMA_CHAN_DSM_CTR_REG, dma_dsm_ctr);
}

/*
 * Disable Flash_IO_dsm_control and Flash_IO_control Register
 */
static void nand_dis_bch()
{
	u32 fio_ctr_reg = 0;

	fio_ctr_reg = readl(FIO_CTR_REG);
	/* Setup FIO Dual Space Mode Control Register */
	fio_ctr_reg |= FIO_CTR_RS;
	fio_ctr_reg &= ~(FIO_CTR_CO |
			 FIO_CTR_ECC_6BIT |
			 FIO_CTR_ECC_8BIT);

	writel(FIO_CTR_REG, fio_ctr_reg);
}

/**
 * Check for bad block.
 */
int nand_is_bad_block(u32 block)
{
	int ret_val = -1, i;
	u8 sbuf[1024], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((uintptr_t)sbuf + 31) & (~31));

#if defined(CONFIG_NAND_USE_FLASH_BBT)
	if(nand_has_bbt())
		return nand_isbad_bbt(block);
#endif

	ret_val = nand_read_spare(block, 0, BAD_BLOCK_PAGES, sbuf_ptr);
	if (ret_val < 0) {
		putstr("check bad block failed >> "
				"read spare data error.\r\n");
		/* Treat as factory bad block */
		return NAND_INITIAL_BAD_BLOCK;
	}

	for (i = 0; i < INIT_BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + i * flnand.spare_size + 5);
		else
			bi = *(sbuf_ptr + i * flnand.spare_size);

		if (bi != 0xff)
			break;
	}


	/* Good block */
	if (i == INIT_BAD_BLOCK_PAGES)
		return NAND_GOOD_BLOCK;

	for (i = INIT_BAD_BLOCK_PAGES; i < BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + i * flnand.spare_size + 5);
		else
			bi = *(sbuf_ptr + i * flnand.spare_size);

		if (bi != 0xff)
			break;
	}

	if (i < BAD_BLOCK_PAGES) {
		/* Late developed bad blocks. */
		return NAND_LATE_DEVEL_BAD_BLOCK;
	} else {
		/* Initial invalid blocks. */
		return NAND_INITIAL_BAD_BLOCK;
	}
}

int nand_correct_offset(u32 start_blk, u32 offset, u32 *c_offset)
{
	int i = 0;
	u32 bad_blk = 0, valid_blk = 0;
	u32 blk_size = flnand.block_size;
	u32 offset_blk = ROUND_DOWN(offset, blk_size)/blk_size;

	for (i = 0, valid_blk = 0; valid_blk < offset_blk; i++) {
		if (nand_is_bad_block(start_blk + i) == NAND_GOOD_BLOCK) {
			valid_blk++;
		} else {
			bad_blk++;
		}
	}
	if (c_offset) {
		*c_offset = (bad_blk * blk_size) + offset;
	}

	return 0;
}

void nand_output_bad_block(u32 block, int bb_type)
{
	if (bb_type & NAND_INITIAL_BAD_BLOCK) {
		putstr("initial bad block. <block ");
	} else if (bb_type & NAND_LATE_DEVEL_BAD_BLOCK) {
		putstr("late developed bad block. <block ");
	} else {
		putstr("other bad block. <block ");
	}
	putdec(block);
	putstr(">\r\n");
	putstr("Try next block...\r\n");
}

/**
 * Mark a bad block.
 */
int nand_mark_bad_block(u32 block)
{
	int ret_val = -1, i;
	u8 sbuf[256], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((uintptr_t)sbuf + 31) & (~31));

#if defined(CONFIG_NAND_USE_FLASH_BBT)
	nand_update_bbt(block, 0);
#endif

	for (i = AMB_BB_START_PAGE; i < BAD_BLOCK_PAGES; i++) {
		memset(sbuf_ptr, 0xff, flnand.spare_size);
		if (flnand.main_size == 512) {
			*(sbuf_ptr + 5) = AMB_BAD_BLOCK_MARKER;
		} else {
			*sbuf_ptr = AMB_BAD_BLOCK_MARKER;
		}

		ret_val = nand_prog_spare(block, i, 1, sbuf_ptr);
		if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"write spare data error.\r\n");
			return ret_val;
		}

		ret_val = nand_read_spare(block, i, 1, sbuf_ptr);
		if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"read spare data error.\r\n");
			return ret_val;
		}

		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + 5);
		else
			bi = *sbuf_ptr;

		if (bi == 0xff) {
			putstr("mark bad block failed >> "
				"verify failed at block ");
			putdec(block);
			putstr("\r\n");
			return -1;
		}
	}

	return 0;
}

/**
 * Initialize NAND parameters.
 */
int nand_init(void)
{
	nand_db_t *nand_db;
	u32 id, id5, sys_config_val;
	int i, match = 0;
	flnand_t *fn = &flnand;
	int sblk, nblk, part_size[HAS_IMG_PARTS];

	mutex_init(&nand_read_mutex);
	init_completion(&dma_completion);

	request_irq(FIODMA_IRQ, IRQ_LEVEL_HIGH, fio_dma_handler, NULL);

	writel(FIO_DMACTR_REG,
	       (readl(FIO_DMACTR_REG) & 0xcfffffff) | FIO_DMACTR_FL);

	/* Force ReadID with 4-cycles */
	writel(NAND_CTR_REG, readl(NAND_CTR_REG) | NAND_CTR_I4);

	/* Reset chip */
	nand_wait_cmd_done(NAND_CMD_RESET);

	/* Read ID with maximun 5 times if id is 0. */
	for (i = 0; i < 5; i++) {
		nand_wait_cmd_done(NAND_CMD_READID);
		id = readl(NAND_ID_REG);
		if (id)
			break;
	}

	ASSERT(id == 0, "NAND chip absence?\n");

	/* Read ID5 with maximun 5 times if id is 0. */
	/* Disable NAND_CTR_I4 */
	writel(NAND_CTR_REG, readl(NAND_CTR_REG) & ~(NAND_CTR_I4));
	writel(NAND_EXT_CTR_REG, readl(NAND_EXT_CTR_REG) | NAND_EXT_CTR_I5);
	for (i = 0; i < 5; i++) {
		nand_wait_cmd_done(NAND_CMD_READID);
		id5 = readl(NAND_EXT_ID5_REG) & 0xff;
		if (id5)
			break;
	}

	for (i = 0; i < ARRAY_SIZE(ns_nand_ids); i++) {
		ASSERT((id == ns_nand_ids[i].nand_id && id5 == ns_nand_ids[i].id5),
				"Unsupport NAND flash\r\n");
	}

	/* Search the NAND-DB for an exact match */
	for (nand_db = __nanddb_start; nand_db < __nanddb_end; nand_db++) {
		if ((nand_db->id == id && nand_db->id5 == id5) ||
			(id == 0x01F1801D && nand_db->id == id)) {
			match = 1;
			break;
		}
	}

	ASSERT(!match, "Non-matched NAND: 0x%x, 0x%x\n", id, id5);

	fn->nandtiming0 = nand_db->timing0;
	fn->nandtiming1 = nand_db->timing1;
	fn->nandtiming2 = nand_db->timing2;
	fn->nandtiming3 = nand_db->timing3;
	fn->nandtiming4 = nand_db->timing4;
	fn->nandtiming5 = nand_db->timing5;
	fn->nandtiming6 = nand_db->timing6;

	/* Setup flash timing register */
	writel(NAND_TIMING0_REG, FLASH_TIMING_MIN(nand_db->timing0, 24)	|
			 FLASH_TIMING_MIN(nand_db->timing0, 16)	|
			 FLASH_TIMING_MIN(nand_db->timing0, 8)	|
			 FLASH_TIMING_MIN(nand_db->timing0, 0));

	writel(NAND_TIMING1_REG, FLASH_TIMING_MIN(nand_db->timing1, 24)	|
			 FLASH_TIMING_MIN(nand_db->timing1, 16)	|
			 FLASH_TIMING_MIN(nand_db->timing1, 8)	|
			 FLASH_TIMING_MIN(nand_db->timing1, 0));

	writel(NAND_TIMING2_REG, FLASH_TIMING_MIN(nand_db->timing2, 24)	|
			 FLASH_TIMING_MIN(nand_db->timing2, 16)	|
			 FLASH_TIMING_MAX(nand_db->timing2, 8)	|
			 FLASH_TIMING_MIN(nand_db->timing2, 0));

	writel(NAND_TIMING3_REG, FLASH_TIMING_MIN(nand_db->timing3, 24)	|
			 FLASH_TIMING_MIN(nand_db->timing3, 16)	|
			 FLASH_TIMING_MAX(nand_db->timing3, 8)	|
			 FLASH_TIMING_MAX(nand_db->timing3, 0));

	writel(NAND_TIMING4_REG, FLASH_TIMING_MIN(nand_db->timing4, 24)	|
			 FLASH_TIMING_MIN(nand_db->timing4, 16)	|
			 FLASH_TIMING_MIN(nand_db->timing4, 8)	|
			 FLASH_TIMING_MIN(nand_db->timing4, 0));

	writel(NAND_TIMING5_REG, FLASH_TIMING_MIN(nand_db->timing5, 16)	|
			 FLASH_TIMING_MAX(nand_db->timing5, 8)	|
			 FLASH_TIMING_MIN(nand_db->timing5, 0));

#if (NAND_CUSTOM_CMD_SUPPORT == 1)
	writel(NAND_TIMING6_REG, FLASH_TIMING_MIN(nand_db->timing6, 16)	|
			 FLASH_TIMING_MIN(nand_db->timing6, 8)	|
			 FLASH_TIMING_MAX(nand_db->timing6, 0));
#endif

	fn->main_size = nand_db->main_size;
	fn->spare_size = nand_db->spare_size;
	fn->blocks_per_bank = nand_db->blocks_per_bank;
	fn->pages_per_block = nand_db->pages_per_block;
	fn->block_size = nand_db->main_size * nand_db->pages_per_block;

	fn->control = NAND_CTR_IE;

	if (fn->blocks_per_bank * fn->pages_per_block > 65536)
		fn->control |= NAND_CTR_P3;

	if (fn->main_size == 2048)
		fn->control |= NAND_CTR_C2 | NAND_CTR_RC;

	switch (fn->block_size * fn->blocks_per_bank) {
	case 8 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_64M;
		break;
	case 16 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_128M;
		break;
	case 32 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_256M;
		break;
	case 64 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_512M;
		break;
	case 128 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_1G;
		break;
	case 256 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_2G;
		break;
	case 512 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_4G;
		break;
	case 1024 * 1024 * 1024:
		fn->control |= NAND_CTR_SZ_8G;
		break;
	default:
		ASSERT(1, "Unexpected NAND flash chipsize\r\n");
		break;
	}

	sys_config_val = rct_get_nand_poc();
	if ((sys_config_val & RCT_BOOT_NAND_ECC_BCH_EN) ==
		RCT_BOOT_NAND_ECC_BCH_EN) {
		if ((sys_config_val & RCT_BOOT_NAND_ECC_SPARE_2X) ==
			RCT_BOOT_NAND_ECC_SPARE_2X) {
			fn->ecc_bits = 8;
		} else {
			fn->ecc_bits = 6;
		}
	} else {
		fn->ecc_bits = 1;
	}

	ASSERT((fn->ecc_bits != 1) && (fn->main_size == 512),
		"Small page does not support multi-bits ECC!\r\n");

	ASSERT((fn->ecc_bits == 8) && (fn->spare_size != 128),
		"Spare size must be 2x for 8-bits ECC!\r\n");

	if (fn->ecc_bits > 1)
		nand_en_bch();

	get_part_size(part_size);

	sblk = nblk = 0;
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if ((get_part_dev(i) & PART_DEV_NAND) != PART_DEV_NAND) {
			continue;
		}

		sblk += nblk;
		nblk = part_size[i] / fn->block_size;
		if ((part_size[i] % fn->block_size) != 0x0)
			nblk++;
		fn->sblk[i] = (nblk == 0) ? 0 : sblk;
		fn->nblk[i] = nblk;
	}
	for (; i < PART_MAX; i++) {
		fn->sblk[i] = 0;
		fn->nblk[i] = 0;
	}

	nblk = (fn->blocks_per_bank > sblk) ? (fn->blocks_per_bank - sblk) : 0;
	//Raw part include BBT, take care!
	fn->sblk[PART_RAW] = (nblk == 0) ? 0 : sblk;
	fn->nblk[PART_RAW] = nblk;
	ASSERT(fn->sblk[PART_RAW] < 2, "No Space for BBT!\r\n");

	/* Reset FIO FIFO, and Exit random read mode */
	setbitsl(FIO_CTR_REG, FIO_CTR_RR);
	rct_timer2_dly_ms(1); /* delay is must have */
	clrbitsl(FIO_CTR_REG, FIO_CTR_RR);

	/* Clear the FIO DMA Status Register */
	writel(FIO_DMASTA_REG, 0x0);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Setup NAND Flash Control Register */
	writel(NAND_CTR_REG, flnand.control);
	writel(NAND_INT_REG, 0x0);

	return 0;
}

/**
 * Read multiple pages from NAND flash with ecc check.
 */
int nand_read_pages(u32 block, u32 page, u32 pages,
		u8 *main_buf, u8 *spare_buf, u32 enable_ecc)
{
	u32 status, addr, mlen, slen;
	u32 spare_buf_addr = 0, fio_ctr_reg = 0;
	u32 nand_ctr_reg = flnand.control, i;
	int ret = 0;

	/* check parameters */
	if ((page + pages) > flnand.pages_per_block || !main_buf) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	mutex_lock(&nand_read_mutex);

	mlen = pages * flnand.main_size;
	clean_flush_d_cache((void *)main_buf, mlen);

	/* Setup Flash Control Register */
#if (NAND_XD_SUPPORT_WAS == 0) && (NAND_SUPPORT_INTLVE == 0)
	nand_ctr_reg = flnand.control;
#elif (NAND_XD_SUPPORT_WAS >= 1) && (NAND_SUPPORT_INTLVE == 0)
	nand_ctr_reg = flnand.control | NAND_CTR_WAS;
#elif (NAND_XD_SUPPORT_WAS >= 1) && (NAND_SUPPORT_INTLVE == 1)
	nand_ctr_reg = flnand.control | NAND_CTR_WAS;

#if defined(NAND_K9K8_INTLVE)
	nand_ctr_reg = flnand.control | NAND_CTR_K9;
#endif
#endif

	spare_buf = spare_buf ? : dummy_buffer_bch;
	spare_buf_addr = (uintptr_t)spare_buf;

	/* Setup Flash IO Control Register */
	if (enable_ecc) {
		if (flnand.ecc_bits > 1) {
			slen = pages * flnand.spare_size;
			clean_flush_d_cache(spare_buf, slen);

			nand_en_bch();
			/* Setup Flash Control Register*/
			/* Don't set NAND_CTR_EC_MAIN, because we use BCH */
			writel(NAND_CTR_REG, nand_ctr_reg | NAND_CTR_SE);
			/* Clean Flash_IO_ecc_rpt_status Register */
			writel(FIO_ECC_RPT_STA_REG, 0x0);
		} else {
			slen = 0;
			/* Setup Flash IO Control Register */
			writel(FIO_CTR_REG, FIO_CTR_XD | FIO_CTR_RS | FIO_CTR_CO);

			/* Setup Flash Control Register*/
			writel(NAND_CTR_REG, nand_ctr_reg | NAND_CTR_SE | NAND_CTR_EC_MAIN);
		}
	} else {
		if (flnand.ecc_bits > 1) {
			slen = pages * flnand.spare_size;
			clean_flush_d_cache(spare_buf, slen);

			nand_dis_bch();
			fio_ctr_reg = FIO_CTR_XD | FIO_CTR_RS;
		} else {
			slen = 0;
			fio_ctr_reg = FIO_CTR_XD;
		}

		/* NO ECC */
		writel(FIO_CTR_REG, fio_ctr_reg);
		writel(NAND_CTR_REG, nand_ctr_reg);
	}

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG, 0x0);
	writel(FDMA_CHAN_SRC_REG, (uintptr_t)FIO_FIFO_BASE);
	writel(FDMA_CHAN_DST_REG, (uintptr_t)main_buf);

	if (flnand.ecc_bits > 1) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG, 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG, (uintptr_t)FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_DST_REG, spare_buf_addr);
		writel(FDMA_CHAN_SPR_CNT_REG, slen);
	}

	writel(FDMA_CHAN_CTR_REG,
	       FDMA_CHAN_CTR_EN		|
	       FDMA_CHAN_CTR_WM		|
	       FDMA_CHAN_CTR_NI		|
	       FDMA_NODC_MN_BURST_SIZE	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr = addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_FL		|
	       FIO_DMACTR_BLK_512B |
	       FIO_DMACTR_TS8B	|
	       (mlen + slen));

	/* Wait for interrupt for DMA done */
	nand_wait_dma_done();
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);	/* clear */
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);	/* clear */
	writel(FDMA_CHAN_STA_REG, 0);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE)) {
		ret = -1;
		goto nand_read_pages_err;
	}

	if (flnand.ecc_bits > 1 && enable_ecc) {
		status = readl(FIO_ECC_RPT_STA_REG);
		if (status & FIO_ECC_RPT_FAIL) {
			if (FIO_SUPPORT_SKIP_BLANK_ECC) {
				if ((readl(FIO_CTR_REG) & FIO_CTR_SKIP_BLANK)) {
					putstr("BCH real corrected failed (0x");
					puthex(status);
					putstr(")!\n\r");
				} else {
					putstr("Should not be here, error path");
				}
			} else {
				/* Workaround for page never used, BCH will failed */
				i = nand_bch_check_blank_pages(pages, (u8 *)main_buf,
						(u8 *)(uintptr_t)spare_buf_addr);

				if (i < 0) {
					putstr("BCH corrected failed (0x");
					puthex(status);
					putstr(")!\n\r");
				}
			}
		} else if (status & FIO_ECC_RPT_ERR) {
#if 0
			putstr("BCH code corrected (0x");
			puthex(status);
			putstr(")!\n\r");
#endif
			/* once bitflip and data corrected happened, BCH will keep
			 * on to report bitflip in next read operation, even though
			 * there is no bitflip happened really. So this is a workaround
			 * to get it back. */
			nand_corrected_recovery();
			ret = 1;
		}
	}
nand_read_pages_err:
	mutex_unlock(&nand_read_mutex);
	return ret;
}

static int nand_read(u32 block, u32 page, u32 pages, u8 *buf)
{
	int rval = 0;
	u32 first_blk_pages, blocks, last_blk_pages;
	u32 bad_blks = 0;

	first_blk_pages = flnand.pages_per_block - page;
	if (pages > first_blk_pages) {
		pages -= first_blk_pages;
		blocks = pages / flnand.pages_per_block;
		last_blk_pages = pages % flnand.pages_per_block;
	} else {
		first_blk_pages = pages;
		blocks = 0;
		last_blk_pages = 0;
	}

	if (first_blk_pages) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, page, first_blk_pages, buf, NULL, 1);
		if (rval < 0)
			return -1;
		block++;
		buf += first_blk_pages * flnand.main_size;
	}

	while (blocks > 0) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, 0, flnand.pages_per_block, buf, NULL, 1);
		if (rval < 0)
			return -1;
		block++;
		blocks--;
		buf += flnand.block_size;
	}

	if (last_blk_pages) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, 0, last_blk_pages, buf, NULL, 1);
		if (rval < 0)
			return -1;
	}

	return bad_blks;
}

static void nand_get_offset_adr(u32 *block, u32 *page, u32 pages, u32 bad_blks)
{
	u32 blocks;

	blocks = pages / flnand.pages_per_block;
	pages  = pages % flnand.pages_per_block;

	*block =  *block + blocks;
	*page += pages;

	if (*page >= flnand.pages_per_block) {
		*page -= flnand.pages_per_block;
		*block += 1;
	}

	*block += bad_blks;
}

/**
 * Read data from NAND flash to memory.
 * dst - address in dram.
 * src - address in nand device.
 * len - length to be read from nand.
 * return - length of read data.
 */
int nand_read_data(u8 *dst, u8 *src, int len)
{
	u32 block, page, pages, pos;
	u32 first_ppage_size, last_ppage_size;
	uintptr_t val, rval = -1;

	/* translate address to block, page, address */
	val = (uintptr_t) src;
	block = val / flnand.block_size;
	val  -= block * flnand.block_size;
	page  = val / flnand.main_size;
	pos   = val % flnand.main_size;
	pages = len / flnand.main_size;

	if (pos == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = flnand.main_size - pos;

	if (len >= first_ppage_size) {
		pages = (len - first_ppage_size) / flnand.main_size;

		last_ppage_size = (len - first_ppage_size) % flnand.main_size;
	} else {
		first_ppage_size = len;
		pages = 0;
		last_ppage_size = 0;
	}

	if (len !=
	    (first_ppage_size + pages * flnand.main_size + last_ppage_size)) {
		return -1;
	}

	len = 0;
	if (first_ppage_size) {
		rval = nand_read(block, page, 1, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) (buffer + pos), first_ppage_size);
		dst += first_ppage_size;
		len += first_ppage_size;
		nand_get_offset_adr(&block, &page, 1, rval);
	}

	if (pages > 0) {
		rval = nand_read(block, page, pages, dst);
		if (rval < 0)
			return len;

		dst += pages * flnand.main_size;
		len += pages * flnand.main_size;
		nand_get_offset_adr(&block, &page, pages, rval);
	}

	if (last_ppage_size > 0) {
		rval = nand_read(block, page, 1, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) buffer, last_ppage_size);
		len += last_ppage_size;
	}

	return len;
}

/**
 * Program a page to NAND flash.
 */
int nand_prog_pages(u32 block, u32 page, u32 pages, u8 *main_buf, u8 *spare_buf)
{
	int i;
	u32 status;
	u32 addr;
	u32 mlen = 0, slen = 0;
	u32 nand_ctr_reg = 0;
	u32 spare_buf_addr = 0;
	u32 dma_burst_ctrl, fio_burst_ctrl;

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (main_buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_pages()");
		return -1;
	}

	for (i = 0; i < pages; i++)
		mlen += flnand.main_size;

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	clean_d_cache((void *) main_buf, mlen);

	/* diable write protect */
	//gpio_set(FL_WP);

	/* Always enable ECC */
	if (flnand.ecc_bits > 1) {
		/* Don't set NAND_CTR_EC_MAIN, because we use BCH */
		nand_ctr_reg = flnand.control | NAND_CTR_SE;

		slen = pages * flnand.spare_size;

		if (spare_buf == NULL) {
			memset(dummy_buffer_bch, 0xff, slen);
			spare_buf_addr = (uintptr_t)dummy_buffer_bch;
			clean_d_cache((void *) dummy_buffer_bch, slen);
		} else {
			spare_buf_addr = (uintptr_t)spare_buf;
			clean_d_cache((void *) spare_buf, slen);
		}
		nand_en_bch();

		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, nand_ctr_reg);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);

		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE | NAND_CTR_EG_MAIN);
	}

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG, 0x0);
	writel(FDMA_CHAN_SRC_REG, (uintptr_t)main_buf);
	writel(FDMA_CHAN_DST_REG, (uintptr_t)FIO_FIFO_BASE);

	dma_burst_ctrl = FDMA_NODC_MN_BURST_SIZE;
	fio_burst_ctrl = FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B;

	if ((flnand.ecc_bits > 1)) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG, 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG, spare_buf_addr);
		writel(FDMA_CHAN_SPR_DST_REG, (uintptr_t)FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_CNT_REG, slen);

		dma_burst_ctrl = FDMA_CHAN_CTR_BLK_512B | FDMA_CHAN_CTR_TS_8B;
		fio_burst_ctrl = FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B;
	}

	writel(FDMA_CHAN_CTR_REG,
	       FDMA_CHAN_CTR_EN		|
	       FDMA_CHAN_CTR_RM		|
	       FDMA_CHAN_CTR_NI		|
	       dma_burst_ctrl	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl	|
			(mlen + slen));

	/* Wait for interrupt for NAND operation done and DMA done */
	nand_wait_dma_done();
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG, 0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	if (flnand.ecc_bits > 1) {
		status = readl(FIO_ECC_RPT_STA_REG);
		if (status & FIO_ECC_RPT_FAIL) {
				putstr("BCH corrected failed (0x");
				puthex(status);
				putstr(")!\n\r");
		} else if (status & FIO_ECC_RPT_ERR) {
			putstr("BCH code corrected (0x");
			puthex(status);
			putstr(")!\n\r");
		}
	}

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STA_REG);

	return (status & 0x1) ? -1 : 0;
}

int nand_prog_pages_noecc(u32 block, u32 page, u32 pages, u8 *buf)
{
	int i;
	u32 status;
	u32 addr;
	u32 mlen = 0, slen = 0;
	u32 spare_buf_addr = 0;
	u32 dma_burst_ctrl, fio_burst_ctrl;

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_pages()");
		return -1;
	}

	for (i = 0; i < pages; i++)
		mlen += flnand.main_size;

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	clean_d_cache((void *)buf, mlen);

	if (flnand.ecc_bits > 1) {
		slen = pages * flnand.spare_size;

		memset(dummy_buffer_bch, 0xff, slen);
		spare_buf_addr = (uintptr_t)dummy_buffer_bch;
		clean_d_cache((void *)dummy_buffer_bch, slen);

		nand_dis_bch();

		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		putstr("ERR: Not implemented for 1-bit ECC yet!\r\n");
		return -1;
	}

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
	/* Setup Flash Control Register */
	writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG, 0x0);
	writel(FDMA_CHAN_SRC_REG, (uintptr_t)buf);
	writel(FDMA_CHAN_DST_REG, (uintptr_t)FIO_FIFO_BASE);

	dma_burst_ctrl = FDMA_NODC_MN_BURST_SIZE;
	fio_burst_ctrl = FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B;

	if ((flnand.ecc_bits > 1)) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG, 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG, spare_buf_addr);
		writel(FDMA_CHAN_SPR_DST_REG, (uintptr_t)FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_CNT_REG, slen);

		dma_burst_ctrl = FDMA_CHAN_CTR_BLK_512B | FDMA_CHAN_CTR_TS_8B;
		fio_burst_ctrl = FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B;
	}

	writel(FDMA_CHAN_CTR_REG,
	       FDMA_CHAN_CTR_EN		|
	       FDMA_CHAN_CTR_RM		|
	       FDMA_CHAN_CTR_NI		|
	       dma_burst_ctrl	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl		|
	       (mlen + slen));

	/* Wait for interrupt for NAND operation done and DMA done */
	nand_wait_dma_done();
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG, 0);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STA_REG);

	return (status & 0x1) ? -1 : 0;
}

/**
 * Read spare area from NAND flash.
 * Always disable ECC.
 */
int nand_read_spare(u32 block, u32 page, u32 pages, u8 *buf)
{
	int i;
	u32 status;
	u32 addr, size = 0, mlen = 0;
	u32 desc_burst_ctrl = 0, fio_burst_ctrl;

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_read_spare()");
		return -1;
	}

	for (i = 0; i < pages; i++) {
		mlen += flnand.main_size;
		size += flnand.spare_size;
	}

	if (flnand.ecc_bits > 1) {
		if (mlen > MAX_SPARE_SIZE_BLK) {
			putstr("ERR: too many pages at one time\n");
		}
		/* Always disable ECC */
		/* Setup DMA main descriptor */
		G_fio_dmadesc.src_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dmadesc.dst_addr = (uintptr_t)dummy_buffer_bch;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (uintptr_t)&G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = mlen;

		/* Setup DMA spare descriptor */
		G_fio_dma_spr_desc.src_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dma_spr_desc.dst_addr = (uintptr_t)buf;
		G_fio_dma_spr_desc.next= 0x0;
		G_fio_dma_spr_desc.rpt_addr = (uintptr_t)&G_fio_dma_spr_desc.rpt;
		G_fio_dma_spr_desc.xfrcnt = size;
		G_fio_dma_spr_desc.rpt = 0x0;

		desc_burst_ctrl = FDMA_DESC_MN_BURST_SIZE;
		fio_burst_ctrl 	= FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B;
	} else {
		/* Setup DMA descriptor */
		G_fio_dmadesc.src_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dmadesc.dst_addr = (uintptr_t)buf;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (uintptr_t)&G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = size;

		desc_burst_ctrl = FDMA_DESC_SP_BURST_SIZE;
		fio_burst_ctrl 	= FIO_DMACTR_BLK_16B | FIO_DMACTR_TS8B;
		mlen = 0;
	}

	G_fio_dmadesc.ctrl =
		FDMA_DESC_WM |
		FDMA_DESC_EOC |
		FDMA_DESC_NI |
		FDMA_DESC_IE |
		FDMA_DESC_ST |
		desc_burst_ctrl;
	G_fio_dmadesc.rpt = 0x0;

	_clean_flush_d_cache();

	if (flnand.ecc_bits > 1) {
		nand_dis_bch();

		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE  | NAND_CTR_SA);
	}

	/* Setup external DMA engine transfer */
	writel(FDMA_CHAN_DA_REG, (uintptr_t)&G_fio_dmadesc);
	writel(FDMA_CHAN_STA_REG, 0x0);

	if (flnand.ecc_bits > 1) {
		/* Setup spare external DMA engine descriptor address */
		writel(FDMA_CHAN_SPR_DA_REG, (uintptr_t)&G_fio_dma_spr_desc);
		writel(FDMA_CHAN_SPR_STA_REG, 0);
	}

	writel(FDMA_CHAN_CTR_REG,   FDMA_CHAN_CTR_D |   FDMA_CHAN_CTR_EN);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr = addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl		|
	       (mlen + size));

	/* Wait for interrupt for NAND operation done and DMA done */
	nand_wait_desc_dma_done();
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG, 0x0);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STA_REG);

	return (status & 0x1) ? -1 : 0;
}

/**
 * Program spare area to NAND flash.
 * Only for mark bad block, disable ECC.
 */
int nand_prog_spare(u32 block, u32 page, u32 pages, u8 *buf)
{
	int i;
	u32 status;
	u32 addr, size = 0, mlen = 0;

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_spare()");
		return -1;
	}

	for (i = 0; i < pages; i++) {
		mlen += flnand.main_size;
		size += flnand.spare_size;
	}

	if (flnand.ecc_bits > 1) {
		if (mlen > MAX_SPARE_SIZE_BLK) {
			putstr("ERR: too many pages at one time\n");
		} else {
			memset(dummy_buffer_bch, 0xff, mlen);
		}

		/* Always disable ECC */
		/* Setup DMA main descriptor */
		G_fio_dmadesc.src_addr = (uintptr_t)dummy_buffer_bch;
		G_fio_dmadesc.dst_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (uintptr_t)&G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = mlen;

		/* Setup DMA spare descriptor */
		G_fio_dma_spr_desc.src_addr = (uintptr_t)buf;
		G_fio_dma_spr_desc.dst_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dma_spr_desc.next= 0x0;
		G_fio_dma_spr_desc.rpt_addr = (uintptr_t)&G_fio_dma_spr_desc.rpt;
		G_fio_dma_spr_desc.xfrcnt = size;
		G_fio_dma_spr_desc.rpt = 0x0;
	} else {
		/* Setup DMA descriptor */
		G_fio_dmadesc.src_addr = (uintptr_t)buf;
		G_fio_dmadesc.dst_addr = (uintptr_t)FIO_FIFO_BASE;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (uintptr_t)&G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = size;
		mlen = 0;
	}

	G_fio_dmadesc.ctrl =
		FDMA_DESC_RM	|
		FDMA_DESC_EOC	|
		FDMA_DESC_NI	|
		FDMA_DESC_IE	|
		FDMA_DESC_ST	|
		FDMA_DESC_SP_BURST_SIZE;
	G_fio_dmadesc.rpt = 0x0;

	_clean_d_cache();

	/* Diable write protect */
	//gpio_set(FL_WP);

	if (flnand.ecc_bits > 1) {
		nand_dis_bch();

		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS);
		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS);
		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE  | NAND_CTR_SA);
	}

	/* Setup main external DMA engine descriptor address */
	writel(FDMA_CHAN_DA_REG, (uintptr_t)&G_fio_dmadesc);
	writel(FDMA_CHAN_STA_REG, 0);

	if (flnand.ecc_bits > 1) {
		writel(FDMA_CHAN_SPR_STA_REG, 0);
		/* Setup spare external DMA engine descriptor address */
		writel(FDMA_CHAN_SPR_DA_REG, (uintptr_t)&G_fio_dma_spr_desc);
	}
	writel(FDMA_CHAN_CTR_REG,   FDMA_CHAN_CTR_D |   FDMA_CHAN_CTR_EN);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       FIO_DMACTR_BLK_16B |
	       FIO_DMACTR_TS8B	|
	       (mlen + size));

	/* Wait for interrupt for NAND operation done and DMA done */
	nand_wait_desc_dma_done();
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG, 0x0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STA_REG);

	return (status & 0x1) ? -1 : 0;
}

/**
 * Erase a NAND flash block.
 */
int nand_erase_block(u32 block)
{
	u32 status;
	u32 addr;

#if defined(DEBUG)
	putstr("nand_erase_block( ");
	putdec(block);
	putstr(" )\r\n");
#endif

	/* Disable write protect */
	//gpio_set(FL_WP);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NAND_CTR_REG, flnand.control);

	/* Erase block */
	addr = addr_from_block_page(block, 0);

	/* Workround for DSM bug!*/
	writel(FIO_DMAADR_REG, addr);

	writel(NAND_INT_REG, 0x0);
	nand_wait_cmd_done(NAND_CMD_ERASE | addr);

	status = readl(FIO_DMASTA_REG);
	writel(FIO_DMASTA_REG, 0x0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STA_REG);

	if ((status & 0x1)) {
		/* Reset chip */
		nand_wait_cmd_done(NAND_CMD_RESET);
		return -1;
	} else {
#if defined(CONFIG_NAND_USE_FLASH_BBT)
		/* If erase success and block is marked as bad in BBT */
		/* then update BBT. */
		if (nand_is_bad_block(block))
			nand_update_bbt(0, block);
#endif
		return 0;
	}
}

