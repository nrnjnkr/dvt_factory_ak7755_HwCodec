/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185.c
 *
 * History:
 *    2015/01/09 - [Hao Zeng] Create
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
#include <linux/ambpriv_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx185.h"

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct imx185_priv {
	void *control_data;
	u32 frame_length_lines;
	u32 line_length;
};

#include "imx185_table.c"

static int imx185_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct imx185_priv *imx185;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	imx185 = (struct imx185_priv *)vdev->priv;
	client = imx185->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX185_SWRESET))
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

static int imx185_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct imx185_priv *imx185;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	imx185 = (struct imx185_priv *)vdev->priv;
	client = imx185->control_data;

	pbuf0[0] = (subaddr &0xff00) >> 8;
	pbuf0[1] = subaddr & 0xff;

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 1;

	rval = i2c_transfer(client->adapter, msgs, 2);
	if (rval < 0){
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int imx185_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx185_config;

	memset(&imx185_config, 0, sizeof (imx185_config));

	imx185_config.interface_type = SENSOR_SERIAL_LVDS;
	imx185_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	imx185_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	imx185_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;

	imx185_config.cap_win.x = format->def_start_x;
	imx185_config.cap_win.y = format->def_start_y;
	imx185_config.cap_win.width = format->def_width;
	imx185_config.cap_win.height = format->def_height;

	imx185_config.sensor_id = GENERIC_SENSOR;
	imx185_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx185_config.bayer_pattern = format->bayer_pattern;
	imx185_config.video_format = format->format;
	imx185_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx185_config);
}

static void imx185_sw_reset(struct vin_device *vdev)
{
	imx185_write_reg(vdev, IMX185_STANDBY, 0x01);	/* STANDBY */
	imx185_write_reg(vdev, IMX185_SWRESET, 0x01);
	msleep(10);
}

static void imx185_start_streaming(struct vin_device *vdev)
{
	imx185_write_reg(vdev, IMX185_XMSTA, 0x00);	/* master mode start */
	imx185_write_reg(vdev, IMX185_STANDBY, 0x00);	/* cancel standby */
}

static int imx185_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	imx185_sw_reset(vdev);

	regs = imx185_share_regs;
	regs_num = ARRAY_SIZE(imx185_share_regs);

	for (i = 0; i < regs_num; i++)
		imx185_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx185_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx185_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx185_pll_regs[pll_idx]);

	for (i = 0; i < regs_num; i++)
		imx185_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx185_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx185_priv *pinfo = (struct imx185_priv *)vdev->priv;

	imx185_read_reg(vdev, IMX185_HMAX_MSB, &val_high);
	imx185_read_reg(vdev, IMX185_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx185_read_reg(vdev, IMX185_VMAX_HSB, &val_high);
	imx185_read_reg(vdev, IMX185_VMAX_MSB, &val_mid);
	imx185_read_reg(vdev, IMX185_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx185_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx185_priv *pinfo = (struct imx185_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx185_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx185_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx185_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		imx185_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = imx185_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx185_get_line_time(vdev);

	/* TG reset release ( Enable Streaming ) */
	imx185_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx185_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx185_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx185_priv *pinfo = (struct imx185_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 1) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 1;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line - 1;
	imx185_write_reg(vdev, IMX185_SHS1_HSB, blank_lines >> 16);
	imx185_write_reg(vdev, IMX185_SHS1_MSB, blank_lines >> 8);
	imx185_write_reg(vdev, IMX185_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx185_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx185_priv *pinfo = (struct imx185_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx185_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx185_set_fps( struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct imx185_priv *pinfo = (struct imx185_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx185_write_reg(vdev, IMX185_VMAX_HSB, (v_lines >> 16) & 0x01);
	imx185_write_reg(vdev, IMX185_VMAX_MSB, (v_lines >> 8) & 0xFF);
	imx185_write_reg(vdev, IMX185_VMAX_LSB, v_lines & 0xFF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx185_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX185_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX185_GAIN_MAX_DB);
		agc_idx = IMX185_GAIN_MAX_DB;
	}

	imx185_write_reg(vdev, IMX185_AGAIN, (u8)(agc_idx&0xFF));

	return 0;
}

static int imx185_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX185_H_MIRROR | IMX185_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX185_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX185_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx185_read_reg(vdev, IMX185_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX185_H_MIRROR | IMX185_V_FLIP);
	tmp_reg |= readmode;
	imx185_write_reg(vdev, IMX185_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops imx185_ops = {
	.init_device		= imx185_init_device,
	.set_format		= imx185_set_format,
	.set_pll			= imx185_set_pll,
	.set_shutter_row	= imx185_set_shutter_row,
	.shutter2row		= imx185_shutter2row,
	.set_frame_rate	= imx185_set_fps,
	.set_agc_index		= imx185_set_agc_index,
	.set_mirror_mode	= imx185_set_mirror_mode,
	.read_reg		= imx185_read_reg,
	.write_reg		= imx185_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int imx185_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx185_priv *imx185;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_IMX185, sizeof(struct imx185_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	/*  48dB */
	vdev->agc_db_min = 0x00000000;	/*  0dB */
	vdev->agc_db_step = 0x004CCCCC;	/*  0.3dB */

	i2c_set_clientdata(client, vdev);

	imx185 = (struct imx185_priv *)vdev->priv;
	imx185->control_data = client;

	rval = ambarella_vin_register_device(vdev, &imx185_ops,
		imx185_formats, ARRAY_SIZE(imx185_formats),
		imx185_plls, ARRAY_SIZE(imx185_plls));

	if (rval < 0)
		goto imx185_probe_err;

	vin_info("IMX185 init(4-lane lvds)\n");

	return 0;

imx185_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx185_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx185_idtable[] = {
	{ "imx185", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx185_idtable);

static struct i2c_driver i2c_driver_imx185 = {
	.driver = {
		.name	= "imx185",
	},

	.id_table	= imx185_idtable,
	.probe		= imx185_probe,
	.remove		= imx185_remove,

};

static int __init imx185_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx185", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx185);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit imx185_exit(void)
{
	i2c_del_driver(&i2c_driver_imx185);
}

module_init(imx185_init);
module_exit(imx185_exit);

MODULE_DESCRIPTION("IMX185 1/1.9 -Inch, 1920x1200, 2.31-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

