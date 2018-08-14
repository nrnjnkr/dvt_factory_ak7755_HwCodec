/*
 * Filename : ov2735_table.c
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

static struct vin_video_pll ov2735_plls[] = {
	{0, 24000000, 42000000},
};

static struct vin_video_format ov2735_formats[] = {
	{
		.video_mode = AMBA_VIDEO_MODE_1080P,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1920,
		.def_height = 1080,
		/* sensor mode */
		.device_mode = 0,
		.pll_idx = 0,
		.width = 1920,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_30,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode = AMBA_VIDEO_MODE_720P,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1280,
		.def_height = 720,
		/* sensor mode */
		.device_mode = 1,
		.pll_idx = 0,
		.width = 1280,
		.height = 720,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_60,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_RG,
	},
};

static struct vin_reg_8_8 ov2735_1080p_regs[] = {
	/* MIPI_1920x1080_2lane_30fps_420Mbps_24M-in_10bits, fpn optimization */
	{0xfd, 0x00},
	{0x20, 0x00},
	{0xfd, 0x00},
	{0x2f, 0x10},
	{0x34, 0x00},
	{0x30, 0x15},
	{0x33, 0x01},
	{0x35, 0x20},
	{0xfd, 0x01},
	{0x0d, 0x00},
	{0x30, 0x00},
	{0x03, 0x01},
	{0x04, 0x8f},
	{0x01, 0x01},
	{0x09, 0x00},
	{0x0a, 0x20},
	{0x06, 0x0a},
	{0x24, 0x10},
	{0x01, 0x01},
	{0xfb, 0x73},
	{0x01, 0x01},
	{0xfd, 0x01},
	{0x1a, 0x6b},
	{0x1c, 0xea},
	{0x16, 0x0c},
	{0x21, 0x00},
	{0x11, 0xe8},
	{0x19, 0xc3},
	{0x26, 0xda},
	{0x29, 0x01},
	{0x33, 0x6f},
	{0x2a, 0xd2},
	{0x2c, 0x40},
	{0xd0, 0x02},
	{0xd1, 0x01},
	{0xd2, 0x20},
	{0xd3, 0x03},
	{0xd4, 0xa4},
	{0x50, 0x00},
	{0x51, 0x2c},
	{0x52, 0x29},
	{0x53, 0x00},
	{0x55, 0x44},
	{0x58, 0x29},
	{0x5a, 0x00},
	{0x5b, 0x00},
	{0x5d, 0x00},
	{0x64, 0x2f},
	{0x66, 0x62},
	{0x68, 0x5b},
	{0x75, 0x46},
	{0x76, 0xf0},
	{0x77, 0x4f},
	{0x78, 0xef},
	{0x72, 0xcf},
	{0x73, 0x36},
	{0x7d, 0x0d},
	{0x7e, 0x0d},
	{0x8a, 0x77},
	{0x8b, 0x77},
	{0xfd, 0x01},
	{0xb1, 0x83}, /* DPHY enable 8b */
	{0xb3, 0x0b},
	{0xb4, 0x14}, /* MIPI PLL enable */
	{0x9d, 0x40}, /* mipi hs dc level */
	{0xa1, 0x05}, /* speed/03 */
	{0x94, 0x44}, /* dphy time */
	{0x95, 0x33}, /* dphy time */
	{0x96, 0x1f}, /* dphy time */
	{0x98, 0x45}, /* dphy time */
	{0x9c, 0x10}, /* dphy time */
	{0xb5, 0x70}, /* 30 */

	{0x25, 0xe0},
	{0x20, 0x7b},
	{0x8f, 0x88},
	{0x91, 0x40},

	{0xfd, 0x02},
	{0x5e, 0x03},
	{0xa1, 0x04},
	{0xa3, 0x40},
	{0xa5, 0x02},
	{0xa7, 0xc4},
	{0xfd, 0x01},
	{0x86, 0x77},
	{0x89, 0x77},
	{0x87, 0x74},
	{0x88, 0x74},
	{0xfc, 0xe0},
	{0xfe, 0xe0},
	{0xf0, 0x40},
	{0xf1, 0x40},
	{0xf2, 0x40},
	{0xf3, 0x40},

	/* crop to 1920x1080 */
	{0xfd, 0x02},
	{0xa0, 0x00},	/* Image vertical start MSB3bits */
	{0xa1, 0x08},	/* Image vertical start LSB8bits */
	{0xa2, 0x04},	/* image vertical size  MSB8bits */
	{0xa3, 0x38},	/* image vertical size  LSB8bits */
	{0xa4, 0x00},
	{0xa5, 0x04},	/* H start 8Lsb, keep center */
	{0xa6, 0x03},
	{0xa7, 0xc0},	/* Half H size Lsb8bits */
	{0xfd, 0x01},
	{0x8e, 0x07},
	{0x8f, 0x80},	/* MIPI column number */
	{0x90, 0x04},	/* MIPI row number */
	{0x91, 0x38},

	{0xfd, 0x03},
	{0xc0, 0x01},	/* enable transfer OTP BP information */
	{0xfd, 0x04},
	{0x21, 0x14},
	{0x22, 0x14},
	{0x23, 0x14},	/* enhance normal and dummy BPC */

	/* Default Mirror_On_Flip_On */
	{0xfd, 0x01},
	{0x3f, 0x03},
	{0xf8, 0x02},

	{0x01, 0x01},
	{0xfd, 0x02},
	{0x62, 0x48},
	{0x63, 0x04},
	{0xfd, 0x01},
	{0x06, 0xe0},	/* insert dummy line , the frame rate is 30.01 */
	{0x01, 0x01},
	{0xa0, 0x01},	/* MIPI enable, stream on */
};

static struct vin_reg_8_8 ov2735_720p_regs[] = {
	/* MIPI_1280x720_raw10_2lane_60fps */
	{0xfd, 0x00},
	{0x20, 0x00},
	{0xfd, 0x00},
	{0x2f, 0x10},
	{0x34, 0x00},
	{0x30, 0x15},
	{0x33, 0x01},
	{0x35, 0x20},
	{0xfd, 0x01},
	{0x31, 0x01},
	{0x0d, 0x00},
	{0x30, 0x00},
	{0x09, 0x00},
	{0x0a, 0x80},
	{0x06, 0x0a},
	{0x24, 0x10},
	{0x01, 0x01},
	{0xfb, 0x73},
	{0xfd, 0x01},
	{0x1a, 0x6b},
	{0x1c, 0xea},
	{0x16, 0x0c},
	{0x21, 0x00},
	{0x11, 0x56},
	{0x19, 0xc3},
	{0x25, 0xe0},
	{0x26, 0x5a},
	{0x29, 0x01},
	{0x33, 0x5f},
	{0x2a, 0xd2},
	{0x2c, 0x40},
	{0xd0, 0x02},
	{0xd1, 0x01},
	{0xd2, 0x20},
	{0xd3, 0x04},
	{0xd4, 0x2a},
	{0x50, 0x00},
	{0x51, 0x2c},
	{0x52, 0x29},
	{0x53, 0x00},
	{0x55, 0x46},
	{0x58, 0x29},
	{0x5a, 0x00},
	{0x5b, 0x00},
	{0x5d, 0x00},
	{0x64, 0x2f},
	{0x66, 0x62},
	{0x68, 0x5b},
	{0x75, 0x46},
	{0x76, 0x36},
	{0x77, 0x4f},
	{0x78, 0xef},
	{0x72, 0xbf},
	{0x73, 0x36},
	{0x7d, 0x0d},
	{0x7e, 0x0d},
	{0x8a, 0x77},
	{0x8b, 0x77},
	{0xfd, 0x01},
	{0xb1, 0x83},/* DPHY enable 8b */
	{0xb3, 0x0b},
	{0xb4, 0x14},/* MIPI PLL enable */
	{0x9d, 0x40},/* mipi hs dc level */
	{0xa1, 0x05},/* speed/03 */
	{0x94, 0x44},/* dphy time */
	{0x95, 0x33},/* dphy time */
	{0x96, 0x1f},/* dphy time */
	{0x98, 0x45},/* dphy time */
	{0x9c, 0x10},/* dphy time */
	{0xb5, 0x30},
	{0xa0, 0x01},/* mipi enable */

	{0xfd, 0x01},
	{0x86, 0x77},
	{0x89, 0x77},
	{0x87, 0x74},
	{0x88, 0x74},
	{0xfc, 0xe0},
	{0xfe, 0xe0},
	{0xf0, 0x40},
	{0xf1, 0x40},
	{0xf2, 0x40},
	{0xf3, 0x40},

	/* crop to 1280x720 */
	{0xfd, 0x02},
	{0xa0, 0x00},/* Image vertical start MSB3bits */
	{0xa1, 0x04},/* Image vertical start LSB8bits */
	{0xa2, 0x02},/* image vertical size  MSB8bits */
	{0xa3, 0xd0},/* image vertical size  LSB8bits */
	{0xa4, 0x00},
	{0xa5, 0x04},/* H start 8Lsb */
	{0xa6, 0x02},
	{0xa7, 0x80},/* Half H size Lsb8bits */
	{0xfd, 0x01},
	{0x8e, 0x05},
	{0x8f, 0x00},/* MIPI column number */
	{0x90, 0x02},
	{0x91, 0xd0},/* MIPI row number */

	/* Default Mirror_On_Flip_On */
	{0xfd, 0x01},
	{0x3f, 0x03},
	{0xf8, 0x02},

	{0xfd, 0x01},
	{0x0e, 0x02},
	{0x0f, 0xf0},
	{0x06, 0x18},/* insert dummy line */
	{0x01, 0x01},
};

/* Gain table */
/* OV2735 global gain table row size */
#define OV2735_GAIN_ROWS  		(256 + 1)
#define OV2735_GAIN_COLS  		(1)
#define OV2735_GAIN_MAXDB		(256)

#define OV2735_GAIN_COL_AGAIN	(0)
static const u8 OV2735_GAIN_TABLE[OV2735_GAIN_ROWS][OV2735_GAIN_COLS] = {
	{0x10}, /* index:0, gain:0.00000db -> x1.000000, again:x1.000000 */
	{0x10}, /* index:1, gain:0.09375db -> x1.010852, again:x1.010852 */
	{0x10}, /* index:2, gain:0.18750db -> x1.021821, again:x1.021821 */
	{0x10}, /* index:3, gain:0.28125db -> x1.032910, again:x1.032910 */
	{0x10}, /* index:4, gain:0.37500db -> x1.044119, again:x1.044119 */
	{0x10}, /* index:5, gain:0.46875db -> x1.055450, again:x1.055450 */
	{0x11}, /* index:6, gain:0.56250db -> x1.066903, again:x1.066903 */
	{0x11}, /* index:7, gain:0.65625db -> x1.078481, again:x1.078481 */
	{0x11}, /* index:8, gain:0.75000db -> x1.090184, again:x1.090184 */
	{0x11}, /* index:9, gain:0.84375db -> x1.102015, again:x1.102015 */
	{0x11}, /* index:10, gain:0.93750db -> x1.113974, again:x1.113974 */
	{0x12}, /* index:11, gain:1.03125db -> x1.126063, again:x1.126063 */
	{0x12}, /* index:12, gain:1.12500db -> x1.138282, again:x1.138282 */
	{0x12}, /* index:13, gain:1.21875db -> x1.150635, again:x1.150635 */
	{0x12}, /* index:14, gain:1.31250db -> x1.163121, again:x1.163121 */
	{0x12}, /* index:15, gain:1.40625db -> x1.175743, again:x1.175743 */
	{0x13}, /* index:16, gain:1.50000db -> x1.188502, again:x1.188502 */
	{0x13}, /* index:17, gain:1.59375db -> x1.201400, again:x1.201400 */
	{0x13}, /* index:18, gain:1.68750db -> x1.214437, again:x1.214437 */
	{0x13}, /* index:19, gain:1.78125db -> x1.227616, again:x1.227616 */
	{0x13}, /* index:20, gain:1.87500db -> x1.240938, again:x1.240938 */
	{0x14}, /* index:21, gain:1.96875db -> x1.254404, again:x1.254404 */
	{0x14}, /* index:22, gain:2.06250db -> x1.268017, again:x1.268017 */
	{0x14}, /* index:23, gain:2.15625db -> x1.281777, again:x1.281777 */
	{0x14}, /* index:24, gain:2.25000db -> x1.295687, again:x1.295687 */
	{0x14}, /* index:25, gain:2.34375db -> x1.309747, again:x1.309747 */
	{0x15}, /* index:26, gain:2.43750db -> x1.323960, again:x1.323960 */
	{0x15}, /* index:27, gain:2.53125db -> x1.338328, again:x1.338328 */
	{0x15}, /* index:28, gain:2.62500db -> x1.352851, again:x1.352851 */
	{0x15}, /* index:29, gain:2.71875db -> x1.367532, again:x1.367532 */
	{0x16}, /* index:30, gain:2.81250db -> x1.382372, again:x1.382372 */
	{0x16}, /* index:31, gain:2.90625db -> x1.397374, again:x1.397374 */
	{0x16}, /* index:32, gain:3.00000db -> x1.412538, again:x1.412538 */
	{0x16}, /* index:33, gain:3.09375db -> x1.427866, again:x1.427866 */
	{0x17}, /* index:34, gain:3.18750db -> x1.443361, again:x1.443361 */
	{0x17}, /* index:35, gain:3.28125db -> x1.459024, again:x1.459024 */
	{0x17}, /* index:36, gain:3.37500db -> x1.474857, again:x1.474857 */
	{0x17}, /* index:37, gain:3.46875db -> x1.490862, again:x1.490862 */
	{0x18}, /* index:38, gain:3.56250db -> x1.507041, again:x1.507041 */
	{0x18}, /* index:39, gain:3.65625db -> x1.523395, again:x1.523395 */
	{0x18}, /* index:40, gain:3.75000db -> x1.539927, again:x1.539927 */
	{0x18}, /* index:41, gain:3.84375db -> x1.556638, again:x1.556638 */
	{0x19}, /* index:42, gain:3.93750db -> x1.573530, again:x1.573530 */
	{0x19}, /* index:43, gain:4.03125db -> x1.590606, again:x1.590606 */
	{0x19}, /* index:44, gain:4.12500db -> x1.607867, again:x1.607867 */
	{0x1a}, /* index:45, gain:4.21875db -> x1.625315, again:x1.625315 */
	{0x1a}, /* index:46, gain:4.31250db -> x1.642952, again:x1.642952 */
	{0x1a}, /* index:47, gain:4.40625db -> x1.660782, again:x1.660782 */
	{0x1a}, /* index:48, gain:4.50000db -> x1.678804, again:x1.678804 */
	{0x1b}, /* index:49, gain:4.59375db -> x1.697022, again:x1.697022 */
	{0x1b}, /* index:50, gain:4.68750db -> x1.715438, again:x1.715438 */
	{0x1b}, /* index:51, gain:4.78125db -> x1.734054, again:x1.734054 */
	{0x1c}, /* index:52, gain:4.87500db -> x1.752871, again:x1.752871 */
	{0x1c}, /* index:53, gain:4.96875db -> x1.771893, again:x1.771893 */
	{0x1c}, /* index:54, gain:5.06250db -> x1.791121, again:x1.791121 */
	{0x1c}, /* index:55, gain:5.15625db -> x1.810558, again:x1.810558 */
	{0x1d}, /* index:56, gain:5.25000db -> x1.830206, again:x1.830206 */
	{0x1d}, /* index:57, gain:5.34375db -> x1.850067, again:x1.850067 */
	{0x1d}, /* index:58, gain:5.43750db -> x1.870144, again:x1.870144 */
	{0x1e}, /* index:59, gain:5.53125db -> x1.890438, again:x1.890438 */
	{0x1e}, /* index:60, gain:5.62500db -> x1.910953, again:x1.910953 */
	{0x1e}, /* index:61, gain:5.71875db -> x1.931690, again:x1.931690 */
	{0x1f}, /* index:62, gain:5.81250db -> x1.952653, again:x1.952653 */
	{0x1f}, /* index:63, gain:5.90625db -> x1.973842, again:x1.973842 */
	{0x1f}, /* index:64, gain:6.00000db -> x1.995262, again:x1.995262 */
	{0x20}, /* index:65, gain:6.09375db -> x2.016915, again:x2.016915 */
	{0x20}, /* index:66, gain:6.18750db -> x2.038802, again:x2.038802 */
	{0x20}, /* index:67, gain:6.28125db -> x2.060927, again:x2.060927 */
	{0x21}, /* index:68, gain:6.37500db -> x2.083291, again:x2.083291 */
	{0x21}, /* index:69, gain:6.46875db -> x2.105899, again:x2.105899 */
	{0x22}, /* index:70, gain:6.56250db -> x2.128752, again:x2.128752 */
	{0x22}, /* index:71, gain:6.65625db -> x2.151852, again:x2.151852 */
	{0x22}, /* index:72, gain:6.75000db -> x2.175204, again:x2.175204 */
	{0x23}, /* index:73, gain:6.84375db -> x2.198809, again:x2.198809 */
	{0x23}, /* index:74, gain:6.93750db -> x2.222670, again:x2.222670 */
	{0x23}, /* index:75, gain:7.03125db -> x2.246790, again:x2.246790 */
	{0x24}, /* index:76, gain:7.12500db -> x2.271172, again:x2.271172 */
	{0x24}, /* index:77, gain:7.21875db -> x2.295818, again:x2.295818 */
	{0x25}, /* index:78, gain:7.31250db -> x2.320732, again:x2.320732 */
	{0x25}, /* index:79, gain:7.40625db -> x2.345916, again:x2.345916 */
	{0x25}, /* index:80, gain:7.50000db -> x2.371374, again:x2.371374 */
	{0x26}, /* index:81, gain:7.59375db -> x2.397107, again:x2.397107 */
	{0x26}, /* index:82, gain:7.68750db -> x2.423120, again:x2.423120 */
	{0x27}, /* index:83, gain:7.78125db -> x2.449416, again:x2.449416 */
	{0x27}, /* index:84, gain:7.87500db -> x2.475996, again:x2.475996 */
	{0x28}, /* index:85, gain:7.96875db -> x2.502865, again:x2.502865 */
	{0x28}, /* index:86, gain:8.06250db -> x2.530026, again:x2.530026 */
	{0x28}, /* index:87, gain:8.15625db -> x2.557482, again:x2.557482 */
	{0x29}, /* index:88, gain:8.25000db -> x2.585235, again:x2.585235 */
	{0x29}, /* index:89, gain:8.34375db -> x2.613289, again:x2.613289 */
	{0x2a}, /* index:90, gain:8.43750db -> x2.641648, again:x2.641648 */
	{0x2a}, /* index:91, gain:8.53125db -> x2.670315, again:x2.670315 */
	{0x2b}, /* index:92, gain:8.62500db -> x2.699293, again:x2.699293 */
	{0x2b}, /* index:93, gain:8.71875db -> x2.728585, again:x2.728585 */
	{0x2c}, /* index:94, gain:8.81250db -> x2.758195, again:x2.758195 */
	{0x2c}, /* index:95, gain:8.90625db -> x2.788127, again:x2.788127 */
	{0x2d}, /* index:96, gain:9.00000db -> x2.818383, again:x2.818383 */
	{0x2d}, /* index:97, gain:9.09375db -> x2.848968, again:x2.848968 */
	{0x2e}, /* index:98, gain:9.18750db -> x2.879884, again:x2.879884 */
	{0x2e}, /* index:99, gain:9.28125db -> x2.911136, again:x2.911136 */
	{0x2f}, /* index:100, gain:9.37500db -> x2.942727, again:x2.942727 */
	{0x2f}, /* index:101, gain:9.46875db -> x2.974661, again:x2.974661 */
	{0x30}, /* index:102, gain:9.56250db -> x3.006942, again:x3.006942 */
	{0x30}, /* index:103, gain:9.65625db -> x3.039572, again:x3.039572 */
	{0x31}, /* index:104, gain:9.75000db -> x3.072557, again:x3.072557 */
	{0x31}, /* index:105, gain:9.84375db -> x3.105900, again:x3.105900 */
	{0x32}, /* index:106, gain:9.93750db -> x3.139605, again:x3.139605 */
	{0x32}, /* index:107, gain:10.03125db -> x3.173675, again:x3.173675 */
	{0x33}, /* index:108, gain:10.12500db -> x3.208116, again:x3.208116 */
	{0x33}, /* index:109, gain:10.21875db -> x3.242930, again:x3.242930 */
	{0x34}, /* index:110, gain:10.31250db -> x3.278121, again:x3.278121 */
	{0x35}, /* index:111, gain:10.40625db -> x3.313695, again:x3.313695 */
	{0x35}, /* index:112, gain:10.50000db -> x3.349654, again:x3.349654 */
	{0x36}, /* index:113, gain:10.59375db -> x3.386004, again:x3.386004 */
	{0x36}, /* index:114, gain:10.68750db -> x3.422749, again:x3.422749 */
	{0x37}, /* index:115, gain:10.78125db -> x3.459892, again:x3.459892 */
	{0x37}, /* index:116, gain:10.87500db -> x3.497438, again:x3.497438 */
	{0x38}, /* index:117, gain:10.96875db -> x3.535391, again:x3.535391 */
	{0x39}, /* index:118, gain:11.06250db -> x3.573757, again:x3.573757 */
	{0x39}, /* index:119, gain:11.15625db -> x3.612539, again:x3.612539 */
	{0x3a}, /* index:120, gain:11.25000db -> x3.651741, again:x3.651741 */
	{0x3b}, /* index:121, gain:11.34375db -> x3.691369, again:x3.691369 */
	{0x3b}, /* index:122, gain:11.43750db -> x3.731427, again:x3.731427 */
	{0x3c}, /* index:123, gain:11.53125db -> x3.771920, again:x3.771920 */
	{0x3d}, /* index:124, gain:11.62500db -> x3.812853, again:x3.812853 */
	{0x3d}, /* index:125, gain:11.71875db -> x3.854229, again:x3.854229 */
	{0x3e}, /* index:126, gain:11.81250db -> x3.896054, again:x3.896054 */
	{0x3f}, /* index:127, gain:11.90625db -> x3.938333, again:x3.938333 */
	{0x3f}, /* index:128, gain:12.00000db -> x3.981072, again:x3.981072 */
	{0x40}, /* index:129, gain:12.09375db -> x4.024274, again:x4.024274 */
	{0x41}, /* index:130, gain:12.18750db -> x4.067944, again:x4.067944 */
	{0x41}, /* index:131, gain:12.28125db -> x4.112089, again:x4.112089 */
	{0x42}, /* index:132, gain:12.37500db -> x4.156712, again:x4.156712 */
	{0x43}, /* index:133, gain:12.46875db -> x4.201821, again:x4.201821 */
	{0x43}, /* index:134, gain:12.56250db -> x4.247418, again:x4.247418 */
	{0x44}, /* index:135, gain:12.65625db -> x4.293510, again:x4.293510 */
	{0x45}, /* index:136, gain:12.75000db -> x4.340103, again:x4.340103 */
	{0x46}, /* index:137, gain:12.84375db -> x4.387200, again:x4.387200 */
	{0x46}, /* index:138, gain:12.93750db -> x4.434810, again:x4.434810 */
	{0x47}, /* index:139, gain:13.03125db -> x4.482936, again:x4.482936 */
	{0x48}, /* index:140, gain:13.12500db -> x4.531584, again:x4.531584 */
	{0x49}, /* index:141, gain:13.21875db -> x4.580759, again:x4.580759 */
	{0x4a}, /* index:142, gain:13.31250db -> x4.630469, again:x4.630469 */
	{0x4a}, /* index:143, gain:13.40625db -> x4.680719, again:x4.680719 */
	{0x4b}, /* index:144, gain:13.50000db -> x4.731513, again:x4.731513 */
	{0x4c}, /* index:145, gain:13.59375db -> x4.782858, again:x4.782858 */
	{0x4d}, /* index:146, gain:13.68750db -> x4.834761, again:x4.834761 */
	{0x4e}, /* index:147, gain:13.78125db -> x4.887227, again:x4.887227 */
	{0x4f}, /* index:148, gain:13.87500db -> x4.940262, again:x4.940262 */
	{0x4f}, /* index:149, gain:13.96875db -> x4.993873, again:x4.993873 */
	{0x50}, /* index:150, gain:14.06250db -> x5.048066, again:x5.048066 */
	{0x51}, /* index:151, gain:14.15625db -> x5.102846, again:x5.102846 */
	{0x52}, /* index:152, gain:14.25000db -> x5.158221, again:x5.158221 */
	{0x53}, /* index:153, gain:14.34375db -> x5.214198, again:x5.214198 */
	{0x54}, /* index:154, gain:14.43750db -> x5.270781, again:x5.270781 */
	{0x55}, /* index:155, gain:14.53125db -> x5.327979, again:x5.327979 */
	{0x56}, /* index:156, gain:14.62500db -> x5.385797, again:x5.385797 */
	{0x57}, /* index:157, gain:14.71875db -> x5.444243, again:x5.444243 */
	{0x58}, /* index:158, gain:14.81250db -> x5.503323, again:x5.503323 */
	{0x59}, /* index:159, gain:14.90625db -> x5.563044, again:x5.563044 */
	{0x59}, /* index:160, gain:15.00000db -> x5.623413, again:x5.623413 */
	{0x5a}, /* index:161, gain:15.09375db -> x5.684437, again:x5.684437 */
	{0x5b}, /* index:162, gain:15.18750db -> x5.746124, again:x5.746124 */
	{0x5c}, /* index:163, gain:15.28125db -> x5.808480, again:x5.808480 */
	{0x5d}, /* index:164, gain:15.37500db -> x5.871513, again:x5.871513 */
	{0x5e}, /* index:165, gain:15.46875db -> x5.935229, again:x5.935229 */
	{0x5f}, /* index:166, gain:15.56250db -> x5.999637, again:x5.999637 */
	{0x61}, /* index:167, gain:15.65625db -> x6.064744, again:x6.064744 */
	{0x62}, /* index:168, gain:15.75000db -> x6.130558, again:x6.130558 */
	{0x63}, /* index:169, gain:15.84375db -> x6.197086, again:x6.197086 */
	{0x64}, /* index:170, gain:15.93750db -> x6.264335, again:x6.264335 */
	{0x65}, /* index:171, gain:16.03125db -> x6.332315, again:x6.332315 */
	{0x66}, /* index:172, gain:16.12500db -> x6.401032, again:x6.401032 */
	{0x67}, /* index:173, gain:16.21875db -> x6.470495, again:x6.470495 */
	{0x68}, /* index:174, gain:16.31250db -> x6.540712, again:x6.540712 */
	{0x69}, /* index:175, gain:16.40625db -> x6.611690, again:x6.611690 */
	{0x6a}, /* index:176, gain:16.50000db -> x6.683439, again:x6.683439 */
	{0x6c}, /* index:177, gain:16.59375db -> x6.755966, again:x6.755966 */
	{0x6d}, /* index:178, gain:16.68750db -> x6.829282, again:x6.829282 */
	{0x6e}, /* index:179, gain:16.78125db -> x6.903392, again:x6.903392 */
	{0x6f}, /* index:180, gain:16.87500db -> x6.978306, again:x6.978306 */
	{0x70}, /* index:181, gain:16.96875db -> x7.054033, again:x7.054033 */
	{0x72}, /* index:182, gain:17.06250db -> x7.130582, again:x7.130582 */
	{0x73}, /* index:183, gain:17.15625db -> x7.207963, again:x7.207963 */
	{0x74}, /* index:184, gain:17.25000db -> x7.286182, again:x7.286182 */
	{0x75}, /* index:185, gain:17.34375db -> x7.365250, again:x7.365250 */
	{0x77}, /* index:186, gain:17.43750db -> x7.445176, again:x7.445176 */
	{0x78}, /* index:187, gain:17.53125db -> x7.525970, again:x7.525970 */
	{0x79}, /* index:188, gain:17.62500db -> x7.607641, again:x7.607641 */
	{0x7b}, /* index:189, gain:17.71875db -> x7.690198, again:x7.690198 */
	{0x7c}, /* index:190, gain:17.81250db -> x7.773650, again:x7.773650 */
	{0x7d}, /* index:191, gain:17.90625db -> x7.858008, again:x7.858008 */
	{0x7f}, /* index:192, gain:18.00000db -> x7.943282, again:x7.943282 */
	{0x80}, /* index:193, gain:18.09375db -> x8.029482, again:x8.029482 */
	{0x81}, /* index:194, gain:18.18750db -> x8.116616, again:x8.116616 */
	{0x83}, /* index:195, gain:18.28125db -> x8.204696, again:x8.204696 */
	{0x84}, /* index:196, gain:18.37500db -> x8.293732, again:x8.293732 */
	{0x86}, /* index:197, gain:18.46875db -> x8.383734, again:x8.383734 */
	{0x87}, /* index:198, gain:18.56250db -> x8.474713, again:x8.474713 */
	{0x89}, /* index:199, gain:18.65625db -> x8.566679, again:x8.566679 */
	{0x8a}, /* index:200, gain:18.75000db -> x8.659643, again:x8.659643 */
	{0x8c}, /* index:201, gain:18.84375db -> x8.753616, again:x8.753616 */
	{0x8d}, /* index:202, gain:18.93750db -> x8.848608, again:x8.848608 */
	{0x8f}, /* index:203, gain:19.03125db -> x8.944633, again:x8.944633 */
	{0x90}, /* index:204, gain:19.12500db -> x9.041698, again:x9.041698 */
	{0x92}, /* index:205, gain:19.21875db -> x9.139817, again:x9.139817 */
	{0x93}, /* index:206, gain:19.31250db -> x9.239000, again:x9.239000 */
	{0x95}, /* index:207, gain:19.40625db -> x9.339260, again:x9.339260 */
	{0x97}, /* index:208, gain:19.50000db -> x9.440609, again:x9.440609 */
	{0x98}, /* index:209, gain:19.59375db -> x9.543057, again:x9.543057 */
	{0x9a}, /* index:210, gain:19.68750db -> x9.646616, again:x9.646616 */
	{0x9c}, /* index:211, gain:19.78125db -> x9.751299, again:x9.751299 */
	{0x9d}, /* index:212, gain:19.87500db -> x9.857118, again:x9.857118 */
	{0x9f}, /* index:213, gain:19.96875db -> x9.964087, again:x9.964087 */
	{0xa1}, /* index:214, gain:20.06250db -> x10.072214, again:x10.072214 */
	{0xa2}, /* index:215, gain:20.15625db -> x10.181517, again:x10.181517 */
	{0xa4}, /* index:216, gain:20.25000db -> x10.292006, again:x10.292006 */
	{0xa6}, /* index:217, gain:20.34375db -> x10.403692, again:x10.403692 */
	{0xa8}, /* index:218, gain:20.43750db -> x10.516592, again:x10.516592 */
	{0xaa}, /* index:219, gain:20.53125db -> x10.630714, again:x10.630714 */
	{0xab}, /* index:220, gain:20.62500db -> x10.746078, again:x10.746078 */
	{0xad}, /* index:221, gain:20.71875db -> x10.862694, again:x10.862694 */
	{0xaf}, /* index:222, gain:20.81250db -> x10.980572, again:x10.980572 */
	{0xb1}, /* index:223, gain:20.90625db -> x11.099733, again:x11.099733 */
	{0xb3}, /* index:224, gain:21.00000db -> x11.220183, again:x11.220183 */
	{0xb5}, /* index:225, gain:21.09375db -> x11.341944, again:x11.341944 */
	{0xb7}, /* index:226, gain:21.18750db -> x11.465026, again:x11.465026 */
	{0xb9}, /* index:227, gain:21.28125db -> x11.589441, again:x11.589441 */
	{0xbb}, /* index:228, gain:21.37500db -> x11.715209, again:x11.715209 */
	{0xbd}, /* index:229, gain:21.46875db -> x11.842338, again:x11.842338 */
	{0xbf}, /* index:230, gain:21.56250db -> x11.970850, again:x11.970850 */
	{0xc1}, /* index:231, gain:21.65625db -> x12.100757, again:x12.100757 */
	{0xc3}, /* index:232, gain:21.75000db -> x12.232071, again:x12.232071 */
	{0xc5}, /* index:233, gain:21.84375db -> x12.364812, again:x12.364812 */
	{0xc7}, /* index:234, gain:21.93750db -> x12.498991, again:x12.498991 */
	{0xca}, /* index:235, gain:22.03125db -> x12.634629, again:x12.634629 */
	{0xcc}, /* index:236, gain:22.12500db -> x12.771739, again:x12.771739 */
	{0xce}, /* index:237, gain:22.21875db -> x12.910334, again:x12.910334 */
	{0xd0}, /* index:238, gain:22.31250db -> x13.050436, again:x13.050436 */
	{0xd3}, /* index:239, gain:22.40625db -> x13.192055, again:x13.192055 */
	{0xd5}, /* index:240, gain:22.50000db -> x13.335214, again:x13.335214 */
	{0xd7}, /* index:241, gain:22.59375db -> x13.479927, again:x13.479927 */
	{0xda}, /* index:242, gain:22.68750db -> x13.626207, again:x13.626207 */
	{0xdc}, /* index:243, gain:22.78125db -> x13.774078, again:x13.774078 */
	{0xde}, /* index:244, gain:22.87500db -> x13.923549, again:x13.923549 */
	{0xe1}, /* index:245, gain:22.96875db -> x14.074647, again:x14.074647 */
	{0xe3}, /* index:246, gain:23.06250db -> x14.227384, again:x14.227384 */
	{0xe6}, /* index:247, gain:23.15625db -> x14.381775, again:x14.381775 */
	{0xe8}, /* index:248, gain:23.25000db -> x14.537845, again:x14.537845 */
	{0xeb}, /* index:249, gain:23.34375db -> x14.695604, again:x14.695604 */
	{0xed}, /* index:250, gain:23.43750db -> x14.855080, again:x14.855080 */
	{0xf0}, /* index:251, gain:23.53125db -> x15.016287, again:x15.016287 */
	{0xf2}, /* index:252, gain:23.62500db -> x15.179238, again:x15.179238 */
	{0xf5}, /* index:253, gain:23.71875db -> x15.343962, again:x15.343962 */
	{0xf8}, /* index:254, gain:23.81250db -> x15.510470, again:x15.510470 */
	{0xfa}, /* index:255, gain:23.90625db -> x15.678788, again:x15.678788 */
	{0xfd}, /* index:256, gain:24.00000db -> x15.848934, again:x15.848934 */
};

