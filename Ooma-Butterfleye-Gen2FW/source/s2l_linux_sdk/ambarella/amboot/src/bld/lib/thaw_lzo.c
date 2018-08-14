/**
 * lib/thaw_lzo.c
 *
 *
 * Copyright (c) 2020 Ambarella, Inc.
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
#include <fio/ftl_const.h>
#include <irq.h>
#include <ambhw/cortex.h>
#include <ambhw/cache.h>

#define __NEW_UTS_LEN		64
#define PAGE_SIZE		0x1000
#define CPU_RESUME_MAIC		0x0badbeef
#define SWAP_RESERVE_PAGE	1
#define THAW_MTD_NAME		"swp"

#define LZO_HEADER		sizeof(size_t)
#define LZO_UNC_PAGES		32
#define LZO_CMP_PAGES		35
#define LZO_UNC_SIZE		(LZO_UNC_PAGES * PAGE_SIZE)
#define LZO_CMP_SIZE		(LZO_CMP_PAGES * PAGE_SIZE)

static int swap_start_blk = 0;
static int swap_total_blk = 0;
static int pages_per_4k = 0;
static int lzo_image_page_offset = 0;
static int original_image_page_offset = 0;
static unsigned int cmp_crc = 0, unc_crc = 0;
static unsigned int nr_cmp_pages = 0, nr_unc_pages = 0;

static void *__lzo_unc_area ;
static void *__lzo_work_area ;
static void *__lzo_cmp_area ;

static unsigned long __lzo_reserve_addr;

struct new_utsname {
	char sysname[__NEW_UTS_LEN + 1];
	char nodename[__NEW_UTS_LEN + 1];
	char release[__NEW_UTS_LEN + 1];
	char version[__NEW_UTS_LEN + 1];
	char machine[__NEW_UTS_LEN + 1];
	char domainname[__NEW_UTS_LEN + 1];
};

struct arch_hibernate_hdr_invariants {
	char	uts_version[__NEW_UTS_LEN + 1];
};

static struct arch_hibernate_hdr {
	struct arch_hibernate_hdr_invariants invariants;

	size_t		ttbr1_el1;
	void		(*reenter_kernel)(void);
	size_t		__hyp_stub_vectors;
	size_t		sleep_cpu_mpidr;
} *resume_hdr;

struct swsusp_info {
	struct new_utsname	uts;
	u32		version_code;
	size_t		num_physpages;
	int		cpus;
	size_t		image_pages;
	size_t		pages;
	size_t		size;
	size_t		magic;
	size_t		addr;
	size_t		lzo_enable;
	size_t		crc32;
} __attribute__((aligned(PAGE_SIZE)));


#if defined(__aarch64__)
int lzo_compress(const unsigned char *, int, unsigned char *, int *, void *);
int lzo_uncompress(const unsigned char *, size_t , unsigned char *, size_t *);

#else
int lzo_compress(const unsigned char *in, int in_len, unsigned char *out,
		int *out_len, void *work_area)
{
	printf("LZO not supported!\n");

	return 0;
}

int lzo_uncompress(const unsigned char *in, size_t in_len, unsigned char *out,
		size_t *out_len)
{
	printf("LZO not supported!\n");

	return 0;
}
#endif

static int block_check_by_page(unsigned int page)
{
	unsigned int block = swap_start_blk + page / flnand.pages_per_block;

	while (nand_is_bad_block(block)){
		printf("page %d or block %d [%d]is a bad block ...\n",
				page, block, block - swap_start_blk);
		block++;
		page += flnand.pages_per_block;
	}

	return page;
}

static int mtd_read_linux_page(unsigned int page_offset, void *buf)
{
	int ofs, block, ret;

	block = page_offset / flnand.pages_per_block;
	ofs   = page_offset % flnand.pages_per_block;

	ret = nand_read_pages(swap_start_blk + block, ofs, pages_per_4k,
			buf, NULL, 1);
	if (ret < 0) {
		printf("THAW: nand read %d page failed\n", page_offset);
		return -1;
	}
	return 0;
}

static int get_swap_info(void)
{
	int i;
	int ret = -1;
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if(strcmp(get_part_str(i), THAW_MTD_NAME))
			continue;

		swap_start_blk = flnand.sblk[i];
		swap_total_blk = flnand.nblk[i];

		ret = 0;
		break;
	}

	pages_per_4k = PAGE_SIZE / flnand.main_size ;

	BUG_ON(!pages_per_4k);

	return ret;
}

static int load_snapshot_header(struct swsusp_info *info, int flag)
{
	int page_offset;
	void *buf;

	/* DATA store in NAND is PAGE_SIZE */
	buf = malloc(PAGE_SIZE);
	if(buf == NULL){
		printf("THAW: failed to malloc for swsusp_info!\n");
		return -1;
	}

	page_offset = SWAP_RESERVE_PAGE * pages_per_4k;

	if(page_offset != block_check_by_page(page_offset))
		return -1;

	if(mtd_read_linux_page(page_offset, buf))
		return -1;

	memcpy(info, buf, sizeof(struct swsusp_info));

	if(info->magic != CPU_RESUME_MAIC) {
		return -1;
	}

	resume_hdr = (struct arch_hibernate_hdr *)&info->uts;

	if (flag) {

		printf("sysname :	%s\n", info->uts.sysname);
		printf("release :	%s\n", info->uts.release);
		printf("version :	%s\n", info->uts.version);
		printf("machine :	%s\n", info->uts.machine);
		printf("image_pages:	%d\n", info->image_pages);
		printf("nr_meta_pages:	%d\n", info->pages - info->image_pages - 1);
		printf("info_magic :	%08x\n", info->magic);
		printf("jump addr :	%08x\n", info->addr);
		printf("lzo:		%s\n", info->lzo_enable ? "enable" : "disable");
		printf("crc:		%08x\n", info->crc32);

	} else {
		/* FIXME: Not checking the CRC when loading snapshot automatically. */
		info->crc32 = 0;
	}

	free(buf);

	return 0;
}

static void jump_to_resume(struct swsusp_info *info)
{
	if(info->magic !=  CPU_RESUME_MAIC){
		printf("Hibernate: Boot Magic error ...\n");
		return ;
	}

#if defined(__aarch64__)
	pr_color(RED, "ARM64: Start to run ... %x\n", info->addr);
#else
	pr_color(RED, "ARM: Start to run... %x\n", info->addr);
#endif
	jump_to_kernel((void *)(unsigned long)info->addr, 0);
	__asm__ __volatile__ ("b .");
}

static int load_image_original_page(size_t addr)
{

	original_image_page_offset = block_check_by_page(original_image_page_offset);

	BUG_ON(!addr);

	if (mtd_read_linux_page(original_image_page_offset, (unsigned char *)(unsigned long)addr))
		return -1;

	original_image_page_offset += pages_per_4k;

	return 0;
}

static void load_image_original(struct swsusp_info *info)
{
	int i, j, nr_pages = 0;
	unsigned int nr_meta_pages, meta_page_offset, crc = 0;
	void *buf;
	size_t *phys_addr;

	buf = malloc(PAGE_SIZE);
	if(buf == NULL){
		printf("THAW: failed malloc for load image lzo\n");
		return ;
	}

	phys_addr = (size_t *)buf;

	nr_meta_pages = info->pages - info->image_pages - 1;

	/* swap partition:
	 *
	 * PAGE-0: reserve
	 * PAGE-1: header
	 * PAGE-2: meta-0
	 * PAGE-3: meta-1
	 *  ...
	 * PAGE-x: lzo compress image
	 *
	 */

	meta_page_offset = (1 + 1) * pages_per_4k;
	original_image_page_offset = nr_meta_pages * pages_per_4k + meta_page_offset;

	for (i = 0; i < nr_meta_pages; i++){
		if (mtd_read_linux_page(meta_page_offset, buf)) {
			free(buf);
			return ;
		}

		for (j = 0; j < PAGE_SIZE / sizeof(size_t); j ++) {
			if ((*(phys_addr + j) & 0xffffffff) == 0xffffffff)
				break;

			if (load_image_original_page(*(phys_addr + j))) {
				break;
			}
			if (info->crc32)
				crc = __crc32(crc, (void *)(unsigned long)(*(phys_addr + j)), PAGE_SIZE);
			nr_pages++;
		}
		meta_page_offset += pages_per_4k;
	}

	printf("LINUX:	%d pages, crc = %08x\n", nr_pages, crc);
	free(buf);

	if (info->crc32)
		BUG_ON(crc != info->crc32);

#if defined(CONFIG_PANDORA_RTOS) && defined(AMBOOT_BOOT_DSP)
	wait_dsp_done();
#endif
	jump_to_resume(info);

}
static int load_image_lzo1x(size_t crc_enable, size_t *target)
{
	size_t lzo_size = 0, offset = 0, unc_size = LZO_UNC_SIZE;
	unsigned char *lzo_cmp_addr = (unsigned char *)__lzo_cmp_area;
	unsigned char *lzo_target_addr = (unsigned char *)__lzo_unc_area;
	int i;
#ifdef LZO_DEBUG
	static int uncompress_loop = 0;
#endif

	lzo_image_page_offset = block_check_by_page(lzo_image_page_offset);

#ifdef LZO_DEBUG
	printf("%d:", lzo_image_page_offset / pages_per_4k);
#endif

	mtd_read_linux_page(lzo_image_page_offset, lzo_cmp_addr + offset);
	lzo_size = *(size_t *)(lzo_cmp_addr + offset);

	offset += PAGE_SIZE;
	lzo_image_page_offset += pages_per_4k;
	nr_cmp_pages++;

	BUG_ON(lzo_size > LZO_CMP_SIZE);

	/* if compress data size > PAGE_SIZE , read more */
	if (offset < lzo_size + LZO_HEADER) {
		for ( ; offset < lzo_size + LZO_HEADER; offset += PAGE_SIZE) {

			lzo_image_page_offset = block_check_by_page(lzo_image_page_offset);
			mtd_read_linux_page(lzo_image_page_offset, lzo_cmp_addr + offset);

			lzo_image_page_offset += pages_per_4k;
			nr_cmp_pages++;
		}
	}

	if (crc_enable)
		cmp_crc = __crc32(cmp_crc, lzo_cmp_addr + LZO_HEADER, lzo_size);

	lzo_uncompress(lzo_cmp_addr + LZO_HEADER,
			(size_t)lzo_size,
			lzo_target_addr,
			(size_t *)&unc_size);

	if (crc_enable)
		unc_crc = __crc32(unc_crc, lzo_target_addr, unc_size);

	nr_unc_pages += unc_size / PAGE_SIZE;


#ifdef LZO_DEBUG
	uncompress_loop ++;
	printf("%d: %08x[%08x] uncompress %08x[%08x]\n",
			uncompress_loop, lzo_size, cmp_crc, unc_size, unc_crc);
#endif

	/* copy to target phys address after uncompress */
	for (i = 0; i < LZO_UNC_PAGES; i++) {
		/* end of the snapshot of kernel, return */
		if ((target[i] & 0xffffffff) == 0xffffffff)
			return -1;

		BUG_ON(target[i] > DRAM_SIZE || target[i] < 0x200000);
		memcpy((void *)(unsigned long)target[i], lzo_target_addr + PAGE_SIZE * i, PAGE_SIZE);
	}

	return 0;
}
static int load_image_lzo(struct swsusp_info *info)
{
	int i, j;
	unsigned int nr_meta_pages, meta_page_offset;
	void *buf;
	size_t *phys_addr;
	size_t lzo_phy_addr[LZO_UNC_PAGES];

	buf = malloc(PAGE_SIZE);
	if(buf == NULL){
		printf("THAW: failed malloc for load image lzo\n");
		return -1;
	}

	phys_addr = (size_t *)buf;

	nr_meta_pages = info->pages - info->image_pages - 1;

	/* swap partition:
	 *
	 * PAGE-0: reserve
	 * PAGE-1: header
	 * PAGE-2: meta-0
	 * PAGE-3: meta-1
	 *  ...
	 * PAGE-x: lzo compress image
	 *
	 */

	meta_page_offset = (1 + 1) * pages_per_4k;
	lzo_image_page_offset = nr_meta_pages * pages_per_4k + meta_page_offset;

	for (i = 0; i < nr_meta_pages; i++){
		if (mtd_read_linux_page(meta_page_offset, buf)) {
			free(buf);
			return -1;
		}

		for (j = 0; j < PAGE_SIZE / sizeof(size_t);
				j += LZO_UNC_PAGES) {

			if (((*(phys_addr + j)) & 0xffffffff) == 0xffffffff)
				break;

			memcpy(lzo_phy_addr, phys_addr + j, sizeof(lzo_phy_addr));

			if (load_image_lzo1x(info->crc32, lzo_phy_addr))
				break;
		}
		meta_page_offset += pages_per_4k;
	}

	printf("LINUX:	%d pages, crc = %08x\n", nr_unc_pages, unc_crc);
	printf("LZO:	%d pages, crc = %08x\n", nr_cmp_pages, cmp_crc);

	free(buf);

	if (info->crc32)
		BUG_ON(info->crc32 != unc_crc);

#if defined(CONFIG_PANDORA_RTOS) && defined(AMBOOT_BOOT_DSP)
	wait_dsp_done();
#endif
	jump_to_resume(info);

	return 0;
}
int thaw_hibernation(int force_disable, int debug)
{
	struct swsusp_info info;

	if (force_disable || get_swap_info())
		return 0;

#if defined(AMBOOT_BOOT_SECONDARY_CORTEX)
	bld_boot_secondary_cortex();
#endif

	__lzo_reserve_addr =
		IDSP_RAM_START - FRAMEBUFFER_SIZE - AMBOOT_LZO_RESERVE_SIZE;
	__lzo_unc_area = (void *)__lzo_reserve_addr;
	__lzo_work_area = (void *)(__lzo_reserve_addr + 0x100000);
	__lzo_cmp_area = (void *)(__lzo_reserve_addr + 0x200000);

	if (load_snapshot_header(&info, debug))
		return 0;

	if (info.lzo_enable)
		load_image_lzo(&info);
	else
		load_image_original(&info);
	return 0;

}

int thaw_hibernation_info(void)
{
	struct swsusp_info info;

	if (get_swap_info())
		return 0;

	if (load_snapshot_header(&info, 1))
		return 0;

	return 0;

}
