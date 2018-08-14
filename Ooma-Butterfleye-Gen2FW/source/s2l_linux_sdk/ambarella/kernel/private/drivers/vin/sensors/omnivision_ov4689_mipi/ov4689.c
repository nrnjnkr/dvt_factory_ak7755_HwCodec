/*
 * Filename : ov4689.c
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
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
#include "ov4689.h"
#include "ov4689_table.c"

#define AMBARELLA_I2C_VIN_FDT_NAME		"ambvin"

static int bus_id = 0;
module_param(bus_id, int, 0644);
MODULE_PARM_DESC(bus_id, "device bus id");

static int addr = 0x6C >> 1;
module_param(addr, int, 0644);
MODULE_PARM_DESC(addr, "device address");

static int lane = 4;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "Set MIPI lane number 2:2 lane 4:4 lane");

static int vinc_id = 0x3210;
module_param(vinc_id, int, 0644);
MODULE_PARM_DESC(vinc_id, "indicate which vin controller is bind to");

struct ov4689_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
	u32 ori_line_length;
	u32 max_middle, max_short;
	u8 ll_mode;
};

static int ov4689_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov4689_priv *ov4689;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov4689 = (struct ov4689_priv *)vdev->priv;
	client = ov4689->control_data;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;

	if (unlikely(subaddr == OV4689_SWRESET))
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

static int ov4689_write_reg2(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov4689_priv *ov4689;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ov4689 = (struct ov4689_priv *)vdev->priv;
	client = ov4689->control_data;

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

static int ov4689_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov4689_priv *ov4689;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov4689 = (struct ov4689_priv *)vdev->priv;
	client = ov4689->control_data;

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

static int ov4689_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov4689_config;

	memset(&ov4689_config, 0, sizeof(ov4689_config));

	ov4689_config.interface_type = SENSOR_MIPI;
	ov4689_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov4689_config.mipi_cfg.lane_number = (lane == 4) ? SENSOR_4_LANE : SENSOR_2_LANE;
	ov4689_config.mipi_cfg.bit_rate = SENSOR_MIPI_BIT_RATE_H;

	ov4689_config.cap_win.x = format->def_start_x;
	ov4689_config.cap_win.y = format->def_start_y;
	ov4689_config.cap_win.width = format->def_width;
	ov4689_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	ov4689_config.hdr_cfg.act_win.x = format->act_start_x;
	ov4689_config.hdr_cfg.act_win.y = format->act_start_y;
	ov4689_config.hdr_cfg.act_win.width = format->act_width;
	ov4689_config.hdr_cfg.act_win.height = format->act_height;

	ov4689_config.sensor_id	= GENERIC_SENSOR;
	ov4689_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov4689_config.bayer_pattern	= format->bayer_pattern;
	ov4689_config.video_format	= format->format;
	ov4689_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov4689_config);
}

static void ov4689_sw_reset(struct vin_device *vdev)
{
	ov4689_write_reg(vdev, OV4689_SWRESET, 0x01);
	msleep(5);
}

static int ov4689_init_device(struct vin_device *vdev)
{
	ov4689_sw_reset(vdev);
	return 0;
}

static void ov4689_start_streaming(struct vin_device *vdev)
{
	ov4689_write_reg(vdev, OV4689_STANDBY, 0x01); /* streaming */
}

static int ov4689_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov4689_update_hv_info(struct vin_device *vdev)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u32 data_h, data_l;

	ov4689_read_reg(vdev, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(vdev, OV4689_HTS_LSB, &data_l);
	pinfo->line_length = (data_h<<8) + data_l;
	pinfo->ori_line_length = pinfo->line_length;
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov4689_read_reg(vdev, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(vdev, OV4689_VTS_LSB, &data_l);
	pinfo->frame_length_lines = (data_h<<8) + data_l;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		ov4689_read_reg(vdev, OV4689_M_MAX_EXPO_MSB, &data_h);
		ov4689_read_reg(vdev, OV4689_M_MAX_EXPO_LSB, &data_l);
		pinfo->max_middle = ((data_h)<<8) + data_l;
		if (unlikely(!pinfo->max_middle)) {
			vin_warn("max_middle is 0!\n");
		}
		pinfo->max_short = 0;
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		ov4689_read_reg(vdev, OV4689_M_MAX_EXPO_MSB, &data_h);
		ov4689_read_reg(vdev, OV4689_M_MAX_EXPO_LSB, &data_l);
		pinfo->max_middle = ((data_h)<<8) + data_l;
		if (unlikely(!pinfo->max_middle)) {
			vin_warn("max_middle is 0!\n");
		}

		ov4689_read_reg(vdev, OV4689_S_MAX_EXPO_MSB, &data_h);
		ov4689_read_reg(vdev, OV4689_S_MAX_EXPO_LSB, &data_l);
		pinfo->max_short = ((data_h)<<8) + data_l;
		if (unlikely(!pinfo->max_short)) {
			vin_warn("max_short is 0!\n");
		}
	}

	vin_debug("line_length:%d, frame_length_lines:%d, max_middle:%d, max_short:%d\n",
		pinfo->line_length, pinfo->frame_length_lines, pinfo->max_middle, pinfo->max_short);

	return 0;
}

static int ov4689_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_16_8 *regs;
	int i, regs_num;
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		if (lane == 4) {
			regs = ov4689_4lane_4m_regs;
			regs_num = ARRAY_SIZE(ov4689_4lane_4m_regs);
			vin_info("4 lane 4M\n");
		} else if (lane == 2) {
			switch (format->device_mode) {
			case 0:
				regs = ov4689_2lane_4m_regs;
				regs_num = ARRAY_SIZE(ov4689_2lane_4m_regs);
				vin_info("2 lane 4M\n");
				break;
			case 1:
				regs = ov4689_2lane_1080p_regs;
				regs_num = ARRAY_SIZE(ov4689_2lane_1080p_regs);
				vin_info("2 lane 1080p\n");
				break;
			case 2:
				regs = ov4689_2lane_720p_regs;
				regs_num = ARRAY_SIZE(ov4689_2lane_720p_regs);
				vin_info("2 lane 720p\n");
				break;
			default:
				vin_error("2 lane mipi doesn't support mode:%d\n", format->video_mode);
				return -EINVAL;
			}
		} else {
			vin_error("OV4689 can only support 2 or 4 lanes mipi\n");
			return -EINVAL;
		}
		vin_info("Linear mode\n");
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		if (format->video_mode == AMBA_VIDEO_MODE_2560_1440 ||
			format->video_mode == AMBA_VIDEO_MODE_2688_1520) {
			regs = ov4689_2x_hdr_4m_regs;
			regs_num = ARRAY_SIZE(ov4689_2x_hdr_4m_regs);
		} else {
			regs = ov4689_2x_hdr_regs;
			regs_num = ARRAY_SIZE(ov4689_2x_hdr_regs);
		}
		vin_info("2x hdr mode\n");
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		if (format->video_mode == AMBA_VIDEO_MODE_2560_1440) {
			regs = ov4689_3x_hdr_4m_regs;
			regs_num = ARRAY_SIZE(ov4689_3x_hdr_4m_regs);
		} else {
			regs = ov4689_3x_hdr_regs;
			regs_num = ARRAY_SIZE(ov4689_3x_hdr_regs);
		}
		vin_info("3x hdr mode\n");
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_info("Unknown mode\n");
		break;
	}

	for (i = 0; i < regs_num; i++)
		ov4689_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ov4689_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		vdev->cur_format->hdr_short1_offset = 0;
		vdev->cur_format->hdr_short2_offset = 0;
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		vdev->cur_format->hdr_short1_offset = pinfo->max_middle * 2 + 1;
		vdev->cur_format->hdr_short2_offset = 0;
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		vdev->cur_format->hdr_short1_offset = pinfo->max_middle * 3 + 1;
		vdev->cur_format->hdr_short2_offset = (pinfo->max_middle + pinfo->max_short) * 3 + 2;
		break;
	default:
		regs = NULL;
		vin_error("Unknown mode\n");
		return -EINVAL;
	}
	pinfo->ll_mode = 0;

	ov4689_get_line_time(vdev);

	/* Enable Streaming */
	ov4689_start_streaming(vdev);

	/* communicate with IAV */
	rval = ov4689_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov4689_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	if (hold_mode)
		ov4689_write_reg(vdev, OV4689_GRP_ACCESS, 0x01);
	else {
		ov4689_write_reg(vdev, OV4689_GRP_ACCESS, 0x11);
		ov4689_write_reg(vdev, OV4689_GRP_ACCESS, 0xA1);
	}

	return 0;
}

static int ov4689_set_shutter_row(struct vin_device *vdev, u32 row)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;

	num_line = row;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 4) */
	min_line = 4;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */

	ov4689_write_reg(vdev, OV4689_L_EXPO_HSB, (num_line >> 16) & 0x0F);
	ov4689_write_reg(vdev, OV4689_L_EXPO_MSB, (num_line >> 8) & 0xFF);
	ov4689_write_reg(vdev, OV4689_L_EXPO_LSB, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ov4689_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (!pinfo->line_length) {
		rval = ov4689_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov4689_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov4689_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	int shutter_long, shutter_short1, shutter_short2, max_middle = 0, max_short = 0;
	u32 frame_length_lines;

	frame_length_lines = pinfo->frame_length_lines;
	if (!pinfo->ll_mode) {
		max_middle = pinfo->max_middle;
		max_short = pinfo->max_short;
	} else {
		max_middle = OV4689_LL_MODE_M_MAX;
		max_short = OV4689_LL_MODE_S_MAX;
	}

	/* long shutter */
	shutter_long = p_shutter_gp->l;

	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->s1;

	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->s2;

	/* shutter limitation check */
	if (shutter_short1 > max_middle - 1) {
		vin_error("middle shutter %d exceeds limitation %d\n", shutter_short1, max_middle - 1);
		return -EPERM;
	}

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if (shutter_short2 > max_short - 1) {
			vin_error("short shutter %d exceeds limitation %d\n", shutter_short2 , max_short - 1);
			return -EPERM;
		}
	}

	if (shutter_long + max_middle + max_short > frame_length_lines - 2) {
		vin_error("shutter exceeds limitation! long:%d, max short1:%d, max short2:%d, V:%d\n",
			shutter_long, max_middle, max_short, frame_length_lines);
		return -EPERM;
	}

	/* long shutter */
	shutter_long  = shutter_long << 4;
	ov4689_write_reg(vdev, OV4689_L_EXPO_HSB, (u8)((shutter_long >> 16) & 0xF));
	ov4689_write_reg(vdev, OV4689_L_EXPO_MSB, (u8)(shutter_long >> 8));
	ov4689_write_reg(vdev, OV4689_L_EXPO_LSB, (u8)(shutter_long & 0xFF));

	/* short shutter 1 */
	shutter_short1  = shutter_short1 << 4;
	ov4689_write_reg(vdev, OV4689_M_EXPO_HSB, (u8)((shutter_short1 >> 16) & 0xF));
	ov4689_write_reg(vdev, OV4689_M_EXPO_MSB, (u8)(shutter_short1 >> 8));
	ov4689_write_reg(vdev, OV4689_M_EXPO_LSB, (u8)(shutter_short1 & 0xFF));

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		/* short shutter 2 */
		shutter_short2  = shutter_short2 << 4;
		ov4689_write_reg(vdev, OV4689_S_EXPO_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		ov4689_write_reg(vdev, OV4689_S_EXPO_MSB, (u8)(shutter_short2 >> 8));
		ov4689_write_reg(vdev, OV4689_S_EXPO_LSB, (u8)(shutter_short2 & 0xFF));

	}
	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("shutter long:%d, short1:%d, short2:%d\n", p_shutter_gp->l,
		p_shutter_gp->s1, p_shutter_gp->s2);

	return 0;
}

static int ov4689_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int ov4689_set_fps(struct vin_device *vdev, int fps)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u64 pixelclk, v_lines, vb_time;
	u32 factor;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->ori_line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	/* workaround for 1fps case */
	if (v_lines > 0xFFFF) {
		vin_debug("h_clks:%d, v_lines:%lld\n", pinfo->ori_line_length, v_lines);

		factor = ((u32)v_lines) / 0xFFFF + 1;
		v_lines = DIV64_CLOSEST(v_lines, factor);
		pinfo->line_length = pinfo->ori_line_length * factor;
		ov4689_write_reg2(vdev, OV4689_HTS_MSB, pinfo->line_length);
	} else if (pinfo->line_length != pinfo->ori_line_length) {
		pinfo->line_length = pinfo->ori_line_length;
		ov4689_write_reg2(vdev, OV4689_HTS_MSB, pinfo->line_length);
	}
	ov4689_write_reg2(vdev, OV4689_VTS_MSB, v_lines & 0xFFFF);

	pinfo->frame_length_lines = v_lines;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		v_lines = pinfo->frame_length_lines - vdev->cur_format->height;
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		v_lines = pinfo->frame_length_lines * 2 - vdev->cur_format->height;
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		v_lines = pinfo->frame_length_lines * 3 - vdev->cur_format->height;
	}

	vb_time = pinfo->line_length * (u64)v_lines * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov4689_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV4689_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV4689_GAIN_0DB);
		agc_idx = OV4689_GAIN_0DB;
	}

	agc_idx = OV4689_GAIN_0DB - agc_idx;

	ov4689_write_reg(vdev, OV4689_L_GAIN_MSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_REG3508]);
	ov4689_write_reg(vdev, OV4689_L_GAIN_LSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_REG3509]);
	/* WB-R */
	ov4689_write_reg(vdev, OV4689_L_WB_R_GAIN_MSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_R_GAIN_LSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_LSB]);
	/* WB-G */
	ov4689_write_reg(vdev, OV4689_L_WB_G_GAIN_MSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_G_GAIN_LSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_LSB]);
	/* WB-B */
	ov4689_write_reg(vdev, OV4689_L_WB_B_GAIN_MSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_B_GAIN_LSB, OV4689_GAIN_TABLE[agc_idx][OV4689_GAIN_COL_DGAIN_LSB]);

	return 0;
}

static int ov4689_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u32 again_index;

	/* long frame */
	again_index = OV4689_AGAIN_0DB - p_again_gp->l;
	ov4689_write_reg(vdev, OV4689_L_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);

	/* short frame 1 */
	again_index = OV4689_AGAIN_0DB - p_again_gp->s1;
	ov4689_write_reg(vdev, OV4689_M_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_M_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);

	/* short frame 2 */
	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		again_index = OV4689_AGAIN_0DB - p_again_gp->s2;
		ov4689_write_reg(vdev, OV4689_S_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
		ov4689_write_reg(vdev, OV4689_S_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);
	}
	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int ov4689_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov4689_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u32 dgain_index;

	/* long frame */
	dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->l;
	/* WB-R */
	ov4689_write_reg(vdev, OV4689_L_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	/* WB-G */
	ov4689_write_reg(vdev, OV4689_L_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	/* WB-B */
	ov4689_write_reg(vdev, OV4689_L_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_L_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);


	/* short frame 1 */
	dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->s1;
	/* WB-R */
	ov4689_write_reg(vdev, OV4689_M_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_M_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	/* WB-G */
	ov4689_write_reg(vdev, OV4689_M_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_M_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	/* WB-B */
	ov4689_write_reg(vdev, OV4689_M_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
	ov4689_write_reg(vdev, OV4689_M_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);


	/* short frame 2 */
	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->s2;
		/* WB-R */
		ov4689_write_reg(vdev, OV4689_S_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(vdev, OV4689_S_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(vdev, OV4689_S_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(vdev, OV4689_S_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(vdev, OV4689_S_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(vdev, OV4689_S_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	}
	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long dgain index:%d, short1 dgain index:%d, short2 dgain index:%d\n",
		p_dgain_gp->l, p_dgain_gp->s1, p_dgain_gp->s2);

	return 0;
}

static int ov4689_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov4689_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (!pinfo->line_length) {
		rval = ov4689_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov4689_get_line_time(vdev);
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

	/* short shutter 2 */
	exposure_lines = p_shutter2row->s2 * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->s2 = (u32)exposure_lines;

	return rval;
}

static int ov4689_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	int errCode = 0;
	u32 tmp_reg, bayer_pattern, vflip = 0, hflip = 0;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_VERTICALLY:
		vflip = OV4689_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_NONE:
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		vflip = OV4689_V_FLIP;
		hflip = OV4689_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		hflip = OV4689_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;
	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= ov4689_read_reg(vdev, OV4689_V_FORMAT, &tmp_reg);
	tmp_reg &= (~OV4689_V_FLIP);
	tmp_reg |= vflip;
	ov4689_write_reg(vdev, OV4689_V_FORMAT, tmp_reg);

	errCode |= ov4689_read_reg(vdev, OV4689_H_FORMAT, &tmp_reg);
	tmp_reg |= OV4689_H_MIRROR;
	tmp_reg ^= hflip;
	ov4689_write_reg(vdev, OV4689_H_FORMAT, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}

static int ov4689_get_chip_status(struct vin_device *vdev,
		struct vindev_chip_status *chip_status)
{
	u32 tmp_reg;

	ov4689_read_reg(vdev, OV4689_TPM_TRIGGER, &tmp_reg);
	tmp_reg |= 0x01;
	ov4689_write_reg(vdev, OV4689_TPM_TRIGGER, tmp_reg);
	/* wait some time to read temperature */
	msleep(5);
	ov4689_read_reg(vdev, OV4689_TPM_READ, &tmp_reg);

	chip_status->temperature = (tmp_reg & 0xFF) - OV4689_TPM_OFFSET;

	return 0;
}

static int ov4689_get_eis_info(struct vin_device *vdev,
	struct vindev_eisinfo *eis_info)
{
	eis_info->sensor_cell_width = 200;/* 2.0 um */
	eis_info->sensor_cell_height = 200;/* 2.0 um */
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;
	eis_info->vb_time = vdev->cur_format->vb_time;

	return 0;
}

static int ov4689_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 4;
	if (!pinfo->ll_mode) {
		aaa_info->sht1_max = pinfo->max_middle - 1;
		aaa_info->sht2_max = pinfo->max_short - 1;
	} else {
		aaa_info->sht1_max = OV4689_LL_MODE_M_MAX - 1;
		aaa_info->sht2_max = OV4689_LL_MODE_S_MAX - 1;
	}
	aaa_info->ll_mode = pinfo->ll_mode;

	return 0;
}

static int ov4689_set_low_light_mode(struct vin_device *vdev,
	u32 ll_mode)
{
	struct ov4689_priv *pinfo = (struct ov4689_priv *)vdev->priv;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if (ll_mode) {
			pinfo->ll_mode = 1;
			vdev->cur_format->hdr_short1_offset = OV4689_LL_MODE_M_MAX * 2 + 1;
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_MSB, (OV4689_LL_MODE_M_MAX >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_LSB, OV4689_LL_MODE_M_MAX & 0xFF);

		} else {
			pinfo->ll_mode = 0;
			vdev->cur_format->hdr_short1_offset = pinfo->max_middle * 2 + 1;
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_MSB, (pinfo->max_middle >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_LSB, pinfo->max_middle & 0xFF);
		}
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if (ll_mode) {
			pinfo->ll_mode = 1;
			vdev->cur_format->hdr_short1_offset = OV4689_LL_MODE_M_MAX * 3 + 1;
			vdev->cur_format->hdr_short2_offset = (OV4689_LL_MODE_M_MAX + OV4689_LL_MODE_S_MAX) * 3 + 2;
			ov4689_write_reg(vdev, OV4689_S_MAX_EXPO_MSB, (OV4689_LL_MODE_S_MAX >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_S_MAX_EXPO_LSB, OV4689_LL_MODE_S_MAX & 0xFF);
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_MSB, (OV4689_LL_MODE_M_MAX >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_LSB, OV4689_LL_MODE_M_MAX & 0xFF);
		} else {
			pinfo->ll_mode = 0;
			vdev->cur_format->hdr_short1_offset = pinfo->max_middle * 3 + 1;
			vdev->cur_format->hdr_short2_offset = (pinfo->max_middle + pinfo->max_short) * 3 + 2;
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_MSB, (pinfo->max_middle >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_M_MAX_EXPO_LSB, pinfo->max_middle & 0xFF);
			ov4689_write_reg(vdev, OV4689_S_MAX_EXPO_MSB, (pinfo->max_short >> 8) & 0xFF);
			ov4689_write_reg(vdev, OV4689_S_MAX_EXPO_LSB, pinfo->max_short & 0xFF);
		}
	} else {
		vin_error("Non HDR mode can't support this API: %s!\n", __func__);
		return -EPERM;
	}

	return 0;
}

#ifdef CONFIG_PM
static int ov4689_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ov4689_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int ov4689_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ov4689_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops ov4689_ops = {
	.init_device		= ov4689_init_device,
	.set_format		= ov4689_set_format,
	.set_shutter_row	= ov4689_set_shutter_row,
	.shutter2row		= ov4689_shutter2row,
	.set_frame_rate	= ov4689_set_fps,
	.set_agc_index		= ov4689_set_agc_index,
	.set_mirror_mode	= ov4689_set_mirror_mode,
	.read_reg			= ov4689_read_reg,
	.write_reg		= ov4689_write_reg,
	.set_hold_mode	= ov4689_set_hold_mode,
	.get_chip_status	= ov4689_get_chip_status,
	.get_eis_info		= ov4689_get_eis_info,
	.get_aaa_info		= ov4689_get_aaa_info,
	.set_low_light_mode	= ov4689_set_low_light_mode,
#ifdef CONFIG_PM
	.suspend		= ov4689_suspend,
	.resume			= ov4689_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = ov4689_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = ov4689_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = ov4689_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = ov4689_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = ov4689_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = ov4689_get_wdr_shutter_row_group,
	.wdr_shutter2row = ov4689_wdr_shutter2row,
};

/* ========================================================================== */
static int ov4689_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov4689_priv *ov4689;
	struct vin_video_format *formats;
	u32 num_formats;
	u32 intf_id;
	static u8 probe_num = 0;

	intf_id = (vinc_id >> (probe_num << 2)) & 0xF;

	if (intf_id == 0xF) /* vinc_id end */
		return 0;

	if (vinc_id > 0xF) { /* multi vin case, use dt ids to load driver automatically */
		client->addr = addr;
		strlcpy(client->name, "ov4689", I2C_NAME_SIZE);
	}

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV4689, sizeof(struct ov4689_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = intf_id;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_2688_1512;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x24000000;  /* 36dB */
	vdev->agc_db_min = 0x00000000;  /* 0dB */
	vdev->agc_db_step = 0x00180000; /* 0.09375dB */
	vdev->pixel_size = 0x00020000;  /* 2.0um */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = OV4689_AGAIN_ROWS - 1;
	vdev->wdr_dgain_idx_min = 0;
	vdev->wdr_dgain_idx_max = OV4689_DGAIN_ROWS - 1;

	i2c_set_clientdata(client, vdev);

	ov4689 = (struct ov4689_priv *)vdev->priv;
	ov4689->control_data = client;

	if (lane == 4) {
		formats = ov4689_4lane_formats;
		num_formats = ARRAY_SIZE(ov4689_4lane_formats);
	} else if (lane == 2) {
		formats = ov4689_2lane_formats;
		num_formats = ARRAY_SIZE(ov4689_2lane_formats);
	} else {
		vin_error("OV4689 can only support 2 or 4 lanes mipi\n");
		rval = -EINVAL;
		goto ov4689_probe_err;
	}

	rval = ambarella_vin_register_device(vdev, &ov4689_ops,
				formats, num_formats,
				ov4689_plls, ARRAY_SIZE(ov4689_plls));
	if (rval < 0)
		goto ov4689_probe_err;

	vin_info("OV4689 init(%d-lane mipi), [vinc:vsrc]%d:%d\n",
		lane, vdev->intf_id, vdev->vsrc_id);

	probe_num++;

	return 0;

ov4689_probe_err:
	ambarella_vin_free_device(vdev);
	strlcpy(client->name, AMBARELLA_I2C_VIN_FDT_NAME, I2C_NAME_SIZE);
	return rval;
}

static int ov4689_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);
	strlcpy(client->name, AMBARELLA_I2C_VIN_FDT_NAME, I2C_NAME_SIZE);

	return 0;
}

static const struct i2c_device_id ov4689_idtable[] = {
	{ "ov4689", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov4689_idtable);

#ifdef CONFIG_OF
static const struct of_device_id ov4689_dt_ids[] = {
	{ .compatible = "ambarella,ambvin", },
	{ }
};
MODULE_DEVICE_TABLE(of, ov4689_dt_ids);
#endif

static struct i2c_driver i2c_driver_ov4689 = {
	.driver = {
		.name	= "ov4689",
		.owner	= THIS_MODULE,
	},

	.id_table	= ov4689_idtable,
	.probe		= ov4689_probe,
	.remove		= ov4689_remove,
};

static int __init ov4689_init(void)
{
	int rval;

	if (vinc_id > 0xF) { /* multi vin case, use dt ids to load driver automatically */
		i2c_driver_ov4689.driver.of_match_table = of_match_ptr(ov4689_dt_ids);
	} else {
		rval = ambpriv_i2c_update_addr("ov4689", bus_id, addr);
		if (rval < 0)
			return rval;
	}

	rval = i2c_add_driver(&i2c_driver_ov4689);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov4689_exit(void)
{
	i2c_del_driver(&i2c_driver_ov4689);
}

module_init(ov4689_init);
module_exit(ov4689_exit);

MODULE_DESCRIPTION("OV4689 1/3 -Inch, 2688x1520, 4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");
