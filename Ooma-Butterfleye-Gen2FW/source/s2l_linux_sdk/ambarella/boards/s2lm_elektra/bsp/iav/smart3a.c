/**
 * boards/s2lm_elektra/bsp/iav/smart3a.c
 *
 * History:
 *    2015/02/11 - [Tao Wu] created file
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


//#define SMART_3A_DEBUG
#include "iav_fastboot.h"
#if defined(CONFIG_PANDORA_RTOS)
#include "dsp/fb_aaa_ctrl.h"
#endif

static unsigned char night_mode = 0;

#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE

static struct image_binary_info image_info[MAX_IMAGE_BINARY_NUM] =
{
	// name									//size	//src 	//dest	//cfg
	{"01_cc_reg.bin",						2304,	0,		0,		300},
	{"02_cc_3d_A.bin",						16384,	2304,	2304,	304},
	{"03_cc_3d_C.bin",						0,		18688,	18688,	0},
	{"04_cc_out.bin",						1024,	18688,	18688,	308},
	{"05_thresh_dark.bin",					768,	19712,	19712,	352},
	{"06_thresh_hot.bin",					768,	20480,	20480,	356},
	{"07_local_exposure_gain.bin",			512,	21248,	21248,	264},
	{"08_cmf_table.bin",					48,		21760,	21760,	360},
	{"09_chroma_scale_gain.bin",			256,	21808,	21808,	296},
	{"10_fir1_A.bin",						256,	22064,	22064,	412},
	{"11_fir2_A.bin",						256,	22320,	22320,	416},
	{"12_coring_A.bin",						256,	22576,	22576,	420},
	{"13_fir1_B.bin",						0,		22832,	22832,	0},
	{"14_fir2_B.bin",						0,		22832,	22832,	0},
	{"15_coring_B.bin",						0,		22832,	22832,	0},
	{"16_wide_chroma_mctf.bin",				0,		22832,	22832,	0},
	{"17_fir1_motion_detection.bin",		256,	22832,	22832,	620},
	{"18_fir1_motion_detection_map.bin",	256,	23088,	23088,	624},
	{"19_coring_motion_detection_map.bin",	256,	23344,	23344,	628},
	{"20_video_mctf.bin",					528,	23600,	23600,	616},
	{"21_md_fir1.bin",						256,	24128,	24128,	620},
	{"22_md_map_fir1.bin",					256,	24384,	24384,	624},
	{"23_md_map_coring.bin",				256,	24640,	24640,	628},
	{"24_cc_reg_combine.bin",				0,		24896,	24896,	792},
	{"25_cc_3d_combine.bin",				0,		24896,	24896,	796},
	{"26_thresh_dark_mo.bin",				0,		24896,	24896,	648},
	{"27_thresh_hot_mo.bin",				0,		24896,	24896,	652},
	{"28_cmf_table_mo.bin",					0,		24896,	24896,	656},
	{"29_fir1_mo.bin",						0,		24896,	24896,	708},
	{"30_fir2_mo.bin",						0,		24896,	24896,	712},
	{"31_coring_mo.bin",					0,		24896,	24896,	716},
};

int dsp_update_aaa_binary_addr(void)
{
	u32 params_base = (u32)DSP_IMAGE_CONFIG_START;
	u32 cfg_base = (u32)DSP_STILL_ISO_CONFIG_START;
	u32 *addr;
	int i;

	for (i = 0; i < MAX_IMAGE_BINARY_NUM; i++) {
		if (image_info[i].size != 0) {
			addr = (u32*)(cfg_base + image_info[i].cfg_offset);
			*addr = params_base + image_info[i].dest_offset;
		}
	}

	return 0;
}

static void load_iso_idsp_param(struct adcfw_header *hdr,
                                int idsp_cfg_index)
{
	u32 addr = 0;

	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset;
	memcpy((void *)DSP_IMAGE_CONFIG_START, (void *)addr, SIZE_IDSP_CFG_ADV_ISO);

      // iso table
	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset + SIZE_IDSP_CFG_ADV_ISO;
	// lens shading
	u8* update_flag = (u8*)(addr+64);
	*update_flag++ = 0;
	*update_flag++ = 2;
	*update_flag++ = 0;
	*update_flag = 2;
	//color
	update_flag = (u8*)(addr+348);
	*update_flag++ = 0;
	*update_flag++ = 0;
	*update_flag++ = 0;
	*update_flag = 0;

       //liso
	update_flag = (u8*)(addr+494);
	*update_flag++ = 0;
	*update_flag  = 0;
	// low iso sharperning
	update_flag = (u8*)(addr+431);
	*update_flag &= ~(1 << 2);
	*update_flag &= ~(1 << 3);
	*update_flag &= ~(1 << 4);
	*update_flag &= ~(1 << 5);
	memcpy((void *)DSP_STILL_ISO_CONFIG_START, (void *)addr, SIZE_LISO_CFG_ADV_ISO);
	dsp_update_aaa_binary_addr();

	// copy AE/AWB data to FB_DATA region.
	struct iav_fb_hdr *fb_hdr = (struct iav_fb_hdr *)DSP_FASTDATA_START;
	fb_hdr->pre_aeb.r_gain = hdr->smart3a[idsp_cfg_index].r_gain;
	fb_hdr->pre_aeb.b_gain = hdr->smart3a[idsp_cfg_index].b_gain;
	fb_hdr->pre_aeb.d_gain = hdr->smart3a[idsp_cfg_index].d_gain;
	fb_hdr->pre_aeb.shutter = hdr->smart3a[idsp_cfg_index].shutter;
	fb_hdr->pre_aeb.agc = hdr->smart3a[idsp_cfg_index].agc;
	// Max value should match those used in amboot 3A
	fb_hdr->shutter_max = 30;
	fb_hdr->agc_max = 42;
	fb_hdr->night_mode = night_mode;

}
#endif

enum idsp_cfg_select_policy {
	IDSP_CFG_SELECT_ONLY_ONE=0,
	IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS,
};

static int select_idsp_cfg(struct adcfw_header* p_hdr)
{
#ifdef CONFIG_S2LMELEKTRA_CHECK_LIGHT
	unsigned int cur_light = check_light();
	//night mode
	if (cur_light < LIGHT_THRESHOLD) {
		putstr("select night mode\r\n");
		night_mode = 1;
		return 1;
	}
	//day mode
	else {
		putstr("select day mode\r\n");
		night_mode = 0;
		return 0;
    }
#else
	//IDSP_CFG_SELECT_ONLY_ONE
	night_mode = 0;
	return 0;
#endif
}

int find_idsp_cfg(flpart_table_t *pptb, struct adcfw_header **p_hdr,
                  int *p_idsp_cfg_index)
{
	int rval = -1;
	int idsp_cfg_index = -1;
	struct adcfw_header *hdr;

	if (!p_hdr || !p_idsp_cfg_index) {
		putstr("Find idspcfg failed, NULL input\r\n");
		return -1;
	}

#ifdef SMART_3A_DEBUG
	u32 time1_MS=0, time2_MS = 0, time3_MS = 0, delta1_MS = 0, delta2_MS = 0;
	time1_MS = rct_timer_get_count();
#endif

	rval = bld_loader_load_partition(PART_ADC, pptb, 0, 0);
	if (rval < 0) {
		putstr("Load PART_ADC  failed\r\n");
		return -1;
	}
#ifdef SMART_3A_DEBUG
	time2_MS = rct_timer_get_count();
#endif

	hdr = (struct adcfw_header *)pptb->part[PART_ADC].mem_addr;
	if (hdr->magic != ADCFW_IMG_MAGIC
		|| hdr->fw_size != pptb->part[PART_ADC].img_len) {
		putstr("Invalid PART_ADC size\r\n");
		return -1;
	}

	idsp_cfg_index = select_idsp_cfg(hdr);
	if (idsp_cfg_index >= hdr->smart3a_num) {
		putstr("idspcfg index=");
		putdec(idsp_cfg_index);
		putstr(", should in range as [0, ");
		puthex(hdr->smart3a_num);
		putstr(")\r\n");
		return -1;
	}
	*p_hdr = hdr;
	*p_idsp_cfg_index = idsp_cfg_index;
#ifdef SMART_3A_DEBUG
	time3_MS = rct_timer_get_count();
	delta1_MS = time2_MS - time1_MS;
	delta2_MS = time3_MS - time2_MS;
	putstr("[LOAD3A]: <Load ADC partition to image> cost 0x");
	puthex(delta1_MS);
	putstr(" MS!\r\n");
	putstr("[LOAD3A]: <Find selected section of ADC image> cost 0x");
	puthex(delta2_MS);
	putstr(" MS!\r\n");
#endif

	return 0;
}

int load_shutter_gain_param(struct adcfw_header *hdr, int idsp_cfg_index)
{

#ifdef SMART_3A_DEBUG
	u32 time3_MS = 0, time4_MS = 0, delta3_MS = 0;
	time3_MS = rct_timer_get_count();
#endif

#if defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L55M)
	idc_bld_write_16_8(IDC_MASTER1, 0x34, 0x0202, hdr->smart3a[idsp_cfg_index].para0);//shutter0
	idc_bld_write_16_8(IDC_MASTER1, 0x34, 0x0203, hdr->smart3a[idsp_cfg_index].para1);//shutter1
	idc_bld_write_16_8(IDC_MASTER1, 0x34, 0x301E, hdr->smart3a[idsp_cfg_index].para2);//gain
#endif

#ifdef SMART_3A_DEBUG
	time4_MS = rct_timer_get_count();
	delta3_MS = time4_MS - time3_MS;
	putstr("[LOAD3A]: <r_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].r_gain);

	putstr(", b_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].b_gain);

	putstr(", d_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].d_gain);

	putstr(", shutter=");
	putdec(hdr->smart3a[idsp_cfg_index].shutter);

	putstr(", agc=");
	putdec(hdr->smart3a[idsp_cfg_index].agc);
#if defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L55M)
	putstr(", shutter0202=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para0);

	putstr(", shutter0203=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para1);

	putstr(", gain301E=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para2);
#endif
	putstr(">");
	putstr(" cost 0x");
	puthex(delta3_MS);
	putstr(" MS!\r\n");
#endif

	return 0;
}

#if defined(CONFIG_PANDORA_RTOS)
void init_pandora_aaa(struct adcfw_header *hdr, int idsp_cfg_index,
       aaa_output_t *init_aeb)
{
       /* AE */
       init_aeb->shutter_row = hdr->smart3a[idsp_cfg_index].shutter;
       init_aeb->agc_index = hdr->smart3a[idsp_cfg_index].agc;
       init_aeb->dgain = hdr->smart3a[idsp_cfg_index].d_gain;

       /* AWB */
       init_aeb->wb_gain.r_gain = hdr->smart3a[idsp_cfg_index].r_gain;
       init_aeb->wb_gain.b_gain = hdr->smart3a[idsp_cfg_index].b_gain;
}

int update_pandora_aaa(struct iav_fb_aeb *pre_aeb)
{
       struct iav_fb_hdr *fb_hdr = (struct iav_fb_hdr *)DSP_FASTDATA_START;
       memcpy(&fb_hdr->pre_aeb, pre_aeb, sizeof(struct iav_fb_aeb));
       return 0;
}
#endif
