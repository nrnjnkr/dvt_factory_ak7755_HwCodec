/**
 * fs/fat.c
 *
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
#include <sdmmc.h>
#include <ambhw/cache.h>
#include <irq.h>
#include <ambhw/sd.h>
#include "fat.h"
#include "fs.h"

#define VFAT_INIT_NECCESSORY()		do { 	\
	if (disk.vfat_magic != 0x55AA) {		\
		printf("WARNING: Please execute 'fs init' first.\n", __func__);	 \
		return -FS_ERROR; 		\
	}		\
} while(0)

#define VFAT_DBG				1
#define VFAT_DUMP				0
#define VFAT_DEFAULT_EXT_TXT 	0
#define FILE_LIST_MAX 			4 * 16

static struct disk_info disk;
static u8 *trash_loading = (u8 *)DEFAULT_BUF_START;
static u8 *tbl_trash_loading = (u8 *)DEFAULT_BUF_START - 16 * SIZE_1MB;

/* fat32 default cluster start is 2 */
static u32 curcluster = 2;

static struct file file_list[FILE_LIST_MAX]
	__attribute__ ((section(".bss.noinit")));;

/* file_list offset */
static int list_offset;

static int progress_speed_up = 0;

static void downcase(char *str)
{
	while (*str != '\0') {
		if (*str <= 'Z' && *str >= 'A') {
			*str +='a' - 'A';
		}
		str++;
	}
}

static void showprogress(int s, int progress)
{
	 int i;
	 int f = (progress * 100) / s;

	 if (f > progress_speed_up)
		 progress_speed_up = f;
	 else
		 return;

	 if (f > 100)
		 return;

	 putchar('[');

	 for (i = 0; i <= f; i++)
		 putchar('=');

	 for(; i <= 100; i++)
		 putchar(' ');

	 printf("] %d%% %d/%d\r", f , progress, s);
}

static int vfat_disk_write(u32 ssec, u32 nsec, void *buffer)
{
	int ret_val = 0;

	ret_val = sdmmc_write_sector(disk.boot_sector + ssec, nsec, (unsigned char *)buffer);
	if (ret_val < 0) {
		printf("%s: Cluster write at %d sector failed\n", __func__, disk.boot_sector + ssec);
		return -FS_ERROR;
	}
	return FS_SUCCESS;
}

static int vfat_disk_read(u32 ssec, u32 nsec, void *buffer)
{
	int ret_val = 0;

	ret_val = sdmmc_read_sector(disk.boot_sector + ssec, nsec, (unsigned char *)buffer);
	if (ret_val < 0) {
		printf("%s: Cluster read at %d sector failed\n", __func__, disk.boot_sector + ssec);
		return -FS_ERROR;
	}
	return FS_SUCCESS;
}

static int vfat_disk_write_cluster(u32 cluster, void *buffer)
{
	return vfat_disk_write(CLUST2SECTOR(cluster), disk.clust_size, buffer);
}

static int vfat_disk_read_cluster(u32 cluster, void *buffer)
{
	return vfat_disk_read(CLUST2SECTOR(cluster), disk.clust_size, buffer);
}

static int vfat_filename_limit(const char *str)
{
	while(*str != '\0'){
		if(*str <= 'Z' && *str >= 'A')
			return -FS_ERROR;
		str++;
	}
	return FS_SUCCESS;
}

#if VFAT_DUMP
static void vfat_dump(void *p, int len)
{
	int i;
	int *d = p;
	for (i = 0; i < len / 4; i++) {
		if ( i % 8 == 0 )
			printf("\n[%08x]  ", i * 4);

		printf("%08x ", d[i]);
	}

	printf("\n");
}
#endif

static int slot2str(dir_slot *slotptr, char *l_name, int *idx)
{
	int j;

	for (j = 0; j <= 8; j += 2) {
		l_name[*idx] = slotptr->name0_4[j];
		if (l_name[*idx] == 0x00)
			return 1;
		(*idx)++;
	}
	for (j = 0; j <= 10; j += 2) {
		l_name[*idx] = slotptr->name5_10[j];
		if (l_name[*idx] == 0x00)
			return 1;
		(*idx)++;
	}
	for (j = 0; j <= 2; j += 2) {
		l_name[*idx] = slotptr->name11_12[j];
		if (l_name[*idx] == 0x00)
			return 1;
		(*idx)++;
	}

	return 0;
}
dir_entry * getvfatname(dir_entry *retdent, char *name, u8 *low)
{
	dir_entry *realdent;
	dir_slot *slotptr = (dir_slot *)retdent;
	u8 *limit = low + disk.sect_size * disk.clust_size;

	u8 counter = (slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff;

	int idx = 0;

	if (counter > VFAT_MAXSEQ) {
		printf("Error: VFAT name is too long\n");
		return NULL;
	}

	while ((u8 *)slotptr < limit) {
		if (counter == 0)
			break;
		if (((slotptr->id & ~LAST_LONG_ENTRY_MASK) & 0xff) != counter)
			return NULL;
		slotptr++;
		counter--;
	}

	if ((u8 *)slotptr >= limit){
		printf("no supported now\n");
		return NULL;
	} else {
		realdent = (dir_entry *)slotptr;
	}

	do {
		slotptr--;
		if (slot2str(slotptr, name, &idx))
			break;
	} while (!(slotptr->id & LAST_LONG_ENTRY_MASK));

	name[idx] = '\0';
	if (*name == DELETED_FLAG)
		*name = '\0';
	else if (*name == aRING)
		*name = DELETED_FLAG;

	return realdent;
}

static int vfat_file_attr(dir_entry *entry)
{
	if((entry->attr & ATTR_DIR) == ATTR_DIR)
		printf("/\t");
	else if((entry->attr & ATTR_ARCH) == ATTR_ARCH)
		printf("\t");
	else
		printf("?\t");

	return 0;
}

void vfat_list_push(dir_entry *entry, char *name, int offset)
{
	strncpy(file_list[offset].name, name, strlen(name));

	file_list[offset].attr = entry->attr;
	file_list[offset].cluster = (entry->starthi << 16)|entry->start;
	file_list[offset].size = entry->size;
}

static void get_name(dir_entry *dirent, char *s_name)
{
	char *ptr;

	memcpy(s_name, dirent->name, 8);
	s_name[8] = '\0';
	ptr = s_name;
	while (*ptr && *ptr != ' ')
		ptr++;
	if (dirent->ext[0] && dirent->ext[0] != ' ') {
		*ptr = '.';
		ptr++;
		memcpy(ptr, dirent->ext, 3);
		ptr[3] = '\0';
		while (*ptr && *ptr != ' ')
			ptr++;
	}
	*ptr = '\0';
	if (*s_name == DELETED_FLAG)
		*s_name = '\0';
	else if (*s_name == aRING)
		*s_name = DELETED_FLAG;

	downcase(s_name);
}

static int vfat_file_list_loading(int verbose)
{
	dir_entry *dirptr;
	char s_name[256];
	unsigned char *buffer = trash_loading;

	list_offset = 0;
	memset(file_list , 0, sizeof(file_list));

	if(vfat_disk_read_cluster(curcluster, buffer))
		return -FS_ERROR;

	dirptr = (dir_entry *)buffer;

	while(((uintptr_t)dirptr - (uintptr_t)buffer) < CLUSTSIZE)
	{
		/* no need show deleted files ... */

		if(dirptr->name[0] == DELETED_FLAG
				|| dirptr->name[0] == 0){
			dirptr++;
			continue;
		}

		if(dirptr->attr & ATTR_VOLUME){

			if((dirptr->attr & ATTR_VFAT) == ATTR_VFAT
					&& (dirptr->name[0] & LAST_LONG_ENTRY_MASK) ){

				dirptr = getvfatname(dirptr, s_name, buffer);
				if(!dirptr)
					return -1;
			}else{
				/* XXX we need this ? */
				dirptr++;
				continue;
			}
		}else{
			/* for having LFN without SFN . Cards have two diffrent rule . why ? */
			get_name(dirptr, s_name);
		}

		if (verbose)
			printf("%s A[%08x] L[%08x] ", dirptr->attr == 0x10 ? "D" : "-",
					(dirptr->starthi << 16)|dirptr->start, dirptr->size);

		vfat_list_push(dirptr, s_name, list_offset);
		list_offset++;

		if (verbose){
			printf("%s", s_name);
			vfat_file_attr(dirptr);
			printf("\n");

			/* just show FILE_LIST_MAX files */
			if(list_offset >= FILE_LIST_MAX){
				printf("overflow ... \n");
				return FS_SUCCESS;
			}
		}
		dirptr++;
	}
	return 0;
}
static int vfat_change_directory(const char *dir)
{
	int i;
	struct file *file;
	for(i = 0; i < list_offset; i++) {
		if(!strcmp(dir, file_list[i].name)){
			if(file_list[i].attr == ATTR_DIR){
				file = &file_list[i];
				if(file->cluster != 0){
					curcluster = file->cluster;
				} else {
					/* if parent dir ".." is root dir, the cluster is 0 */
					curcluster = 2;
				}
				printf("Change directory: %s %d\n", dir, curcluster);
				return FS_SUCCESS;
			} else {
				printf("WARNING: %s is not a directory\n", dir);
				return -FS_ERROR;
			}
		}
	}
	printf("ERROR:%s directory is not exist\n", dir);
	return -FS_ERROR;

}
static int vfat_magic_check(unsigned char *s)
{
	/* Check if it's actually a DOS volume */
	if (memcmp(s + DOS_PART_MAGIC_OFFSET , "\x55\xAA", 2) != 0) {
		printf("%s: vfat magic is not matched!\n");
		return -FS_ERROR;
	}
	return FS_SUCCESS;
}
static int vfat_format_check(unsigned char *s)
{
	return (!memcmp(s + DOS_FS_TYPE_OFFSET, "FAT", 3)
			|| !memcmp(s + DOS_FS32_TYPE_OFFSET, "FAT32", 5));
}

static int vfat_dbr_loading(void)
{
	unsigned char *data = trash_loading;
	struct boot_sector *dbr;

	if (vfat_disk_read(0, 1, data))
		return -FS_ERROR;

	if(vfat_magic_check(data)) {
		printf("%s: magic dismatch!\n");
		return -FS_ERROR;
	}

	if (vfat_format_check(data)) {
		dbr = (struct boot_sector *)data;

		disk.data_sector	= dbr->fats * dbr->fat32_length + dbr->reserved;
		disk.tbl_addr 		= dbr->reserved;
		disk.fatlength 		= dbr->fat32_length;
		disk.fat_sect 		= dbr->reserved;
		disk.sect_size 		= dbr->sector_size[0]|(dbr->sector_size[1] << 8);
		disk.clust_size 	= dbr->cluster_size;
		disk.vfat_magic		= 0x55AA;

		curcluster = 2;

		return FS_SUCCESS;

	}

	printf("%s: no match vfat filesystem, unbelievable!\n",__func__);
	return -FS_ERROR;
}

static int vfat_filesystem_loading(unsigned char *data)
{
	struct boot_sector *dbr;
	dos_partition_t *partition;

	if (vfat_format_check(data)) {
		dbr = (struct boot_sector *)data;

		disk.boot_sector 	+= 0;
		disk.total_sector 	= dbr->total_sect;
		disk.data_sector	= dbr->fats * dbr->fat32_length + dbr->reserved;
		disk.tbl_addr 		= dbr->reserved;
		disk.fatlength 		= dbr->fat32_length;
		disk.fat_sect 		= dbr->reserved;
		disk.sect_size 		= dbr->sector_size[0]|(dbr->sector_size[1] << 8);
		disk.clust_size 	= dbr->cluster_size;
		disk.vfat_magic		= 0x55AA;

		curcluster = 2;

		return FS_SUCCESS;
	}

	/* If the first sector has correct magic, and is mismatched the FAT32 filesystem,
	 * then it's a MBR sector */


	partition = (dos_partition_t *)(data + DOS_PART_TBL_OFFSET);

	disk.boot_sector += partition->start4[0]
		| (partition->start4[1] << 8)
		| (partition->start4[2] << 16)
		| (partition->start4[3] << 24);

	disk.total_sector = partition->size4[0]
		| (partition->size4[1] << 8)
		| (partition->size4[2] << 16)
		| (partition->size4[3] << 24);

	printf("\nvfat filesystem has MBR SECTOR. DBR offset is %d.\n", disk.boot_sector);

	vfat_dbr_loading();

	return FS_SUCCESS;
}

static int vfat_mbr_probe(void)
{
	int ret_val = FS_SUCCESS;
	unsigned char *data = trash_loading;

	ret_val = sdmmc_read_sector(disk.boot_sector, 1, data);
	if (ret_val < 0) {
		printf("%s: read %d sector error!\n", __func__, disk.boot_sector);
		return ret_val;
	}

	if(vfat_magic_check(data)) {
		printf("%s: magic dismatch!\n");
		return -FS_ERROR;
	}

	ret_val = vfat_filesystem_loading(data);
	if (ret_val < 0)
		return ret_val;

	return ret_val;
}

static int get_disk_partition_position(void)
{
	/* FIXME: we also support vfat filesystem on EMMC partition. Please return
	 * your partition offset on the EMMC disk over here. Normal SD Card offset
	 * is zero. */
	return 0;
}

static void vfat_info_verbose(void)
{
	printf("Boot:		%d\n", disk.boot_sector);
	printf("Total:		%d\n", disk.total_sector);
	printf("Playload:	%d\n", disk.data_sector);
	printf("Table:		%d\n", disk.tbl_addr);
	printf("FatLen:		%d\n", disk.fatlength);
	printf("FatSect:	%d\n", disk.fat_sect);
	printf("SectSize:	%d\n", disk.sect_size);
	printf("ClustSize:	%d\n", disk.clust_size);

	printf("\n");
}

static int vfat_disk_probe(void)
{
	memset(&disk, 0, sizeof(disk));

	disk.boot_sector = get_disk_partition_position();

	if (vfat_mbr_probe())
		return -FS_ERROR;

	vfat_info_verbose();
	return FS_SUCCESS;
}

static struct file *vfat_file_detect(const char *filename)
{

	int i;
	struct file *file = NULL;

	for(i = 0; i < list_offset; i++){
		if(!strncmp(filename, file_list[i].name, 8)){
				file = &file_list[i];
				break;
		}
	}
	return file;
}
static int vfat_tbl_is_invalid(void)
{
	int *data = (int *)tbl_trash_loading;
	/* FIXME: We don't support multiple files, so one TBL is enough for customers */
	if (vfat_disk_read(disk.tbl_addr, disk.fatlength, data))
		return -FS_ERROR;

	if(data[0] != FATTBL_MAGIC) {
		printf("%s: vfat TBL magic [%08x] dismatch [%08x]\n", data[0], FATTBL_MAGIC);
		return -FS_ERROR;
	}

	return FS_SUCCESS;
}
static int vfat_update_tbl(void)
{
	return	vfat_disk_write(disk.tbl_addr, disk.fatlength, tbl_trash_loading);
}

static int vfat_program_write(dir_entry *dirptr, void *addr, u32 size)
{

	int s;
	int progress = 0;
	u32 *fat_tbl = (u32 *)tbl_trash_loading;
	int nr_clust =  size / CLUSTSIZE;
	u32 cluster ;
	u32 *next_clust = NULL;
	unsigned char *source = addr;

	if(nr_clust * CLUSTSIZE < size)
		nr_clust++;

	s = nr_clust;

	progress_speed_up = 0;

	if(vfat_tbl_is_invalid())
		return -FS_ERROR;

	while(((uintptr_t)fat_tbl - (uintptr_t)tbl_trash_loading)
			< (disk.fatlength * disk.sect_size))
	{
		if (*fat_tbl) {
			fat_tbl++;
			continue;
		}

		cluster = ((uintptr_t)fat_tbl - (uintptr_t)tbl_trash_loading) / 4;

		/* next_clust record next cluster */
		if(next_clust != NULL)
			*next_clust = cluster;

		next_clust = fat_tbl;

		if(dirptr->start == 0 && dirptr->starthi == 0){
			dirptr->start = cluster & 0xffff;
			dirptr->starthi = (cluster >> 16) & 0xffff;
		}

		showprogress(s, ++progress);

		vfat_disk_write_cluster(cluster, source);

		source += CLUSTSIZE;

		if(!--nr_clust){
			*next_clust = FATTBL_END;
			break;
		}
		fat_tbl++;
	}

	printf("\n");
	vfat_update_tbl();
	return 0;
}

static int vfat_write_progress(const char *filename, void *addr, u32 size)
{
	int i;
	dir_entry *dirptr;
	char l_name[8];
	char default_ext[] = {0x20, 0x20, 0x20};
	void *data = trash_loading;

	memset(l_name, 0x20, 8);

	if(strlen(filename) >= 8)
		memcpy(l_name, filename, 8);
	else
		memcpy(l_name, filename, strlen(filename));

	if(vfat_disk_read_cluster(curcluster, data))
		return -FS_ERROR;

	dirptr = (dir_entry *)data;

	for(i = 0; i < DIRENTSPERCLUST; i++){
		if(dirptr->name[0] != DELETED_FLAG
				&& dirptr->name[0] != 0){
			dirptr++;
			continue;
		}

		memset(dirptr, 0, 32);

		dirptr->attr = ATTR_ARCH;
		dirptr->size = size;
		memcpy(dirptr->name, l_name, 8);

#if  VFAT_DEFAULT_EXT_TXT
		memcpy(dirptr->ext,"TXT",3);
#else
		memcpy(dirptr->ext, default_ext, 3);
#endif

		dirptr->lcase    = DEFAULT_LCASE;
		dirptr->ctime_ms = DEFAULT_CTIME_MS;
		dirptr->ctime    = DEFAULT_CTIME;
		dirptr->cdate    = DEFAULT_CDATE;
		dirptr->adate    = DEFAULT_ADATE;
		dirptr->time     = DEFAULT_TIME;
		dirptr->date     = DEFAULT_DATE;

		vfat_program_write(dirptr, addr, size);
		if (vfat_disk_write_cluster(curcluster, data))
			return -FS_ERROR;

		printf("%s: %d bytes write at %08x successfully ...\n", filename, size, addr);
		break;
	}

	return FS_SUCCESS;

}
static int vfat_file_write(const char *filename, void *src, u32 size)
{
	if (vfat_file_detect(filename)) {
		printf("Collision: %s is exist!\n", filename);
		return -FS_ERROR;
	}

	if (size <= 0) {
		printf("WARNING: size = %d\n", size);
		return -FS_ERROR;
	}

	if (vfat_tbl_is_invalid())
		return -FS_ERROR;

	return vfat_write_progress(filename, src, size);
}


static int vfat_program_read(void *tbl, unsigned int cluster, void *addr, unsigned int nr_clust)
{
	int s;
	int progress = 0;
	int i = 0;
	unsigned char *load = addr;
	unsigned int *offset = tbl;
	u32 clust_next = cluster;
	s = nr_clust;

	progress_speed_up = 0;

	do {
		i++;
		vfat_disk_read_cluster(clust_next, load);

		load += CLUSTSIZE;
		clust_next = *(offset + clust_next);

		showprogress(s, ++progress);

		if(clust_next == FATTBL_END
				|| clust_next == FATTBL_BAD
				|| clust_next == FATTBL_MAGIC
				|| !clust_next)
			break;

	} while(1);

	printf("\n");
	return 0;
}

static int vfat_file_read(const char *name, void *addr)
{
	struct file *file;
	void *vfat_tbl = tbl_trash_loading;

	file = vfat_file_detect(name);
	if (!file) {
		printf("WARNING: %s is not found!\n", name);
		return -FS_ERROR;
	}

	if (vfat_tbl_is_invalid())
		return -FS_ERROR;

	vfat_program_read(vfat_tbl, file->cluster, addr, file->size / CLUSTSIZE);

	printf("%s: %d bytes read at %08x successfully ...\n", name, file->size, addr);

	return FS_SUCCESS;

}

int file_fat_init(void)
{
	return vfat_disk_probe();
}

int file_fat_ls(const char *dir)
{
	VFAT_INIT_NECCESSORY();
	return vfat_file_list_loading(1);
}

int file_fat_chdir(const char *dir)
{
	VFAT_INIT_NECCESSORY();
	if (vfat_change_directory(dir))
		return -FS_ERROR;

	return vfat_file_list_loading(0);
}

int file_fat_info(void)
{
	return FS_SUCCESS;
}

int file_fat_read(const char *filename, u32 addr, int exec)
{
	VFAT_INIT_NECCESSORY();

	void *ptr;
	void (*jump)(void) = (void *)(uintptr_t)addr;

	if(addr < DRAM_START_ADDR || addr > DRAM_START_ADDR + DRAM_SIZE-1){
		printf("vfat: invalid addr %08x\n", addr);
		return -FS_ERROR;
	}

	vfat_file_list_loading(0);

	if (vfat_file_read(filename, (void *)(uintptr_t)addr))
		return -FS_ERROR;

	if (exec) {
		putstr("Start to run...\r\n");
		_clean_flush_all_cache();
		_disable_icache();
		_disable_dcache();
		disable_interrupts();

		/* put the return address in 0xc00ffffc
		 * and let jump() to read and return back */
		ptr = &&_return_;
		*(volatile u32 *)(DRAM_START_ADDR + 0x000ffffc) = (uintptr_t)ptr;

		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");

		jump ();

		/* being here only if REBOOT_AFTER_BURNING */
_return_:
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");

	}
	return FS_SUCCESS;

}
int file_fat_write(const char *filename, u32 addr, u32 size)
{

	VFAT_INIT_NECCESSORY();
	vfat_file_list_loading(0);

	if(addr < DRAM_START_ADDR
			|| ( addr + size > DRAM_START_ADDR + DRAM_SIZE - 1)){
		printf("invalid addr and size\n");
		return -FS_ERROR;
	}

	if(vfat_filename_limit(filename)){
		printf("file name is only supported lowcase, FIXME!\n");
		return -FS_ERROR;
	}

	return vfat_file_write(filename, (void *)(uintptr_t)addr, size);
}
