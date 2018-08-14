/*
 * Filename : ov2735.c
 *
 * History:
 *    2017/01/03 - [Hao Zeng] created file
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
#include "ov2735.h"
#include "ov2735_table.c"

static int bus_addr = (0 << 16) | (0x78 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ov2735_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
	u32 vblank;
	u32 vts;
};

static int ov2735_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2735_priv *ov2735;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	ov2735 = (struct ov2735_priv *)vdev->priv;
	client = ov2735->control_data;

	pbuf[0] = (subaddr & 0xff);
	pbuf[1] = data;

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == OV2735_SWRESET))
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

static int ov2735_write_reg2(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2735_priv *ov2735;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov2735 = (struct ov2735_priv *)vdev->priv;
	client = ov2735->control_data;

	pbuf[0] = (subaddr & 0xff);
	pbuf[1] = (data >> 8) & 0xff;
	pbuf[2] = (data & 0xff);

	msgs[0].len = 3;
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

static int ov2735_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov2735_priv *ov2735;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[1];
	u8 pbuf[1];

	ov2735 = (struct ov2735_priv *)vdev->priv;
	client = ov2735->control_data;

	pbuf0[0] = (subaddr & 0xff);

	msgs[0].len = 1;
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

static int ov2735_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov2735_config;

	memset(&ov2735_config, 0, sizeof(ov2735_config));

	ov2735_config.interface_type = SENSOR_MIPI;
	ov2735_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov2735_config.mipi_cfg.lane_number = SENSOR_2_LANE;

	ov2735_config.cap_win.x = format->def_start_x;
	ov2735_config.cap_win.y = format->def_start_y;
	ov2735_config.cap_win.width = format->def_width;
	ov2735_config.cap_win.height = format->def_height;

	ov2735_config.sensor_id	= GENERIC_SENSOR;
	ov2735_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov2735_config.bayer_pattern	= format->bayer_pattern;
	ov2735_config.video_format	= format->format;
	ov2735_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov2735_config);
}

static void ov2735_sw_reset(struct vin_device *vdev)
{
	ov2735_write_reg(vdev, OV2735_PAGE_FLG, 0x00);
	ov2735_write_reg(vdev, OV2735_SWRESET, 0x00);/* Software reset */
	msleep(3);
}

static int ov2735_init_device(struct vin_device *vdev)
{
	ov2735_sw_reset(vdev);
	return 0;
}

static void ov2735_start_streaming(struct vin_device *vdev)
{
	ov2735_write_reg(vdev, OV2735_FR_SYNC_EN, 0x01);
	/* wait some time for register effective */
	msleep(30);
}

static int ov2735_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov2735_priv *pinfo = (struct ov2735_priv *)vdev->priv;

	ov2735_write_reg(vdev, OV2735_PAGE_FLG, 0x01);

	ov2735_read_reg(vdev, OV2735_HTS_MSB, &val_high);
	ov2735_read_reg(vdev, OV2735_HTS_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov2735_read_reg(vdev, OV2735_VTS_MSB, &val_high);
	ov2735_read_reg(vdev, OV2735_VTS_LSB, &val_low);
	pinfo->vts = (val_high << 8) + val_low;

	ov2735_read_reg(vdev, OV2735_FR_LENGTH_MSB, &val_high);
	ov2735_read_reg(vdev, OV2735_FR_LENGTH_LSB, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	ov2735_read_reg(vdev, OV2735_VB_MSB, &val_high);
	ov2735_read_reg(vdev, OV2735_VB_LSB, &val_low);
	pinfo->vblank = (val_high << 8) + val_low;

	vin_debug("hts:%d, vts:%d, frame_length_lines:%d, vblank:%d\n",
		pinfo->line_length, pinfo->vts, pinfo->frame_length_lines, pinfo->vblank);

	return 0;
}

static int ov2735_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov2735_priv *pinfo = (struct ov2735_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov2735_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_8_8 *regs;
	int i, regs_num;

	switch (format->video_mode) {
	case AMBA_VIDEO_MODE_1080P:
		regs = ov2735_1080p_regs;
		regs_num = ARRAY_SIZE(ov2735_1080p_regs);
		break;
	case AMBA_VIDEO_MODE_720P:
		regs = ov2735_720p_regs;
		regs_num = ARRAY_SIZE(ov2735_720p_regs);
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_error("Unknown mode\n");
		return -EINVAL;
	}
	for (i = 0; i < regs_num; i++) {
		ov2735_write_reg(vdev, regs[i].addr, regs[i].data);
		if (unlikely(regs[i].addr == OV2735_SWRESET))
			msleep(3);
	}

	/* Enable Streaming */
	ov2735_start_streaming(vdev);

	rval = ov2735_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov2735_get_line_time(vdev);

	/* communicate with IAV */
	rval = ov2735_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov2735_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	if (hold_mode)
		ov2735_write_reg(vdev, OV2735_PAGE_FLG, 0x01);
	else
		ov2735_write_reg(vdev, OV2735_FR_SYNC_EN, 0x01);

	return 0;
}

static int ov2735_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int rval = 0;
	struct ov2735_priv *pinfo = (struct ov2735_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 2 ~ (Frame format(V) - 4) */
	min_line = 2;
	max_line = pinfo->vts - 4;
	num_line = clamp(num_line, min_line, max_line);

	ov2735_write_reg2(vdev, OV2735_EXPO_MSB, num_line & 0xFFFF);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return rval;
}

static int ov2735_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov2735_priv *pinfo = (struct ov2735_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (unlikely(!pinfo->line_length)) {
		rval = ov2735_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov2735_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	u32 vb_lines;
	struct ov2735_priv *pinfo = (struct ov2735_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	if (unlikely(v_lines < pinfo->frame_length_lines + 1)) {
		vin_error("VTS:%lld should be larger than frame_length_lines:%d\n",
			v_lines, pinfo->frame_length_lines);
		return -EPERM;
	} else {
		vb_lines = v_lines - pinfo->frame_length_lines - 1;
	}

	ov2735_write_reg(vdev, OV2735_PAGE_FLG, 0x01);
	ov2735_write_reg2(vdev, OV2735_VB_MSB, vb_lines & 0xFFFF);
	ov2735_write_reg(vdev, OV2735_FR_SYNC_EN, 0x01);

	pinfo->vts = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov2735_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV2735_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV2735_GAIN_MAXDB);
		agc_idx = OV2735_GAIN_MAXDB;
	}

	/* Analog Gain */
	ov2735_write_reg(vdev, OV2735_AGAIN_LSB, OV2735_GAIN_TABLE[agc_idx][OV2735_GAIN_COL_AGAIN]);

	return 0;
}

static int ov2735_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	int rval = 0;
	u32 tmp_reg, bayer_pattern, readmode = 0;
	u32 reg_f8;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		reg_f8 = 0x02;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = OV2735_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		reg_f8 = 0x00;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = OV2735_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		reg_f8 = 0x00;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = OV2735_V_FLIP + OV2735_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		reg_f8 = 0x02;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov2735_write_reg(vdev, OV2735_PAGE_FLG, 0x01);
	ov2735_read_reg(vdev, OV2735_UPDOWN_MIRROR, &tmp_reg);
	tmp_reg |= OV2735_MIRROR_MASK;
	tmp_reg ^= readmode;
	ov2735_write_reg(vdev, OV2735_UPDOWN_MIRROR, tmp_reg);
	ov2735_write_reg(vdev, 0xf8, reg_f8);
	ov2735_write_reg(vdev, OV2735_FR_SYNC_EN, 0x01);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return rval;
}

static struct vin_ops ov2735_ops = {
	.init_device		= ov2735_init_device,
	.set_format		= ov2735_set_format,
	.set_shutter_row	= ov2735_set_shutter_row,
	.shutter2row		= ov2735_shutter2row,
	.set_frame_rate	= ov2735_set_fps,
	.set_agc_index		= ov2735_set_agc_index,
	.set_mirror_mode	= ov2735_set_mirror_mode,
	.set_hold_mode		= ov2735_set_hold_mode,
	.shutter2row		= ov2735_shutter2row,
	.read_reg		= ov2735_read_reg,
	.write_reg		= ov2735_write_reg,
};

/* ========================================================================== */
static int ov2735_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov2735_priv *ov2735;
	u32 cid_l, cid_h;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_OV2735, sizeof(struct ov2735_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x18000000;  /* 24dB */
	vdev->agc_db_min = 0x00000000;  /* 0dB */
	vdev->agc_db_step = 0x00180000; /* 0.09375dB */

	i2c_set_clientdata(client, vdev);

	ov2735 = (struct ov2735_priv *)vdev->priv;
	ov2735->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ov2735_ops,
		ov2735_formats, ARRAY_SIZE(ov2735_formats),
		ov2735_plls, ARRAY_SIZE(ov2735_plls));
	if (rval < 0)
		goto ov2735_probe_err;

	/* query sensor id and revision */
	ov2735_read_reg(vdev, OV2735_CHIP_ID_H, &cid_h);
	ov2735_read_reg(vdev, OV2735_CHIP_ID_L, &cid_l);
	vin_info("OV2735 init(2-lane mipi), Chip ID:0x%x\n", (cid_h<<8)+cid_l);

	return 0;

ov2735_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov2735_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov2735_idtable[] = {
	{ "ov2735", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2735_idtable);

static struct i2c_driver i2c_driver_ov2735 = {
	.driver = {
		.name	= "ov2735",
	},

	.id_table	= ov2735_idtable,
	.probe		= ov2735_probe,
	.remove		= ov2735_remove,

};

static int __init ov2735_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov2735", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov2735);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov2735_exit(void)
{
	i2c_del_driver(&i2c_driver_ov2735);
}

module_init(ov2735_init);
module_exit(ov2735_exit);

MODULE_DESCRIPTION("OV2735 1/2.7-Inch, 1920x1080, 2-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

