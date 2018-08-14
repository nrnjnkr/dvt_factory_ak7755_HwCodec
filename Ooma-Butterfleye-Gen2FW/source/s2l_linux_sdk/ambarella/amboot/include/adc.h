/**
 * adc.h
 *
 * Author: Roy Su <qiangsu@ambarella.com>
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

#ifndef __AMBOOT_ADC_H__
#define __AMBOOT_ADC_H__

/* ADC partition memory layout
 *
 * +-------------------------------------------------------------------------+
 * |  ADC  | adc_node |            | adc_node |            |
 * |       |----------| iso config |----------| tone_curve |...rgb2yuv...le...
 * |  hdr  |   next   |            |  next    |            |
 * +-------------------------------------------------------------------------+
 *              |                  ^    |                  ^ ......
 *              |__________________|    |__________________|
 *
 */

//add for smart 3a params in adc partition
#if defined(CONFIG_ARCH_S2L)
#define ADCFW_IMG_MAGIC			(0x12160513)
#elif defined(CONFIG_ARCH_S3L)
#define ADCFW_IMG_MAGIC			(0x13160513)
#elif defined(CONFIG_ARCH_S5)
#define ADCFW_IMG_MAGIC			(0x11160513)
#else
#define ADCFW_IMG_MAGIC			(0xFF160513)
#endif

#define LAST_NODE_OFFSET	(0xFFFFFFFF)
struct adc_node {
	unsigned int pf_id;
	unsigned int bin_size;
	unsigned int next_offset;
}__attribute__((packed));

struct smart3a_file_info { /* 4 + (4 * 5) + 6 + 2 = 32 */
	unsigned int	offset;
	/* AWB */
	unsigned int	r_gain;
	unsigned int	b_gain;
	/* AE */
	unsigned int	d_gain;
	unsigned int	shutter;
	unsigned int	agc;
	/* Sensor 3 Shutter Register */
	unsigned char para0;
	unsigned char para1;
	unsigned char para2;
	/* Sensor 2 AGC Register */
	unsigned char para3;
	unsigned char para4;
	unsigned char para5;

	unsigned char rev[2];
}__attribute__((packed));

struct params_info { /* (4 * 1) + (5 * 4) + (5 * 4) + 32 + 2 + 18= 96 */
	unsigned char enable_audio;
	unsigned char enable_fastosd;
	unsigned char enable_ldc;
	unsigned char rotation_mode;

	unsigned int stream0_enable;
	unsigned int stream0_resolution;
	unsigned int stream0_fmt;
	unsigned int stream0_fps;
	unsigned int stream0_bitrate;

	unsigned int stream1_enable;
	unsigned int stream1_resolution;
	unsigned int stream1_fmt;
	unsigned int stream1_fps;
	unsigned int stream1_bitrate;

	char fastosd_string[32];
	unsigned char enable_vca;
	unsigned char vca_frame_num;
	unsigned char reserved[18];
}__attribute__((packed));

struct adcfw_header { /* 4 + 4 + 2 + 2 + 20 + (32 * 5) + 96 = 288 */
	unsigned int	magic;
	unsigned int	fw_size; /* totally fw size including the header */
	unsigned short	smart3a_num;
	unsigned short	smart3a_size;
	unsigned int	adc_hdr_size;
	unsigned char reserved[16];
	struct smart3a_file_info smart3a[5];
	struct params_info params_in_amboot;
}__attribute__((packed));

#endif
