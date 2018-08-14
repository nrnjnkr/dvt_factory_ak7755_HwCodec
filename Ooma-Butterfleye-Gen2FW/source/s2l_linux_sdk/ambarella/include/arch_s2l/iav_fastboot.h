/*
 * iav_fastboot.h
 *
 * History:
 *	2016/03/9 - [Xu Liang] Created file
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

#ifndef __IAV_SMART_3A_H__
#define __IAV_SMART_3A_H__

/* defined for mode 4 support in fastboot mode */
/* Note:
 * 1. SIZE_IDSP_CFG_ADV_ISO corresponds to the total size of the idsp config
 *    binaries that 3A middleware dumped under advanced ISO mode, except liso_cfg.bin
 * 2. SIZE_LISO_CFG_ADV_ISO corresponds to the size of a12_low_iso_param_t structure
 * 3. COLOR_UPDATE_FLAG_OFFSET represents the relative offset of
 *    a12_low_iso_param_t.color.update_all to the beginning of the structure
 *    a12_low_iso_param_t
 * 4. user can store user specified data beginning from FASTBOOT_USER_DATA_OFFSET
 *    but size must NOT exceed 3 KB. see FBDATA layout in amba_arch_mem.h
 */

#define PRELOAD_AWB_FILE		"/tmp/idsp_awb"
#define PRELOAD_AE_FILE		"/tmp/idsp_ae"
#define IDSP_LISO_DUMP_DIR		"/tmp"
#define IDSP_LISO_DUMP_FILE		"liso_cfg.bin"
#define PRELOAD_TONE_CURVE		"/tmp/idsp_tone_curve"
#define PRELOAD_RGB2YUV_MAT		"/tmp/idsp_rgb2yuv"
#define PRELOAD_LOCAL_EXPO		"/tmp/idsp_local_expo"

#define IMAGE_NAME_SIZE			(48)
#define MAX_IMAGE_BINARY_NUM		(32)

#if defined(CONFIG_ISO_TYPE_ADVANCED)
#define SIZE_IDSP_CFG_ADV_ISO		(26192)
#else
#define SIZE_IDSP_CFG_ADV_ISO		(24896)
#endif

#define SIZE_LISO_CFG_ADV_ISO		(808)
#define IDSP_CFG_BIN_SIZE		(SIZE_LISO_CFG_ADV_ISO + SIZE_IDSP_CFG_ADV_ISO)

#define COLOR_UPDATE_FLAG_OFFSET	(348)
#define DEFAULT_BINARY_IDSPCFG_OFFSET	(8208)
#define IDSPCFG_BINARY_HEAD_SIZE	(64)

#define FASTBOOT_USER_DATA_OFFSET	(1024)

struct image_binary_info { /* 48 + 4 + 4 + 4 + 4 = 64 */
	char		name[IMAGE_NAME_SIZE];
	unsigned int	size;
	unsigned int	src_offset;
	unsigned int	dest_offset;
	unsigned int	cfg_offset;
}__attribute__((packed));

struct  idsp_cfg_file_info {
	char	name[IMAGE_NAME_SIZE];
	unsigned int	size;
};

enum iav_fb_bin_id {
	IAV_FB_VIN_VIDEO_FORMAT = 0,
	IAV_FB_VIN_DSP_CONFIG = 1,
	IAV_FB_ENC_CONFIG = 2,
	IAV_FB_AAA_TONE_CURVE = 3,
	IAV_FB_AAA_RGB2YUV_MAT = 4,
	IAV_FB_AAA_LOCAL_EXPO = 5,
	IAV_FB_DATA_MAX_NUM,
	IAV_FB_DATA_FIRST = IAV_FB_VIN_VIDEO_FORMAT,
	IAV_FB_DATA_LAST = IAV_FB_DATA_MAX_NUM,
};

struct iav_fb_aeb {
	/* AWB */
	unsigned int	r_gain;
	unsigned int	b_gain;
	/* AE */
	unsigned int	d_gain;
	unsigned int	shutter;
	unsigned int	agc;
};

struct iav_fb_node {
	unsigned int offset;
	unsigned int size;
};

struct iav_fb_hdr {  /*  4 + 20 + 8 + 8 * 6 + 48 = 128  */
	unsigned int			dsp_status;
	struct iav_fb_aeb 		pre_aeb;
	unsigned char			fast_thaw : 1;
	unsigned char			night_mode : 1;
	unsigned char			reserve0 : 6;
	unsigned char			shutter_max; 	/* unit: denominator seconds */
	unsigned char			agc_max;		/* unit: db */
	unsigned char			reserve1[5];
	struct iav_fb_node		bin[IAV_FB_DATA_MAX_NUM];
	unsigned char			reserve2[48];
}__attribute__((packed));

#endif
