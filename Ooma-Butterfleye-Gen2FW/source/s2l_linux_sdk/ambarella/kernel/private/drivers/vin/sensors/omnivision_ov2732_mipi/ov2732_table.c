/*
 * Filename : ov2732_table.c
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

static struct vin_video_pll ov2732_plls[] = {
	/* for linear mode */
	{0, 24000000, 45000000},
	/* for hdr mode */
	{0, 24000000, 22500000},
};

static struct vin_reg_16_8 ov2732_mode_regs[][16] = {
	{	/* 1080p linear */
		{0x3800, 0x00},
		{0x3801, 0x00},
		{0x3802, 0x00},
		{0x3803, 0x04},
		{0x3804, 0x07},
		{0x3805, 0x8f},
		{0x3806, 0x04},
		{0x3807, 0x43},
		{0x3808, 0x07},
		{0x3809, 0x80},
		{0x380a, 0x04},
		{0x380b, 0x38},
		{0x380c, 0x02},
		{0x380d, 0x78},
		{0x380e, 0x04},
		{0x380f, 0xa0},
	},
	{	/* 720p linear */
		{0x3800, 0x01},
		{0x3801, 0x40},
		{0x3802, 0x00},
		{0x3803, 0xb8},
		{0x3804, 0x06},
		{0x3805, 0x4f},
		{0x3806, 0x03},
		{0x3807, 0x8f},
		{0x3808, 0x05},
		{0x3809, 0x00},
		{0x380a, 0x02},
		{0x380b, 0xd0},
		{0x380c, 0x02},
		{0x380d, 0x78},
		{0x380e, 0x03},
		{0x380f, 0x16},
	},
	{	/* 1080p hdr */
		{0x3800, 0x00},
		{0x3801, 0x00},
		{0x3802, 0x00},
		{0x3803, 0x04},
		{0x3804, 0x07},
		{0x3805, 0x8f},
		{0x3806, 0x04},
		{0x3807, 0x43},
		{0x3808, 0x07},
		{0x3809, 0x80},
		{0x380a, 0x04},
		{0x380b, 0x38},
		{0x380c, 0x02},
		{0x380d, 0x78},
		{0x380e, 0x05},
		{0x380f, 0xa0},
	},
	{	/* 720p hdr */
		{0x3800, 0x01},
		{0x3801, 0x40},
		{0x3802, 0x00},
		{0x3803, 0xb8},
		{0x3804, 0x06},
		{0x3805, 0x4f},
		{0x3806, 0x03},
		{0x3807, 0x8f},
		{0x3808, 0x05},
		{0x3809, 0x00},
		{0x380a, 0x02},
		{0x380b, 0xd0},
		{0x380c, 0x02},
		{0x380d, 0x78},
		{0x380e, 0x05},
		{0x380f, 0xa0},
	},
};

static struct vin_video_format ov2732_formats[] = {
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
		.max_fps = AMBA_VIDEO_FPS_60,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
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
		.max_fps = AMBA_VIDEO_FPS(90),
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
	},
	{
		.video_mode = AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= (1080 + (S_MAX_EXPO_1080P>>1)) * 2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 2,
		.pll_idx	= 1,
		.width		= 1920 * 2,
		.height		= 1080 + S_MAX_EXPO_1080P,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_25,
		.default_fps	= AMBA_VIDEO_FPS_25,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (S_MAX_EXPO_1080P>>1) * 2 + 1, /* 2 x S_MAX_EXPO + 1 */
	},
	{
		.video_mode = AMBA_VIDEO_MODE_720P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= (720 + (S_MAX_EXPO_720P>>1)) * 2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 720,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 3,
		.pll_idx	= 1,
		.width		= 1280 * 2,
		.height		= 720 + S_MAX_EXPO_1080P,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (S_MAX_EXPO_720P>>1) * 2 + 1, /* 2 x S_MAX_EXPO + 1 */
	},
};

static struct vin_reg_16_8 ov2732_linear_mode_regs[] = {
	{0x0103, 0x01},
	{0x0305, 0x3c},
	{0x0307, 0x00},
	{0x0308, 0x03},
	{0x0309, 0x03},
	{0x0327, 0x07},
	{0x3016, 0x32},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x3002, 0x00},
	{0x3013, 0x00},
	{0x301f, 0xf0},
	{0x3023, 0xf0},
	{0x3020, 0x9b},
	{0x3022, 0x51},
	{0x3106, 0x11},
	{0x3107, 0x01},
	{0x3500, 0x00},
	{0x3501, 0x40},
	{0x3502, 0x00},
	{0x3503, 0x88},
	{0x3505, 0x83},
	{0x3508, 0x01},
	{0x3509, 0x80},
	{0x350a, 0x04},
	{0x350b, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3510, 0x00},
	{0x3511, 0x00},
	{0x3512, 0x20},
	{0x3600, 0x55},
	{0x3601, 0x54},
	{0x3612, 0xb5},
	{0x3613, 0xb3},
	{0x3616, 0x83},
	{0x3621, 0x00},
	{0x3624, 0x06},
	{0x3642, 0x88},
	{0x3660, 0x00},
	{0x3661, 0x00},
	{0x366a, 0x64},
	{0x366c, 0x00},
	{0x366e, 0x44},
	{0x366f, 0x4f},
	{0x3677, 0x11},
	{0x3678, 0x11},
	{0x3680, 0xff},
	{0x3681, 0xd2},
	{0x3682, 0xa9},
	{0x3683, 0x91},
	{0x3684, 0x8a},
	{0x3620, 0x80},
	{0x3662, 0x10},
	{0x3663, 0x24},
	{0x3665, 0xa0},
	{0x3667, 0xa6},
	{0x3674, 0x01},
	{0x373d, 0x24},
	{0x3741, 0x28},
	{0x3743, 0x28},
	{0x3745, 0x28},
	{0x3747, 0x28},
	{0x3748, 0x00},
	{0x3749, 0x78},
	{0x374a, 0x00},
	{0x374b, 0x78},
	{0x374c, 0x00},
	{0x374d, 0x78},
	{0x374e, 0x00},
	{0x374f, 0x78},
	{0x3766, 0x12},
	{0x37e0, 0x00},
	{0x37e6, 0x04},
	{0x37e5, 0x04},
	{0x37e1, 0x04},
	{0x3737, 0x04},
	{0x37d0, 0x0a},
	{0x37d8, 0x04},
	{0x37e2, 0x08},
	{0x3739, 0x10},
	{0x37e4, 0x18},
	{0x37e3, 0x04},
	{0x37d9, 0x10},
	{0x4040, 0x04},
	{0x4041, 0x0f},
	{0x4008, 0x00},
	{0x4009, 0x0d},
	{0x37a1, 0x14},
	{0x37a8, 0x16},
	{0x37ab, 0x10},
	{0x37c2, 0x04},
	{0x3705, 0x00},
	{0x3706, 0x28},
	{0x370a, 0x00},
	{0x370b, 0x78},
	{0x3714, 0x24},
	{0x371a, 0x1e},
	{0x372a, 0x03},
	{0x3756, 0x00},
	{0x3757, 0x0e},
	{0x377b, 0x00},
	{0x377c, 0x0c},
	{0x377d, 0x20},
	{0x3790, 0x28},
	{0x3791, 0x78},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x07},
	{0x3805, 0x8f},
	{0x3806, 0x04},
	{0x3807, 0x43},
	{0x3808, 0x07},
	{0x3809, 0x80},
	{0x380a, 0x04},
	{0x380b, 0x38},
	{0x380c, 0x02},
	{0x380d, 0x78},
	{0x380e, 0x04},
	{0x380f, 0xa0},
	{0x3811, 0x08},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x01},
	{0x3817, 0x01},
	{0x381d, 0x40},
	{0x381e, 0x02},
	{0x3820, 0x88},
	{0x3821, 0x00},
	{0x3822, 0x04},
	{0x3835, 0x00},
	{0x4303, 0x19},
	{0x4304, 0x19},
	{0x4305, 0x03},
	{0x4306, 0x81},
	{0x4503, 0x00},
	{0x4508, 0x14},
	{0x450a, 0x00},
	{0x450b, 0x40},
	{0x4833, 0x08},
	{0x5000, 0xa9},
	{0x5001, 0x09},
	{0x3b00, 0x00},
	{0x3b02, 0x00},
	{0x3b03, 0x00},
	{0x3c80, 0x08},
	{0x3c82, 0x00},
	{0x3c83, 0xb1},
	{0x3c87, 0x08},
	{0x3c8c, 0x10},
	{0x3c8d, 0x00},
	{0x3c90, 0x00},
	{0x3c91, 0x00},
	{0x3c92, 0x00},
	{0x3c93, 0x00},
	{0x3c94, 0x00},
	{0x3c95, 0x00},
	{0x3c96, 0x00},
	{0x3c97, 0x00},
	{0x3c98, 0x00},
	{0x4000, 0xf3},
	{0x4001, 0x60},
	{0x4002, 0x00},
	{0x4003, 0x40},
	{0x4090, 0x14},
	{0x4601, 0x10},
	{0x4701, 0x00},
	{0x4708, 0x09},
	{0x470a, 0x00},
	{0x470b, 0x40},
	{0x470c, 0x81},
	{0x480c, 0x12},
	{0x4710, 0x06},
	{0x4711, 0x00},
	{0x4837, 0x12},
	{0x4800, 0x00},
	{0x4c01, 0x00},
	{0x5036, 0x00},
	{0x5037, 0x00},
	{0x580b, 0x0f},
	{0x4903, 0x80},
	{0x4003, 0x40},
	{0x5000, 0xf9},
	{0x5200, 0x1b},
	{0x4837, 0x16},
	{0x3500, 0x00},
	{0x3501, 0x49},
	{0x3502, 0x80},
	{0x3508, 0x02},
	{0x3509, 0x80},
	{0x3d8c, 0x11},
	{0x3d8d, 0xf0},
	{0x5180, 0x00},
	{0x5181, 0x10},
	{0x36a0, 0x16},
	{0x36a1, 0x50},
	{0x36a2, 0x60},
	{0x36a3, 0x80},
	{0x36a4, 0x00},
	{0x36a5, 0xa0},
	{0x36a6, 0x00},
	{0x36a7, 0x50},
	{0x36a8, 0x00},
	{0x36a9, 0x50},
	{0x36aa, 0x00},
	{0x36ab, 0x50},
	{0x36ac, 0x00},
	{0x36ad, 0x50},
	{0x36ae, 0x00},
	{0x36af, 0x50},
	{0x36b0, 0x00},
	{0x36b1, 0x50},
	{0x36b2, 0x00},
	{0x36b3, 0x50},
	{0x36b4, 0x00},
	{0x36b5, 0x50},
	{0x36b9, 0xee},
	{0x36ba, 0xee},
	{0x36bb, 0xee},
	{0x36bc, 0xee},
	{0x36bd, 0x0e},
	{0x36b6, 0x08},
	{0x36b7, 0x08},
	{0x36b8, 0x10},
	{0x0100, 0x01},
};

static struct vin_reg_16_8 ov2732_hdr_mode_regs[] = {
	{0x0103, 0x01},
	{0x0305, 0x3c},
	{0x0307, 0x00},
	{0x0308, 0x03},
	{0x0309, 0x03},
	{0x0327, 0x07},
	{0x3016, 0x32},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x3002, 0x00},
	{0x3013, 0x00},
	{0x301f, 0xf0},
	{0x3023, 0xf0},
	{0x3020, 0x9b},
	{0x3022, 0x51},
	{0x3106, 0x11},
	{0x3107, 0x01},
	{0x3500, 0x00},
	{0x3501, 0x40},
	{0x3502, 0x00},
	{0x3503, 0x88},
	{0x3505, 0x83},
	{0x3508, 0x01},
	{0x3509, 0x80},
	{0x350a, 0x04},
	{0x350b, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x350e, 0x04},
	{0x350f, 0x00},
	{0x3510, 0x00},
	{0x3511, 0x00},
	{0x3512, 0x20},
	{0x3600, 0x55},
	{0x3601, 0x54},
	{0x3612, 0xb5},
	{0x3613, 0xb3},
	{0x3616, 0x83},
	{0x3621, 0x00},
	{0x3624, 0x06},
	{0x3642, 0x88},
	{0x3660, 0x00},
	{0x3661, 0x00},
	{0x366a, 0x64},
	{0x366c, 0x00},
	{0x366e, 0x44},
	{0x366f, 0x4f},
	{0x3677, 0x11},
	{0x3678, 0x11},
	{0x3680, 0xff},
	{0x3681, 0xd2},
	{0x3682, 0xa9},
	{0x3683, 0x91},
	{0x3684, 0x8a},
	{0x3620, 0x80},
	{0x3662, 0x10},
	{0x3663, 0x24},
	{0x3665, 0xa0},
	{0x3667, 0xa6},
	{0x3674, 0x01},
	{0x373d, 0x24},
	{0x3741, 0x28},
	{0x3743, 0x28},
	{0x3745, 0x28},
	{0x3747, 0x28},
	{0x3748, 0x00},
	{0x3749, 0x78},
	{0x374a, 0x00},
	{0x374b, 0x78},
	{0x374c, 0x00},
	{0x374d, 0x78},
	{0x374e, 0x00},
	{0x374f, 0x78},
	{0x3766, 0x12},
	{0x37e0, 0x00},
	{0x37e6, 0x04},
	{0x37e5, 0x04},
	{0x37e1, 0x04},
	{0x3737, 0x04},
	{0x37d0, 0x0a},
	{0x37d8, 0x04},
	{0x37e2, 0x08},
	{0x3739, 0x10},
	{0x37e4, 0x18},
	{0x37e3, 0x04},
	{0x37d9, 0x10},
	{0x4040, 0x04},
	{0x4041, 0x0f},
	{0x4008, 0x00},
	{0x4009, 0x0d},
	{0x37a1, 0x14},
	{0x37a8, 0x16},
	{0x37ab, 0x10},
	{0x37c2, 0x04},
	{0x3705, 0x00},
	{0x3706, 0x28},
	{0x370a, 0x00},
	{0x370b, 0x78},
	{0x3714, 0x24},
	{0x371a, 0x1e},
	{0x372a, 0x03},
	{0x3756, 0x00},
	{0x3757, 0x0e},
	{0x377b, 0x00},
	{0x377c, 0x0c},
	{0x377d, 0x20},
	{0x3790, 0x28},
	{0x3791, 0x78},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x07},
	{0x3805, 0x8f},
	{0x3806, 0x04},
	{0x3807, 0x43},
	{0x3808, 0x07},
	{0x3809, 0x80},
	{0x380a, 0x04},
	{0x380b, 0x38},
	{0x380c, 0x02},
	{0x380d, 0x78},
	{0x380e, 0x04},
	{0x380f, 0xa0},
	{0x3811, 0x08},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x01},
	{0x3817, 0x01},
	{0x381d, 0x40},
	{0x381e, 0x02},
	{0x3820, 0x88},
	{0x3821, 0x00},
	{0x3822, 0x04},
	{0x3835, 0x00},
	{0x4303, 0x19},
	{0x4304, 0x19},
	{0x4305, 0x03},
	{0x4306, 0x81},
	{0x4503, 0x00},
	{0x4508, 0x14},
	{0x450a, 0x00},
	{0x450b, 0x40},
	{0x4833, 0x08},
	{0x5000, 0xa9},
	{0x5001, 0x09},
	{0x3b00, 0x00},
	{0x3b02, 0x00},
	{0x3b03, 0x00},
	{0x3c80, 0x08},
	{0x3c82, 0x00},
	{0x3c83, 0xb1},
	{0x3c87, 0x08},
	{0x3c8c, 0x10},
	{0x3c8d, 0x00},
	{0x3c90, 0x00},
	{0x3c91, 0x00},
	{0x3c92, 0x00},
	{0x3c93, 0x00},
	{0x3c94, 0x00},
	{0x3c95, 0x00},
	{0x3c96, 0x00},
	{0x3c97, 0x00},
	{0x3c98, 0x00},
	{0x4000, 0xf3},
	{0x4001, 0x60},
	{0x4002, 0x00},
	{0x4003, 0x40},
	{0x4090, 0x14},
	{0x4601, 0x10},
	{0x4701, 0x00},
	{0x4708, 0x09},
	{0x470a, 0x00},
	{0x470b, 0x40},
	{0x470c, 0x81},
	{0x480c, 0x12},
	{0x4710, 0x06},
	{0x4711, 0x00},
	{0x4837, 0x12},
	{0x4800, 0x00},
	{0x4c01, 0x00},
	{0x5036, 0x00},
	{0x5037, 0x00},
	{0x580b, 0x0f},
	{0x4903, 0x80},
	{0x4003, 0x40},
	{0x5000, 0xf9},
	{0x5200, 0x1b},
	{0x4837, 0x16},
	{0x380e, 0x04},
	{0x380f, 0xa0},
	{0x3500, 0x00},
	{0x3501, 0x49},
	{0x3502, 0x80},
	{0x3508, 0x02},
	{0x3509, 0x80},
	{0x3d8c, 0x11},
	{0x3d8d, 0xf0},
	{0x5180, 0x00},
	{0x5181, 0x10},
	{0x36a0, 0x16},
	{0x36a1, 0x50},
	{0x36a2, 0x60},
	{0x36a3, 0x80},
	{0x36a4, 0x00},
	{0x36a5, 0xa0},
	{0x36a6, 0x00},
	{0x36a7, 0x50},
	{0x36a8, 0x00},
	{0x36a9, 0x50},
	{0x36aa, 0x00},
	{0x36ab, 0x50},
	{0x36ac, 0x00},
	{0x36ad, 0x50},
	{0x36ae, 0x00},
	{0x36af, 0x50},
	{0x36b0, 0x00},
	{0x36b1, 0x50},
	{0x36b2, 0x00},
	{0x36b3, 0x50},
	{0x36b4, 0x00},
	{0x36b5, 0x50},
	{0x36b9, 0xee},
	{0x36ba, 0xee},
	{0x36bb, 0xee},
	{0x36bc, 0xee},
	{0x36bd, 0x0e},
	{0x36b6, 0x08},
	{0x36b7, 0x08},
	{0x36b8, 0x10},
	{0x3508, 0x03},
	{0x3509, 0xaa},
	{0x350c, 0x01},
	{0x350d, 0x55},
	{0x3511, 0x0f},
	{0x3512, 0xc0},
	{0x366c, 0x30},
	{0x372a, 0x83},
	{0x3757, 0x00},
	{0x377b, 0xa8},
	{0x377c, 0x02},
	{0x377d, 0x00},
	{0x380e, 0x05},
	{0x3821, 0x04},
	{0x4305, 0x1b},
	{0x4306, 0x80},
	{0x5036, 0x80},
	{0x5037, 0x01},
	{0x4903, 0x81},
	{0x0100, 0x01},
};

#ifdef CONFIG_PM
static struct vin_reg_16_8 pm_regs[] = {
	{OV2732_L_EXPO_HSB, 0x00},
	{OV2732_L_EXPO_MSB, 0x00},
	{OV2732_L_EXPO_LSB, 0x00},
	{OV2732_S_EXPO_HSB, 0x00},
	{OV2732_S_EXPO_MSB, 0x00},
	{OV2732_S_EXPO_LSB, 0x00},
	{OV2732_L_AGAIN_MSB, 0x00},
	{OV2732_L_AGAIN_LSB, 0x00},
	{OV2732_L_DGAIN_MSB, 0x00},
	{OV2732_L_DGAIN_LSB, 0x00},
	{OV2732_S_AGAIN_MSB, 0x00},
	{OV2732_S_AGAIN_LSB, 0x00},
	{OV2732_S_DGAIN_MSB, 0x00},
	{OV2732_S_DGAIN_LSB, 0x00},
};
#endif

/* Gain table */
#define OV2732_GAIN_ROWS               (513)
#define OV2732_GAIN_COLS               (2)
#define OV2732_GAIN_MAXDB              (512)

#define OV2732_GAIN_COL_AGAIN          (0)
#define OV2732_GAIN_COL_DGAIN          (1)

static const u16 OV2732_GAIN_TABLE[OV2732_GAIN_ROWS][OV2732_GAIN_COLS] = {
	{0x0080, 0x0400}, /* index:0, gain:0.00000db -> x1.000000, again:x1.000000, dgain:x1.000000 */
	{0x0081, 0x0400}, /* index:1, gain:0.09375db -> x1.010852, again:x1.010852, dgain:x1.000000 */
	{0x0082, 0x0400}, /* index:2, gain:0.18750db -> x1.021821, again:x1.021821, dgain:x1.000000 */
	{0x0084, 0x0400}, /* index:3, gain:0.28125db -> x1.032910, again:x1.032910, dgain:x1.000000 */
	{0x0085, 0x0400}, /* index:4, gain:0.37500db -> x1.044119, again:x1.044119, dgain:x1.000000 */
	{0x0087, 0x0400}, /* index:5, gain:0.46875db -> x1.055450, again:x1.055450, dgain:x1.000000 */
	{0x0088, 0x0400}, /* index:6, gain:0.56250db -> x1.066903, again:x1.066903, dgain:x1.000000 */
	{0x008a, 0x0400}, /* index:7, gain:0.65625db -> x1.078481, again:x1.078481, dgain:x1.000000 */
	{0x008b, 0x0400}, /* index:8, gain:0.75000db -> x1.090184, again:x1.090184, dgain:x1.000000 */
	{0x008d, 0x0400}, /* index:9, gain:0.84375db -> x1.102015, again:x1.102015, dgain:x1.000000 */
	{0x008e, 0x0400}, /* index:10, gain:0.93750db -> x1.113974, again:x1.113974, dgain:x1.000000 */
	{0x0090, 0x0400}, /* index:11, gain:1.03125db -> x1.126063, again:x1.126063, dgain:x1.000000 */
	{0x0091, 0x0400}, /* index:12, gain:1.12500db -> x1.138282, again:x1.138282, dgain:x1.000000 */
	{0x0093, 0x0400}, /* index:13, gain:1.21875db -> x1.150635, again:x1.150635, dgain:x1.000000 */
	{0x0094, 0x0400}, /* index:14, gain:1.31250db -> x1.163121, again:x1.163121, dgain:x1.000000 */
	{0x0096, 0x0400}, /* index:15, gain:1.40625db -> x1.175743, again:x1.175743, dgain:x1.000000 */
	{0x0098, 0x0400}, /* index:16, gain:1.50000db -> x1.188502, again:x1.188502, dgain:x1.000000 */
	{0x0099, 0x0400}, /* index:17, gain:1.59375db -> x1.201400, again:x1.201400, dgain:x1.000000 */
	{0x009b, 0x0400}, /* index:18, gain:1.68750db -> x1.214437, again:x1.214437, dgain:x1.000000 */
	{0x009d, 0x0400}, /* index:19, gain:1.78125db -> x1.227616, again:x1.227616, dgain:x1.000000 */
	{0x009e, 0x0400}, /* index:20, gain:1.87500db -> x1.240938, again:x1.240938, dgain:x1.000000 */
	{0x00a0, 0x0400}, /* index:21, gain:1.96875db -> x1.254404, again:x1.254404, dgain:x1.000000 */
	{0x00a2, 0x0400}, /* index:22, gain:2.06250db -> x1.268017, again:x1.268017, dgain:x1.000000 */
	{0x00a4, 0x0400}, /* index:23, gain:2.15625db -> x1.281777, again:x1.281777, dgain:x1.000000 */
	{0x00a5, 0x0400}, /* index:24, gain:2.25000db -> x1.295687, again:x1.295687, dgain:x1.000000 */
	{0x00a7, 0x0400}, /* index:25, gain:2.34375db -> x1.309747, again:x1.309747, dgain:x1.000000 */
	{0x00a9, 0x0400}, /* index:26, gain:2.43750db -> x1.323960, again:x1.323960, dgain:x1.000000 */
	{0x00ab, 0x0400}, /* index:27, gain:2.53125db -> x1.338328, again:x1.338328, dgain:x1.000000 */
	{0x00ad, 0x0400}, /* index:28, gain:2.62500db -> x1.352851, again:x1.352851, dgain:x1.000000 */
	{0x00af, 0x0400}, /* index:29, gain:2.71875db -> x1.367532, again:x1.367532, dgain:x1.000000 */
	{0x00b0, 0x0400}, /* index:30, gain:2.81250db -> x1.382372, again:x1.382372, dgain:x1.000000 */
	{0x00b2, 0x0400}, /* index:31, gain:2.90625db -> x1.397374, again:x1.397374, dgain:x1.000000 */
	{0x00b4, 0x0400}, /* index:32, gain:3.00000db -> x1.412538, again:x1.412538, dgain:x1.000000 */
	{0x00b6, 0x0400}, /* index:33, gain:3.09375db -> x1.427866, again:x1.427866, dgain:x1.000000 */
	{0x00b8, 0x0400}, /* index:34, gain:3.18750db -> x1.443361, again:x1.443361, dgain:x1.000000 */
	{0x00ba, 0x0400}, /* index:35, gain:3.28125db -> x1.459024, again:x1.459024, dgain:x1.000000 */
	{0x00bc, 0x0400}, /* index:36, gain:3.37500db -> x1.474857, again:x1.474857, dgain:x1.000000 */
	{0x00be, 0x0400}, /* index:37, gain:3.46875db -> x1.490862, again:x1.490862, dgain:x1.000000 */
	{0x00c0, 0x0400}, /* index:38, gain:3.56250db -> x1.507041, again:x1.507041, dgain:x1.000000 */
	{0x00c2, 0x0400}, /* index:39, gain:3.65625db -> x1.523395, again:x1.523395, dgain:x1.000000 */
	{0x00c5, 0x0400}, /* index:40, gain:3.75000db -> x1.539927, again:x1.539927, dgain:x1.000000 */
	{0x00c7, 0x0400}, /* index:41, gain:3.84375db -> x1.556638, again:x1.556638, dgain:x1.000000 */
	{0x00c9, 0x0400}, /* index:42, gain:3.93750db -> x1.573530, again:x1.573530, dgain:x1.000000 */
	{0x00cb, 0x0400}, /* index:43, gain:4.03125db -> x1.590606, again:x1.590606, dgain:x1.000000 */
	{0x00cd, 0x0400}, /* index:44, gain:4.12500db -> x1.607867, again:x1.607867, dgain:x1.000000 */
	{0x00d0, 0x0400}, /* index:45, gain:4.21875db -> x1.625315, again:x1.625315, dgain:x1.000000 */
	{0x00d2, 0x0400}, /* index:46, gain:4.31250db -> x1.642952, again:x1.642952, dgain:x1.000000 */
	{0x00d4, 0x0400}, /* index:47, gain:4.40625db -> x1.660782, again:x1.660782, dgain:x1.000000 */
	{0x00d6, 0x0400}, /* index:48, gain:4.50000db -> x1.678804, again:x1.678804, dgain:x1.000000 */
	{0x00d9, 0x0400}, /* index:49, gain:4.59375db -> x1.697022, again:x1.697022, dgain:x1.000000 */
	{0x00db, 0x0400}, /* index:50, gain:4.68750db -> x1.715438, again:x1.715438, dgain:x1.000000 */
	{0x00dd, 0x0400}, /* index:51, gain:4.78125db -> x1.734054, again:x1.734054, dgain:x1.000000 */
	{0x00e0, 0x0400}, /* index:52, gain:4.87500db -> x1.752871, again:x1.752871, dgain:x1.000000 */
	{0x00e2, 0x0400}, /* index:53, gain:4.96875db -> x1.771893, again:x1.771893, dgain:x1.000000 */
	{0x00e5, 0x0400}, /* index:54, gain:5.06250db -> x1.791121, again:x1.791121, dgain:x1.000000 */
	{0x00e7, 0x0400}, /* index:55, gain:5.15625db -> x1.810558, again:x1.810558, dgain:x1.000000 */
	{0x00ea, 0x0400}, /* index:56, gain:5.25000db -> x1.830206, again:x1.830206, dgain:x1.000000 */
	{0x00ec, 0x0400}, /* index:57, gain:5.34375db -> x1.850067, again:x1.850067, dgain:x1.000000 */
	{0x00ef, 0x0400}, /* index:58, gain:5.43750db -> x1.870144, again:x1.870144, dgain:x1.000000 */
	{0x00f1, 0x0400}, /* index:59, gain:5.53125db -> x1.890438, again:x1.890438, dgain:x1.000000 */
	{0x00f4, 0x0400}, /* index:60, gain:5.62500db -> x1.910953, again:x1.910953, dgain:x1.000000 */
	{0x00f7, 0x0400}, /* index:61, gain:5.71875db -> x1.931690, again:x1.931690, dgain:x1.000000 */
	{0x00f9, 0x0400}, /* index:62, gain:5.81250db -> x1.952653, again:x1.952653, dgain:x1.000000 */
	{0x00fc, 0x0400}, /* index:63, gain:5.90625db -> x1.973842, again:x1.973842, dgain:x1.000000 */
	{0x00ff, 0x0400}, /* index:64, gain:6.00000db -> x1.995262, again:x1.995262, dgain:x1.000000 */
	{0x0102, 0x0400}, /* index:65, gain:6.09375db -> x2.016915, again:x2.016915, dgain:x1.000000 */
	{0x0104, 0x0400}, /* index:66, gain:6.18750db -> x2.038802, again:x2.038802, dgain:x1.000000 */
	{0x0107, 0x0400}, /* index:67, gain:6.28125db -> x2.060927, again:x2.060927, dgain:x1.000000 */
	{0x010a, 0x0400}, /* index:68, gain:6.37500db -> x2.083291, again:x2.083291, dgain:x1.000000 */
	{0x010d, 0x0400}, /* index:69, gain:6.46875db -> x2.105899, again:x2.105899, dgain:x1.000000 */
	{0x0110, 0x0400}, /* index:70, gain:6.56250db -> x2.128752, again:x2.128752, dgain:x1.000000 */
	{0x0113, 0x0400}, /* index:71, gain:6.65625db -> x2.151852, again:x2.151852, dgain:x1.000000 */
	{0x0116, 0x0400}, /* index:72, gain:6.75000db -> x2.175204, again:x2.175204, dgain:x1.000000 */
	{0x0119, 0x0400}, /* index:73, gain:6.84375db -> x2.198809, again:x2.198809, dgain:x1.000000 */
	{0x011c, 0x0400}, /* index:74, gain:6.93750db -> x2.222670, again:x2.222670, dgain:x1.000000 */
	{0x011f, 0x0400}, /* index:75, gain:7.03125db -> x2.246790, again:x2.246790, dgain:x1.000000 */
	{0x0122, 0x0400}, /* index:76, gain:7.12500db -> x2.271172, again:x2.271172, dgain:x1.000000 */
	{0x0125, 0x0400}, /* index:77, gain:7.21875db -> x2.295818, again:x2.295818, dgain:x1.000000 */
	{0x0129, 0x0400}, /* index:78, gain:7.31250db -> x2.320732, again:x2.320732, dgain:x1.000000 */
	{0x012c, 0x0400}, /* index:79, gain:7.40625db -> x2.345916, again:x2.345916, dgain:x1.000000 */
	{0x012f, 0x0400}, /* index:80, gain:7.50000db -> x2.371374, again:x2.371374, dgain:x1.000000 */
	{0x0132, 0x0400}, /* index:81, gain:7.59375db -> x2.397107, again:x2.397107, dgain:x1.000000 */
	{0x0136, 0x0400}, /* index:82, gain:7.68750db -> x2.423120, again:x2.423120, dgain:x1.000000 */
	{0x0139, 0x0400}, /* index:83, gain:7.78125db -> x2.449416, again:x2.449416, dgain:x1.000000 */
	{0x013c, 0x0400}, /* index:84, gain:7.87500db -> x2.475996, again:x2.475996, dgain:x1.000000 */
	{0x0140, 0x0400}, /* index:85, gain:7.96875db -> x2.502865, again:x2.502865, dgain:x1.000000 */
	{0x0143, 0x0400}, /* index:86, gain:8.06250db -> x2.530026, again:x2.530026, dgain:x1.000000 */
	{0x0147, 0x0400}, /* index:87, gain:8.15625db -> x2.557482, again:x2.557482, dgain:x1.000000 */
	{0x014a, 0x0400}, /* index:88, gain:8.25000db -> x2.585235, again:x2.585235, dgain:x1.000000 */
	{0x014e, 0x0400}, /* index:89, gain:8.34375db -> x2.613289, again:x2.613289, dgain:x1.000000 */
	{0x0152, 0x0400}, /* index:90, gain:8.43750db -> x2.641648, again:x2.641648, dgain:x1.000000 */
	{0x0155, 0x0400}, /* index:91, gain:8.53125db -> x2.670315, again:x2.670315, dgain:x1.000000 */
	{0x0159, 0x0400}, /* index:92, gain:8.62500db -> x2.699293, again:x2.699293, dgain:x1.000000 */
	{0x015d, 0x0400}, /* index:93, gain:8.71875db -> x2.728585, again:x2.728585, dgain:x1.000000 */
	{0x0161, 0x0400}, /* index:94, gain:8.81250db -> x2.758195, again:x2.758195, dgain:x1.000000 */
	{0x0164, 0x0400}, /* index:95, gain:8.90625db -> x2.788127, again:x2.788127, dgain:x1.000000 */
	{0x0168, 0x0400}, /* index:96, gain:9.00000db -> x2.818383, again:x2.818383, dgain:x1.000000 */
	{0x016c, 0x0400}, /* index:97, gain:9.09375db -> x2.848968, again:x2.848968, dgain:x1.000000 */
	{0x0170, 0x0400}, /* index:98, gain:9.18750db -> x2.879884, again:x2.879884, dgain:x1.000000 */
	{0x0174, 0x0400}, /* index:99, gain:9.28125db -> x2.911136, again:x2.911136, dgain:x1.000000 */
	{0x0178, 0x0400}, /* index:100, gain:9.37500db -> x2.942727, again:x2.942727, dgain:x1.000000 */
	{0x017c, 0x0400}, /* index:101, gain:9.46875db -> x2.974661, again:x2.974661, dgain:x1.000000 */
	{0x0180, 0x0400}, /* index:102, gain:9.56250db -> x3.006942, again:x3.006942, dgain:x1.000000 */
	{0x0185, 0x0400}, /* index:103, gain:9.65625db -> x3.039572, again:x3.039572, dgain:x1.000000 */
	{0x0189, 0x0400}, /* index:104, gain:9.75000db -> x3.072557, again:x3.072557, dgain:x1.000000 */
	{0x018d, 0x0400}, /* index:105, gain:9.84375db -> x3.105900, again:x3.105900, dgain:x1.000000 */
	{0x0191, 0x0400}, /* index:106, gain:9.93750db -> x3.139605, again:x3.139605, dgain:x1.000000 */
	{0x0196, 0x0400}, /* index:107, gain:10.03125db -> x3.173675, again:x3.173675, dgain:x1.000000 */
	{0x019a, 0x0400}, /* index:108, gain:10.12500db -> x3.208116, again:x3.208116, dgain:x1.000000 */
	{0x019f, 0x0400}, /* index:109, gain:10.21875db -> x3.242930, again:x3.242930, dgain:x1.000000 */
	{0x01a3, 0x0400}, /* index:110, gain:10.31250db -> x3.278121, again:x3.278121, dgain:x1.000000 */
	{0x01a8, 0x0400}, /* index:111, gain:10.40625db -> x3.313695, again:x3.313695, dgain:x1.000000 */
	{0x01ac, 0x0400}, /* index:112, gain:10.50000db -> x3.349654, again:x3.349654, dgain:x1.000000 */
	{0x01b1, 0x0400}, /* index:113, gain:10.59375db -> x3.386004, again:x3.386004, dgain:x1.000000 */
	{0x01b6, 0x0400}, /* index:114, gain:10.68750db -> x3.422749, again:x3.422749, dgain:x1.000000 */
	{0x01ba, 0x0400}, /* index:115, gain:10.78125db -> x3.459892, again:x3.459892, dgain:x1.000000 */
	{0x01bf, 0x0400}, /* index:116, gain:10.87500db -> x3.497438, again:x3.497438, dgain:x1.000000 */
	{0x01c4, 0x0400}, /* index:117, gain:10.96875db -> x3.535391, again:x3.535391, dgain:x1.000000 */
	{0x01c9, 0x0400}, /* index:118, gain:11.06250db -> x3.573757, again:x3.573757, dgain:x1.000000 */
	{0x01ce, 0x0400}, /* index:119, gain:11.15625db -> x3.612539, again:x3.612539, dgain:x1.000000 */
	{0x01d3, 0x0400}, /* index:120, gain:11.25000db -> x3.651741, again:x3.651741, dgain:x1.000000 */
	{0x01d8, 0x0400}, /* index:121, gain:11.34375db -> x3.691369, again:x3.691369, dgain:x1.000000 */
	{0x01dd, 0x0400}, /* index:122, gain:11.43750db -> x3.731427, again:x3.731427, dgain:x1.000000 */
	{0x01e2, 0x0400}, /* index:123, gain:11.53125db -> x3.771920, again:x3.771920, dgain:x1.000000 */
	{0x01e8, 0x0400}, /* index:124, gain:11.62500db -> x3.812853, again:x3.812853, dgain:x1.000000 */
	{0x01ed, 0x0400}, /* index:125, gain:11.71875db -> x3.854229, again:x3.854229, dgain:x1.000000 */
	{0x01f2, 0x0400}, /* index:126, gain:11.81250db -> x3.896054, again:x3.896054, dgain:x1.000000 */
	{0x01f8, 0x0400}, /* index:127, gain:11.90625db -> x3.938333, again:x3.938333, dgain:x1.000000 */
	{0x01fd, 0x0400}, /* index:128, gain:12.00000db -> x3.981072, again:x3.981072, dgain:x1.000000 */
	{0x0203, 0x0400}, /* index:129, gain:12.09375db -> x4.024274, again:x4.024274, dgain:x1.000000 */
	{0x0208, 0x0400}, /* index:130, gain:12.18750db -> x4.067944, again:x4.067944, dgain:x1.000000 */
	{0x020e, 0x0400}, /* index:131, gain:12.28125db -> x4.112089, again:x4.112089, dgain:x1.000000 */
	{0x0214, 0x0400}, /* index:132, gain:12.37500db -> x4.156712, again:x4.156712, dgain:x1.000000 */
	{0x0219, 0x0400}, /* index:133, gain:12.46875db -> x4.201821, again:x4.201821, dgain:x1.000000 */
	{0x021f, 0x0400}, /* index:134, gain:12.56250db -> x4.247418, again:x4.247418, dgain:x1.000000 */
	{0x0225, 0x0400}, /* index:135, gain:12.65625db -> x4.293510, again:x4.293510, dgain:x1.000000 */
	{0x022b, 0x0400}, /* index:136, gain:12.75000db -> x4.340103, again:x4.340103, dgain:x1.000000 */
	{0x0231, 0x0400}, /* index:137, gain:12.84375db -> x4.387200, again:x4.387200, dgain:x1.000000 */
	{0x0237, 0x0400}, /* index:138, gain:12.93750db -> x4.434810, again:x4.434810, dgain:x1.000000 */
	{0x023d, 0x0400}, /* index:139, gain:13.03125db -> x4.482936, again:x4.482936, dgain:x1.000000 */
	{0x0244, 0x0400}, /* index:140, gain:13.12500db -> x4.531584, again:x4.531584, dgain:x1.000000 */
	{0x024a, 0x0400}, /* index:141, gain:13.21875db -> x4.580759, again:x4.580759, dgain:x1.000000 */
	{0x0250, 0x0400}, /* index:142, gain:13.31250db -> x4.630469, again:x4.630469, dgain:x1.000000 */
	{0x0257, 0x0400}, /* index:143, gain:13.40625db -> x4.680719, again:x4.680719, dgain:x1.000000 */
	{0x025d, 0x0400}, /* index:144, gain:13.50000db -> x4.731513, again:x4.731513, dgain:x1.000000 */
	{0x0264, 0x0400}, /* index:145, gain:13.59375db -> x4.782858, again:x4.782858, dgain:x1.000000 */
	{0x026a, 0x0400}, /* index:146, gain:13.68750db -> x4.834761, again:x4.834761, dgain:x1.000000 */
	{0x0271, 0x0400}, /* index:147, gain:13.78125db -> x4.887227, again:x4.887227, dgain:x1.000000 */
	{0x0278, 0x0400}, /* index:148, gain:13.87500db -> x4.940262, again:x4.940262, dgain:x1.000000 */
	{0x027f, 0x0400}, /* index:149, gain:13.96875db -> x4.993873, again:x4.993873, dgain:x1.000000 */
	{0x0286, 0x0400}, /* index:150, gain:14.06250db -> x5.048066, again:x5.048066, dgain:x1.000000 */
	{0x028d, 0x0400}, /* index:151, gain:14.15625db -> x5.102846, again:x5.102846, dgain:x1.000000 */
	{0x0294, 0x0400}, /* index:152, gain:14.25000db -> x5.158221, again:x5.158221, dgain:x1.000000 */
	{0x029b, 0x0400}, /* index:153, gain:14.34375db -> x5.214198, again:x5.214198, dgain:x1.000000 */
	{0x02a2, 0x0400}, /* index:154, gain:14.43750db -> x5.270781, again:x5.270781, dgain:x1.000000 */
	{0x02a9, 0x0400}, /* index:155, gain:14.53125db -> x5.327979, again:x5.327979, dgain:x1.000000 */
	{0x02b1, 0x0400}, /* index:156, gain:14.62500db -> x5.385797, again:x5.385797, dgain:x1.000000 */
	{0x02b8, 0x0400}, /* index:157, gain:14.71875db -> x5.444243, again:x5.444243, dgain:x1.000000 */
	{0x02c0, 0x0400}, /* index:158, gain:14.81250db -> x5.503323, again:x5.503323, dgain:x1.000000 */
	{0x02c8, 0x0400}, /* index:159, gain:14.90625db -> x5.563044, again:x5.563044, dgain:x1.000000 */
	{0x02cf, 0x0400}, /* index:160, gain:15.00000db -> x5.623413, again:x5.623413, dgain:x1.000000 */
	{0x02d7, 0x0400}, /* index:161, gain:15.09375db -> x5.684437, again:x5.684437, dgain:x1.000000 */
	{0x02df, 0x0400}, /* index:162, gain:15.18750db -> x5.746124, again:x5.746124, dgain:x1.000000 */
	{0x02e7, 0x0400}, /* index:163, gain:15.28125db -> x5.808480, again:x5.808480, dgain:x1.000000 */
	{0x02ef, 0x0400}, /* index:164, gain:15.37500db -> x5.871513, again:x5.871513, dgain:x1.000000 */
	{0x02f7, 0x0400}, /* index:165, gain:15.46875db -> x5.935229, again:x5.935229, dgain:x1.000000 */
	{0x02ff, 0x0400}, /* index:166, gain:15.56250db -> x5.999637, again:x5.999637, dgain:x1.000000 */
	{0x0308, 0x0400}, /* index:167, gain:15.65625db -> x6.064744, again:x6.064744, dgain:x1.000000 */
	{0x0310, 0x0400}, /* index:168, gain:15.75000db -> x6.130558, again:x6.130558, dgain:x1.000000 */
	{0x0319, 0x0400}, /* index:169, gain:15.84375db -> x6.197086, again:x6.197086, dgain:x1.000000 */
	{0x0321, 0x0400}, /* index:170, gain:15.93750db -> x6.264335, again:x6.264335, dgain:x1.000000 */
	{0x032a, 0x0400}, /* index:171, gain:16.03125db -> x6.332315, again:x6.332315, dgain:x1.000000 */
	{0x0333, 0x0400}, /* index:172, gain:16.12500db -> x6.401032, again:x6.401032, dgain:x1.000000 */
	{0x033c, 0x0400}, /* index:173, gain:16.21875db -> x6.470495, again:x6.470495, dgain:x1.000000 */
	{0x0345, 0x0400}, /* index:174, gain:16.31250db -> x6.540712, again:x6.540712, dgain:x1.000000 */
	{0x034e, 0x0400}, /* index:175, gain:16.40625db -> x6.611690, again:x6.611690, dgain:x1.000000 */
	{0x0357, 0x0400}, /* index:176, gain:16.50000db -> x6.683439, again:x6.683439, dgain:x1.000000 */
	{0x0360, 0x0400}, /* index:177, gain:16.59375db -> x6.755966, again:x6.755966, dgain:x1.000000 */
	{0x036a, 0x0400}, /* index:178, gain:16.68750db -> x6.829282, again:x6.829282, dgain:x1.000000 */
	{0x0373, 0x0400}, /* index:179, gain:16.78125db -> x6.903392, again:x6.903392, dgain:x1.000000 */
	{0x037d, 0x0400}, /* index:180, gain:16.87500db -> x6.978306, again:x6.978306, dgain:x1.000000 */
	{0x0386, 0x0400}, /* index:181, gain:16.96875db -> x7.054033, again:x7.054033, dgain:x1.000000 */
	{0x0390, 0x0400}, /* index:182, gain:17.06250db -> x7.130582, again:x7.130582, dgain:x1.000000 */
	{0x039a, 0x0400}, /* index:183, gain:17.15625db -> x7.207963, again:x7.207963, dgain:x1.000000 */
	{0x03a4, 0x0400}, /* index:184, gain:17.25000db -> x7.286182, again:x7.286182, dgain:x1.000000 */
	{0x03ae, 0x0400}, /* index:185, gain:17.34375db -> x7.365250, again:x7.365250, dgain:x1.000000 */
	{0x03b8, 0x0400}, /* index:186, gain:17.43750db -> x7.445176, again:x7.445176, dgain:x1.000000 */
	{0x03c3, 0x0400}, /* index:187, gain:17.53125db -> x7.525970, again:x7.525970, dgain:x1.000000 */
	{0x03cd, 0x0400}, /* index:188, gain:17.62500db -> x7.607641, again:x7.607641, dgain:x1.000000 */
	{0x03d8, 0x0400}, /* index:189, gain:17.71875db -> x7.690198, again:x7.690198, dgain:x1.000000 */
	{0x03e3, 0x0400}, /* index:190, gain:17.81250db -> x7.773650, again:x7.773650, dgain:x1.000000 */
	{0x03ed, 0x0400}, /* index:191, gain:17.90625db -> x7.858008, again:x7.858008, dgain:x1.000000 */
	{0x03f8, 0x0400}, /* index:192, gain:18.00000db -> x7.943282, again:x7.943282, dgain:x1.000000 */
	{0x0403, 0x0400}, /* index:193, gain:18.09375db -> x8.029482, again:x8.029482, dgain:x1.000000 */
	{0x040e, 0x0400}, /* index:194, gain:18.18750db -> x8.116616, again:x8.116616, dgain:x1.000000 */
	{0x041a, 0x0400}, /* index:195, gain:18.28125db -> x8.204696, again:x8.204696, dgain:x1.000000 */
	{0x0425, 0x0400}, /* index:196, gain:18.37500db -> x8.293732, again:x8.293732, dgain:x1.000000 */
	{0x0431, 0x0400}, /* index:197, gain:18.46875db -> x8.383734, again:x8.383734, dgain:x1.000000 */
	{0x043c, 0x0400}, /* index:198, gain:18.56250db -> x8.474713, again:x8.474713, dgain:x1.000000 */
	{0x0448, 0x0400}, /* index:199, gain:18.65625db -> x8.566679, again:x8.566679, dgain:x1.000000 */
	{0x0454, 0x0400}, /* index:200, gain:18.75000db -> x8.659643, again:x8.659643, dgain:x1.000000 */
	{0x0460, 0x0400}, /* index:201, gain:18.84375db -> x8.753616, again:x8.753616, dgain:x1.000000 */
	{0x046c, 0x0400}, /* index:202, gain:18.93750db -> x8.848608, again:x8.848608, dgain:x1.000000 */
	{0x0478, 0x0400}, /* index:203, gain:19.03125db -> x8.944633, again:x8.944633, dgain:x1.000000 */
	{0x0485, 0x0400}, /* index:204, gain:19.12500db -> x9.041698, again:x9.041698, dgain:x1.000000 */
	{0x0491, 0x0400}, /* index:205, gain:19.21875db -> x9.139817, again:x9.139817, dgain:x1.000000 */
	{0x049e, 0x0400}, /* index:206, gain:19.31250db -> x9.239000, again:x9.239000, dgain:x1.000000 */
	{0x04ab, 0x0400}, /* index:207, gain:19.40625db -> x9.339260, again:x9.339260, dgain:x1.000000 */
	{0x04b8, 0x0400}, /* index:208, gain:19.50000db -> x9.440609, again:x9.440609, dgain:x1.000000 */
	{0x04c5, 0x0400}, /* index:209, gain:19.59375db -> x9.543057, again:x9.543057, dgain:x1.000000 */
	{0x04d2, 0x0400}, /* index:210, gain:19.68750db -> x9.646616, again:x9.646616, dgain:x1.000000 */
	{0x04e0, 0x0400}, /* index:211, gain:19.78125db -> x9.751299, again:x9.751299, dgain:x1.000000 */
	{0x04ed, 0x0400}, /* index:212, gain:19.87500db -> x9.857118, again:x9.857118, dgain:x1.000000 */
	{0x04fb, 0x0400}, /* index:213, gain:19.96875db -> x9.964087, again:x9.964087, dgain:x1.000000 */
	{0x0509, 0x0400}, /* index:214, gain:20.06250db -> x10.072214, again:x10.072214, dgain:x1.000000 */
	{0x0517, 0x0400}, /* index:215, gain:20.15625db -> x10.181517, again:x10.181517, dgain:x1.000000 */
	{0x0525, 0x0400}, /* index:216, gain:20.25000db -> x10.292006, again:x10.292006, dgain:x1.000000 */
	{0x0533, 0x0400}, /* index:217, gain:20.34375db -> x10.403692, again:x10.403692, dgain:x1.000000 */
	{0x0542, 0x0400}, /* index:218, gain:20.43750db -> x10.516592, again:x10.516592, dgain:x1.000000 */
	{0x0550, 0x0400}, /* index:219, gain:20.53125db -> x10.630714, again:x10.630714, dgain:x1.000000 */
	{0x055f, 0x0400}, /* index:220, gain:20.62500db -> x10.746078, again:x10.746078, dgain:x1.000000 */
	{0x056e, 0x0400}, /* index:221, gain:20.71875db -> x10.862694, again:x10.862694, dgain:x1.000000 */
	{0x057d, 0x0400}, /* index:222, gain:20.81250db -> x10.980572, again:x10.980572, dgain:x1.000000 */
	{0x058c, 0x0400}, /* index:223, gain:20.90625db -> x11.099733, again:x11.099733, dgain:x1.000000 */
	{0x059c, 0x0400}, /* index:224, gain:21.00000db -> x11.220183, again:x11.220183, dgain:x1.000000 */
	{0x05ab, 0x0400}, /* index:225, gain:21.09375db -> x11.341944, again:x11.341944, dgain:x1.000000 */
	{0x05bb, 0x0400}, /* index:226, gain:21.18750db -> x11.465026, again:x11.465026, dgain:x1.000000 */
	{0x05cb, 0x0400}, /* index:227, gain:21.28125db -> x11.589441, again:x11.589441, dgain:x1.000000 */
	{0x05db, 0x0400}, /* index:228, gain:21.37500db -> x11.715209, again:x11.715209, dgain:x1.000000 */
	{0x05eb, 0x0400}, /* index:229, gain:21.46875db -> x11.842338, again:x11.842338, dgain:x1.000000 */
	{0x05fc, 0x0400}, /* index:230, gain:21.56250db -> x11.970850, again:x11.970850, dgain:x1.000000 */
	{0x060c, 0x0400}, /* index:231, gain:21.65625db -> x12.100757, again:x12.100757, dgain:x1.000000 */
	{0x061d, 0x0400}, /* index:232, gain:21.75000db -> x12.232071, again:x12.232071, dgain:x1.000000 */
	{0x062e, 0x0400}, /* index:233, gain:21.84375db -> x12.364812, again:x12.364812, dgain:x1.000000 */
	{0x063f, 0x0400}, /* index:234, gain:21.93750db -> x12.498991, again:x12.498991, dgain:x1.000000 */
	{0x0651, 0x0400}, /* index:235, gain:22.03125db -> x12.634629, again:x12.634629, dgain:x1.000000 */
	{0x0662, 0x0400}, /* index:236, gain:22.12500db -> x12.771739, again:x12.771739, dgain:x1.000000 */
	{0x0674, 0x0400}, /* index:237, gain:22.21875db -> x12.910334, again:x12.910334, dgain:x1.000000 */
	{0x0686, 0x0400}, /* index:238, gain:22.31250db -> x13.050436, again:x13.050436, dgain:x1.000000 */
	{0x0698, 0x0400}, /* index:239, gain:22.40625db -> x13.192055, again:x13.192055, dgain:x1.000000 */
	{0x06aa, 0x0400}, /* index:240, gain:22.50000db -> x13.335214, again:x13.335214, dgain:x1.000000 */
	{0x06bd, 0x0400}, /* index:241, gain:22.59375db -> x13.479927, again:x13.479927, dgain:x1.000000 */
	{0x06d0, 0x0400}, /* index:242, gain:22.68750db -> x13.626207, again:x13.626207, dgain:x1.000000 */
	{0x06e3, 0x0400}, /* index:243, gain:22.78125db -> x13.774078, again:x13.774078, dgain:x1.000000 */
	{0x06f6, 0x0400}, /* index:244, gain:22.87500db -> x13.923549, again:x13.923549, dgain:x1.000000 */
	{0x0709, 0x0400}, /* index:245, gain:22.96875db -> x14.074647, again:x14.074647, dgain:x1.000000 */
	{0x071d, 0x0400}, /* index:246, gain:23.06250db -> x14.227384, again:x14.227384, dgain:x1.000000 */
	{0x0730, 0x0400}, /* index:247, gain:23.15625db -> x14.381775, again:x14.381775, dgain:x1.000000 */
	{0x0744, 0x0400}, /* index:248, gain:23.25000db -> x14.537845, again:x14.537845, dgain:x1.000000 */
	{0x0759, 0x0400}, /* index:249, gain:23.34375db -> x14.695604, again:x14.695604, dgain:x1.000000 */
	{0x076d, 0x0400}, /* index:250, gain:23.43750db -> x14.855080, again:x14.855080, dgain:x1.000000 */
	{0x0782, 0x0400}, /* index:251, gain:23.53125db -> x15.016287, again:x15.016287, dgain:x1.000000 */
	{0x0796, 0x0400}, /* index:252, gain:23.62500db -> x15.179238, again:x15.179238, dgain:x1.000000 */
	{0x07ac, 0x0400}, /* index:253, gain:23.71875db -> x15.343962, again:x15.343962, dgain:x1.000000 */
	{0x07c1, 0x0400}, /* index:254, gain:23.81250db -> x15.510470, again:x15.510470, dgain:x1.000000 */
	{0x07d6, 0x0400}, /* index:255, gain:23.90625db -> x15.678788, again:x15.678788, dgain:x1.000000 */
	{0x07ec, 0x0400}, /* index:256, gain:24.00000db -> x15.848934, again:x15.848934, dgain:x1.000000 */
	{0x07ff, 0x0401}, /* index:257, gain:24.09375db -> x16.020921, again:x15.992188, dgain:x1.001797 */
	{0x07ff, 0x040c}, /* index:258, gain:24.18750db -> x16.194779, again:x15.992188, dgain:x1.012668 */
	{0x07ff, 0x0418}, /* index:259, gain:24.28125db -> x16.370519, again:x15.992188, dgain:x1.023657 */
	{0x07ff, 0x0423}, /* index:260, gain:24.37500db -> x16.548171, again:x15.992188, dgain:x1.034766 */
	{0x07ff, 0x042f}, /* index:261, gain:24.46875db -> x16.727751, again:x15.992188, dgain:x1.045995 */
	{0x07ff, 0x043a}, /* index:262, gain:24.56250db -> x16.909275, again:x15.992188, dgain:x1.057346 */
	{0x07ff, 0x0446}, /* index:263, gain:24.65625db -> x17.092773, again:x15.992188, dgain:x1.068820 */
	{0x07ff, 0x0452}, /* index:264, gain:24.75000db -> x17.278258, again:x15.992188, dgain:x1.080419 */
	{0x07ff, 0x045e}, /* index:265, gain:24.84375db -> x17.465760, again:x15.992188, dgain:x1.092143 */
	{0x07ff, 0x046a}, /* index:266, gain:24.93750db -> x17.655298, again:x15.992188, dgain:x1.103995 */
	{0x07ff, 0x0476}, /* index:267, gain:25.03125db -> x17.846887, again:x15.992188, dgain:x1.115975 */
	{0x07ff, 0x0483}, /* index:268, gain:25.12500db -> x18.040560, again:x15.992188, dgain:x1.128086 */
	{0x07ff, 0x048f}, /* index:269, gain:25.21875db -> x18.236330, again:x15.992188, dgain:x1.140327 */
	{0x07ff, 0x049c}, /* index:270, gain:25.31250db -> x18.434230, again:x15.992188, dgain:x1.152702 */
	{0x07ff, 0x04a9}, /* index:271, gain:25.40625db -> x18.634277, again:x15.992188, dgain:x1.165211 */
	{0x07ff, 0x04b6}, /* index:272, gain:25.50000db -> x18.836490, again:x15.992188, dgain:x1.177856 */
	{0x07ff, 0x04c3}, /* index:273, gain:25.59375db -> x19.040902, again:x15.992188, dgain:x1.190638 */
	{0x07ff, 0x04d0}, /* index:274, gain:25.68750db -> x19.247528, again:x15.992188, dgain:x1.203558 */
	{0x07ff, 0x04dd}, /* index:275, gain:25.78125db -> x19.456401, again:x15.992188, dgain:x1.216619 */
	{0x07ff, 0x04eb}, /* index:276, gain:25.87500db -> x19.667540, again:x15.992188, dgain:x1.229822 */
	{0x07ff, 0x04f9}, /* index:277, gain:25.96875db -> x19.880966, again:x15.992188, dgain:x1.243167 */
	{0x07ff, 0x0506}, /* index:278, gain:26.06250db -> x20.096713, again:x15.992188, dgain:x1.256658 */
	{0x07ff, 0x0514}, /* index:279, gain:26.15625db -> x20.314795, again:x15.992188, dgain:x1.270295 */
	{0x07ff, 0x0522}, /* index:280, gain:26.25000db -> x20.535250, again:x15.992188, dgain:x1.284080 */
	{0x07ff, 0x0531}, /* index:281, gain:26.34375db -> x20.758098, again:x15.992188, dgain:x1.298015 */
	{0x07ff, 0x053f}, /* index:282, gain:26.43750db -> x20.983357, again:x15.992188, dgain:x1.312101 */
	{0x07ff, 0x054e}, /* index:283, gain:26.53125db -> x21.211067, again:x15.992188, dgain:x1.326339 */
	{0x07ff, 0x055c}, /* index:284, gain:26.62500db -> x21.441243, again:x15.992188, dgain:x1.340732 */
	{0x07ff, 0x056b}, /* index:285, gain:26.71875db -> x21.673922, again:x15.992188, dgain:x1.355282 */
	{0x07ff, 0x057a}, /* index:286, gain:26.81250db -> x21.909126, again:x15.992188, dgain:x1.369989 */
	{0x07ff, 0x058a}, /* index:287, gain:26.90625db -> x22.146876, again:x15.992188, dgain:x1.384856 */
	{0x07ff, 0x0599}, /* index:288, gain:27.00000db -> x22.387213, again:x15.992188, dgain:x1.399884 */
	{0x07ff, 0x05a9}, /* index:289, gain:27.09375db -> x22.630151, again:x15.992188, dgain:x1.415075 */
	{0x07ff, 0x05b8}, /* index:290, gain:27.18750db -> x22.875732, again:x15.992188, dgain:x1.430432 */
	{0x07ff, 0x05c8}, /* index:291, gain:27.28125db -> x23.123978, again:x15.992188, dgain:x1.445955 */
	{0x07ff, 0x05d8}, /* index:292, gain:27.37500db -> x23.374912, again:x15.992188, dgain:x1.461646 */
	{0x07ff, 0x05e8}, /* index:293, gain:27.46875db -> x23.628575, again:x15.992188, dgain:x1.477507 */
	{0x07ff, 0x05f9}, /* index:294, gain:27.56250db -> x23.884984, again:x15.992188, dgain:x1.493541 */
	{0x07ff, 0x0609}, /* index:295, gain:27.65625db -> x24.144182, again:x15.992188, dgain:x1.509749 */
	{0x07ff, 0x061a}, /* index:296, gain:27.75000db -> x24.406193, again:x15.992188, dgain:x1.526132 */
	{0x07ff, 0x062b}, /* index:297, gain:27.84375db -> x24.671041, again:x15.992188, dgain:x1.542693 */
	{0x07ff, 0x063c}, /* index:298, gain:27.93750db -> x24.938770, again:x15.992188, dgain:x1.559435 */
	{0x07ff, 0x064e}, /* index:299, gain:28.03125db -> x25.209397, again:x15.992188, dgain:x1.576357 */
	{0x07ff, 0x065f}, /* index:300, gain:28.12500db -> x25.482967, again:x15.992188, dgain:x1.593464 */
	{0x07ff, 0x0671}, /* index:301, gain:28.21875db -> x25.759507, again:x15.992188, dgain:x1.610756 */
	{0x07ff, 0x0683}, /* index:302, gain:28.31250db -> x26.039040, again:x15.992188, dgain:x1.628235 */
	{0x07ff, 0x0695}, /* index:303, gain:28.40625db -> x26.321614, again:x15.992188, dgain:x1.645905 */
	{0x07ff, 0x06a7}, /* index:304, gain:28.50000db -> x26.607248, again:x15.992188, dgain:x1.663765 */
	{0x07ff, 0x06ba}, /* index:305, gain:28.59375db -> x26.895988, again:x15.992188, dgain:x1.681820 */
	{0x07ff, 0x06cc}, /* index:306, gain:28.68750db -> x27.187861, again:x15.992188, dgain:x1.700071 */
	{0x07ff, 0x06df}, /* index:307, gain:28.78125db -> x27.482895, again:x15.992188, dgain:x1.718520 */
	{0x07ff, 0x06f2}, /* index:308, gain:28.87500db -> x27.781137, again:x15.992188, dgain:x1.737169 */
	{0x07ff, 0x0706}, /* index:309, gain:28.96875db -> x28.082609, again:x15.992188, dgain:x1.756020 */
	{0x07ff, 0x0719}, /* index:310, gain:29.06250db -> x28.387360, again:x15.992188, dgain:x1.775077 */
	{0x07ff, 0x072d}, /* index:311, gain:29.15625db -> x28.695417, again:x15.992188, dgain:x1.794340 */
	{0x07ff, 0x0741}, /* index:312, gain:29.25000db -> x29.006810, again:x15.992188, dgain:x1.813811 */
	{0x07ff, 0x0755}, /* index:313, gain:29.34375db -> x29.321590, again:x15.992188, dgain:x1.833495 */
	{0x07ff, 0x0769}, /* index:314, gain:29.43750db -> x29.639778, again:x15.992188, dgain:x1.853391 */
	{0x07ff, 0x077e}, /* index:315, gain:29.53125db -> x29.961427, again:x15.992188, dgain:x1.873504 */
	{0x07ff, 0x0793}, /* index:316, gain:29.62500db -> x30.286567, again:x15.992188, dgain:x1.893835 */
	{0x07ff, 0x07a8}, /* index:317, gain:29.71875db -> x30.615226, again:x15.992188, dgain:x1.914386 */
	{0x07ff, 0x07bd}, /* index:318, gain:29.81250db -> x30.947461, again:x15.992188, dgain:x1.935161 */
	{0x07ff, 0x07d3}, /* index:319, gain:29.90625db -> x31.283292, again:x15.992188, dgain:x1.956161 */
	{0x07ff, 0x07e8}, /* index:320, gain:30.00000db -> x31.622777, again:x15.992188, dgain:x1.977389 */
	{0x07ff, 0x07fe}, /* index:321, gain:30.09375db -> x31.965945, again:x15.992188, dgain:x1.998848 */
	{0x07ff, 0x0815}, /* index:322, gain:30.18750db -> x32.312829, again:x15.992188, dgain:x2.020538 */
	{0x07ff, 0x082b}, /* index:323, gain:30.28125db -> x32.663485, again:x15.992188, dgain:x2.042465 */
	{0x07ff, 0x0842}, /* index:324, gain:30.37500db -> x33.017938, again:x15.992188, dgain:x2.064629 */
	{0x07ff, 0x0859}, /* index:325, gain:30.46875db -> x33.376247, again:x15.992188, dgain:x2.087034 */
	{0x07ff, 0x0870}, /* index:326, gain:30.56250db -> x33.738444, again:x15.992188, dgain:x2.109683 */
	{0x07ff, 0x0887}, /* index:327, gain:30.65625db -> x34.104562, again:x15.992188, dgain:x2.132576 */
	{0x07ff, 0x089f}, /* index:328, gain:30.75000db -> x34.474663, again:x15.992188, dgain:x2.155719 */
	{0x07ff, 0x08b7}, /* index:329, gain:30.84375db -> x34.848770, again:x15.992188, dgain:x2.179112 */
	{0x07ff, 0x08cf}, /* index:330, gain:30.93750db -> x35.226947, again:x15.992188, dgain:x2.202760 */
	{0x07ff, 0x08e8}, /* index:331, gain:31.03125db -> x35.609227, again:x15.992188, dgain:x2.226664 */
	{0x07ff, 0x0900}, /* index:332, gain:31.12500db -> x35.995646, again:x15.992188, dgain:x2.250827 */
	{0x07ff, 0x0919}, /* index:333, gain:31.21875db -> x36.386269, again:x15.992188, dgain:x2.275253 */
	{0x07ff, 0x0933}, /* index:334, gain:31.31250db -> x36.781120, again:x15.992188, dgain:x2.299943 */
	{0x07ff, 0x094c}, /* index:335, gain:31.40625db -> x37.180267, again:x15.992188, dgain:x2.324902 */
	{0x07ff, 0x0966}, /* index:336, gain:31.50000db -> x37.583745, again:x15.992188, dgain:x2.350132 */
	{0x07ff, 0x0980}, /* index:337, gain:31.59375db -> x37.991591, again:x15.992188, dgain:x2.375634 */
	{0x07ff, 0x099b}, /* index:338, gain:31.68750db -> x38.403873, again:x15.992188, dgain:x2.401415 */
	{0x07ff, 0x09b5}, /* index:339, gain:31.78125db -> x38.820619, again:x15.992188, dgain:x2.427474 */
	{0x07ff, 0x09d0}, /* index:340, gain:31.87500db -> x39.241898, again:x15.992188, dgain:x2.453817 */
	{0x07ff, 0x09eb}, /* index:341, gain:31.96875db -> x39.667748, again:x15.992188, dgain:x2.480445 */
	{0x07ff, 0x0a07}, /* index:342, gain:32.06250db -> x40.098209, again:x15.992188, dgain:x2.507362 */
	{0x07ff, 0x0a23}, /* index:343, gain:32.15625db -> x40.533352, again:x15.992188, dgain:x2.534572 */
	{0x07ff, 0x0a3f}, /* index:344, gain:32.25000db -> x40.973206, again:x15.992188, dgain:x2.562076 */
	{0x07ff, 0x0a5c}, /* index:345, gain:32.34375db -> x41.417845, again:x15.992188, dgain:x2.589880 */
	{0x07ff, 0x0a78}, /* index:346, gain:32.43750db -> x41.867309, again:x15.992188, dgain:x2.617985 */
	{0x07ff, 0x0a95}, /* index:347, gain:32.53125db -> x42.321639, again:x15.992188, dgain:x2.646395 */
	{0x07ff, 0x0ab3}, /* index:348, gain:32.62500db -> x42.780911, again:x15.992188, dgain:x2.675113 */
	{0x07ff, 0x0ad1}, /* index:349, gain:32.71875db -> x43.245154, again:x15.992188, dgain:x2.704143 */
	{0x07ff, 0x0aef}, /* index:350, gain:32.81250db -> x43.714448, again:x15.992188, dgain:x2.733488 */
	{0x07ff, 0x0b0d}, /* index:351, gain:32.90625db -> x44.188835, again:x15.992188, dgain:x2.763151 */
	{0x07ff, 0x0b2c}, /* index:352, gain:33.00000db -> x44.668357, again:x15.992188, dgain:x2.793136 */
	{0x07ff, 0x0b4b}, /* index:353, gain:33.09375db -> x45.153095, again:x15.992188, dgain:x2.823447 */
	{0x07ff, 0x0b6a}, /* index:354, gain:33.18750db -> x45.643081, again:x15.992188, dgain:x2.854086 */
	{0x07ff, 0x0b8a}, /* index:355, gain:33.28125db -> x46.138397, again:x15.992188, dgain:x2.885059 */
	{0x07ff, 0x0baa}, /* index:356, gain:33.37500db -> x46.639088, again:x15.992188, dgain:x2.916367 */
	{0x07ff, 0x0bca}, /* index:357, gain:33.46875db -> x47.145199, again:x15.992188, dgain:x2.948014 */
	{0x07ff, 0x0beb}, /* index:358, gain:33.56250db -> x47.656816, again:x15.992188, dgain:x2.980006 */
	{0x07ff, 0x0c0c}, /* index:359, gain:33.65625db -> x48.173972, again:x15.992188, dgain:x3.012344 */
	{0x07ff, 0x0c2e}, /* index:360, gain:33.75000db -> x48.696753, again:x15.992188, dgain:x3.045034 */
	{0x07ff, 0x0c4f}, /* index:361, gain:33.84375db -> x49.225207, again:x15.992188, dgain:x3.078078 */
	{0x07ff, 0x0c72}, /* index:362, gain:33.93750db -> x49.759382, again:x15.992188, dgain:x3.111481 */
	{0x07ff, 0x0c94}, /* index:363, gain:34.03125db -> x50.299367, again:x15.992188, dgain:x3.145246 */
	{0x07ff, 0x0cb7}, /* index:364, gain:34.12500db -> x50.845199, again:x15.992188, dgain:x3.179377 */
	{0x07ff, 0x0cdb}, /* index:365, gain:34.21875db -> x51.396968, again:x15.992188, dgain:x3.213880 */
	{0x07ff, 0x0cfe}, /* index:366, gain:34.31250db -> x51.954725, again:x15.992188, dgain:x3.248757 */
	{0x07ff, 0x0d22}, /* index:367, gain:34.40625db -> x52.518520, again:x15.992188, dgain:x3.284011 */
	{0x07ff, 0x0d47}, /* index:368, gain:34.50000db -> x53.088447, again:x15.992188, dgain:x3.319649 */
	{0x07ff, 0x0d6c}, /* index:369, gain:34.59375db -> x53.664545, again:x15.992188, dgain:x3.355673 */
	{0x07ff, 0x0d91}, /* index:370, gain:34.68750db -> x54.246909, again:x15.992188, dgain:x3.392088 */
	{0x07ff, 0x0db7}, /* index:371, gain:34.78125db -> x54.835593, again:x15.992188, dgain:x3.428899 */
	{0x07ff, 0x0ddd}, /* index:372, gain:34.87500db -> x55.430651, again:x15.992188, dgain:x3.466108 */
	{0x07ff, 0x0e03}, /* index:373, gain:34.96875db -> x56.032181, again:x15.992188, dgain:x3.503722 */
	{0x07ff, 0x0e2a}, /* index:374, gain:35.06250db -> x56.640223, again:x15.992188, dgain:x3.541743 */
	{0x07ff, 0x0e52}, /* index:375, gain:35.15625db -> x57.254879, again:x15.992188, dgain:x3.580178 */
	{0x07ff, 0x0e79}, /* index:376, gain:35.25000db -> x57.876205, again:x15.992188, dgain:x3.619030 */
	{0x07ff, 0x0ea2}, /* index:377, gain:35.34375db -> x58.504258, again:x15.992188, dgain:x3.658302 */
	{0x07ff, 0x0eca}, /* index:378, gain:35.43750db -> x59.139143, again:x15.992188, dgain:x3.698002 */
	{0x07ff, 0x0ef3}, /* index:379, gain:35.53125db -> x59.780900, again:x15.992188, dgain:x3.738132 */
	{0x07ff, 0x0f1d}, /* index:380, gain:35.62500db -> x60.429639, again:x15.992188, dgain:x3.778698 */
	{0x07ff, 0x0f47}, /* index:381, gain:35.71875db -> x61.085418, again:x15.992188, dgain:x3.819704 */
	{0x07ff, 0x0f71}, /* index:382, gain:35.81250db -> x61.748296, again:x15.992188, dgain:x3.861154 */
	{0x07ff, 0x0f9c}, /* index:383, gain:35.90625db -> x62.418384, again:x15.992188, dgain:x3.903055 */
	{0x07ff, 0x0fc8}, /* index:384, gain:36.00000db -> x63.095728, again:x15.992188, dgain:x3.945409 */
	{0x07ff, 0x0ff3}, /* index:385, gain:36.09375db -> x63.780438, again:x15.992188, dgain:x3.988225 */
	{0x07ff, 0x1020}, /* index:386, gain:36.18750db -> x64.472580, again:x15.992188, dgain:x4.031505 */
	{0x07ff, 0x104d}, /* index:387, gain:36.28125db -> x65.172214, again:x15.992188, dgain:x4.075253 */
	{0x07ff, 0x107a}, /* index:388, gain:36.37500db -> x65.879459, again:x15.992188, dgain:x4.119478 */
	{0x07ff, 0x10a8}, /* index:389, gain:36.46875db -> x66.594360, again:x15.992188, dgain:x4.164181 */
	{0x07ff, 0x10d6}, /* index:390, gain:36.56250db -> x67.317038, again:x15.992188, dgain:x4.209370 */
	{0x07ff, 0x1105}, /* index:391, gain:36.65625db -> x68.047559, again:x15.992188, dgain:x4.255050 */
	{0x07ff, 0x1134}, /* index:392, gain:36.75000db -> x68.785987, again:x15.992188, dgain:x4.301224 */
	{0x07ff, 0x1164}, /* index:393, gain:36.84375db -> x69.532449, again:x15.992188, dgain:x4.347901 */
	{0x07ff, 0x1194}, /* index:394, gain:36.93750db -> x70.286991, again:x15.992188, dgain:x4.395083 */
	{0x07ff, 0x11c5}, /* index:395, gain:37.03125db -> x71.049741, again:x15.992188, dgain:x4.442778 */
	{0x07ff, 0x11f6}, /* index:396, gain:37.12500db -> x71.820768, again:x15.992188, dgain:x4.490991 */
	{0x07ff, 0x1228}, /* index:397, gain:37.21875db -> x72.600143, again:x15.992188, dgain:x4.539726 */
	{0x07ff, 0x125b}, /* index:398, gain:37.31250db -> x73.387995, again:x15.992188, dgain:x4.588990 */
	{0x07ff, 0x128e}, /* index:399, gain:37.40625db -> x74.184377, again:x15.992188, dgain:x4.638789 */
	{0x07ff, 0x12c1}, /* index:400, gain:37.50000db -> x74.989421, again:x15.992188, dgain:x4.689128 */
	{0x07ff, 0x12f5}, /* index:401, gain:37.59375db -> x75.803201, again:x15.992188, dgain:x4.740015 */
	{0x07ff, 0x132a}, /* index:402, gain:37.68750db -> x76.625792, again:x15.992188, dgain:x4.791452 */
	{0x07ff, 0x135f}, /* index:403, gain:37.78125db -> x77.457330, again:x15.992188, dgain:x4.843448 */
	{0x07ff, 0x1395}, /* index:404, gain:37.87500db -> x78.297871, again:x15.992188, dgain:x4.896008 */
	{0x07ff, 0x13cb}, /* index:405, gain:37.96875db -> x79.147554, again:x15.992188, dgain:x4.949139 */
	{0x07ff, 0x1402}, /* index:406, gain:38.06250db -> x80.006459, again:x15.992188, dgain:x5.002846 */
	{0x07ff, 0x143a}, /* index:407, gain:38.15625db -> x80.874662, again:x15.992188, dgain:x5.057136 */
	{0x07ff, 0x1472}, /* index:408, gain:38.25000db -> x81.752308, again:x15.992188, dgain:x5.112015 */
	{0x07ff, 0x14ab}, /* index:409, gain:38.34375db -> x82.639457, again:x15.992188, dgain:x5.167489 */
	{0x07ff, 0x14e4}, /* index:410, gain:38.43750db -> x83.536255, again:x15.992188, dgain:x5.223566 */
	{0x07ff, 0x151e}, /* index:411, gain:38.53125db -> x84.442785, again:x15.992188, dgain:x5.280252 */
	{0x07ff, 0x1559}, /* index:412, gain:38.62500db -> x85.359129, again:x15.992188, dgain:x5.337552 */
	{0x07ff, 0x1594}, /* index:413, gain:38.71875db -> x86.285441, again:x15.992188, dgain:x5.395475 */
	{0x07ff, 0x15d0}, /* index:414, gain:38.81250db -> x87.221781, again:x15.992188, dgain:x5.454024 */
	{0x07ff, 0x160d}, /* index:415, gain:38.90625db -> x88.168307, again:x15.992188, dgain:x5.513211 */
	{0x07ff, 0x164a}, /* index:416, gain:39.00000db -> x89.125104, again:x15.992188, dgain:x5.573040 */
	{0x07ff, 0x1688}, /* index:417, gain:39.09375db -> x90.092259, again:x15.992188, dgain:x5.633517 */
	{0x07ff, 0x16c7}, /* index:418, gain:39.18750db -> x91.069934, again:x15.992188, dgain:x5.694651 */
	{0x07ff, 0x1706}, /* index:419, gain:39.28125db -> x92.058194, again:x15.992188, dgain:x5.756448 */
	{0x07ff, 0x1746}, /* index:420, gain:39.37500db -> x93.057204, again:x15.992188, dgain:x5.818917 */
	{0x07ff, 0x1787}, /* index:421, gain:39.46875db -> x94.067055, again:x15.992188, dgain:x5.882063 */
	{0x07ff, 0x17c8}, /* index:422, gain:39.56250db -> x95.087839, again:x15.992188, dgain:x5.945893 */
	{0x07ff, 0x180a}, /* index:423, gain:39.65625db -> x96.119726, again:x15.992188, dgain:x6.010418 */
	{0x07ff, 0x184d}, /* index:424, gain:39.75000db -> x97.162784, again:x15.992188, dgain:x6.075641 */
	{0x07ff, 0x1890}, /* index:425, gain:39.84375db -> x98.217189, again:x15.992188, dgain:x6.141573 */
	{0x07ff, 0x18d5}, /* index:426, gain:39.93750db -> x99.283036, again:x15.992188, dgain:x6.208221 */
	{0x07ff, 0x191a}, /* index:427, gain:40.03125db -> x100.360449, again:x15.992188, dgain:x6.275592 */
	{0x07ff, 0x195f}, /* index:428, gain:40.12500db -> x101.449499, again:x15.992188, dgain:x6.343691 */
	{0x07ff, 0x19a6}, /* index:429, gain:40.21875db -> x102.550422, again:x15.992188, dgain:x6.412533 */
	{0x07ff, 0x19ed}, /* index:430, gain:40.31250db -> x103.663293, again:x15.992188, dgain:x6.482121 */
	{0x07ff, 0x1a35}, /* index:431, gain:40.40625db -> x104.788240, again:x15.992188, dgain:x6.552464 */
	{0x07ff, 0x1a7e}, /* index:432, gain:40.50000db -> x105.925396, again:x15.992188, dgain:x6.623571 */
	{0x07ff, 0x1ac8}, /* index:433, gain:40.59375db -> x107.074833, again:x15.992188, dgain:x6.695446 */
	{0x07ff, 0x1b12}, /* index:434, gain:40.68750db -> x108.236802, again:x15.992188, dgain:x6.768105 */
	{0x07ff, 0x1b5d}, /* index:435, gain:40.78125db -> x109.411381, again:x15.992188, dgain:x6.841552 */
	{0x07ff, 0x1ba9}, /* index:436, gain:40.87500db -> x110.598706, again:x15.992188, dgain:x6.915796 */
	{0x07ff, 0x1bf6}, /* index:437, gain:40.96875db -> x111.798917, again:x15.992188, dgain:x6.990846 */
	{0x07ff, 0x1c44}, /* index:438, gain:41.06250db -> x113.012089, again:x15.992188, dgain:x7.066706 */
	{0x07ff, 0x1c92}, /* index:439, gain:41.15625db -> x114.238490, again:x15.992188, dgain:x7.143394 */
	{0x07ff, 0x1ce2}, /* index:440, gain:41.25000db -> x115.478198, again:x15.992188, dgain:x7.220913 */
	{0x07ff, 0x1d32}, /* index:441, gain:41.34375db -> x116.731361, again:x15.992188, dgain:x7.299274 */
	{0x07ff, 0x1d83}, /* index:442, gain:41.43750db -> x117.998122, again:x15.992188, dgain:x7.378485 */
	{0x07ff, 0x1dd5}, /* index:443, gain:41.53125db -> x119.278565, again:x15.992188, dgain:x7.458552 */
	{0x07ff, 0x1e28}, /* index:444, gain:41.62500db -> x120.572968, again:x15.992188, dgain:x7.539492 */
	{0x07ff, 0x1e7c}, /* index:445, gain:41.71875db -> x121.881418, again:x15.992188, dgain:x7.621310 */
	{0x07ff, 0x1ed0}, /* index:446, gain:41.81250db -> x123.204068, again:x15.992188, dgain:x7.704016 */
	{0x07ff, 0x1f26}, /* index:447, gain:41.90625db -> x124.541071, again:x15.992188, dgain:x7.787619 */
	{0x07ff, 0x1f7d}, /* index:448, gain:42.00000db -> x125.892514, again:x15.992188, dgain:x7.872126 */
	{0x07ff, 0x1fd4}, /* index:449, gain:42.09375db -> x127.258691, again:x15.992188, dgain:x7.957554 */
	{0x07ff, 0x202c}, /* index:450, gain:42.18750db -> x128.639694, again:x15.992188, dgain:x8.043909 */
	{0x07ff, 0x2086}, /* index:451, gain:42.28125db -> x130.035684, again:x15.992188, dgain:x8.131201 */
	{0x07ff, 0x20e0}, /* index:452, gain:42.37500db -> x131.446823, again:x15.992188, dgain:x8.219440 */
	{0x07ff, 0x213c}, /* index:453, gain:42.46875db -> x132.873203, again:x15.992188, dgain:x8.308632 */
	{0x07ff, 0x2198}, /* index:454, gain:42.56250db -> x134.315135, again:x15.992188, dgain:x8.398797 */
	{0x07ff, 0x21f5}, /* index:455, gain:42.65625db -> x135.772714, again:x15.992188, dgain:x8.489940 */
	{0x07ff, 0x2254}, /* index:456, gain:42.75000db -> x137.246111, again:x15.992188, dgain:x8.582072 */
	{0x07ff, 0x22b3}, /* index:457, gain:42.84375db -> x138.735497, again:x15.992188, dgain:x8.675205 */
	{0x07ff, 0x2313}, /* index:458, gain:42.93750db -> x140.240969, again:x15.992188, dgain:x8.769342 */
	{0x07ff, 0x2375}, /* index:459, gain:43.03125db -> x141.762855, again:x15.992188, dgain:x8.864507 */
	{0x07ff, 0x23d7}, /* index:460, gain:43.12500db -> x143.301257, again:x15.992188, dgain:x8.960704 */
	{0x07ff, 0x243b}, /* index:461, gain:43.21875db -> x144.856353, again:x15.992188, dgain:x9.057945 */
	{0x07ff, 0x249f}, /* index:462, gain:43.31250db -> x146.428325, again:x15.992188, dgain:x9.156241 */
	{0x07ff, 0x2505}, /* index:463, gain:43.40625db -> x148.017275, again:x15.992188, dgain:x9.255599 */
	{0x07ff, 0x256c}, /* index:464, gain:43.50000db -> x149.623549, again:x15.992188, dgain:x9.356040 */
	{0x07ff, 0x25d4}, /* index:465, gain:43.59375db -> x151.247255, again:x15.992188, dgain:x9.457571 */
	{0x07ff, 0x263d}, /* index:466, gain:43.68750db -> x152.888580, again:x15.992188, dgain:x9.560204 */
	{0x07ff, 0x26a7}, /* index:467, gain:43.78125db -> x154.547717, again:x15.992188, dgain:x9.663951 */
	{0x07ff, 0x2713}, /* index:468, gain:43.87500db -> x156.224774, again:x15.992188, dgain:x9.768818 */
	{0x07ff, 0x277f}, /* index:469, gain:43.96875db -> x157.920115, again:x15.992188, dgain:x9.874829 */
	{0x07ff, 0x27ed}, /* index:470, gain:44.06250db -> x159.633854, again:x15.992188, dgain:x9.981990 */
	{0x07ff, 0x285c}, /* index:471, gain:44.15625db -> x161.366191, again:x15.992188, dgain:x10.090314 */
	{0x07ff, 0x28cc}, /* index:472, gain:44.25000db -> x163.117327, again:x15.992188, dgain:x10.199813 */
	{0x07ff, 0x293d}, /* index:473, gain:44.34375db -> x164.887375, again:x15.992188, dgain:x10.310495 */
	{0x07ff, 0x29b0}, /* index:474, gain:44.43750db -> x166.676723, again:x15.992188, dgain:x10.422384 */
	{0x07ff, 0x2a24}, /* index:475, gain:44.53125db -> x168.485488, again:x15.992188, dgain:x10.535487 */
	{0x07ff, 0x2a99}, /* index:476, gain:44.62500db -> x170.313882, again:x15.992188, dgain:x10.649818 */
	{0x07ff, 0x2b0f}, /* index:477, gain:44.71875db -> x172.162117, again:x15.992188, dgain:x10.765389 */
	{0x07ff, 0x2b87}, /* index:478, gain:44.81250db -> x174.030314, again:x15.992188, dgain:x10.882208 */
	{0x07ff, 0x2c00}, /* index:479, gain:44.90625db -> x175.918880, again:x15.992188, dgain:x11.000301 */
	{0x07ff, 0x2c7a}, /* index:480, gain:45.00000db -> x177.827941, again:x15.992188, dgain:x11.119676 */
	{0x07ff, 0x2cf6}, /* index:481, gain:45.09375db -> x179.757719, again:x15.992188, dgain:x11.240346 */
	{0x07ff, 0x2d73}, /* index:482, gain:45.18750db -> x181.708438, again:x15.992188, dgain:x11.362325 */
	{0x07ff, 0x2df1}, /* index:483, gain:45.28125db -> x183.680226, again:x15.992188, dgain:x11.485622 */
	{0x07ff, 0x2e70}, /* index:484, gain:45.37500db -> x185.673512, again:x15.992188, dgain:x11.610264 */
	{0x07ff, 0x2ef1}, /* index:485, gain:45.46875db -> x187.688429, again:x15.992188, dgain:x11.736257 */
	{0x07ff, 0x2f74}, /* index:486, gain:45.56250db -> x189.725212, again:x15.992188, dgain:x11.863619 */
	{0x07ff, 0x2ff8}, /* index:487, gain:45.65625db -> x191.784098, again:x15.992188, dgain:x11.992362 */
	{0x07ff, 0x307d}, /* index:488, gain:45.75000db -> x193.865221, again:x15.992188, dgain:x12.122496 */
	{0x07ff, 0x3104}, /* index:489, gain:45.84375db -> x195.969034, again:x15.992188, dgain:x12.254048 */
	{0x07ff, 0x318c}, /* index:490, gain:45.93750db -> x198.095678, again:x15.992188, dgain:x12.387028 */
	{0x07ff, 0x3215}, /* index:491, gain:46.03125db -> x200.245400, again:x15.992188, dgain:x12.521451 */
	{0x07ff, 0x32a1}, /* index:492, gain:46.12500db -> x202.418450, again:x15.992188, dgain:x12.657333 */
	{0x07ff, 0x332d}, /* index:493, gain:46.21875db -> x204.614970, again:x15.992188, dgain:x12.794683 */
	{0x07ff, 0x33bb}, /* index:494, gain:46.31250db -> x206.835439, again:x15.992188, dgain:x12.933530 */
	{0x07ff, 0x344b}, /* index:495, gain:46.40625db -> x209.080004, again:x15.992188, dgain:x13.073884 */
	{0x07ff, 0x34dc}, /* index:496, gain:46.50000db -> x211.348927, again:x15.992188, dgain:x13.215761 */
	{0x07ff, 0x356f}, /* index:497, gain:46.59375db -> x213.642472, again:x15.992188, dgain:x13.359178 */
	{0x07ff, 0x3604}, /* index:498, gain:46.68750db -> x215.960789, again:x15.992188, dgain:x13.504143 */
	{0x07ff, 0x369a}, /* index:499, gain:46.78125db -> x218.304381, again:x15.992188, dgain:x13.650689 */
	{0x07ff, 0x3731}, /* index:500, gain:46.87500db -> x220.673407, again:x15.992188, dgain:x13.798826 */
	{0x07ff, 0x37cb}, /* index:501, gain:46.96875db -> x223.068141, again:x15.992188, dgain:x13.948570 */
	{0x07ff, 0x3866}, /* index:502, gain:47.06250db -> x225.488862, again:x15.992188, dgain:x14.099939 */
	{0x07ff, 0x3903}, /* index:503, gain:47.15625db -> x227.935728, again:x15.992188, dgain:x14.252942 */
	{0x07ff, 0x39a1}, /* index:504, gain:47.25000db -> x230.409272, again:x15.992188, dgain:x14.407614 */
	{0x07ff, 0x3a41}, /* index:505, gain:47.34375db -> x232.909659, again:x15.992188, dgain:x14.563965 */
	{0x07ff, 0x3ae3}, /* index:506, gain:47.43750db -> x235.437180, again:x15.992188, dgain:x14.722012 */
	{0x07ff, 0x3b86}, /* index:507, gain:47.53125db -> x237.992130, again:x15.992188, dgain:x14.881775 */
	{0x07ff, 0x3c2c}, /* index:508, gain:47.62500db -> x240.574673, again:x15.992188, dgain:x15.043262 */
	{0x07ff, 0x3cd3}, /* index:509, gain:47.71875db -> x243.185374, again:x15.992188, dgain:x15.206511 */
	{0x07ff, 0x3d7c}, /* index:510, gain:47.81250db -> x245.824407, again:x15.992188, dgain:x15.371531 */
	{0x07ff, 0x3e27}, /* index:511, gain:47.90625db -> x248.492078, again:x15.992188, dgain:x15.538342 */
	{0x07ff, 0x3ed3}, /* index:512, gain:48.00000db -> x251.188698, again:x15.992188, dgain:x15.706963 */
};

