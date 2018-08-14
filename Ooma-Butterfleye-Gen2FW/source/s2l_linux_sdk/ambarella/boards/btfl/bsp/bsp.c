/**
 * boards/s2lmelektra/bsp/bsp.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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


#include <bldfunc.h>
#include <ambhw/drctl.h>
#include <ambhw/idc.h>
#include <eth/network.h>
#include <ambhw/gpio.h>
#include <ambhw/adc.h>
#include <dsp/dsp.h>
#include <adc.h>
#include <libfdt.h>

#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
#include "iav_fastboot.h"
#include "iav/sensor_light.c"
#include "iav/smart3a.c"

#if defined(CONFIG_S2LMBTFL_VCA)
#include "iav/vca.c"
#endif

#if defined (CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L55M)
#include "iav/sensor_imx322.c"
#endif

#if defined(CONFIG_S2LMBTFL_DSP_FASTOSD)
#include "iav/iav_fastosd.c"
#endif


#if defined(CONFIG_S2LMBTFL_AUDIO_BOOT)

#if defined(CONFIG_S2LMBTFL_WM8974)
#include "iav/codec_wm8974.c"
#elif defined(CONFIG_S2LMBTFL_AK7755)
#include "iav/codec_ak7755.c"
#endif
#endif

int smart3a_ret = -1;
int idsp_cfg_index = -1;
struct adcfw_header *hdr = NULL;
#endif


/*
typedef enum fastboot_mode {
    FASTBOOT_MODE_WLAN_CONFIG = 0x00,//000
    FASTBOOT_MODE_WLAN_RECONNECT = 0x01,//001
    FASTBOOT_MODE_NOTIFY = 0x02, //010
    FASTBOOT_MODE_RECORDING = 0x04,//100
    FASTBOOT_MODE_STREAMING = 0x05,// 101
} FASTBOOT_MODE;
*/

#define IS_ENABLE_DSP(mode) (mode & 0x04)

static int check_boot_mode()
{
	//int enable_dsp = gpio_get(38);
	//return enable_dsp;
	int mode = 0;
	mode |=  gpio_get(92);
	mode |=  gpio_get(91) << 1;
	mode |=  gpio_get(38) << 2;
	//return mode;
    // Naresh K :As of now hard coding to record to start dsp always
	return 0x04;
}
char *boot_mode_name(int boot_mode){
	switch(boot_mode){
	case 0: return "MODE_WLAN_CONFIG";
	case 1: return "MODE_WLAN_RECONNECT";
	case 2: return "MODE_NOTIFY";
	case 3: return "MODE_REBUILD_HIBERNATION";
	case 4: return "MODE_RECORDING";
	case 5: return "MODE_STREAMING";
	default:return "UNKNOWN";
	}
}

/*static void reset_wifi_chip(void)
{
	gpio_config_sw_out(112);
	gpio_set(112);
	rct_timer_dly_ms(100);
	gpio_clr(112);
}*/

#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
static void amboot_fast_boot_sensor(flpart_table_t *pptb)
{
	/* prepare setup sensor */
#if defined(CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L22M)
    /* comsume this period in amboot_bsp_early_init */
	//rct_timer_dly_ms(10);
	gpio_set(98);//reset done
	imx322_init();
	vin_phy_init_pre(SENSOR_PARALLEL_LVCMOS);
#endif

	smart3a_ret = find_idsp_cfg(pptb, &hdr, &idsp_cfg_index);
	putstrdec("iDSP config index: ", idsp_cfg_index);
	if (!smart3a_ret) {
		load_shutter_gain_param(hdr, idsp_cfg_index);
	}

}

#if defined(CONFIG_PANDORA_RTOS)
int amboot_init_pandora_aaa(void *init_aeb)
{
	init_pandora_aaa(hdr, idsp_cfg_index, (aaa_output_t *)init_aeb);
	return 0;
}

int amboot_update_pandora_aaa(void* pre_aeb)
{
	update_pandora_aaa((struct iav_fb_aeb *)pre_aeb);
	return 0;
}
#endif
#endif
/* ==========================================================================*/
int amboot_bsp_early_init()
{
    /* Clear Sensor GPIO in here and set it in amboot_fast_boot_sensor() */
#if defined(CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L22M)
    gpio_config_sw_out(100);//power
    gpio_config_sw_out(98);//reset
    gpio_set(100);//power on
    gpio_clr(98);
#endif
    return 0;
}


int amboot_bsp_hw_init()
{
	int retval = 0;
	// Change VD0 Driving Strength
	writel(0xec170324, 0x1fffffff);
	writel(0xec17032c, 0xfffe2000);
	writel(0xec170330, 0xffffdfff);

#if defined(CONFIG_S2LMBTFL_AUDIO_BOOT)
	codec_init();
#endif
	flpart_table_t ptb;
	flprog_get_part_table(&ptb);

	if (ptb.dev.magic != FLPART_MAGIC) {
		putstr("sanitized ptb.dev\r\n");
		flprog_set_part_table(&ptb);
	}
#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
	amboot_fast_boot_sensor(&ptb);
#endif
	return retval;
}

#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
#if defined(CONFIG_S2LMBTFL_DSP_FASTOSD)
static void load_osd_resources(flpart_table_t *pptb){
       u32 offset, size = 0;
       int rval;
       struct dspfw_header *hdr = (struct dspfw_header *)pptb->part[PART_DSP].mem_addr;
       rval = dsp_get_bin_by_name(hdr, "font_index.bin", &offset, &size);
	if (rval >= 0){
		bld_loader_load_partition_partial(PART_DSP, offset, size, DSP_OVERLAY_FONT_INDEX_START, 0);
       }
	rval = dsp_get_bin_by_name(hdr, "font_map.bin", &offset, &size);
	if (rval >= 0){
		bld_loader_load_partition_partial(PART_DSP, offset, size, DSP_OVERLAY_FONT_MAP_START, 0);
	}
	rval = dsp_get_bin_by_name(hdr, "clut.bin", &offset, &size);
	if (rval >= 0){
		bld_loader_load_partition_partial(PART_DSP, offset, size, DSP_OVERLAY_CLUT_START, 0);
	}
}
#endif
#endif

/* ==========================================================================*/
#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
static void amboot_fast_boot(flpart_table_t *pptb,int boot_mode)
{
	/* store dsp status in amboot, restore it after enter Linux IAV */
	u32 *p_iav_status = NULL;
	u32 iav_status = IAV_STATE_IDLE;
	/* Change the value after call dsp_boot() */
	struct iav_fb_hdr *fb_hdr =
		(struct iav_fb_hdr *)DSP_FASTDATA_START;
	p_iav_status = &(fb_hdr->dsp_status);
#if defined(AMBOOT_THAW_HIBERNATION)
	// 0 means re-parse all fastboot commands when resumed.
	fb_hdr->fast_thaw = 0;
#endif

#if defined (CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L22M)
	vin_phy_init_post(SENSOR_PARALLEL_LVCMOS);
	store_vin_format_config();
#endif
	iav_boot_preview(hdr, boot_mode);
	iav_status = IAV_STATE_PREVIEW;

	iav_boot_dsplog();

	iav_boot_encode(hdr, boot_mode);
	iav_status = IAV_STATE_ENCODING;
	/* show fastosd */

	// TODO: fastosd support
	if (hdr->params_in_amboot.enable_fastosd) {
		// load_osd_resources(pptb);
		// iav_fastosd(hdr->params_in_amboot.timezone, hdr->params_in_amboot.fastosd_string);
	}

#if defined(CONFIG_S2LMBTFL_ENABLE_ADVANCED_ISO_MODE)
	load_iso_idsp_param(hdr, idsp_cfg_index);
#endif

	/* now boot up dsp */
	dsp_boot_pre();
	dsp_init_post(pptb);
	*p_iav_status = iav_status;
#if defined(CONFIG_PANDORA_RTOS)
	set_dsp_state(iav_status);
#endif
	printf("enable_audio=%d\n"
		"enable_ldc=%d\n"
		"rotation_mode=%d\n"
		"stream0_enable=%d\n"
		"stream0_resolution=%d\n"
		"stream0_fmt=%d\n"
		"stream0_fps=%d\n"
		"stream0_bitrate=%d\n"
		"enable_vca=%d\n"
		"vca_frame_num=%d\n",
		hdr->params_in_amboot.enable_audio,
		hdr->params_in_amboot.enable_ldc,
		hdr->params_in_amboot.rotation_mode,
		hdr->params_in_amboot.stream0_enable,
		hdr->params_in_amboot.stream0_resolution,
		hdr->params_in_amboot.stream0_fmt,
		hdr->params_in_amboot.stream0_fps,
		hdr->params_in_amboot.stream0_bitrate,
		hdr->params_in_amboot.enable_vca,
		hdr->params_in_amboot.vca_frame_num);
}

#endif

#define MAX_LEVEL	32
static int is_ramfs_bootup(const void *fdt, const char *path, int print_prop)
{
	int offset, nextoffset, level;
	u32 tag;
	const struct fdt_property *fdt_prop;
	const char *pathp;

       offset = fdt_path_offset(fdt, path);
	if (offset < 0) {
		putstr("libfdt fdt_path_offset() error\n");
		return 0;
	}

	level = 0;
	while(level >= 0) {
		char buf[64];

		tag = fdt_next_tag(fdt, offset, &nextoffset);

		switch(tag) {
		case FDT_BEGIN_NODE:
			fdt_get_path(fdt, offset, buf, sizeof(buf));
			level++;
			if (level >= MAX_LEVEL) {
				putstr("Man, too deep!\n");
				return 0;
			}
			break;

		case FDT_END_NODE:
			level--;
			if (level == 0)
				level = -1;	/* exit the loop */
			break;

		case FDT_PROP:
			fdt_prop = fdt_offset_ptr(fdt, offset, sizeof(*fdt_prop));
			pathp = fdt_string(fdt, fdt32_to_cpu(fdt_prop->nameoff));
			if (!strcmp(pathp,"bootargs")) {
				if(strstr(fdt_prop->data, "rootfs=ramfs", strlen(fdt_prop->data))){
                                return 1;
                            }
				return 0;
			}
			break;

		case FDT_NOP:
			putstr("NOP tag\r\n");
			break;

		case FDT_END:
			return 0;

		default:
			putstr("Unknown tag = ");
                     putdec(tag);
                     putstr("\r\n");
			return 0;
		}
		offset = nextoffset;
	}

	return 0;
}

static  int is_pba_bootup(){
    int rval = 0;
    void *fdt = (void *)bld_hugebuf_addr;
    /*read DTB from the address to fdt*/
    rval = flprog_get_dtb((void *)fdt);
    if (rval < 0) {
        putstr("Get dtb failed\r\n");
        return 0;
    }else{
        return is_ramfs_bootup(fdt,"/chosen", 1);
     }
}

int amboot_bsp_entry(flpart_table_t *pptb)
{
	int retval = 0;
	flpart_table_t ptb;

	int boot_mode = check_boot_mode();
	putstr(boot_mode_name(boot_mode));
	putstr("\r\n");
	if (is_pba_bootup()) {
		putstr("PBA Boot Up, do not boot DSP\r\n");
		boot_mode = 0;
	}
#if defined(CONFIG_S2LMBTFL_DSP_BOOT)
	//hibernation is enabled, in config mode or rebuild hibernation mode also need to start encoding
	if (IS_ENABLE_DSP(boot_mode) || (0 == boot_mode) || (3 == boot_mode)) {
		amboot_fast_boot(pptb, boot_mode);
#if defined(CONFIG_S2LMBTFL_AUDIO_BOOT)
		audio_start();
#endif
	}

#endif

	/* Read the partition table */
	retval = flprog_get_part_table(&ptb);
	if (retval < 0) {
		return retval;
	}

	/* BIOS boot */
	if (ptb.dev.rsv[0] > 0) {
		printf("Find BIOS boot flag, ptb.dev.rsv[0]=%d\n", ptb.dev.rsv[0]);
		retval = ptb.dev.rsv[0];
	}

	//hibernation is enabled and if current mode is rebuild-hibernation mode, disable hibernation boot
	if ((3 == boot_mode) && (0 == retval)) {
		retval = 4;
	}

	return retval;
}

/* ==========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
#define MII_KSZ80X1R_CTRL			0x1F
#define KSZ80X1R_CTRL_INT_ACTIVE_HIGH		(1 << 9)
#define KSZ80X1R_RMII_50MHZ_CLK			(1 << 7)

extern u16 eth_mii_read(struct bld_eth_dev_s *dev, u8 addr, u8 reg);
extern void eth_mii_write(struct bld_eth_dev_s *dev, u8 addr, u8 reg, u16 data);
extern u8 eth_scan_phy_addr(struct bld_eth_dev_s *dev);

int amboot_bsp_eth_init_post(void *dev)
{
	u8 phy_addr;
	u16 phy_reg;
	u32 phy_id;

	phy_addr = eth_scan_phy_addr(dev);
	phy_reg = eth_mii_read(dev, phy_addr, 0x02);	//PHYSID1
	phy_id = (phy_reg & 0xffff) << 16;
	phy_reg = eth_mii_read(dev, phy_addr, 0x03);	//PHYSID2
	phy_id |= (phy_reg & 0xffff);

	if (phy_id == 0x00221560) {
	phy_reg = eth_mii_read(dev, phy_addr, MII_KSZ80X1R_CTRL);
	phy_reg |= KSZ80X1R_RMII_50MHZ_CLK;
	eth_mii_write(dev, phy_addr, MII_KSZ80X1R_CTRL, phy_reg);
	}

	return 0;
}
#endif

