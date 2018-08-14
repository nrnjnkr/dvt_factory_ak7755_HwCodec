/*
 * Filename : ov2735.h
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

#ifndef __OV2735_H__
#define __OV2735_H__

/****** page 0 ******/
#define OV2735_SWRESET		0x20

#define OV2735_CHIP_ID_H		0x02
#define OV2735_CHIP_ID_L		0x03

#define OV2735_PAGE_FLG		0xFD

/****** page 1 ******/
#define OV2735_FR_SYNC_EN	0x01

#define OV2735_EXPO_MSB		0x03
#define OV2735_EXPO_LSB		0x04

#define OV2735_AGAIN_MSB		0x38
#define OV2735_AGAIN_LSB		0x24
#define OV2735_DGAIN			0x39
#define OV2735_R_GAIN			0x40
#define OV2735_GR_GAIN		0x41
#define OV2735_GB_GAIN		0x42
#define OV2735_B_GAIN			0x43

#define OV2735_VB_MSB			0x05
#define OV2735_VB_LSB			0x06

#define OV2735_FR_LENGTH_CTRL	0x0D
#define OV2735_FR_LENGTH_MSB		0x0E
#define OV2735_FR_LENGTH_LSB		0x0F
#define OV2735_VTS_MSB		0x4E
#define OV2735_VTS_LSB		0x4F
#define OV2735_HTS_MSB		0x8C
#define OV2735_HTS_LSB		0x8D

#define OV2735_UPDOWN_MIRROR	0x3F

#define OV2735_V_FLIP				(1<<1)
#define OV2735_H_MIRROR			(1<<0)
#define OV2735_MIRROR_MASK		(OV2735_H_MIRROR + OV2735_V_FLIP)

#endif /* __OV2735_H__ */

