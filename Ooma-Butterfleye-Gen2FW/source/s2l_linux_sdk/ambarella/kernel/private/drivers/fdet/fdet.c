/*
 * kernel/private/drivers/ambarella/fdet/fdet.c
 *
 * History:
 *    2012/06/28 - [Zhenwu Xue] Create
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


#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/fb.h>
#include <linux/seq_file.h>
#include <plat/ambcache.h>
#include <ambhw/chip.h>
#include <asm/uaccess.h>
#include <iav_utils.h>
#include <amba_fdet.h>
#include "fdet.h"


#define FDET_DEBUG(format, arg...)	\
	do {	\
		if (pfdet_info->config.policy & FDET_POLICY_DEBUG) {	\
			printk(format , ## arg);	\
		}	\
	} while (0)

#define FDET_MAX_FACES			32

#define FDET_MAX_INPUT_WIDTH		1024
#define FDET_MAX_INPUT_HEIGHT		1024
#define FDET_TEMPLATE_SIZE		20

#define FDET_MAX_FACE_CLASSIFIERS	24
#define FDET_MAX_ORIENTATIONS		8

#define FDET_SCALED_PARTITION_SIZE	40

#define FDET_NUM_EVALS			1

#define FDET_MAX_UNMERGED_FACES		2048
#define FDET_MAX_MERGED_FACES		128

#define FDET_FS_CMD_BUF_SIZE		(8 * 1024)
#define FDET_FS_RESULT_BUF_SIZE		(8 * 1024)
#define FDET_TS_CMD_BUF_SIZE		(4 * 1024)
#define FDET_TS_RESULT_BUF_SIZE		(64 * 1024)

#define FDET_ORIG_TARGET_BUF_SIZE	(FDET_MAX_INPUT_WIDTH * FDET_MAX_INPUT_HEIGHT)
#define FDET_TMP_TARGET_BUF_SIZE	(FDET_MAX_INPUT_WIDTH * FDET_MAX_INPUT_HEIGHT)
#define FDET_CLASSIFIER_BINARY_SIZE	(128 * 1024)

#define FDET_VM_DELAY			(300)

#define FDET_NAME			"fdet"
#define FDET_MAJOR			248
#define FDET_MINOR			148

/* ========================================================================== */
const int hitcount_threshold_still[8] = {14, 12, 12, 3, 3, 3, 3, 0};
const int hitcount_threshold_video[8] = {14,  9,  9, 4, 4, 4, 4, 0};

const int search_radius[32] = {
	5, 3, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
};

struct fdet_adjacent_update_info {
	unsigned char				eval_id;
	unsigned char				orientation_bitmask;
};
struct fdet_orientation_info {
	unsigned char				eval_id;
	unsigned char				orientation_mode;
	unsigned char				adjacent_num_evals;
	struct fdet_adjacent_update_info	adjacent_update_info[FDET_MAX_ORIENTATIONS];
};

struct fdet_orientation_info fdet_orientation_table[FDET_MAX_ORIENTATIONS] = {
	{0, 0, 3, {{0,0x01}, {1,0x03}, {2,0x03}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{1, 0, 2, {{0,0x01}, {1,0x01}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{1, 1, 2, {{0,0x01}, {1,0x02}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 0, 2, {{0,0x01}, {2,0x09}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 1, 2, {{0,0x01}, {2,0x22}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 3, 1, {{2,0x09}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 5, 1, {{2,0x22}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{0, 0, 0, {{0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
};

enum fdet_vm_state {
	FDET_VM_STATE_IDLE	= 0x0,
	FDET_VM_STATE_READY,
	FDET_VM_STATE_RUNNING,
};

struct fdet_classifier_info {
	unsigned short			offset;
	unsigned short			sz;
	unsigned short			left_offset;
	unsigned short			top_offset;
	unsigned char			orientation_bitmasks;
	unsigned char			orientation_result[FDET_MAX_ORIENTATIONS];
};

struct fdet_classifier_binary_info {
	unsigned int			cls_base;

	unsigned int			num_fs_classifiers;
	unsigned int			num_ts_classifiers;

	struct fdet_classifier_info	fs_cls_info[FDET_MAX_FACE_CLASSIFIERS];
	struct fdet_classifier_info	ts_cls_info[FDET_MAX_FACE_CLASSIFIERS];
};

struct fdet_unmerged_face {
	unsigned int			x;
	unsigned int			y;
	unsigned int			si;
	unsigned int			sz;
	int				cluster;
	int				oi;
};

struct fdet_merged_face {
	unsigned int			x;
	unsigned int			y;
	unsigned int			sz;
	unsigned int			num_si[32];
	unsigned int			hit_count[8];
	unsigned int			hit_sum;
	unsigned int			best_oi;
	unsigned int			best_hitcount;
	enum fdet_result_type		type;
};

struct amba_fdet_info {
	char					fs_cmd_buf[2][FDET_FS_CMD_BUF_SIZE];
	char					fs_result_buf[2][FDET_FS_RESULT_BUF_SIZE];
	char					ts_cmd_buf[2][FDET_TS_CMD_BUF_SIZE];
	char					ts_result_buf[2][FDET_TS_RESULT_BUF_SIZE];
	int					current_fs_buf_id;
	int					current_fs_result_id;
	int					current_ts_buf_id;
	int					current_ts_result_id;
	int					current_tmp_target_id;
	int					current_fs_buf_sz;
	int					current_ts_buf_sz;
	int					last_fs_cmd_id;
	enum fdet_vm_state			vm_state;

	char					*orig_target_buf;
	char					*tmp_target_buf;
	char					*classifier_binary;
	int					orig_len;
	int					cls_bin_len;
	enum fdet_mmap_type			mmap_type;

	struct fdet_configuration		config;

	int					num_scales;
	int					num_sub_scales;
	int					num_total_scales;
	unsigned int				scale_factor[32];
	unsigned int				recip_scale_factor[32];
	unsigned int				scale_factor_regs[32];

	struct fdet_classifier_binary_info	classifierBinaryInfo;
	unsigned int				evaluation_id[32];
	unsigned int				evaluation_num[32];

	struct fdet_unmerged_face		unmerged_faces[FDET_MAX_UNMERGED_FACES];
	struct fdet_unmerged_face		ts_unmerged_faces[FDET_MAX_UNMERGED_FACES];
	struct fdet_merged_face			merged_faces[FDET_MAX_MERGED_FACES];
	struct fdet_merged_face			latest_faces[FDET_MAX_MERGED_FACES];
	int					num_faces;
	int					latest_faces_num;
	unsigned int				fs_found_faces[2];

	int					irq;
	struct cdev				char_dev;

	struct timer_list			timer;
	struct completion			result_completion;

	unsigned long				fs_tick;
	unsigned long				ts_tick;
};

static struct amba_fdet_info	*pfdet_info = NULL;

/* ========================================================================== */
int fdet_get_result_still(void);
int fdet_get_result_video_fs(void);
int fdet_get_result_video_ts(void);
void fdet_print_faces(void);
void fdet_config(void);

static int amba_enable_fdet(void)
{
	int		i, ret = -1;
	u32		val;

	amba_writel(FDET_ENABLE_REG, FDET_ENABLE);
	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);

	for (i = 0; i < 5; i++) {
		val = amba_readl(FDET_ENABLE_REG);
		if (val == FDET_ENABLE) {
			ret = 0;
			break;
		}
	}

	if (ret) {
		printk("%s: Fail to enable fdet!\n", __func__);
	} else {
		amba_writel(FDET_GO_REG, 0);
		amba_writel(FDET_CONFIG_DONE_REG, 0);
		amba_writel(FDET_RESET_REG, 0);
		amba_writel(FDET_ERROR_STATUS_REG, 0xffff);
	}

	return ret;
}

static int amba_disable_fdet(void)
{
	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_RESET_REG, FDET_RESET);
	amba_writel(FDET_ENABLE_REG, FDET_DISABLE);

	return 0;
}

static void fdet_timer(unsigned long context)
{
	del_timer(&pfdet_info->timer);

	if (pfdet_info->vm_state == FDET_VM_STATE_IDLE) {
		pfdet_info->vm_state = FDET_VM_STATE_READY;
		amba_writel(FDET_CONFIG_DONE_REG, FDET_CONFIG_DONE);
	}
}

static irqreturn_t amba_fdet_isr(int irq, void *dev_data)
{
	int			num = -1;
	unsigned long		tick;

	if (pfdet_info->config.input_mode == FDET_MODE_STILL) {
		num = fdet_get_result_still();
		FDET_DEBUG("Found %d Faces:\n", num);
	} else {
		switch (pfdet_info->vm_state) {
		case FDET_VM_STATE_IDLE:
			pfdet_info->timer.expires = jiffies + FDET_VM_DELAY * HZ / 1000;
			add_timer(&pfdet_info->timer);
			break;

		case FDET_VM_STATE_READY:
			num = fdet_get_result_video_fs();
			FDET_DEBUG("Found %d Faces:\n", num);
			break;

		case FDET_VM_STATE_RUNNING:
			num = fdet_get_result_video_fs();
			num = fdet_get_result_video_ts();

			if (pfdet_info->config.policy & FDET_POLICY_MEASURE_TIME) {
				tick			= jiffies;
				printk("Ts used time: %lu ms\n", 1000 * (tick - pfdet_info->ts_tick) / HZ);
				pfdet_info->ts_tick	= tick;
			}
			FDET_DEBUG("Found %d Faces:\n", num);
			break;

		default:
			break;
		}
	}

	if (num < 0) {
		goto amba_fdet_isr_exit;
	}

	if (num > 0) {
		memcpy(pfdet_info->latest_faces, pfdet_info->merged_faces, num * sizeof(struct fdet_merged_face));
		pfdet_info->latest_faces_num = num;
		fdet_print_faces();
	}

	complete(&pfdet_info->result_completion);

amba_fdet_isr_exit:
	amba_writel(FDET_ERROR_STATUS_REG, 0xffffffff);
	return IRQ_HANDLED;
}


/* ========================================================================== */
int fdet_get_sf(int width, int height)
{
	int			num_scales, num_sub_scales, num_total_scales;
	int			si, sr, srr;
	unsigned int 		swi, csw, csh, csf;
	unsigned int		crf;
	long long		csrr;
	unsigned int		sfMant;
	int			sfExpn;
	unsigned short		hwsf_mant;
	unsigned char		hwsf_expn;
	unsigned int		rsf_mant;
	unsigned int		rsf_expn;
	unsigned short		hwrsf_mant;
	unsigned char		hwrsf_expn;

	if (width > 320) {
		swi	= 320;
	} else {
		swi	= (width / FDET_TEMPLATE_SIZE) * FDET_TEMPLATE_SIZE;
	}

	if (pfdet_info->config.input_mode == FDET_MODE_STILL) {
		sr	= 58514;
		srr	= 73400;
	} else {
		sr	= 56988;
		srr	= 75366;
	}

	csw	= swi;
	csh	= (csw * height + (width >> 2)) / width;
	csrr	= (long long)sr;
	for (si = 0; si < 32; si++) {
		csw	= (swi * (int)csrr + (1 << 15)) >> 16;
		csh	= (csw * height + (width >> 2)) / width;
		if (csw < FDET_TEMPLATE_SIZE || csh < FDET_TEMPLATE_SIZE) {
			break;
		}
		csrr = (csrr * sr + (1 << 15)) >> 16;
	}
	num_scales = si + 1;

	if (num_scales < 1) {
		return 0;
	}

	csrr	= (long long)srr;
	for (si = num_scales; si < 32; si++) {
		csw	= (swi * (int)csrr + (1 << 15)) >> 16;
		csh	= (csw * height + (width >> 2)) / width;
		if (csw > width || csh > height) {
			break;
		}
		csrr	= (csrr * srr + (1 << 15)) >> 16;
	}
	num_total_scales	= si;
	num_sub_scales		= num_total_scales - num_scales;

	csw	= swi;
	csh	= (csw * height + (width >> 2)) / width;
	csrr	= (long long)sr;
	for (si = num_sub_scales; si < num_total_scales; si++) {
		csf = ((csw << (16 + 1)) + width) / (2 * width);
		pfdet_info->scale_factor[si] = csf;

		sfMant	= csf;
		sfExpn	= 0;
		while (sfMant < 65536) {
			sfMant = sfMant << 1;
			sfExpn--;
		}
		hwsf_mant = sfMant >> (16 - 10);
		hwsf_expn = abs(sfExpn);

		crf = ((width << (16 + 1)) + csw) / (2 * csw);
		pfdet_info->recip_scale_factor[si] = crf;

		rsf_mant = crf;
		rsf_expn = 0;
		while (rsf_mant >= 2 * 65536) {
			rsf_mant = rsf_mant >> 1;
			rsf_expn++;
		}
		hwrsf_mant = rsf_mant >> (16 - 10);
		hwrsf_expn = rsf_expn;
		pfdet_info->scale_factor_regs[si] = (hwrsf_mant << 17) | (hwrsf_expn << 14) | (hwsf_mant << 3) | hwsf_expn;

		if (hwsf_mant == 1024 && hwsf_expn == 0) {
			pfdet_info->scale_factor[si]		= 65472;
			hwsf_mant				= 1023;
			hwsf_expn				= 0;
			pfdet_info->recip_scale_factor[si]	= 65600;
			hwrsf_mant				= 1025;
			hwrsf_expn				= 0;
			pfdet_info->scale_factor_regs[si]	= (hwrsf_mant << 17) | (hwrsf_expn << 14) | (hwsf_mant << 3) | hwsf_expn;
		}

		csw	= (swi * (int)csrr + (1 << 15)) >> 16;
		csh	= (csw * height + (width >> 2 )) / width;
		csrr	= (csrr * sr + (1 << 15)) >> 16;
	}

	csrr	= (long long)srr;
	for (si = num_sub_scales - 1; si >= 0; si--) {
		csw	= (swi * (int)csrr + (1 << 15)) >> 16;
		csh	= (csw * height + (width >> 2)) / width;
		csf	= ((csw << (16 + 1)) + width) / (2 * width);
		pfdet_info->scale_factor[si] = csf;

		sfMant = csf;
		sfExpn = 0;
		while (sfMant < 65536) {
			sfMant = sfMant << 1;
			sfExpn--;
		}
		hwsf_mant = sfMant >> (16 - 10);
		hwsf_expn = abs(sfExpn);

		crf = ((width << (16 + 1)) + csw) / (2 * csw);
		pfdet_info->recip_scale_factor[si] = crf;

		rsf_mant = crf;
		rsf_expn = 0;
		while (rsf_mant >= 2 * 65536) {
			rsf_mant = rsf_mant >> 1;
			rsf_expn++;
		}
		hwrsf_mant = rsf_mant >> (16 - 10);
		hwrsf_expn = rsf_expn;
		pfdet_info->scale_factor_regs[si] = (hwrsf_mant << 17) | (hwrsf_expn << 14) | (hwsf_mant << 3) | hwsf_expn;

		if (hwsf_mant == 1024 && hwsf_expn == 0) {
			pfdet_info->scale_factor[si]		= 65472;
			hwsf_mant				= 1023;
			hwsf_expn				= 0;
			pfdet_info->recip_scale_factor[si]	= 65600;
			hwrsf_mant				= 1025;
			hwrsf_expn				= 0;
			pfdet_info->scale_factor_regs[si]	= (hwrsf_mant << 17) | (hwrsf_expn << 14) | (hwsf_mant << 3) | hwsf_expn;

		}

		csrr = (csrr * srr + (1 << 15)) >> 16;
	}

	pfdet_info->num_scales		= num_scales;
	pfdet_info->num_sub_scales	= num_sub_scales;
	pfdet_info->num_total_scales	= num_total_scales;

	return num_total_scales;
}

int fdet_get_evaluation_ids(void)
{
	char				*currentPtr = pfdet_info->classifier_binary;
	int				num_fs_classifiers, num_ts_classifiers;
	int				i, j;
	unsigned char			*class_stream;
	unsigned short			num_stages;
	unsigned int			pointer_word;
	int				left_stream_ptr, top_stream_ptr;
	int				cls_cache_addr = 0;
	struct fdet_classifier_info	*p_classifier_info;

	currentPtr += 34;
	pfdet_info->classifierBinaryInfo.cls_base = *(unsigned short *)currentPtr;
	currentPtr += 2;

	num_fs_classifiers = pfdet_info->classifierBinaryInfo.num_fs_classifiers = *(unsigned short *)currentPtr;
	currentPtr += 2;

	p_classifier_info = pfdet_info->classifierBinaryInfo.fs_cls_info;
	for (i = 0; i < num_fs_classifiers; i++) {
		p_classifier_info[i].offset			= *(unsigned short *)(currentPtr);
		p_classifier_info[i].sz				= *(unsigned short *)(currentPtr + 2);
		p_classifier_info[i].left_offset		= *(unsigned short *)(currentPtr + 4);
		p_classifier_info[i].top_offset			= *(unsigned short *)(currentPtr + 6);
		p_classifier_info[i].orientation_bitmasks	= *(unsigned char *)(currentPtr + 8);
		for (j = 0; j < 8; j++) {
			p_classifier_info[i].orientation_result[j]	= *(unsigned char *)(currentPtr + 9 + j);
		}
		currentPtr += 18;
	}

	num_ts_classifiers = pfdet_info->classifierBinaryInfo.num_ts_classifiers = *(unsigned short *)currentPtr;
	currentPtr += 2;
	for (i = 0; i < num_ts_classifiers; i++) {
		pfdet_info->classifierBinaryInfo.ts_cls_info[i].offset		= *(unsigned short *)(currentPtr);
		pfdet_info->classifierBinaryInfo.ts_cls_info[i].sz		= *(unsigned short *)(currentPtr + 2);
		pfdet_info->classifierBinaryInfo.ts_cls_info[i].left_offset	= *(unsigned short *)(currentPtr + 4);
		pfdet_info->classifierBinaryInfo.ts_cls_info[i].top_offset	= *(unsigned short *)(currentPtr + 6);
		pfdet_info->classifierBinaryInfo.ts_cls_info[i].orientation_bitmasks= *(unsigned char *)(currentPtr + 8);
		for (j = 0; j < 8; j++) {
			pfdet_info->classifierBinaryInfo.ts_cls_info[i].orientation_result[j]	= *(unsigned char *)(currentPtr + 9 + j);
		}
		currentPtr += 18;
	}

	for (i = 0; i < num_fs_classifiers; i++) {
		class_stream	= (unsigned char *)(pfdet_info->classifier_binary + pfdet_info->classifierBinaryInfo.cls_base + (p_classifier_info[i].offset << 3));
		num_stages	= (class_stream[0] << 8) | class_stream[1];
		pointer_word	= (class_stream[2] << 24) | (class_stream[3] << 16) | (class_stream[4] << 8) | class_stream[5];
		left_stream_ptr	= (pointer_word) >> 19;
		top_stream_ptr	= (pointer_word & 0x0007ffff) >> 6;

		if (i == 0) {
			cls_cache_addr = 0;
		} else {
			cls_cache_addr += p_classifier_info[i - 1].sz;
		}
		left_stream_ptr	= cls_cache_addr + p_classifier_info[i].left_offset;
		top_stream_ptr	= cls_cache_addr + p_classifier_info[i].top_offset;
		pointer_word	= (left_stream_ptr << 19) | (top_stream_ptr << 6) | (pointer_word & 0x0000003f);
		class_stream[2]	= pointer_word >> 24;
		class_stream[3]	= (pointer_word & 0x00ff0000) >> 16;
		class_stream[4]	= (pointer_word & 0x0000ff00) >> 8;
		class_stream[5]	= pointer_word & 0x000000ff;

		pfdet_info->evaluation_id[i] = cls_cache_addr;
		if (pfdet_info->config.input_mode == FDET_MODE_STILL) {
			pfdet_info->evaluation_num[i] = num_stages << 16;
		} else {
			pfdet_info->evaluation_num[i] = num_stages;
		}
	}

	return 0;
}

void fdet_get_cmds_video_fs(void)
{
	int				si;
	int				width, height;
	int				row, col, rows, cols;
	unsigned int			center_x, center_y;
	unsigned int			*pfs_cmd_buf = (unsigned int *)pfdet_info->fs_cmd_buf[pfdet_info->current_fs_buf_id];
	int				words = 0;
	int				cmd_id = 0, x, y;
	int				rx, ry, rf, merge;
	int				si_start, num_scales;
	int				num_evals, ei_start, ei;
	int				cls_cache_addr;
	int				cls_len;
	unsigned int			cls_dram_addr;
	unsigned char			orientation_bitmasks[4];
	unsigned char			eval_id[4];
	struct fdet_classifier_info	*p_classifier_info;

	p_classifier_info = pfdet_info->classifierBinaryInfo.fs_cls_info;
	for (ei = 0; ei < 3; ei++) {
		eval_id[ei]			= ei;
		orientation_bitmasks[ei]	= p_classifier_info[ei].orientation_bitmasks;
	}

	cls_cache_addr		= 0;
	cls_len			= p_classifier_info[0].sz + p_classifier_info[1].sz + p_classifier_info[2].sz;
	cls_len			= cls_len >> 1;
	cls_dram_addr		= (unsigned int)(pfdet_info->classifier_binary) + pfdet_info->classifierBinaryInfo.cls_base + (p_classifier_info[0].offset << 3);
	pfs_cmd_buf[words++]	= ((cls_len - 1) << (2 + 13 + 2)) | (cls_cache_addr << 2) | (0);
	pfs_cmd_buf[words++]	= ambarella_virt_to_phys(cls_dram_addr);

	for (si = pfdet_info->num_sub_scales; si < pfdet_info->num_total_scales; si++) {
		width	= (pfdet_info->config.input_width  * pfdet_info->scale_factor[si] + (1 << 15)) >> 16;
		height	= (pfdet_info->config.input_height * pfdet_info->scale_factor[si] + (1 << 15)) >> 16;
		rows	= (height - FDET_TEMPLATE_SIZE / 2 + FDET_SCALED_PARTITION_SIZE - 1) / FDET_SCALED_PARTITION_SIZE;
		cols	= (width  - FDET_TEMPLATE_SIZE / 2 + FDET_SCALED_PARTITION_SIZE - 1) / FDET_SCALED_PARTITION_SIZE;

		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				center_x	= col * FDET_SCALED_PARTITION_SIZE + FDET_SCALED_PARTITION_SIZE / 2;
				center_y	= row * FDET_SCALED_PARTITION_SIZE + FDET_SCALED_PARTITION_SIZE / 2;
				x		= (center_x * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;
				y		= (center_y * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;

				rx		= (FDET_SCALED_PARTITION_SIZE + FDET_TEMPLATE_SIZE) / 2;
				ry		= (FDET_SCALED_PARTITION_SIZE + FDET_TEMPLATE_SIZE) / 2;
				rf		= 1;

				merge		= 0;
				ei_start	= 0;
				num_evals	= FDET_NUM_EVALS;

				cmd_id		+= 1;
				si_start	= si;
				num_scales	= 1;
				ei		= ei_start;

				pfs_cmd_buf[words++]	= (0x1 << 0) | (cmd_id << 2) | ((x & 0x3ff) << 12) | ((y & 0x3ff) << 22);
				pfs_cmd_buf[words++]	= ((num_evals - 1) << 0) | (merge << 2) | (rf << 3) | (si_start << 4) | (((num_scales - 1) & 0x1f) << 9) | ((rx & 0x1ff) << 14) | ((ry & 0x1ff) << 23);

				switch (num_evals) {
				case 1:
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei] << 0) | (eval_id[ei] << 8);
					break;

				case 2:
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei] << 0) | (eval_id[ei] << 8) | (orientation_bitmasks[ei + 1] << 16) | (eval_id[ei + 1] << 24);
					break;

				case 3:
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei] << 0) | (eval_id[ei] << 8) | (orientation_bitmasks[ei + 1] << 16) | (eval_id[ei + 1] << 24);
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei + 2] << 0) | (eval_id[ei + 2] << 8);
					break;

				default:
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei] << 0) | (eval_id[ei] << 8) | (orientation_bitmasks[ei + 1] << 16) | (eval_id[ei + 1] << 24);
					pfs_cmd_buf[words++] = (orientation_bitmasks[ei + 2] << 0) | (eval_id[ei + 2] << 8) | (orientation_bitmasks[ei + 3] << 16) | (eval_id[ei + 3] << 24);
					break;
				}
			}
		}
	}

	pfdet_info->current_fs_buf_sz	= words;
	pfdet_info->last_fs_cmd_id	= cmd_id;
}

void fdet_get_cmds_video_ts(void)
{
	int					i, cmd_id = 0;
	int					x, y ,radius_x, radius_y;
	int					rf, si_start, num_scales;
	int					num_evals;
	int					eval_num;
	int					words = 0;
	int					best_oi;
	int					merge = 0;
	int					size, face_size;
	unsigned int				*pts_cmd_buf;
	struct fdet_adjacent_update_info	*padj_info;

	pts_cmd_buf	= (unsigned int *)pfdet_info->ts_cmd_buf[pfdet_info->current_ts_buf_id];

	if (pfdet_info->config.policy & FDET_POLICY_DISABLE_TS) {
		pfdet_info->num_faces = 0;
	}

	for (i = 0; i < pfdet_info->num_faces; i++) {
		x		= pfdet_info->merged_faces[i].x;
		y		= pfdet_info->merged_faces[i].y;
		size		= pfdet_info->merged_faces[i].sz;
		cmd_id		+= 1;
		radius_x	= 10 + search_radius[pfdet_info->num_faces - 1];
		radius_y	= 10 + search_radius[pfdet_info->num_faces - 1];
		rf		= 1;

		for (si_start = pfdet_info->num_sub_scales; ; si_start++) {
			face_size = pfdet_info->scale_factor[si_start] * size >> 16;
			if (face_size < FDET_TEMPLATE_SIZE) {
				break;
			}
		}
		si_start	-= 2;
		num_scales	= 4;
		if (si_start < 0) {
			si_start = 0;
		}
		if (si_start >= pfdet_info->num_total_scales) {
			si_start = pfdet_info->num_total_scales - 1;
		}
		if (num_scales > pfdet_info->num_total_scales - si_start) {
			num_scales = pfdet_info->num_total_scales - si_start;
		}

		best_oi	= pfdet_info->merged_faces[i].best_oi;
		num_evals		= FDET_NUM_EVALS;
		eval_num		= 0;
		padj_info		= fdet_orientation_table[best_oi].adjacent_update_info;

		pts_cmd_buf[words++]	= (0x1 << 0) | (cmd_id << 2) |  ((x & 0x3ff) << 12) | ((y & 0x3ff) << 22);
		pts_cmd_buf[words++]	= ((num_evals - 1) << 0 ) | (merge << 2) | (rf << 3) | (si_start << 4) | (((num_scales - 1) & 0x1f) << 9) | ((radius_x & 0x1ff) << 14) | ((radius_y & 0x1ff) << 23);

		switch (num_evals) {
		case 1:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8);
			break;

		case 2:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			break;

		case 3:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			pts_cmd_buf[words++] = (padj_info[eval_num + 2].orientation_bitmask << 0) | ((padj_info[eval_num + 2].eval_id) << 8);
			break;

		default:
			pts_cmd_buf[words++] = (padj_info[eval_num].orientation_bitmask << 0) | ((padj_info[eval_num].eval_id) << 8) | (padj_info[eval_num + 1].orientation_bitmask << 16) | ((padj_info[eval_num + 1].eval_id) << 24);
			pts_cmd_buf[words++] = (padj_info[eval_num + 2].orientation_bitmask << 0) | ((padj_info[eval_num + 2].eval_id) << 8) | (padj_info[eval_num + 3].orientation_bitmask << 16) | ((padj_info[eval_num + 3].eval_id) << 24);
			break;
		}

	}

	pfdet_info->current_ts_buf_sz = words;
}

void fdet_get_cmds_still(void)
{
	unsigned int				*pts_cmd_buf = NULL;
	int					words = 0;
	int					cmd_id = 0, x, y;
	int					radius_x, radius_y, rf;
	int					merge, si_start, num_scales;
	int					cls_cache_addr;
	int					cls_len;
	unsigned int				cls_dram_addr;
	int					num_evals, ei_start, ei;
	unsigned char				orientation_bitmasks[4];
	unsigned char				eval_id[4];
	struct fdet_classifier_info		*p_classifier_info;

	pts_cmd_buf		= (unsigned int *)pfdet_info->ts_cmd_buf[pfdet_info->current_ts_buf_id];
	p_classifier_info	= pfdet_info->classifierBinaryInfo.fs_cls_info;

	for (ei = 0; ei <= 2; ei++) {
		eval_id[ei]			= ei;
		orientation_bitmasks[ei]	= p_classifier_info[ei].orientation_bitmasks;
	}

	cls_cache_addr		= 0;
	cls_len			= p_classifier_info[0].sz + p_classifier_info[1].sz + p_classifier_info[2].sz;
	cls_len			= cls_len >> 1;
	cls_dram_addr		= (unsigned int)(pfdet_info->classifier_binary) + pfdet_info->classifierBinaryInfo.cls_base + (p_classifier_info[0].offset << 3);
	pts_cmd_buf[words++]	= ((cls_len - 1) << (2 + 13 + 2)) | (cls_cache_addr << 2) | (0);
	pts_cmd_buf[words++]	= ambarella_virt_to_phys(cls_dram_addr);
	cmd_id			+= 1;
	x			= pfdet_info->config.input_width / 2;
	y			= pfdet_info->config.input_height / 2;
	radius_x		= x - 1;
	radius_y		= y - 1;
	rf			= 0;
	merge			= 0;
	si_start		= pfdet_info->num_sub_scales;
	num_scales		= pfdet_info->num_scales;
	ei_start		= 0;
	num_evals		= FDET_NUM_EVALS;
	ei			= ei_start;

	pts_cmd_buf[words++]	= (0x1 << 0) | (cmd_id << 2) |((x & 0x3ff) << 12) | ((y & 0x3ff) << 22);
	pts_cmd_buf[words++]	= ((num_evals - 1) << 0) | (merge << 2) |	(rf << 3) | (si_start << 4) | (((num_scales - 1) & 0x1f) << 9) | ((radius_x & 0x1ff) << 14) | ((radius_y & 0x1ff) << 23);
	pts_cmd_buf[words++]	= (orientation_bitmasks[ei] << 0) | (eval_id[ei] << 8) | (orientation_bitmasks[ei + 1] << 16) | (eval_id[ei + 1] << 24);
	pts_cmd_buf[words++]	= (orientation_bitmasks[ei + 2] << 0) | (eval_id[ei + 2] << 8);

	pfdet_info->current_ts_buf_sz = words;
}

int fdet_get_orientation_index(short eval_id, unsigned char orientation)
{
	if (eval_id == 0) {
		if (orientation == 0) {
			return 0;
		}

		return -1;
	}

	if (eval_id == 1) {
		if (orientation == 0) {
			return 1;
		}

		if (orientation == 1) {
			return 2;
		}

		return -1;
	}

	if (eval_id == 2) {
		if (orientation == 0) {
			return 3;
		}

		if (orientation == 1) {
			return 4;
		}

		if (orientation == 3) {
			return 5;
		}

		if (orientation == 5) {
			return 6;
		}

		return -1;
	}

	return -1;
}

int fdet_is_equivalent_face(int i, int j, int merged)
{
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face	*pm;
	int				distance;

	pu		= pfdet_info->unmerged_faces;
	pm		= pfdet_info->merged_faces;

	if (!merged) {
		distance	= pu[i].sz >> 2;

		if (	(pu[j].x  < pu[i].x + distance) &&
			(pu[j].x  > pu[i].x - distance) &&
			(pu[j].y  < pu[i].y + distance) &&
			(pu[j].y  > pu[i].y - distance) &&
			(pu[j].sz < (pu[i].sz * 5) >> 2) &&
			(pu[i].sz < (pu[j].sz * 5) >> 2) ) {
			return 1;
		} else {
			return 0;
		}
	} else {
		distance	= pm[i].sz >> 2;

		if (	(pm[j].x  < pm[i].x + distance) &&
			(pm[j].x  > pm[i].x - distance) &&
			(pm[j].y  < pm[i].y + distance) &&
			(pm[j].y  > pm[i].y - distance) &&
			(pm[j].sz < (pm[i].sz * 5) >> 2) &&
			(pm[i].sz < (pm[j].sz * 5) >> 2) ) {
			return 1;
		} else {
			return 0;
		}
	}
}

int fdet_merge_faces(int unmerged_faces)
{
	int				i, j;
	int				cluster, clusters;
	int				hs, ths;
	int				max_hitcount;
	int				si, max_scales;
	unsigned short			best_oi;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		*pm;

	pu = pfdet_info->unmerged_faces;
	pm = pfdet_info->merged_faces;

	clusters = 0;
	for (i = 0; i < unmerged_faces; i++) {

		if (pu[i].cluster < 0) {
			pu[i].cluster	= clusters;

			pm[clusters].x	= pu[i].x;
			pm[clusters].y	= pu[i].y;
			pm[clusters].num_si[pu[i].si]++;
			pm[clusters].hit_count[pu[i].oi]++;
			pm[clusters].hit_sum = 1;

			if (++clusters >= FDET_MAX_MERGED_FACES) {
				break;
			}
		}

		for (j = i + 1; j < unmerged_faces; j++) {
			if (pu[j].cluster < 0) {
				if (fdet_is_equivalent_face(j, i, 0)) {
					cluster		= pu[i].cluster;
					pu[j].cluster	= cluster;

					pm[cluster].x	+= pu[j].x;
					pm[cluster].y	+= pu[j].y;
					pm[cluster].num_si[pu[j].si]++;
					pm[cluster].hit_count[pu[j].oi]++;
					pm[cluster].hit_sum++;
				}
			}
		}
	}

	for (i = 0; i < clusters; i++) {
		hs		= pm[i].hit_sum;
		ths		= hs << 1;
		pm[i].x		= (2 * pm[i].x  + hs) / ths;
		pm[i].y		= (2 * pm[i].y  + hs) / ths;

		si		= 0;
		max_scales	= 0;
		for (j = pfdet_info->num_sub_scales; j < pfdet_info->num_total_scales; j++) {
			if (pm[i].num_si[j] > max_scales) {
				max_scales	= pm[i].num_si[j];
				si		= j;
			}
		}
		pm[i].sz	= (FDET_TEMPLATE_SIZE * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;
		pm[i].x		-= pfdet_info->recip_scale_factor[si] >> 18;
		pm[i].y		+= pfdet_info->recip_scale_factor[si] >> 18;

		max_hitcount	= 0;
		best_oi		= 0;
		for (j = 0; j < 7; j++) {
			if (pm[i].hit_count[j] > max_hitcount) {
				max_hitcount	= pm[i].hit_count[j];
				best_oi		= j;
			}
		}
		pm[i].best_oi		= best_oi;
		pm[i].best_hitcount	= max_hitcount;
		pm[i].type		= FDET_RESULT_TYPE_FS;
	}

	return clusters;
}

struct fdet_merged_face fdet_merge_ts_faces(int unmerged_faces)
{
	int				i, j, k;
	int				oi;
	int				x1, x2, y1, y2;
	int				xl, xr, yl, yh;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		merged_faces;

	pu = pfdet_info->ts_unmerged_faces;
	memset(&merged_faces, 0, sizeof(merged_faces));

	if (unmerged_faces <= 0) {
		return merged_faces;
	}

	xl = -10000;
	xr = +10000;
	yl = -10000;
	yh = +10000;

	for (i = 0, j = 0, k = 0; i < unmerged_faces; i++) {
		if (!k) {
			x1 = pu[i].x - pu[i].sz / 2;
			x2 = pu[i].x + pu[i].sz / 2;
			y1 = pu[i].y - pu[i].sz / 2;
			y2 = pu[i].y + pu[i].sz / 2;

			x1 = max(xl, x1);
			x2 = min(xr, x2);
			y1 = max(yl, y1);
			y2 = min(yh, y2);

			if (x2 >= x1 && y2 >= y1) {
				xl = x1;
				xr = x2;
				yl = y1;
				yh = y2;
				j++;
			} else {
				k = 1;
			}
		}

		merged_faces.sz	+= pu[i].sz;
		oi		=  pu[i].oi;
		merged_faces.hit_count[oi]++;
		merged_faces.hit_sum++;
	}


	merged_faces.x	= (xl + xr + 1) / 2;
	merged_faces.y	= (yl + yh + 1) / 2;
	merged_faces.sz	= (2 * merged_faces.sz + unmerged_faces) / (unmerged_faces << 1);

	for (i = 0; i < 7; i++) {
		if (merged_faces.hit_count[i] > merged_faces.best_hitcount) {
			merged_faces.best_hitcount = merged_faces.hit_count[i];
			merged_faces.best_oi = i;
		}
	}

	return merged_faces;
}

int fdet_is_valid_hitcount(unsigned int *hit_count, const int *pth)
{
	int				o;
	int				hitcount_45;

	for (o = 0; o < 7; o++) {
		if (hit_count[o] >=  pth[o]) {
			return 1;
		}
	}

	hitcount_45 = hit_count[3] + hit_count[5];
	if (hitcount_45 >= pth[3]) {
		return 1;
	}

	hitcount_45 = hit_count[4] + hit_count[6];
	if (hitcount_45 >= pth[4]) {
		return 1;
	}

	return 0;
}

int fdet_filter_merged_faces(int merged_faces, const int *pth)
{
	int				i, faces, valid;
	struct fdet_merged_face		*pm;

	pm	= pfdet_info->merged_faces;
	faces	= 0;

	for (i = 0; i < merged_faces; i++) {
		valid = fdet_is_valid_hitcount(pm[i].hit_count, pth);
		if (valid) {
			pm[faces] = pm[i];
			faces++;
		}
	}

	return faces;
}

int fdet_eliminate_overlapping_faces(int merged_faces)
{
	struct fdet_merged_face		*pm;
	int				dx, dy;
	int				distance, threshold;
	int				i, j;

	pm	= pfdet_info->merged_faces;

	for (i = 0; i < merged_faces; i++) {
		if (!pm[i].sz) {
			continue;
		}

		for (j = i + 1; j < merged_faces; j++) {
			if (!pm[j].sz) {
				continue;
			}

			dx		= abs((int)pm[i].x - (int)pm[j].x);
			dy		= abs((int)pm[i].y - (int)pm[j].y);
			threshold	= max(pm[i].sz, pm[j].sz) >> 1;

			if (dx >= dy) {
				if (dx > (dy << 2)) {
					distance = (dx * 66047) >> 16;
				} else if (dx > (dy << 1)) {
					distance = (dx * 69992) >> 16;
				} else if (3 * dx > (dy << 2)) {
					distance = (dx * 77280) >> 16;
				} else {
					distance = (dx * 87084) >> 16;
				}
			} else {
				if (dy > (dx << 2)) {
					distance = (dy * 66047) >> 16;
				} else if (dy > (dx << 1)) {
					distance = (dy * 69992) >> 16;
				} else if (3*dy > (dx << 2)) {
					distance = (dy * 77280) >> 16;
				} else {
					distance = (dy * 87084) >> 16;
				}
			}

			if (distance < threshold) {
				if (pm[j].best_hitcount <= pm[i].best_hitcount) {
					pm[j].sz = 0;
				} else {
					pm[i].sz = 0;
				}
			}
		}
	}

	for (i = 0, j = 0; i < merged_faces; i++) {
		if (pm[i].sz) {
			pm[j++] = pm[i];
		}
	}

	return j;
}

void fdet_print_faces(void)
{
	struct fdet_merged_face		*pm;
	int				i;

	pm = pfdet_info->merged_faces;

	for (i = 0; i < pfdet_info->num_faces; i++) {
		FDET_DEBUG("\tFace %2d: (%3d, %3d), Size: %3d\n", i, pm[i].x, pm[i].y, pm[i].sz);
	}
}

int fdet_get_result_still(void)
{
	unsigned int			*result, words;
	unsigned int			type, cmd_id, x, y, sz, eval_id, si, orientation, oi;
	unsigned int			i, unmerged_faces, merged_faces;
	struct fdet_unmerged_face	*pu;

	result	= (unsigned int *)pfdet_info->ts_result_buf[pfdet_info->current_ts_result_id ^ 1];
	invalidate_d_cache(result, FDET_TS_RESULT_BUF_SIZE);
	words	= FDET_TS_RESULT_NUM(amba_readl(FDET_RESULT_STATUS_REG));

	if (amba_readl(FDET_ERROR_STATUS_REG) & FDET_ERR_TS_RESULT_OVERFLOW
		|| words > (FDET_TS_RESULT_BUF_SIZE >> 2)) {
		printk("%s: Ts Result Overflow!\n", __func__);
		words = FDET_TS_RESULT_BUF_SIZE >> 2;
	}

	unmerged_faces = 0;
	memset(pfdet_info->unmerged_faces, 0, sizeof(pfdet_info->unmerged_faces));
	memset(pfdet_info->merged_faces, 0, sizeof(pfdet_info->merged_faces));
	pfdet_info->num_faces = 0;
	pu = pfdet_info->unmerged_faces;
	for (i = 0; i < words; i += 2) {
		type	= (result[i] & 0x00000003) >> 0;
		cmd_id	= (result[i] & 0x00000fff) >> 2;

		if (type == 0 && cmd_id) {
			x		= (result[i]     & 0x003ff000) >> 12;
			y		= (result[i]     & 0xffc00000) >> 22;
			eval_id		= (result[i + 1] & 0x0000001f) >> 0;
			si		= (result[i + 1] & 0x00001f00) >> 8;
			orientation	= (result[i + 1] & 0x0000e000) >> 13;
			sz		= (FDET_TEMPLATE_SIZE * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;
			oi		= fdet_get_orientation_index(eval_id, orientation);

 			if (oi >= 0) {
				if (unmerged_faces >= FDET_MAX_UNMERGED_FACES) {
					unmerged_faces = FDET_MAX_UNMERGED_FACES - 1;
				}

				pu[unmerged_faces].x		= x;
				pu[unmerged_faces].y		= y;
				pu[unmerged_faces].si		= si;
				pu[unmerged_faces].sz		= sz;
				pu[unmerged_faces].oi		= oi;
				pu[unmerged_faces].cluster	= -1;

				unmerged_faces++;
			}
		}
	}
	FDET_DEBUG("Unmerged Faces: %d\n", unmerged_faces);

	merged_faces = fdet_merge_faces(unmerged_faces);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces before Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_filter_merged_faces(merged_faces, hitcount_threshold_still);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces after  Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_eliminate_overlapping_faces(merged_faces);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces: %d\n", merged_faces);

	return merged_faces;
}

int fdet_get_result_video_fs(void)
{
	unsigned int			*result, words, pwords;
	unsigned int			cmd_id, type, x, y, sz, eval_id, si, orientation, oi;
	unsigned int			i, unmerged_faces, merged_faces = 0;
	struct fdet_unmerged_face	*pu;
	int				end = -1;
	static unsigned int		offset = 0;
	unsigned long			tick;

	words	= FDET_FS_RESULT_NUM(amba_readl(FDET_RESULT_STATUS_REG));
	if (amba_readl(FDET_ERROR_STATUS_REG) & FDET_ERR_FS_RESULT_OVERFLOW
		|| words > (FDET_FS_RESULT_BUF_SIZE >> 2)) {
		printk("%s: Fs Result Overflow!\n", __func__);
		words = FDET_FS_RESULT_BUF_SIZE >> 2;
	}
	unmerged_faces = offset;
	memset(pfdet_info->merged_faces, 0, sizeof(pfdet_info->merged_faces));
	pfdet_info->num_faces = 0;
	pu = pfdet_info->unmerged_faces;

	result	= (unsigned int *)pfdet_info->fs_result_buf[pfdet_info->current_fs_result_id];
	pwords	= FDET_FS_RESULT_BUF_SIZE >> 2;
	invalidate_d_cache(pfdet_info->fs_result_buf[pfdet_info->current_fs_result_id], FDET_FS_RESULT_BUF_SIZE);
	for (i = 0; i < pwords; i += 2) {
		type	= (result[i] & 0x00000003) >> 0;
		cmd_id	= (result[i] & 0x00000fff) >> 2;

		if (type == 0 && cmd_id) {
			x		= (result[i]     & 0x003ff000) >> 12;
			y		= (result[i]     & 0xffc00000) >> 22;
			eval_id		= (result[i + 1] & 0x0000001f) >> 0;
			si		= (result[i + 1] & 0x00001f00) >> 8;
			orientation	= (result[i + 1] & 0x0000e000) >> 13;
			sz		= (FDET_TEMPLATE_SIZE * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;
			oi		= fdet_get_orientation_index(eval_id, orientation);

 			if (oi >= 0) {
				if (unmerged_faces >= FDET_MAX_UNMERGED_FACES) {
					unmerged_faces = FDET_MAX_UNMERGED_FACES - 1;
				}

				pu[unmerged_faces].x		= x;
				pu[unmerged_faces].y		= y;
				pu[unmerged_faces].si		= si;
				pu[unmerged_faces].sz		= sz;
				pu[unmerged_faces].oi		= oi;
				pu[unmerged_faces].cluster	= -1;
				unmerged_faces++;
			}
		}

		if (type == 2) {
			if (cmd_id == pfdet_info->last_fs_cmd_id) {
				end = unmerged_faces;
			}
		}
	}
	memset((void *)result, 0, pwords << 2);
	clean_d_cache(pfdet_info->fs_result_buf[pfdet_info->current_fs_result_id], FDET_FS_RESULT_BUF_SIZE);

	result	= (unsigned int *)pfdet_info->fs_result_buf[pfdet_info->current_fs_result_id ^ 1];
	invalidate_d_cache(result, FDET_FS_RESULT_BUF_SIZE);
	for (i = 0; i < words; i += 2) {
		type	= (result[i] & 0x00000003) >> 0;
		cmd_id	= (result[i] & 0x00000fff) >> 2;

		if (type == 0 && cmd_id) {
			x		= (result[i]     & 0x003ff000) >> 12;
			y		= (result[i]     & 0xffc00000) >> 22;
			eval_id		= (result[i + 1] & 0x0000001f) >> 0;
			si		= (result[i + 1] & 0x00001f00) >> 8;
			orientation	= (result[i + 1] & 0x0000e000) >> 13;
			sz		= (FDET_TEMPLATE_SIZE * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;
			oi		= fdet_get_orientation_index(eval_id, orientation);

 			if (oi >= 0) {
				if (unmerged_faces >= FDET_MAX_UNMERGED_FACES) {
					unmerged_faces = FDET_MAX_UNMERGED_FACES - 1;
				}

				pu[unmerged_faces].x		= x;
				pu[unmerged_faces].y		= y;
				pu[unmerged_faces].si		= si;
				pu[unmerged_faces].sz		= sz;
				pu[unmerged_faces].oi		= oi;
				pu[unmerged_faces].cluster	= -1;
				unmerged_faces++;
			}
		}

		if (type == 2) {
			if (cmd_id == pfdet_info->last_fs_cmd_id) {
				end = unmerged_faces;
			}
		}
	}
	memset((void *)result, 0, words << 2);
	clean_d_cache((void *)result, words << 2);

	if (end >= 0) {
		pfdet_info->fs_found_faces[0] = pfdet_info->fs_found_faces[1];
		pfdet_info->fs_found_faces[1] = 0;
	}

	if (end >= 0 && pfdet_info->config.policy & FDET_POLICY_MEASURE_TIME) {
		tick			= jiffies;
		printk("Fs used time: %lu ms\n", 1000 * (tick - pfdet_info->fs_tick) / HZ);
		pfdet_info->fs_tick	= tick;
	}

	FDET_DEBUG("Unmerged Faces: %d\n", unmerged_faces);

	if ((pfdet_info->config.policy & FDET_POLICY_WAIT_FS_COMPLETE) == 0) {
		end = unmerged_faces;
	}

	if (end < 0) {
		offset = unmerged_faces;
		goto fdet_get_result_video_fs_exit;
	}

	merged_faces = fdet_merge_faces(end);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces before Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_filter_merged_faces(merged_faces, hitcount_threshold_video);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces after  Hitcount Check: %d\n", merged_faces);

	merged_faces = fdet_eliminate_overlapping_faces(merged_faces);
	pfdet_info->num_faces = merged_faces;
	FDET_DEBUG("Merged Faces: %d\n", merged_faces);

	if (merged_faces) {
		pfdet_info->fs_found_faces[1]++;
	}

	offset = unmerged_faces - end;
	memcpy(pu, &pu[end], offset * sizeof(struct fdet_unmerged_face));

fdet_get_result_video_fs_exit:
	return merged_faces;
}

int fdet_get_result_video_ts(void)
{
	int				words, i, hit;
	unsigned int			wi, type;
	unsigned int			x = 0, y = 0;
	unsigned int			cmd_id, size = 0;
	unsigned int			eval_id, si, orientation_mode;
	int				oi, faces = pfdet_info->num_faces;
	unsigned int			*result;
	unsigned int			result_status, error_status;
	int				unmerged_faces = 0;
	struct fdet_unmerged_face	*pu;
	struct fdet_merged_face		*pm, *pl;

	result_status	= amba_readl(FDET_RESULT_STATUS_REG);
	error_status	= amba_readl(FDET_ERROR_STATUS_REG);
	words		= FDET_TS_RESULT_NUM(result_status);
	pu		= pfdet_info->ts_unmerged_faces;
	pm		= pfdet_info->merged_faces;
	pl		= pfdet_info->latest_faces;

	FDET_DEBUG("%s: result status: 0x%08x, error status: 0x%08x\n", __func__, result_status, error_status);

	if (error_status & FDET_ERR_TS_TOO_LONG || error_status & FDET_ERR_TS_MERGE_OVERFLOW) {
		goto fdet_get_result_video_ts_exit;
	}

	if (error_status & FDET_ERR_TS_RESULT_OVERFLOW || words > (FDET_TS_RESULT_BUF_SIZE >> 2)) {
		words = FDET_TS_RESULT_BUF_SIZE >> 2;
	}

	result = (unsigned int *)pfdet_info->ts_result_buf[pfdet_info->current_ts_result_id ^ 1];
	invalidate_d_cache(result, FDET_TS_RESULT_BUF_SIZE);
	for (wi = 0; wi < words; wi += 2) {
		type	= (result[wi] & 0x00000003) >> 0;
		cmd_id	= (result[wi] & 0x00000fff) >> 2;

		if (!cmd_id) {
			continue;
		}

		if (type == 0) {
			x		= (result[wi]     & 0x003ff000) >> 12;
			y		= (result[wi]     & 0xffc00000) >> 22;
			eval_id		= (result[wi + 1] & 0x0000001f) >> 0;
			si		= (result[wi + 1] & 0x00001f00) >> 8;
			orientation_mode= (result[wi + 1] & 0x0000e000) >> 13;
			size		= (FDET_TEMPLATE_SIZE * pfdet_info->recip_scale_factor[si] + (1 << 15)) >> 16;

			if (unmerged_faces > FDET_MAX_UNMERGED_FACES - 1) {
				unmerged_faces = FDET_MAX_UNMERGED_FACES - 1;
			}

			oi = fdet_get_orientation_index(eval_id, orientation_mode);
			if (oi >= 0) {
				pu[unmerged_faces].x		= x;
				pu[unmerged_faces].y		= y;
				pu[unmerged_faces].sz		= size;
				pu[unmerged_faces].cluster	= -1;
				pu[unmerged_faces].oi		= oi;
				unmerged_faces++;
			}
		}

		if (type == 2) {
			struct fdet_merged_face merged_faces;

			merged_faces = fdet_merge_ts_faces(unmerged_faces);

			if (merged_faces.hit_sum >= 15 && pfdet_info->fs_found_faces[0]) {
				if (abs(merged_faces.x - pl[cmd_id - 1].x) < merged_faces.sz / FDET_TEMPLATE_SIZE &&
					abs(merged_faces.y - pl[cmd_id - 1].y) < merged_faces.sz / FDET_TEMPLATE_SIZE &&
					5 * merged_faces.sz > 4 * pl[cmd_id - 1].sz &&
					4 * merged_faces.sz < 5 * pl[cmd_id - 1].sz) {
					pm[faces].x		= pl[cmd_id - 1].x;
					pm[faces].y		= pl[cmd_id - 1].y;
					pm[faces].sz		= pl[cmd_id - 1].sz;
					pm[faces].best_oi	= merged_faces.best_oi;
				} else {
					pm[faces]		= merged_faces;
				}

				hit = 0;
				for (i = 0; i < pfdet_info->num_faces; i++) {
					if (fdet_is_equivalent_face(i, faces, 1)) {
						hit = 1;
						break;
					}
				}

				if (!hit) {
					pm[faces].type		= FDET_RESULT_TYPE_TS;
					faces++;
				}
			}

			unmerged_faces = 0;
			memset(pu, 0, sizeof(pfdet_info->unmerged_faces));
		}
	}

	if (faces) {
		faces = fdet_eliminate_overlapping_faces(faces);
	}

	pfdet_info->num_faces = faces;

fdet_get_result_video_ts_exit:
	return faces;
}

static int fdet_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int fdet_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int fdet_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int			ret;

	switch (pfdet_info->mmap_type) {
	case FDET_MMAP_TYPE_ORIG_BUFFER:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pfdet_info->orig_target_buf) >> PAGE_SHIFT;
		pfdet_info->orig_len = vma->vm_end - vma->vm_start;
		FDET_DEBUG("%s: Orig Buffer Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;

	case FDET_MMAP_TYPE_TMP_BUFFER:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pfdet_info->tmp_target_buf) >> PAGE_SHIFT;
		FDET_DEBUG("%s: Tmp Buffer Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;

	default:
		vma->vm_pgoff = ambarella_virt_to_phys((u32)pfdet_info->classifier_binary) >> PAGE_SHIFT;
		pfdet_info->cls_bin_len = vma->vm_end - vma->vm_start;
		FDET_DEBUG("%s: Classifier Binary Address: 0x%08x\n", __func__, (unsigned int)(vma->vm_pgoff << PAGE_SHIFT));
		break;
	}

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);

	return ret;
}

void fdet_config(void)
{
	char			*pfs_result_buf;
	char			*pts_cmd_buf, *pts_result_buf;
	char			*ptmp_target_buf;
	unsigned int		*p2;
	u32			p[6];
	int			i;

	FDET_DEBUG("%s: \n", __func__);

	pfs_result_buf	= pfdet_info->fs_result_buf[pfdet_info->current_fs_result_id];
	pts_cmd_buf	= pfdet_info->ts_cmd_buf[pfdet_info->current_ts_buf_id];
	pts_result_buf	= pfdet_info->ts_result_buf[pfdet_info->current_ts_result_id];
	ptmp_target_buf	= pfdet_info->tmp_target_buf + (FDET_TMP_TARGET_BUF_SIZE / 2) * pfdet_info->current_tmp_target_id;
	p2		= (unsigned int *)pts_cmd_buf;

	p[1]		= ambarella_virt_to_phys((u32)pfs_result_buf);
	p[2]		= ambarella_virt_to_phys((u32)pts_cmd_buf);
	p[3]		= ambarella_virt_to_phys((u32)pts_result_buf);
	p[4]		= ambarella_virt_to_phys((u32)pfdet_info->orig_target_buf);
	p[5]		= ambarella_virt_to_phys((u32)ptmp_target_buf);

	FDET_DEBUG("\tTracking Search Command Buffer Size: %d\n", pfdet_info->current_ts_buf_sz);
	for (i = 0; i < pfdet_info->current_ts_buf_sz; i++) {
		FDET_DEBUG("\t\tTs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	clean_d_cache(pts_cmd_buf, pfdet_info->current_ts_buf_sz << 2);

	amba_writel(FDET_FS_RESULT_BUF_PTR_REG, p[1]);
	amba_writel(FDET_FS_RESULT_BUF_SIZE_REG, (FDET_FS_RESULT_BUF_SIZE >> 2) - 1);
	FDET_DEBUG("\t fs result buffer pointer:   0x%08x\n", p[1]);

	amba_writel(FDET_TS_CMD_LIST_PTR_REG, p[2]);
	if (pfdet_info->current_ts_buf_sz) {
		amba_writel(FDET_TS_CMD_LIST_SIZE_REG, pfdet_info->current_ts_buf_sz - 1);
	} else {
		amba_writel(FDET_TS_CMD_LIST_SIZE_REG, 0);
	}
	FDET_DEBUG("\t ts cmd list pointer:        0x%08x\n", p[2]);

	memset(pts_result_buf, 0, FDET_TS_RESULT_BUF_SIZE);
	clean_d_cache(pts_result_buf, FDET_TS_RESULT_BUF_SIZE);
	amba_writel(FDET_TS_RESULT_BUF_PTR_REG, p[3]);
	amba_writel(FDET_TS_RESULT_BUF_SIZE_REG, (FDET_TS_RESULT_BUF_SIZE >> 2) - 1);
	FDET_DEBUG("\t ts result buffer pointer:   0x%08x\n", p[3]);

	if (pfdet_info->config.input_source) {
		amba_writel(FDET_ORIG_TARGET_PTR_REG, DSP_DRAM_START + pfdet_info->config.input_offset);
		amba_writel(FDET_ORIG_TARGET_PITCH_REG, pfdet_info->config.input_pitch);
		FDET_DEBUG("\t orig buffer pointer:        0x%08x\n", DSP_DRAM_START + pfdet_info->config.input_offset);
	} else {
		amba_writel(FDET_ORIG_TARGET_PTR_REG, p[4]);
		amba_writel(FDET_ORIG_TARGET_PITCH_REG, pfdet_info->config.input_pitch);
		FDET_DEBUG("\t orig buffer pointer:        0x%08x\n", p[4]);
		clean_d_cache(pfdet_info->orig_target_buf, pfdet_info->orig_len);
	}

	memset(ptmp_target_buf, 0, FDET_TMP_TARGET_BUF_SIZE / 2);
	clean_d_cache(ptmp_target_buf, FDET_TMP_TARGET_BUF_SIZE / 2);
	amba_writel(FDET_TMP_TARGET_PTR_REG, p[5]);
	FDET_DEBUG("\t tmp buffer pointer:         0x%08x\n", p[5]);

	pfdet_info->current_fs_result_id	^= 1;
	pfdet_info->current_ts_buf_id		^= 1;
	pfdet_info->current_ts_result_id	^= 1;
	pfdet_info->current_tmp_target_id	^= 1;
	pfdet_info->current_ts_buf_sz		 = 0;

	amba_writel(FDET_CONFIG_DONE_REG, FDET_CONFIG_DONE);
}

static int fdet_start_video(unsigned long arg)
{
	char			*pfs_cmd_buf;
	unsigned int		*p2;
	u32			p;
	int			i, ret;

	FDET_DEBUG("%s: \n", __func__);

	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_INPUT_WIDTH_REG, pfdet_info->config.input_width - 1);
	amba_writel(FDET_INPUT_HEIGHT_REG, pfdet_info->config.input_height - 1);
	FDET_DEBUG("\t input width: %d, input height: %d\n",
		pfdet_info->config.input_width, pfdet_info->config.input_height);
	amba_writel(FDET_DEADLINE_REG, 1);
	amba_writel(FDET_SKIP_FIRST_INT_REG, FDET_SKIP_FIRST_INTERRUPT);

	/* Scale Factors */
	ret = fdet_get_sf(pfdet_info->config.input_width, pfdet_info->config.input_height);
	if (ret <= 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Scales: %d, Sub Face Scales: %d\n", pfdet_info->num_scales, pfdet_info->num_sub_scales);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_SCALE_FACTOR_REG(i), pfdet_info->scale_factor_regs[i]);
		FDET_DEBUG("\t\tScale Factor[%02d]: 0x%08x\n", i, pfdet_info->scale_factor_regs[i]);
	}

	/* Evaluation IDs */
	ret = fdet_get_evaluation_ids();
	if (ret < 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Fs Classifiers: %d, Face Ts Classifiers: %d\n",
		pfdet_info->classifierBinaryInfo.num_fs_classifiers,
		pfdet_info->classifierBinaryInfo.num_ts_classifiers);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_ID_REG(i), pfdet_info->evaluation_id[i]);
		FDET_DEBUG("\t\tEvaluation  ID[%02d]: 0x%08x\n", i, pfdet_info->evaluation_id[i]);
	}
#ifdef FDET_HAVE_EVAL_NUM_REGS
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_NUM_REG(i), pfdet_info->evaluation_num[i]);
		FDET_DEBUG("\t\tEvaluation NUM[%02d]: 0x%08x\n", i, pfdet_info->evaluation_num[i]);
	}
#endif

	/* Full Search Command Buffer */
	pfs_cmd_buf	= pfdet_info->fs_cmd_buf[pfdet_info->current_fs_buf_id];
	p2		= (unsigned int *)pfs_cmd_buf;
	p		= ambarella_virt_to_phys((u32)pfs_cmd_buf);
	fdet_get_cmds_video_fs();
	pfdet_info->current_fs_buf_id	^= 1;
	FDET_DEBUG("\tFull Search Command Buffer Size: %d\n", pfdet_info->current_fs_buf_sz);
	for (i = 0; i < 32; i++) {
		FDET_DEBUG("\t\tFs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	for (i = pfdet_info->current_fs_buf_sz - 32; i < pfdet_info->current_fs_buf_sz; i++) {
		FDET_DEBUG("\t\tFs Command Word[%02d]: 0x%08x\n", i, p2[i]);
	}
	clean_d_cache(pfs_cmd_buf, pfdet_info->current_fs_buf_sz << 2);
	clean_d_cache(pfdet_info->classifier_binary, pfdet_info->cls_bin_len);

	amba_writel(FDET_FS_CMD_LIST_PTR_REG, p);
	amba_writel(FDET_FS_CMD_LIST_SIZE_REG, pfdet_info->current_fs_buf_sz - 1);
	FDET_DEBUG("\t fs cmd list pointer:        0x%08x\n", p);

	pfdet_info->current_ts_buf_sz = 0;
	fdet_config();

	pfdet_info->vm_state = FDET_VM_STATE_IDLE;
	init_completion(&pfdet_info->result_completion);
	pfdet_info->fs_tick = jiffies;

	amba_writel(FDET_GO_REG, FDET_START);

	return 0;
}

static int fdet_start_still(unsigned long arg)
{
	char			*pfs_cmd_buf;
	u32			p;
	int			i, ret;

	FDET_DEBUG("%s: \n", __func__);

	amba_writel(FDET_BASE_ADDRESS_REG, FDET_CONFIG_BASE_NORMAL);
	amba_writel(FDET_INPUT_WIDTH_REG, pfdet_info->config.input_width - 1);
	amba_writel(FDET_INPUT_HEIGHT_REG, pfdet_info->config.input_height - 1);
	FDET_DEBUG("\t input width: %d, input height: %d\n",
		pfdet_info->config.input_width, pfdet_info->config.input_height);
	amba_writel(FDET_DEADLINE_REG, 0);
	amba_writel(FDET_SKIP_FIRST_INT_REG, FDET_SKIP_FIRST_INTERRUPT);

	/* Scale Factors */
	ret = fdet_get_sf(pfdet_info->config.input_width, pfdet_info->config.input_height);
	if (ret <= 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Scales: %d, Sub Face Scales: %d\n", pfdet_info->num_scales, pfdet_info->num_sub_scales);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_SCALE_FACTOR_REG(i), pfdet_info->scale_factor_regs[i]);
		FDET_DEBUG("\t\tScale Factor[%02d]: 0x%08x\n", i, pfdet_info->scale_factor_regs[i]);
	}

	/* Evaluation IDs */
	ret = fdet_get_evaluation_ids();
	if (ret < 0) {
		return -EINVAL;
	}
	FDET_DEBUG("\tFace Fs Classifiers: %d, Face Ts Classifiers: %d\n",
		pfdet_info->classifierBinaryInfo.num_fs_classifiers,
		pfdet_info->classifierBinaryInfo.num_ts_classifiers);
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_ID_REG(i), pfdet_info->evaluation_id[i]);
		FDET_DEBUG("\t\tEvaluation  ID[%02d]: 0x%08x\n", i, pfdet_info->evaluation_id[i]);
	}
#ifdef FDET_HAVE_EVAL_NUM_REGS
	for (i = 0; i < 32; i++) {
		amba_writel(FDET_EVALUATION_NUM_REG(i), pfdet_info->evaluation_num[i]);
		FDET_DEBUG("\t\tEvaluation NUM[%02d]: 0x%08x\n", i, pfdet_info->evaluation_num[i]);
	}
#endif

	/* Tracking Search Command Buffer */
	fdet_get_cmds_still();
	clean_d_cache(pfdet_info->classifier_binary, pfdet_info->cls_bin_len);

	pfs_cmd_buf	= pfdet_info->fs_cmd_buf[pfdet_info->current_fs_buf_id];
	p		= ambarella_virt_to_phys((u32)pfs_cmd_buf);
	pfdet_info->current_fs_buf_id	^= 1;
	amba_writel(FDET_FS_CMD_LIST_PTR_REG, p);
	amba_writel(FDET_FS_CMD_LIST_SIZE_REG, 0);
	FDET_DEBUG("\t fs cmd list pointer:        0x%08x\n", p);

	init_completion(&pfdet_info->result_completion);
	fdet_config();
	amba_writel(FDET_GO_REG, FDET_START);

	return 0;
}

static int fdet_stop(unsigned long arg)
{
	amba_writel(FDET_GO_REG, FDET_STOP);
	complete_all(&pfdet_info->result_completion);

	return 0;
}

static int fdet_get_result(unsigned long arg)
{
	struct fdet_merged_face		*pm;
	int				i, ret;
	struct fdet_face		faces[FDET_MAX_FACES];

	wait_for_completion_interruptible(&pfdet_info->result_completion);

	pm = pfdet_info->merged_faces;

	for (i = 0; i < pfdet_info->num_faces; i++) {
		faces[i].x	= pm[i].x;
		faces[i].y	= pm[i].y;
		faces[i].size	= pm[i].sz;
		faces[i].type	= pm[i].type;
	}

	ret = copy_to_user((void *)arg, faces, i * sizeof(struct fdet_face));
	if (ret < 0) {
		i = -EINVAL;
	}

	return i;
}

static int fdet_set_mmap_type(unsigned long arg)
{
	switch (arg) {
	case FDET_MMAP_TYPE_ORIG_BUFFER:
	case FDET_MMAP_TYPE_TMP_BUFFER:
	case FDET_MMAP_TYPE_CLASSIFIER_BINARY:
		pfdet_info->mmap_type = arg;
		return 0;

	default:
		printk("%s: Invalid mmap type!\n", __func__);
		return -EINVAL;
	}
}

static int fdet_set_configuration(unsigned long arg)
{
	int				ret = 0;
	struct fdet_configuration	cfg;

	ret = copy_from_user(&cfg, (void *)arg, sizeof(cfg));
	if (ret < 0) {
		printk("%s: Error occurred when copying argument from user space!\n", __func__);
		goto fdet_set_configuration_exit;
	}

	if (cfg.input_width > FDET_MAX_INPUT_WIDTH || cfg.input_height > FDET_MAX_INPUT_HEIGHT) {
		printk("%s: Invalid input width or height!\n", __func__);
		ret = -EINVAL;
		goto fdet_set_configuration_exit;
	}

	if (cfg.input_mode != FDET_MODE_VIDEO && cfg.input_mode != FDET_MODE_STILL) {
		printk("%s: Invalid input mode!\n", __func__);
		ret = -EINVAL;
		goto fdet_set_configuration_exit;
	}

	memcpy(&pfdet_info->config, &cfg, sizeof(cfg));

fdet_set_configuration_exit:
	return ret;
}

static int fdet_track_face(unsigned long arg)
{
	if (pfdet_info->vm_state == FDET_VM_STATE_IDLE) {
		return -EINVAL;
	} else {
		pfdet_info->vm_state = FDET_VM_STATE_RUNNING;
	}

	pfdet_info->config.input_offset = arg;
	fdet_get_cmds_video_ts();
	pfdet_info->ts_tick = jiffies;
	fdet_config();

	return 0;
}

static long fdet_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int		ret = -ENOIOCTLCMD;

	switch (cmd) {
	case FDET_IOCTL_START:
		if (pfdet_info->config.input_mode == FDET_MODE_VIDEO) {
			ret = fdet_start_video(arg);
		} else {
			ret = fdet_start_still(arg);
		}
		break;

	case FDET_IOCTL_STOP:
		ret = fdet_stop(arg);
		break;

	case FDET_IOCTL_GET_RESULT:
		ret = fdet_get_result(arg);
		break;

	case FDET_IOCTL_SET_MMAP_TYPE:
		ret = fdet_set_mmap_type(arg);
		break;

	case FDET_IOCTL_SET_CONFIGURATION:
		ret = fdet_set_configuration(arg);
		break;

	case FDET_IOCTL_TRACK_FACE:
		ret = fdet_track_face(arg);
		break;

	default:
		break;
	}

	return ret;
}

static struct file_operations fdet_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl		= fdet_ioctl,
	.mmap			= fdet_mmap,
	.open			= fdet_open,
	.release		= fdet_release
};

/* ========================================================================== */
static int __init amba_fdet_init(void)
{
	int		ret = 0;
//	u32		val;
	dev_t		dev_id;

	pfdet_info = kzalloc(sizeof(*pfdet_info), GFP_KERNEL);
	if (!pfdet_info) {
		printk("%s: Fail to allocate fdet info!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	} else {
		pfdet_info->mmap_type = FDET_MMAP_TYPE_CLASSIFIER_BINARY;
	}

	pfdet_info->orig_target_buf = kzalloc(FDET_ORIG_TARGET_BUF_SIZE, GFP_KERNEL);
	if (!pfdet_info->orig_target_buf) {
		printk("%s: Fail to allocate orig buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}

	pfdet_info->tmp_target_buf = kzalloc(FDET_TMP_TARGET_BUF_SIZE, GFP_KERNEL);
	if (!pfdet_info->tmp_target_buf) {
		printk("%s: Fail to allocate tmp buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}

	pfdet_info->classifier_binary = kzalloc(FDET_CLASSIFIER_BINARY_SIZE, GFP_KERNEL);
	if (!pfdet_info->classifier_binary) {
		printk("%s: Fail to allocate classifier binary buffer!\n", __func__);
		ret = -ENOMEM;
		goto amba_fdet_init_exit;
	}


	pfdet_info->irq = FDET_IRQ;
	ret = request_irq(pfdet_info->irq, amba_fdet_isr, FDET_IRQF, "fdet", pfdet_info);

	if (ret) {
		printk("%s: Fail to request fdet irq!\n", __func__);
		pfdet_info->irq = 0;
		goto amba_fdet_init_exit;
	}

	/*val	= amba_readl(RCT_REG(0x54));
	val	&= 0x00ffffff;
	val	|= 0x70000000;
	amba_writel(RCT_REG(0x54), val | 0x1);
	amba_writel(RCT_REG(0x54), val);
	amba_writel(RCT_REG(0x234), 0x01);*/

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
	printk("%s: Fdet Freq: %d MHz.\n", __func__, amb_get_fdet_clock_frequency(HAL_BASE_VP) / 1000000);
#else
#error "Not supported!"
#endif

	ret = amba_enable_fdet();
	if (ret) {
		goto amba_fdet_init_exit;
	}

	dev_id = MKDEV(FDET_MAJOR, FDET_MINOR);
	ret = register_chrdev_region(dev_id, 1, FDET_NAME);
	if (ret) {
		goto amba_fdet_init_exit;
	}

	cdev_init(&pfdet_info->char_dev, &fdet_fops);
	pfdet_info->char_dev.owner = THIS_MODULE;
	ret = cdev_add(&pfdet_info->char_dev, dev_id, 1);
	if (ret) {
		unregister_chrdev_region(dev_id, 1);
		goto amba_fdet_init_exit;
	}

	pfdet_info->timer.function	= fdet_timer;
	pfdet_info->timer.data		= 0;
	init_timer(&pfdet_info->timer);

	init_completion(&pfdet_info->result_completion);

amba_fdet_init_exit:
 	if (ret && pfdet_info) {
		if (pfdet_info->irq) {
			free_irq(pfdet_info->irq, pfdet_info);
		}
		if (pfdet_info->classifier_binary) {
			kfree(pfdet_info->classifier_binary);
			pfdet_info->classifier_binary = NULL;
		}
		if (pfdet_info->tmp_target_buf) {
			kfree(pfdet_info->tmp_target_buf);
			pfdet_info->tmp_target_buf = NULL;
		}
		if (pfdet_info->orig_target_buf) {
			kfree(pfdet_info->orig_target_buf);
			pfdet_info->orig_target_buf = NULL;
		}
		kfree(pfdet_info);
		pfdet_info = NULL;
	}
	return ret;
}

static void __exit amba_fdet_exit(void)
{
	dev_t		dev_id;

	amba_disable_fdet();
	if (pfdet_info) {
		if (pfdet_info->irq) {
			free_irq(pfdet_info->irq, pfdet_info);
		}

		cdev_del(&pfdet_info->char_dev);

		dev_id = MKDEV(FDET_MAJOR, FDET_MINOR);
		unregister_chrdev_region(dev_id, 1);

		if (pfdet_info->classifier_binary) {
			kfree(pfdet_info->classifier_binary);
			pfdet_info->classifier_binary = NULL;
		}
		if (pfdet_info->tmp_target_buf) {
			kfree(pfdet_info->tmp_target_buf);
			pfdet_info->tmp_target_buf = NULL;
		}
		if (pfdet_info->orig_target_buf) {
			kfree(pfdet_info->orig_target_buf);
			pfdet_info->orig_target_buf = NULL;
		}

		kfree(pfdet_info);
		pfdet_info = NULL;
	}
}

module_init(amba_fdet_init);
module_exit(amba_fdet_exit);

MODULE_DESCRIPTION("Ambarella A7 / S2 Fdet driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

