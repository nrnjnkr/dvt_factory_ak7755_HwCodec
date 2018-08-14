/*
 * Filename : ov2732.h
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

#ifndef __OV2732_H__
#define __OV2732_H__

#define OV2732_STANDBY			0x0100
#define OV2732_SWRESET			0x0103

#define OV2732_CID_M			0x300B
#define OV2732_CID_L			0x300C

#define OV2732_GRP_ACCESS		0x3208

#define OV2732_HTS_MSB			0x380C
#define OV2732_HTS_LSB			0x380D
#define OV2732_VTS_MSB			0x380E
#define OV2732_VTS_LSB			0x380F

#define OV2732_L_EXPO_HSB		0x3500
#define OV2732_L_EXPO_MSB		0x3501
#define OV2732_L_EXPO_LSB		0x3502

#define OV2732_L_AGAIN_MSB		0x3508
#define OV2732_L_AGAIN_LSB		0x3509
#define OV2732_L_DGAIN_MSB		0x350A
#define OV2732_L_DGAIN_LSB		0x350B

#define OV2732_S_AGAIN_MSB		0x350C
#define OV2732_S_AGAIN_LSB		0x350D
#define OV2732_S_DGAIN_MSB		0x350E
#define OV2732_S_DGAIN_LSB		0x350F

#define OV2732_S_EXPO_HSB		0x3510
#define OV2732_S_EXPO_MSB		0x3511
#define OV2732_S_EXPO_LSB		0x3512

#define OV2732_S_MAX_EXPO_MSB	0x377C
#define OV2732_S_MAX_EXPO_LSB	0x377D

#define OV2732_MIPI_CTRL00		0x4800

#define OV2732_MIPI_GATE		(1<<5)

#define OV2732_FORMAT1		0x3820

#define OV2732_V_FLIP			(1<<4)
#define OV2732_H_MIRROR		(1<<3)
#define OV2732_MIRROR_MASK	(OV2732_H_MIRROR + OV2732_V_FLIP)

#define OV2732_TPM_TRIGGER	0x4D12
#define OV2732_TPM_READ		0x4D13
#define OV2732_TPM_OFFSET	64

#define S_MAX_EXPO_1080P		288
#define S_MAX_EXPO_720P		288

#endif /* __OV2732_H__ */

