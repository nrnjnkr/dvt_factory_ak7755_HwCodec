/*
 * Filename : ov2732.c
 *
 * History:
 *    2016/12/16 - [Hao Zeng] created file
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

#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "ov2732.h"
#include "ov2732_table.c"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ov2732_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
	u32 max_short;
};

static int ov2732_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2732_priv *ov2732;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov2732 = (struct ov2732_priv *)vdev->priv;
	client = ov2732->control_data;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;

	if (unlikely(subaddr == OV2732_SWRESET))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
		msgs[0].flags = client->flags;

	msgs[0].buf = pbuf;
	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}

	return 0;
}

static int ov2732_write_reg2(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2732_priv *ov2732;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ov2732 = (struct ov2732_priv *)vdev->priv;
	client = ov2732->control_data;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}

	return 0;
}

static int ov2732_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov2732_priv *ov2732;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov2732 = (struct ov2732_priv *)vdev->priv;
	client = ov2732->control_data;

	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 1;

	rval = i2c_transfer(client->adapter, msgs, 2);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int ov2732_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov2732_config;

	memset(&ov2732_config, 0, sizeof(ov2732_config));

	ov2732_config.interface_type = SENSOR_MIPI;
	ov2732_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov2732_config.mipi_cfg.lane_number = SENSOR_2_LANE;

	ov2732_config.cap_win.x = format->def_start_x;
	ov2732_config.cap_win.y = format->def_start_y;
	ov2732_config.cap_win.width = format->def_width;
	ov2732_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	ov2732_config.hdr_cfg.act_win.x = format->act_start_x;
	ov2732_config.hdr_cfg.act_win.y = format->act_start_y;
	ov2732_config.hdr_cfg.act_win.width = format->act_width;
	ov2732_config.hdr_cfg.act_win.height = format->act_height;

	ov2732_config.sensor_id	= GENERIC_SENSOR;
	ov2732_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov2732_config.bayer_pattern	= format->bayer_pattern;
	ov2732_config.video_format	= format->format;
	ov2732_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov2732_config);
}

static void ov2732_sw_reset(struct vin_device *vdev)
{
	ov2732_write_reg(vdev, OV2732_SWRESET, 0x01);
	msleep(5);
}

static int ov2732_init_device(struct vin_device *vdev)
{
	ov2732_sw_reset(vdev);
	return 0;
}

static void ov2732_start_streaming(struct vin_device *vdev)
{
	ov2732_write_reg(vdev, OV2732_STANDBY, 0x01); /* streaming */
}

static int ov2732_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov2732_update_hv_info(struct vin_device *vdev)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u32 data_h, data_l;

	ov2732_read_reg(vdev, OV2732_HTS_MSB, &data_h);
	ov2732_read_reg(vdev, OV2732_HTS_LSB, &data_l);
	pinfo->line_length = (data_h<<8) + data_l;
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov2732_read_reg(vdev, OV2732_VTS_MSB, &data_h);
	ov2732_read_reg(vdev, OV2732_VTS_LSB, &data_l);
	pinfo->frame_length_lines = (data_h<<8) + data_l;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_1080P)
			pinfo->max_short = S_MAX_EXPO_1080P;
		else if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P)
			pinfo->max_short = S_MAX_EXPO_720P;
		ov2732_write_reg2(vdev, OV2732_S_MAX_EXPO_MSB, pinfo->max_short);
	}

	vin_debug("line_length:%d, frame_length_lines:%d, max_short:%d\n",
		pinfo->line_length, pinfo->frame_length_lines, pinfo->max_short);

	return 0;
}

static int ov2732_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		regs = ov2732_linear_mode_regs;
		regs_num = ARRAY_SIZE(ov2732_linear_mode_regs);
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		regs = ov2732_hdr_mode_regs;
		regs_num = ARRAY_SIZE(ov2732_hdr_mode_regs);
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_error("Unknown mode\n");
		return -EINVAL;
	}
	for (i = 0; i < regs_num; i++)
		ov2732_write_reg(vdev, regs[i].addr, regs[i].data);

	regs = ov2732_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ov2732_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		ov2732_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ov2732_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov2732_get_line_time(vdev);

	/* Enable Streaming */
	ov2732_start_streaming(vdev);

	/* communicate with IAV */
	rval = ov2732_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov2732_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	if (hold_mode) {
		ov2732_write_reg(vdev, OV2732_GRP_ACCESS, 0x00);
	} else {
		ov2732_write_reg(vdev, OV2732_GRP_ACCESS, 0x10);
		ov2732_write_reg(vdev, OV2732_GRP_ACCESS, 0xA0);
	}

	return 0;
}

static int ov2732_set_shutter_row(struct vin_device *vdev, u32 row)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int rval = 0;

	num_line = row;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 4) */
	min_line = 4;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov2732_write_reg2(vdev, OV2732_L_EXPO_HSB, (num_line >> 8) & 0x0FFF);
	ov2732_write_reg(vdev, OV2732_L_EXPO_LSB, num_line & 0xFF);
	num_line >>= 4;

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return rval;
}

static int ov2732_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (!pinfo->line_length) {
		rval = ov2732_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov2732_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov2732_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	int shutter_long, shutter_short1, max_short;
	u32 frame_length_lines;

	frame_length_lines = pinfo->frame_length_lines;
	max_short = (pinfo->max_short >> 1) - 4;

	/* long shutter */
	shutter_long = p_shutter_gp->l;

	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->s1;

	/* shutter limitation check */
	if (shutter_short1 > max_short) {
		vin_error("short shutter %d exceeds limitation %d\n", shutter_short1, max_short);
		return -EPERM;
	}

	if (shutter_long + max_short > frame_length_lines - 4) {
		vin_error("shutter exceeds limitation! long:%d, max short:%d, V:%d\n",
			shutter_long, max_short, frame_length_lines);
		return -EPERM;
	}

	/* long shutter */
	shutter_long = shutter_long << 4;
	ov2732_write_reg2(vdev, OV2732_L_EXPO_HSB, (shutter_long >> 8) & 0x0FFF);
	ov2732_write_reg(vdev, OV2732_L_EXPO_LSB, shutter_long & 0xFF);

	/* short shutter 1 */
	shutter_short1 = shutter_short1 << 4;
	ov2732_write_reg2(vdev, OV2732_S_EXPO_HSB, (shutter_short1 >> 8) & 0x0FFF);
	ov2732_write_reg(vdev, OV2732_S_EXPO_LSB, shutter_short1 & 0xFF);

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("shutter long:%d, short1:%d\n", p_shutter_gp->l,
		p_shutter_gp->s1);

	return 0;
}

static int ov2732_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int ov2732_set_fps(struct vin_device *vdev, int fps)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u64 pixelclk, v_lines, vb_time;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	/* temporally disable VTS write for hdr mode */
	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE)
		ov2732_write_reg2(vdev, OV2732_VTS_MSB, v_lines & 0xFFFF);

	pinfo->frame_length_lines = v_lines;

	v_lines = pinfo->frame_length_lines - vdev->cur_format->height;
	vb_time = pinfo->line_length * (u64)v_lines * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov2732_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV2732_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV2732_GAIN_MAXDB);
		agc_idx = OV2732_GAIN_MAXDB;
	}

	/* Analog Gain */
	ov2732_write_reg2(vdev, OV2732_L_AGAIN_MSB, OV2732_GAIN_TABLE[agc_idx][OV2732_GAIN_COL_AGAIN]);
	/* Digital Gain */
	ov2732_write_reg2(vdev, OV2732_L_DGAIN_MSB, OV2732_GAIN_TABLE[agc_idx][OV2732_GAIN_COL_DGAIN]);

	return 0;
}

static int ov2732_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u32 gain_index;

	/* long frame */
	gain_index = p_again_gp->l;
	ov2732_write_reg2(vdev, OV2732_L_AGAIN_MSB, OV2732_GAIN_TABLE[gain_index][OV2732_GAIN_COL_AGAIN]);
	ov2732_write_reg2(vdev, OV2732_L_DGAIN_MSB, OV2732_GAIN_TABLE[gain_index][OV2732_GAIN_COL_DGAIN]);

	/* short frame 1 */
	gain_index = p_again_gp->s1;
	ov2732_write_reg2(vdev, OV2732_S_AGAIN_MSB, OV2732_GAIN_TABLE[gain_index][OV2732_GAIN_COL_AGAIN]);
	ov2732_write_reg2(vdev, OV2732_S_DGAIN_MSB, OV2732_GAIN_TABLE[gain_index][OV2732_GAIN_COL_DGAIN]);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d\n",
		p_again_gp->l, p_again_gp->s1);

	return 0;
}

static int ov2732_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov2732_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long dgain index:%d, short1 dgain index:%d\n",
		p_dgain_gp->l, p_dgain_gp->s1);

	return 0;
}

static int ov2732_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov2732_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (!pinfo->line_length) {
		rval = ov2732_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov2732_get_line_time(vdev);
	}

	/* long shutter */
	exposure_lines = p_shutter2row->l * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->l = (u32)exposure_lines;

	/* short shutter 1 */
	exposure_lines = p_shutter2row->s1 * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->s1 = (u32)exposure_lines;

	return rval;
}

static int ov2732_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	int rval = 0;
	u32 tmp_reg, bayer_pattern, v_flip = 0, h_mirror = 0;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		v_flip = OV2732_V_FLIP;
		h_mirror = OV2732_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		v_flip = 0;
		h_mirror = OV2732_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		v_flip = OV2732_V_FLIP;
		h_mirror = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_NONE:
		v_flip = 0;
		h_mirror = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov2732_read_reg(vdev, OV2732_FORMAT1, &tmp_reg);
	tmp_reg &= (~OV2732_V_FLIP);
	tmp_reg |= OV2732_H_MIRROR;
	tmp_reg |= v_flip;
	tmp_reg ^= h_mirror;
	ov2732_write_reg(vdev, OV2732_FORMAT1, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return rval;
}

static int ov2732_get_chip_status(struct vin_device *vdev,
	struct vindev_chip_status *chip_status)
{
	u32 tmp_reg;

	ov2732_read_reg(vdev, OV2732_TPM_TRIGGER, &tmp_reg);
	tmp_reg |= 0x01;
	ov2732_write_reg(vdev, OV2732_TPM_TRIGGER, tmp_reg);
	/* wait some time to read temperature */
	msleep(5);
	ov2732_read_reg(vdev, OV2732_TPM_READ, &tmp_reg);

	chip_status->temperature = (tmp_reg & 0xFF) - OV2732_TPM_OFFSET;

	return 0;
}

static int ov2732_get_eis_info(struct vin_device *vdev,
	struct vindev_eisinfo *eis_info)
{
	eis_info->sensor_cell_width = 200;/* 2.0 um */
	eis_info->sensor_cell_height = 200;/* 2.0 um */
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;
	eis_info->vb_time = vdev->cur_format->vb_time;

	return 0;
}

static int ov2732_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct ov2732_priv *pinfo = (struct ov2732_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 4;
	aaa_info->sht1_max = (pinfo->max_short >> 1) - 4;

	return 0;
}

#ifdef CONFIG_PM
static int ov2732_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ov2732_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int ov2732_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++)
		ov2732_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);

	return 0;
}
#endif

static struct vin_ops ov2732_ops = {
	.init_device		= ov2732_init_device,
	.set_format		= ov2732_set_format,
	.set_shutter_row	= ov2732_set_shutter_row,
	.shutter2row		= ov2732_shutter2row,
	.set_frame_rate	= ov2732_set_fps,
	.set_agc_index		= ov2732_set_agc_index,
	.set_mirror_mode	= ov2732_set_mirror_mode,
	.set_hold_mode		= ov2732_set_hold_mode,
	.get_chip_status	= ov2732_get_chip_status,
	.get_eis_info		= ov2732_get_eis_info,
	.get_aaa_info		= ov2732_get_aaa_info,
	.read_reg			= ov2732_read_reg,
	.write_reg		= ov2732_write_reg,
#ifdef CONFIG_PM
	.suspend		= ov2732_suspend,
	.resume			= ov2732_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = ov2732_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = ov2732_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = ov2732_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = ov2732_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = ov2732_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = ov2732_get_wdr_shutter_row_group,
	.wdr_shutter2row = ov2732_wdr_shutter2row,
};

/* ========================================================================== */
static int ov2732_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov2732_priv *ov2732;
	u32 cid_l, cid_m;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_OV2732, sizeof(struct ov2732_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x24000000;  /* 36dB */
	vdev->agc_db_min = 0x00000000;  /* 0dB */
	vdev->agc_db_step = 0x00180000; /* 0.09375dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = OV2732_GAIN_MAXDB;
	vdev->wdr_dgain_idx_min = 0;
	vdev->wdr_dgain_idx_max = 0;

	i2c_set_clientdata(client, vdev);

	ov2732 = (struct ov2732_priv *)vdev->priv;
	ov2732->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ov2732_ops,
		ov2732_formats, ARRAY_SIZE(ov2732_formats),
		ov2732_plls, ARRAY_SIZE(ov2732_plls));
	if (rval < 0)
		goto ov2732_probe_err;

	/* query sensor id */
	ov2732_read_reg(vdev, OV2732_CID_M, &cid_m);
	ov2732_read_reg(vdev, OV2732_CID_L, &cid_l);
	vin_info("OV2732 init(2-lane mipi), sensor ID: 0x%x\n", (cid_m<<8)+cid_l);

	return 0;

ov2732_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov2732_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov2732_idtable[] = {
	{ "ov2732", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2732_idtable);

static struct i2c_driver i2c_driver_ov2732 = {
	.driver = {
		.name	= "ov2732",
	},

	.id_table	= ov2732_idtable,
	.probe		= ov2732_probe,
	.remove		= ov2732_remove,

};

static int __init ov2732_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov2732", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov2732);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov2732_exit(void)
{
	i2c_del_driver(&i2c_driver_ov2732);
}

module_init(ov2732_init);
module_exit(ov2732_exit);

MODULE_DESCRIPTION("OV2732 1/4-Inch, 1920x1080, 2-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

