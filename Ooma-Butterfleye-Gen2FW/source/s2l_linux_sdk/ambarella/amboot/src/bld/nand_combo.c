/**
 * bld/nand.c
 *
 * Flash controller functions with NAND chips.
 *
 * History:
 *    2017/05/12 - [Cao Rongrong] created file
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
#include <ambhw/cache.h>
#include <fio/ftl_const.h>
#include <flash/nanddb.h>

#define NAND_SUPPORT_WP			1

/* ==========================================================================*/
#define NAND_CMD_MASK			0xf
#define NAND_CMD_NOP			0x0
#define NAND_CMD_RESET			0x2
#define NAND_CMD_READID			0xa
#define NAND_CMD_READSTATUS		0xc
#define NAND_CMD_READ			0xe
#define NAND_CMD_PROGRAM		0xf

#define NAND_CMD_TIMEOUT		1000
/* ==========================================================================*/
/* For NO-ECC USAGE */
#define NAND_DSM_JP_DEFAULT		0xB6
/* ==========================================================================*/
extern flnand_t flnand;

#define MAX_SCRATCH_BUFFER_SIZE		8192
static u8 scratch_buffer[MAX_SCRATCH_BUFFER_SIZE] __attribute__ ((aligned(32)));
static u8 page_buffer[4096] __attribute__ ((aligned(32)));

static u32 fdma_dsm_ctrl;
static u32 fdma_dsm_no_ecc_ctrl = 0;
/* ==========================================================================*/

static void nand_wait_cmd_done(u32 cmd)
{
	u32 rval;

	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	if ((cmd & NAND_CMD_MASK) != NAND_CMD_NOP)
		writel(NAND_CMD_REG, cmd);

	rct_timer2_reset_count();

	while(1) {
		rval = readl(FIO_RAW_INT_STATUS_REG);

		if (rval & (FIO_INT_ECC_RPT_UNCORR |
				FIO_INT_ECC_RPT_THRESH |
				FIO_INT_OPERATION_DONE))
			break;

		if (rct_timer2_get_count() >= NAND_CMD_TIMEOUT) {
			printf("nand cmd timeout: %d\n", cmd);
			while(1);
		}
	}

	writel(FIO_RAW_INT_STATUS_REG, rval);
}

static void nand_enable_wp()
{
#if (NAND_SUPPORT_WP == 1)
	setbitsl(NAND_CTRL_REG, NAND_CTRL_WP);
#endif
}

static void nand_disable_wp()
{
#if (NAND_SUPPORT_WP == 1)
	clrbitsl(NAND_CTRL_REG, NAND_CTRL_WP);
#endif
}

static void nand_init_hw(void)
{
	u32 poc = rct_get_nand_poc();

	ASSERT(!(poc & RCT_BOOT_NAND_ECC_BCH_EN), "Please enable BCH in POC!\n\r");

	/* Reset FIO FIFO, and Exit random read mode */
	setbitsl(FIO_CTRL_REG, FIO_CTRL_RANDOM_READ);
	rct_timer2_dly_ms(1); /* delay is must have */
	clrbitsl(FIO_CTRL_REG, FIO_CTRL_RANDOM_READ);

	if (poc & RCT_BOOT_NAND_ECC_SPARE_2X) {
		fdma_dsm_ctrl = FDMA_DSM_MAIN_JP_SIZE_512B | FDMA_DSM_SPARE_JP_SIZE_32B;
		setbitsl(NAND_EXT_CTRL_REG, NAND_EXT_CTRL_SPARE_2X);
	}
	else {
		fdma_dsm_ctrl = FDMA_DSM_MAIN_JP_SIZE_512B | FDMA_DSM_SPARE_JP_SIZE_16B;
		clrbitsl(NAND_EXT_CTRL_REG, NAND_EXT_CTRL_SPARE_2X);
	}

	writel(FDMA_DSM_CTRL_REG, fdma_dsm_ctrl);
	setbitsl(FIO_CTRL_REG, FIO_CTRL_RDERR_STOP | FIO_CTRL_SKIP_BLANK_ECC);

	if (poc & RCT_BOOT_NAND_PAGE_SIZE)
		clrbitsl(NAND_EXT_CTRL_REG, NAND_EXT_CTRL_4K_PAGE);
	else
		setbitsl(NAND_EXT_CTRL_REG, NAND_EXT_CTRL_4K_PAGE);

	/* disable and clear all of NAND Interrupt status */
	writel(FIO_INT_ENABLE_REG, 0x00);
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	flnand.ecc_bits = (poc & RCT_BOOT_NAND_ECC_SPARE_2X) ? 8 : 6;
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
		bi = *(sbuf_ptr + i * flnand.spare_size);
		if (bi != 0xff)
			break;
	}


	/* Good block */
	if (i == INIT_BAD_BLOCK_PAGES)
		return NAND_GOOD_BLOCK;

	for (i = INIT_BAD_BLOCK_PAGES; i < BAD_BLOCK_PAGES; i++) {
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
	u8 sbuf[1024], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((uintptr_t)sbuf + 31) & (~31));

#if defined(CONFIG_NAND_USE_FLASH_BBT)
	nand_update_bbt(block, 0);
#endif

	for (i = AMB_BB_START_PAGE; i < BAD_BLOCK_PAGES; i++) {
		memset(sbuf_ptr, 0xff, flnand.spare_size);
		*sbuf_ptr = AMB_BAD_BLOCK_MARKER;

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
	flnand_t *fn = &flnand;
	nand_db_t *nand_db;
	int sblk, nblk, part_size[HAS_IMG_PARTS];
	u32 i, id, id5, match = 0;

	nand_init_hw();

	/* Force ReadID with 4-cycles */
	setbitsl(NAND_CTRL_REG, NAND_CTRL_I4);

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
	clrbitsl(NAND_CTRL_REG, NAND_CTRL_I4);	/* Disable NAND_CTR_I4 */
	setbitsl(NAND_EXT_CTRL_REG, NAND_EXT_CTRL_I5);
	for (i = 0; i < 5; i++) {
		nand_wait_cmd_done(NAND_CMD_READID);
		id5 = readl(NAND_EXT_ID_REG) & 0xff;
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

	writel(NAND_TIMING6_REG, FLASH_TIMING_MIN(nand_db->timing6, 16)	|
			 FLASH_TIMING_MIN(nand_db->timing6, 8)	|
			 FLASH_TIMING_MAX(nand_db->timing6, 0));

	fn->main_size = nand_db->main_size;
	fn->spare_size = nand_db->spare_size;
	fn->blocks_per_bank = nand_db->blocks_per_bank;
	fn->pages_per_block = nand_db->pages_per_block;
	fn->block_size = nand_db->main_size * nand_db->pages_per_block;

	switch (fn->block_size * fn->blocks_per_bank) {
	case 8 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_64M;
		break;
	case 16 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_128M;
		break;
	case 32 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_256M;
		break;
	case 64 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_512M;
		break;
	case 128 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_1G;
		break;
	case 256 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_2G;
		break;
	case 512 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_4G;
		break;
	case 1024 * 1024 * 1024:
		fn->control = NAND_CTRL_SIZE_8G;
		break;
	default:
		ASSERT(1, "Unexpected NAND flash chipsize\r\n");
		break;
	}

	if (fn->blocks_per_bank * fn->pages_per_block > 65536)
		fn->control |= NAND_CTRL_P3;

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
	fn->sblk[PART_RAW] = (nblk == 0) ? 0 : sblk; /* Raw part has BBT, take care!*/
	fn->nblk[PART_RAW] = nblk;
	ASSERT(fn->sblk[PART_RAW] < 2, "No Space for BBT!\r\n");

	fdma_dsm_no_ecc_ctrl = NAND_DSM_JP_DEFAULT;
	fdma_dsm_no_ecc_ctrl += (((fn->main_size >> 12 ) << 4) | (fn->spare_size >> 7));
	return 0;
}

/**
 * Read multiple pages from NAND flash with ecc check.
 */
int nand_read_pages(u32 block, u32 page, u32 pages,
		u8 *main_buf, u8 *spare_buf, u32 enable_ecc)
{
	u32 mlen, slen, status, addr;
	u64 addr64;

	/* check parameters */
	if ((page + pages) > flnand.pages_per_block || (!main_buf && !spare_buf)) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	mlen = pages * flnand.main_size;
	BUG_ON(!main_buf && mlen > MAX_SCRATCH_BUFFER_SIZE);
	main_buf = main_buf ? : scratch_buffer;
	clean_flush_d_cache((void *)main_buf, mlen);

	slen = pages * flnand.spare_size;
	BUG_ON(!spare_buf && slen > MAX_SCRATCH_BUFFER_SIZE);
	spare_buf = spare_buf ? : scratch_buffer;
	clean_flush_d_cache(spare_buf, slen);

	/* clear all of NAND Interrupt status */
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	/* Setup Flash IO Control Register */
	if (enable_ecc && main_buf) {
		setbitsl(FIO_CTRL_REG, FIO_CTRL_ECC_BCH_ENABLE);
		writel(FDMA_DSM_CTRL_REG, fdma_dsm_ctrl);
	} else {
		clrbitsl(FIO_CTRL_REG, FIO_CTRL_ECC_BCH_ENABLE);
		writel(FDMA_DSM_CTRL_REG, fdma_dsm_no_ecc_ctrl);
	}

	/* Setup FDMA engine transfer */
	writel(FDMA_MN_MEM_ADDR_REG, (uintptr_t)main_buf);
	writel(FDMA_SP_MEM_ADDR_REG, (uintptr_t)spare_buf);
	writel(FDMA_MN_CTRL_REG, FDMA_CTRL_ENABLE | FDMA_CTRL_WRITE_MEM |
				 FDMA_CTRL_BLK_SIZE_512B | (mlen + slen));

	/* Write start address for memory target to */
	addr64 = (u64)(block * flnand.pages_per_block + page) * flnand.main_size;

	addr = (addr64 >> 32) << 28;
	writel(NAND_CTRL_REG, addr | flnand.control);

	addr = addr64 & 0xffffffff;
	nand_wait_cmd_done(addr | NAND_CMD_READ);

	status = readl(FDMA_MN_STATUS_REG);
	if (status & (FDMA_STATUS_DMA_BUS_ERR | FDMA_STATUS_DMA_ADDR_ERR))
		return -1;

	if (enable_ecc && main_buf) {
		status = readl(FIO_ECC_RPT_STATUS_REG);
		if (status & FIO_ECC_RPT_CORR_FAIL) {
			printf("BCH real corrected failed (0x%x)!\n", status);
			return -1;
		}
	}

	_clean_flush_d_cache();

	return 0;
}

static int nand_read(u32 block, u32 page, u32 pages, u8 *buf)
{
	u32 first_blk_pages, blocks, last_blk_pages, bad_blks = 0;
	int rval = 0;

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

static void nand_get_offset_addr(u32 *block, u32 *page, u32 pages, u32 bad_blks)
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
	val = (uintptr_t)src;
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

	if (len != (first_ppage_size + pages * flnand.main_size + last_ppage_size))
		return -1;

	len = 0;

	if (first_ppage_size) {
		rval = nand_read(block, page, 1, page_buffer);
		if (rval < 0)
			return len;

		memcpy(dst, page_buffer + pos, first_ppage_size);
		dst += first_ppage_size;
		len += first_ppage_size;
		nand_get_offset_addr(&block, &page, 1, rval);
	}

	if (pages > 0) {
		rval = nand_read(block, page, pages, dst);
		if (rval < 0)
			return len;

		dst += pages * flnand.main_size;
		len += pages * flnand.main_size;
		nand_get_offset_addr(&block, &page, pages, rval);
	}

	if (last_ppage_size > 0) {
		rval = nand_read(block, page, 1, page_buffer);
		if (rval < 0)
			return len;

		memcpy(dst, page_buffer, last_ppage_size);
		len += last_ppage_size;
	}

	return len;
}

/**
 * Program a page to NAND flash.
 */
int nand_prog_pages(u32 block, u32 page, u32 pages, u8 *main_buf, u8 *spare_buf)
{
	u32 mlen, slen, status, addr;
	u64 addr64;

	/* check parameters */
	if ((page + pages) > flnand.pages_per_block || !main_buf) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	mlen = pages * flnand.main_size;
	clean_d_cache((void *)main_buf, mlen);

	slen = pages * flnand.spare_size;
	if (spare_buf == NULL) {
		spare_buf = scratch_buffer;
		memset(spare_buf, 0xff, slen);
	}
	clean_d_cache(spare_buf, slen);

	nand_disable_wp();

	/* enable BCH */
	writel(FDMA_DSM_CTRL_REG, fdma_dsm_ctrl);
	setbitsl(FIO_CTRL_REG, FIO_CTRL_ECC_BCH_ENABLE);

	/* clear all of NAND Interrupt status */
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	/* Setup FDMA engine transfer */
	writel(FDMA_MN_MEM_ADDR_REG, (uintptr_t)main_buf);
	writel(FDMA_SP_MEM_ADDR_REG, (uintptr_t)spare_buf);
	writel(FDMA_MN_CTRL_REG, FDMA_CTRL_ENABLE | FDMA_CTRL_READ_MEM |
				 FDMA_CTRL_BLK_SIZE_512B | (mlen + slen));

	/* Write start address for memory target to */
	addr64 = (u64)(block * flnand.pages_per_block + page) * flnand.main_size;

	addr = (addr64 >> 32) << 28;
	writel(NAND_CTRL_REG, addr | flnand.control);

	addr = addr64 & 0xffffffff;
	nand_wait_cmd_done(addr | NAND_CMD_PROGRAM);

	nand_enable_wp();

	status = readl(FDMA_MN_STATUS_REG);
	if (status & (FDMA_STATUS_DMA_BUS_ERR | FDMA_STATUS_DMA_ADDR_ERR))
		return -1;

	status = readl(FIO_ECC_RPT_STATUS_REG);
	if (status & FIO_ECC_RPT_CORR_FAIL) {
		printf("BCH real corrected failed (0x%x)!\n", status);
		return -1;
	}

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STATUS_REG);

	return (status & 0x1) ? -1 : 0;
}

int nand_prog_pages_noecc(u32 block, u32 page, u32 pages, u8 *buf)
{
	u32 mlen, slen, status, addr;
	u64 addr64;

	/* check parameters */
	if ((page + pages) > flnand.pages_per_block || !buf) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	mlen = pages * flnand.main_size;
	clean_d_cache((void *)buf, mlen);

	slen = pages * flnand.spare_size;
	memset(scratch_buffer, 0xff, slen);
	clean_d_cache(scratch_buffer, slen);

	nand_disable_wp();

	/* disable BCH */
	writel(FDMA_DSM_CTRL_REG, fdma_dsm_no_ecc_ctrl);
	clrbitsl(FIO_CTRL_REG, FIO_CTRL_ECC_BCH_ENABLE);

	/* clear all of NAND Interrupt status */
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	/* Setup FDMA engine transfer */
	writel(FDMA_MN_MEM_ADDR_REG, (uintptr_t)buf);
	writel(FDMA_SP_MEM_ADDR_REG, (uintptr_t)scratch_buffer);
	writel(FDMA_MN_CTRL_REG, FDMA_CTRL_ENABLE | FDMA_CTRL_READ_MEM |
				 FDMA_CTRL_BLK_SIZE_512B | (mlen + slen));

	/* Write start address for memory target to */
	addr64 = (u64)(block * flnand.pages_per_block + page) * flnand.main_size;

	addr = (addr64 >> 32) << 28;
	writel(NAND_CTRL_REG, addr | flnand.control);

	addr = addr64 & 0xffffffff;
	nand_wait_cmd_done(addr | NAND_CMD_PROGRAM);

	nand_enable_wp();

	status = readl(FDMA_MN_STATUS_REG);
	if (status & (FDMA_STATUS_DMA_BUS_ERR | FDMA_STATUS_DMA_ADDR_ERR))
		return -1;

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STATUS_REG);

	return (status & 0x1) ? -1 : 0;
}

/**
 * Read spare area from NAND flash. PS: always disable ECC.
 */
int nand_read_spare(u32 block, u32 page, u32 pages, u8 *spare_buf)
{
	return nand_read_pages(block, page, pages, NULL, spare_buf, 0);
}

/**
 * Program spare area to NAND flash.
 * Only for mark bad block, disable ECC.
 */
int nand_prog_spare(u32 block, u32 page, u32 pages, u8 *spare_buf)
{
	u32 mlen, slen, status, addr;
	u64 addr64;

	/* check parameters */
	if ((page + pages) > flnand.pages_per_block || !spare_buf) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	mlen = pages * flnand.main_size;
	memset(scratch_buffer, 0xff, mlen);
	clean_d_cache((void *)scratch_buffer, mlen);

	slen = pages * flnand.spare_size;
	clean_d_cache(spare_buf, slen);

	nand_disable_wp();

	/* disable BCH */
	writel(FDMA_DSM_CTRL_REG, fdma_dsm_no_ecc_ctrl);
	clrbitsl(FIO_CTRL_REG, FIO_CTRL_ECC_BCH_ENABLE);

	/* clear all of NAND Interrupt status */
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	/* Setup FDMA engine transfer */
	writel(FDMA_MN_MEM_ADDR_REG, (uintptr_t)scratch_buffer);
	writel(FDMA_SP_MEM_ADDR_REG, (uintptr_t)spare_buf);
	writel(FDMA_MN_CTRL_REG, FDMA_CTRL_ENABLE | FDMA_CTRL_READ_MEM |
				 FDMA_CTRL_BLK_SIZE_512B | (mlen + slen));

	/* Write start address for memory target to */
	addr64 = (u64)(block * flnand.pages_per_block + page) * flnand.main_size;

	addr = (addr64 >> 32) << 28;
	writel(NAND_CTRL_REG, addr | flnand.control);

	addr = addr64 & 0xffffffff;
	nand_wait_cmd_done(addr | NAND_CMD_PROGRAM);

	nand_enable_wp();

	status = readl(FDMA_MN_STATUS_REG);
	if (status & (FDMA_STATUS_DMA_BUS_ERR | FDMA_STATUS_DMA_ADDR_ERR))
		return -1;

	status = readl(FIO_ECC_RPT_STATUS_REG);
	if (status & FIO_ECC_RPT_CORR_FAIL) {
		printf("BCH real corrected failed (0x%x)!\n", status);
		return -1;
	}

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STATUS_REG);

	return (status & 0x1) ? -1 : 0;
}

/**
 * Erase a NAND flash block.
 */
int nand_erase_block(u32 block)
{
	u64 addr64 = (u64)block * flnand.block_size;
	u32 status, addr, val;

	nand_disable_wp();

	/* clear all of NAND Interrupt status */
	writel(FIO_RAW_INT_STATUS_REG, 0xff);

	/* Setup Flash Control Register */
	addr = (addr64 >> 32) << 28;
	writel(NAND_CTRL_REG, addr | flnand.control);

	addr = addr64 & 0xffffffff;
	writel(NAND_CMD_REG, addr);

	val = NAND_CC_WORD_CMD1VAL0(0x60) | NAND_CC_WORD_CMD2VAL0(0xD0);
	writel(NAND_CC_WORD_REG, val);

	val = NAND_CC_DATA_CYCLE(5) | NAND_CC_WAIT_RB | NAND_CC_RW_NODATA |
		NAND_CC_CMD2(1) | NAND_CC_ADDR_CYCLE(3) | NAND_CC_CMD1(1) |
		NAND_CC_ADDR_SRC(1) | NAND_CC_DATA_SRC_REGISTER | NAND_CC_TERMINATE_CE;
	writel(NAND_CC_REG, val);

	nand_wait_cmd_done(NAND_CMD_NOP);

	nand_enable_wp();

	/* Read Status */
	nand_wait_cmd_done(NAND_CMD_READSTATUS);
	status = readl(NAND_STATUS_REG);

	if ((status & 0x1)) {
		/* Reset chip */
		nand_wait_cmd_done(NAND_CMD_RESET);
		return -1;
	} else {
#if defined(CONFIG_NAND_USE_FLASH_BBT)
		/*
		 * If erase successfully and block is marked as bad in BBT,
		 * then update BBT.
		 */
		if (nand_is_bad_block(block))
			nand_update_bbt(0, block);
#endif
		return 0;
	}
}

