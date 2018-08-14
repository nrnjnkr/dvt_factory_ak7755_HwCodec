/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185_table.c
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

static struct vin_video_pll imx185_plls[] = {
	{0, 37125000, 74250000},
};

static struct vin_reg_16_8 imx185_pll_regs[][6] = {
	{
		{IMX185_INCKSEL1, 0x20},
		{IMX185_INCKSEL2, 0x00},
		{IMX185_INCKSEL3, 0x18},
		{IMX185_INCKSEL4, 0x00},
		{IMX185_INCKSEL5, 0x74},
	},
};

static struct vin_reg_16_8 imx185_mode_regs[][12] = {
	{	/* 1920x1200(12bits)@50fps */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE WUXGA MODE */
		{0x3009, 0x01}, /* FRSEL */
		{0x3018, 0x28}, /* VMAX_LSB */
		{0x3019, 0x05}, /* VMAX_MSB */
		{0x301A, 0x00}, /* VMAX_HSB */
		{0x301B, 0x65}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0xE1}, /* ODBIT_OPORTSEL, 12bits */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
	},
	{	/* 1920x1080(12bits)@60fps */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x10}, /* WINMODE 1080P MODE */
		{0x3009, 0x01}, /* FRSEL */
		{0x3018, 0x65}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301A, 0x00}, /* VMAX_HSB */
		{0x301B, 0x4C}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0xE1}, /* ODBIT_OPORTSEL, 12bits */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
	},
	{	/* 1280x720(10bits)@120fps */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x20}, /* WINMODE 720P MODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0xEE}, /* VMAX_LSB */
		{0x3019, 0x02}, /* VMAX_MSB */
		{0x301A, 0x00}, /* VMAX_HSB */
		{0x301B, 0x39}, /* HMAX_LSB */
		{0x301C, 0x03}, /* HMAX_MSB */
		{0x3044, 0xE0}, /* ODBIT_OPORTSEL, 10bits */
		{0x300A, 0x3C}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
	},
};

static struct vin_video_format imx185_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_WUXGA,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+14+4+8,
		.def_width	= 1920,
		.def_height	= 1200,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1920,
		.height		= 1200,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS(50),
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+8+4+8,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+4+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_120,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
};

static struct vin_reg_16_8 imx185_share_regs[] = {
	/* chip ID = 02h, do not change */
	{0x301D, 0x08},
	{0x301E, 0x02},
	{0x3048, 0x33},

	/* chip ID = 03h, do not change */
	{0x311D, 0x0A},
	{0x3123, 0x0F},
	{0x3147, 0x87},
	{0x31E1, 0x9E},
	{0x31E2, 0x01},
	{0x31E5, 0x05},
	{0x31E6, 0x05},
	{0x31E7, 0x3A},
	{0x31E8, 0x3A},

	/* chip ID = 04h, do not change */
	{0x3203, 0xC8},
	{0x3207, 0x54},
	{0x3213, 0x16},
	{0x3215, 0xF6},
	{0x321A, 0x14},
	{0x321B, 0x51},
	{0x3229, 0xE7},
	{0x322A, 0xF0},
	{0x322B, 0x10},
	{0x3231, 0xE7},
	{0x3232, 0xF0},
	{0x3233, 0x10},
	{0x323C, 0xE8},
	{0x323D, 0x70},
	{0x3243, 0x08},
	{0x3244, 0xE1},
	{0x3245, 0x10},
	{0x3247, 0xE7},
	{0x3248, 0x60},
	{0x3249, 0x1E},
	{0x324B, 0x00},
	{0x324C, 0x41},
	{0x3250, 0x30},
	{0x3251, 0x0A},
	{0x3252, 0xFF},
	{0x3253, 0xFF},
	{0x3254, 0xFF},
	{0x3255, 0x02},
	{0x3257, 0xF0},
	{0x325A, 0xA6},
	{0x325D, 0x14},
	{0x325E, 0x51},
	{0x3260, 0x00},
	{0x3261, 0x61},
	{0x3266, 0x30},
	{0x3267, 0x05},
	{0x3275, 0xE7},
	{0x3281, 0xEA},
	{0x3282, 0x70},
	{0x3285, 0xFF},
	{0x328A, 0xF0},
	{0x328D, 0xB6},
	{0x328E, 0x40},
	{0x3290, 0x42},
	{0x3291, 0x51},
	{0x3292, 0x1E},
	{0x3294, 0xC4},
	{0x3295, 0x20},
	{0x3297, 0x50},
	{0x3298, 0x31},
	{0x3299, 0x1F},
	{0x329B, 0xC0},
	{0x329C, 0x60},
	{0x329E, 0x4C},
	{0x329F, 0x71},
	{0x32A0, 0x1F},
	{0x32A2, 0xB6},
	{0x32A3, 0xC0},
	{0x32A4, 0x0B},
	{0x32A9, 0x24},
	{0x32AA, 0x41},
	{0x32B0, 0x25},
	{0x32B1, 0x51},
	{0x32B7, 0x1C},
	{0x32B8, 0xC1},
	{0x32B9, 0x12},
	{0x32BE, 0x1D},
	{0x32BF, 0xD1},
	{0x32C0, 0x12},
	{0x32C2, 0xA8},
	{0x32C3, 0xC0},
	{0x32C4, 0x0A},
	{0x32C5, 0x1E},
	{0x32C6, 0x21},
	{0x32C9, 0xB0},
	{0x32CA, 0x40},
	{0x32CC, 0x26},
	{0x32CD, 0xA1},
	{0x32D0, 0xB6},
	{0x32D1, 0xC0},
	{0x32D2, 0x0B},
	{0x32D4, 0xE2},
	{0x32D5, 0x40},
	{0x32D8, 0x4E},
	{0x32D9, 0xA1},
	{0x32EC, 0xF0},
};

#define IMX185_GAIN_MAX_DB	160
#define IMX185_GAIN_ROWS	161

