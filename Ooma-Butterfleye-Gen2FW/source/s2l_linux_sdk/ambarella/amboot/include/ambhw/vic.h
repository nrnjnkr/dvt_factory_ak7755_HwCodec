/**
 * ambhw/vic.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __AMBHW__VIC_H__
#define __AMBHW__VIC_H__

#include <ambhw/chip.h>

/* ==========================================================================*/
#if (CHIP_REV == S3L)
#define VIC_INSTANCES			(3)
#define VIC_SUPPORT_CPU_OFFLOAD		(2)
#define VIC_SUPPORT_REPRIORITIZE	(1)
#elif (CHIP_REV == S2L)
#define VIC_INSTANCES			(3)
#define VIC_SUPPORT_CPU_OFFLOAD		(1)
#define VIC_SUPPORT_REPRIORITIZE	(1)
#elif (CHIP_REV == S3)
#define VIC_INSTANCES			(4)
#define VIC_SUPPORT_CPU_OFFLOAD		(2)
#define VIC_SUPPORT_REPRIORITIZE	(1)
#else
#error "Not supported!"
#endif

#if (VIC_SUPPORT_CPU_OFFLOAD == 2)
#if (CHIP_REV == S3)
#define VIC_NULL_PRI_IRQ_VAL		(0x00000060)
#define VIC_NULL_PRI_IRQ_FIX		(0)
#else
#define VIC_NULL_PRI_IRQ_VAL		(0xffffffff)
#define VIC_NULL_PRI_IRQ_FIX		(0)
#endif
#endif

#if (CHIP_REV == S3) || (CHIP_REV == S3L)
#define VIC_2NDGEN_BITASSIGNMENT	(1)
#endif

/* ==========================================================================*/
#define VIC0_OFFSET			0x3000
#define VIC1_OFFSET			0x10000
#define VIC2_OFFSET			0x1C000
#define VIC3_OFFSET			0x11000

#define VIC0_BASE			(AHB_BASE + VIC0_OFFSET)
#define VIC1_BASE			(AHB_BASE + VIC1_OFFSET)
#define VIC2_BASE			(AHB_BASE + VIC2_OFFSET)
#define VIC3_BASE			(AHB_BASE + VIC3_OFFSET)

#define VIC0_REG(x)			(VIC0_BASE + (x))
#define VIC1_REG(x)			(VIC1_BASE + (x))
#define VIC2_REG(x)			(VIC2_BASE + (x))
#define VIC3_REG(x)			(VIC3_BASE + (x))
#define VIC_REG(b, x)			((b) == 0 ? VIC0_REG((x)) : \
					 (b) == 1 ? VIC1_REG((x)) : \
					 (b) == 2 ? VIC2_REG((x)) : \
					 	    VIC3_REG((x)))

/* ==========================================================================*/
#define NR_VIC_IRQ_SIZE			(32)

#define VIC0_INT_VEC(x)			((x) + NR_VIC_IRQ_SIZE * 0)
#define VIC1_INT_VEC(x)			((x) + NR_VIC_IRQ_SIZE * 1)
#define VIC2_INT_VEC(x)			((x) + NR_VIC_IRQ_SIZE * 2)
#define VIC3_INT_VEC(x)			((x) + NR_VIC_IRQ_SIZE * 3)

/* ==========================================================================*/
#define VIC_IRQ_STA_OFFSET			(0x00)
#define VIC_FIQ_STA_OFFSET			(0x04)
#define VIC_RAW_STA_OFFSET			(0x08)
#define VIC_INT_SEL_OFFSET			(0x0c)
#define VIC_INTEN_OFFSET			(0x10)
#define VIC_INTEN_CLR_OFFSET			(0x14)
#define VIC_SOFTEN_OFFSET			(0x18)
#define VIC_SOFTEN_CLR_OFFSET			(0x1c)
#define VIC_PROTEN_OFFSET			(0x20)
#define VIC_SENSE_OFFSET			(0x24)
#define VIC_BOTHEDGE_OFFSET			(0x28)
#define VIC_EVENT_OFFSET			(0x2c)
#define VIC_INT_PTR0_OFFSET			(0x30)
#define VIC_INT_PTR1_OFFSET			(0x34)
#define VIC_EDGE_CLR_OFFSET			(0x38)
#define VIC_INT_SEL_INT_OFFSET			(0x3c)
#define VIC_INT_SEL_CLR_INT_OFFSET		(0x40)
#define VIC_INT_EN_INT_OFFSET			(0x44)
#define VIC_INT_EN_CLR_INT_OFFSET		(0x48)
#define VIC_SOFT_INT_INT_OFFSET			(0x4c)
#define VIC_SOFT_INT_CLR_INT_OFFSET		(0x50)
#define VIC_INT_SENSE_INT_OFFSET		(0x54)
#define VIC_INT_SENSE_CLR_INT_OFFSET		(0x58)
#define VIC_INT_BOTHEDGE_INT_OFFSET		(0x5c)
#define VIC_INT_BOTHEDGE_CLR_INT_OFFSET		(0x60)
#define VIC_INT_EVT_INT_OFFSET			(0x64)
#define VIC_INT_EVT_CLR_INT_OFFSET		(0x68)
#define VIC_INT_PENDING_OFFSET			(0x6c)
#define VIC_INT_RE_PRIORITIZE_EN_OFFSET		(0x70)
#define VIC_INT_PRIORITY_0_OFFSET		(0x74)
#define VIC_INT_PRIORITY_1_OFFSET		(0x78)
#define VIC_INT_PRIORITY_2_OFFSET		(0x7c)
#define VIC_INT_PRIORITY_3_OFFSET		(0x80)
#define VIC_INT_PRIORITY_4_OFFSET		(0x84)
#define VIC_INT_PRIORITY_5_OFFSET		(0x88)
#define VIC_INT_DELAY_EN_OFFSET			(0x8c)
#define VIC_INT_DELAY_OFFSET			(0x90)
#define VIC_INT_PENDING_C1_REG			(0x94)
#define VIC_INT_EDGE_CLR_OFFSET			(0x98)

/* ==========================================================================*/
#if (CHIP_REV == S2L)
#define USBVBUS_IRQ			VIC0_INT_VEC(0)
#define VOUT_IRQ			VIC0_INT_VEC(1)
#define VIN_IRQ				VIC0_INT_VEC(2)
#define CODE_VDSP_0_IRQ			VIC0_INT_VEC(3)
#define USBC_IRQ			VIC0_INT_VEC(4)
#define USB_CHARGE_IRQ			VIC0_INT_VEC(5)
#define SD2CD_IRQ			VIC0_INT_VEC(6)
#define I2STX_IRQ			VIC0_INT_VEC(7)
#define I2SRX_IRQ			VIC0_INT_VEC(8)
#define UART0_IRQ			VIC0_INT_VEC(9)
#define GPIO0_IRQ			VIC0_INT_VEC(10)
#define GPIO1_IRQ			VIC0_INT_VEC(11)
#define TIMER1_IRQ			VIC0_INT_VEC(12)
#define TIMER2_IRQ			VIC0_INT_VEC(13)
#define TIMER3_IRQ			VIC0_INT_VEC(14)
#define DMA_IRQ				VIC0_INT_VEC(15)
#define FIOCMD_IRQ			VIC0_INT_VEC(16)
#define FIODMA_IRQ			VIC0_INT_VEC(17)
#define SD_IRQ				VIC0_INT_VEC(18)
#define IDC_IRQ				VIC0_INT_VEC(19)
#define SD3_IRQ				VIC0_INT_VEC(20)
#define WDT_IRQ				VIC0_INT_VEC(21)
#define IRIF_IRQ			VIC0_INT_VEC(22)
#define SD1CD_IRQ			VIC0_INT_VEC(23)
#define SD0CD_IRQ			VIC0_INT_VEC(24)
#define UART1_IRQ			VIC0_INT_VEC(25)
#define MOTOR_IRQ			VIC0_INT_VEC(26)
#define ETH_IRQ				VIC0_INT_VEC(27)
#define USB_CONNECT_CHANGE_IRQ		VIC0_INT_VEC(28)
#define GPIO3_IRQ			VIC0_INT_VEC(29)
#define GPIO2_IRQ			VIC0_INT_VEC(30)

#define ETH_PMT_IRQ			VIC1_INT_VEC(0)
#define DMA_FIOS_IRQ			VIC1_INT_VEC(1)
#define ADC_LEVEL_IRQ			VIC1_INT_VEC(2)
#define SSI_MASTER0_IRQ			VIC1_INT_VEC(3)
#define IDC3_IRQ			VIC1_INT_VEC(4)
#define SSI_MASTER1_IRQ			VIC1_INT_VEC(5)
#define SSI_SLAVE_IRQ			VIC1_INT_VEC(6)
#define USB_EHCI_IRQ			VIC1_INT_VEC(7)
#define HDMI_IRQ			VIC1_INT_VEC(8)
#define FIOS_ECC_IRQ			VIC1_INT_VEC(9)
#define VOUT_TV_SYNC_IRQ		VIC1_INT_VEC(10)
#define VOUT_LCD_SYNC_IRQ		VIC1_INT_VEC(11)
#define USB_OHCI_IRQ			VIC1_INT_VEC(12)
#define NOR_SPI				VIC1_INT_VEC(13)
#define ORC_VOUT0_IRQ			VIC1_INT_VEC(14)
#define GDMA_IRQ			VIC1_INT_VEC(18)
#define IDC2_IRQ			VIC1_INT_VEC(19)
#define SD2_IRQ				VIC1_INT_VEC(20)
#define IDSP_PIP_VSYNC_IRQ		VIC1_INT_VEC(21)
#define IDSP_PIP_SOF_IRQ		VIC1_INT_VEC(22)
#define IDSP_PIP_MVSYNC_IRQ		VIC1_INT_VEC(23)
#define IDSP_PIP_LAST_PIXEL_IRQ		VIC1_INT_VEC(24)
#define IDSP_PIP_DVSYNC_IRQ		VIC1_INT_VEC(25)
#define VDSP_PIP_CODING_IRQ		VIC1_INT_VEC(26)
#define TIMER4_IRQ			VIC1_INT_VEC(27)
#define TIMER5_IRQ			VIC1_INT_VEC(28)
#define TIMER6_IRQ			VIC1_INT_VEC(29)
#define TIMER7_IRQ			VIC1_INT_VEC(30)
#define TIMER8_IRQ			VIC1_INT_VEC(31)

#define IDSP_VIN_MVSYNC_IRQ		VIC2_INT_VEC(0)
#define IDSP_VIN_VSYNC_IRQ		VIC2_INT_VEC(1)
#define IDSP_VIN_SOF_IRQ		VIC2_INT_VEC(2)
#define IDSP_VIN_DVSYNC_IRQ		VIC2_INT_VEC(3)
#define IDSP_VIN_LAST_PIXEL_IRQ		VIC2_INT_VEC(4)
#define L2CC_INTR_IRQ			VIC2_INT_VEC(21)
#define MD5_IRQ				VIC2_INT_VEC(22)
#define DES_IRQ				VIC2_INT_VEC(23)
#define AES_IRQ				VIC2_INT_VEC(24)
#define SHA1_IRQ			VIC2_INT_VEC(25)
#define USB_DIGITAL_ID_CHANGE_IRQ	VIC2_INT_VEC(27)
#define PMU_IRQ				VIC2_INT_VEC(28)
#define L2CC_DECERR_IRQ			VIC2_INT_VEC(29)
#define L2CC_SLVERR_IRQ			VIC2_INT_VEC(30)
#define L2CC_ECNTR_IRQ			VIC2_INT_VEC(31)

#define NR_IRQS				VIC2_INT_VEC(32)

/* ==========================================================================*/
#elif (CHIP_REV == S3)
#define IPI00_IRQ			VIC0_INT_VEC(0)
#define IPI01_IRQ			VIC0_INT_VEC(1)
#define IPI02_IRQ			VIC0_INT_VEC(2)
#define IPI03_IRQ			VIC0_INT_VEC(3)
#define IPI04_IRQ			VIC0_INT_VEC(4)
#define IPI05_IRQ			VIC0_INT_VEC(5)
#define IPI06_IRQ			VIC0_INT_VEC(6)
#define TIMER1_IRQ			VIC0_INT_VEC(7)
#define TIMER2_IRQ			VIC0_INT_VEC(8)
#define TIMER3_IRQ			VIC0_INT_VEC(9)
#define TIMER4_IRQ			VIC0_INT_VEC(10)
#define TIMER5_IRQ			VIC0_INT_VEC(11)
#define TIMER6_IRQ			VIC0_INT_VEC(12)
#define TIMER7_IRQ			VIC0_INT_VEC(13)
#define TIMER8_IRQ			VIC0_INT_VEC(14)
#define PMU_IRQ				VIC0_INT_VEC(15)
#define IPI10_IRQ			VIC0_INT_VEC(16)
#define IPI11_IRQ			VIC0_INT_VEC(17)
#define IPI12_IRQ			VIC0_INT_VEC(18)
#define IPI13_IRQ			VIC0_INT_VEC(19)
#define IPI14_IRQ			VIC0_INT_VEC(20)
#define IPI15_IRQ			VIC0_INT_VEC(21)
#define IPI16_IRQ			VIC0_INT_VEC(22)
#define TIMER11_IRQ			VIC0_INT_VEC(23)
#define TIMER12_IRQ			VIC0_INT_VEC(24)
#define TIMER13_IRQ			VIC0_INT_VEC(25)
#define TIMER14_IRQ			VIC0_INT_VEC(26)
#define TIMER15_IRQ			VIC0_INT_VEC(27)
#define TIMER16_IRQ			VIC0_INT_VEC(28)
#define TIMER17_IRQ			VIC0_INT_VEC(29)
#define TIMER18_IRQ			VIC0_INT_VEC(30)
#define PMU1_IRQ			VIC0_INT_VEC(31)

#define IDSP_VIN_STAT_IRQ		VIC1_INT_VEC(0)
#define IDSP_VIN_MVSYNC_IRQ		VIC1_INT_VEC(1)
#define IDSP_VIN_VSYNC_IRQ		VIC1_INT_VEC(2)
#define IDSP_VIN_SOF_IRQ		VIC1_INT_VEC(3)
#define IDSP_VIN_DVSYNC_IRQ		VIC1_INT_VEC(4)
#define IDSP_VIN_LAST_PIXEL_IRQ		VIC1_INT_VEC(5)
#define IDSP_PIP_STAT_IRQ		VIC1_INT_VEC(6)
#define IDSP_PIP_MVSYNC_IRQ		VIC1_INT_VEC(7)
#define IDSP_PIP_VSYNC_IRQ		VIC1_INT_VEC(8)
#define IDSP_PIP_SOF_IRQ		VIC1_INT_VEC(9)
#define IDSP_PIP_DVSYNC_IRQ		VIC1_INT_VEC(10)
#define IDSP_PIP_LAST_PIXEL_IRQ		VIC1_INT_VEC(11)
#define VOUT_TV_SYNC_IRQ		VIC1_INT_VEC(12)
#define VOUT_LCD_SYNC_IRQ		VIC1_INT_VEC(13)
#define CODE_VDSP_0_IRQ			VIC1_INT_VEC(14)
#define CODE_VDSP_1_IRQ			VIC1_INT_VEC(15)
#define CODE_VDSP_2_IRQ			VIC1_INT_VEC(16)
#define CODE_VDSP_3_IRQ			VIC1_INT_VEC(17)
#define CODE_VDSP_4_IRQ			VIC1_INT_VEC(18)
#define CODE_VDSP_5_IRQ			VIC1_INT_VEC(19)
#define CODE_VDSP_6_IRQ			VIC1_INT_VEC(20)
#define CODE_VDSP_7_IRQ			VIC1_INT_VEC(21)
#define VIN_IRQ				VIC1_INT_VEC(22)
#define ORC_VOUT0_IRQ			VIC1_INT_VEC(23)
#define VOUT_IRQ			VIC1_INT_VEC(24)
#define VDSP_ORC_BKPT_IRQ		VIC1_INT_VEC(25)
#define VDSP_EORC0_BKPT_IRQ		VIC1_INT_VEC(26)
#define VDSP_DORC_BRPT_IRQ		VIC1_INT_VEC(27)
#define VDSP_PIP_CODING_IRQ		VIC1_INT_VEC(28)
#define FDET_IRQ			VIC1_INT_VEC(29)
#define TIMER10_IRQ			VIC1_INT_VEC(30)
#define TIMER9_IRQ			VIC1_INT_VEC(31)

#define CANC_IRQ			VIC2_INT_VEC(0)
#define ETH_IRQ				VIC2_INT_VEC(1)
#define USB_EHCI_IRQ			VIC2_INT_VEC(2)
#define USB_OHCI_IRQ			VIC2_INT_VEC(3)
#define USBC_IRQ			VIC2_INT_VEC(4)
#define DMA_IRQ				VIC2_INT_VEC(5)
#define DMA_FIOS_IRQ			VIC2_INT_VEC(6)
#define FIOS_ECC_IRQ			VIC2_INT_VEC(7)
#define FIOCMD_IRQ			VIC2_INT_VEC(8)
#define FIODMA_IRQ			VIC2_INT_VEC(9)
#define GDMA_IRQ			VIC2_INT_VEC(10)
#define SD3_IRQ				VIC2_INT_VEC(11)
#define SD2_IRQ				VIC2_INT_VEC(12)
#define SD_IRQ				VIC2_INT_VEC(13)
#define NOR_SPI				VIC2_INT_VEC(14)
#define SSI_MASTER1_IRQ			VIC2_INT_VEC(15)
#define SSI_MASTER0_IRQ			VIC2_INT_VEC(16)
#define SSI_SLAVE_IRQ			VIC2_INT_VEC(17)
#define UART1_IRQ			VIC2_INT_VEC(18)
#define IDC3_IRQ			VIC2_INT_VEC(19)
#define IDC2_IRQ			VIC2_INT_VEC(20)
#define IDC_IRQ				VIC2_INT_VEC(21)
#define IRIF_IRQ			VIC2_INT_VEC(22)
#define I2STX_IRQ			VIC2_INT_VEC(23)
#define I2SRX_IRQ			VIC2_INT_VEC(24)
#define IDC_SLAVE_IRQ			VIC2_INT_VEC(25)
#define HIF_ARM2_IRQ			VIC2_INT_VEC(26)
#define HIF_ARM1_IRQ			VIC2_INT_VEC(27)
#define TS_CH1_RX_IRQ			VIC2_INT_VEC(28)
#define TS_CH0_RX_IRQ			VIC2_INT_VEC(29)
#define TS_CH1_TX_IRQ			VIC2_INT_VEC(30)
#define TS_CH0_TX_IRQ			VIC2_INT_VEC(31)

#define USBVBUS_IRQ			VIC3_INT_VEC(0)
#define USB_DIGITAL_ID_CHANGE_IRQ	VIC3_INT_VEC(1)
#define USB_CONNECT_CHANGE_IRQ		VIC3_INT_VEC(2)
#define USB_CHARGE_IRQ			VIC3_INT_VEC(3)
#define SD2CD_IRQ			VIC3_INT_VEC(4)
#define SD1CD_IRQ			VIC3_INT_VEC(5)
#define SD0CD_IRQ			VIC3_INT_VEC(6)
#define ADC_LEVEL_IRQ			VIC3_INT_VEC(7)
#define HDMI_IRQ			VIC3_INT_VEC(8)
#define WDT_IRQ				VIC3_INT_VEC(9)
#define SLIM_IRQ			VIC3_INT_VEC(10)
#define ETH_PMT_IRQ			VIC3_INT_VEC(11)
#define UART0_IRQ			VIC3_INT_VEC(12)
#define MOTOR_IRQ			VIC3_INT_VEC(13)
#define SHA1_IRQ			VIC3_INT_VEC(14)
#define AES_IRQ				VIC3_INT_VEC(15)
#define DES_IRQ				VIC3_INT_VEC(16)
#define MD5_IRQ				VIC3_INT_VEC(17)
#define L2CC_INTR_IRQ			VIC3_INT_VEC(18)
#define L2CC_INTR1_IRQ			VIC3_INT_VEC(19)
#define AXI_SWI_IRQ			VIC3_INT_VEC(20)
#define AXI_SWI1_IRQ			VIC3_INT_VEC(21)
#define GPIO6_IRQ			VIC3_INT_VEC(22)
#define GPIO5_IRQ			VIC3_INT_VEC(23)
#define GPIO4_IRQ			VIC3_INT_VEC(24)
#define GPIO3_IRQ			VIC3_INT_VEC(25)
#define GPIO2_IRQ			VIC3_INT_VEC(26)
#define GPIO1_IRQ			VIC3_INT_VEC(27)
#define GPIO0_IRQ			VIC3_INT_VEC(28)
#define DMIC_IRQ			VIC3_INT_VEC(29)
#define TIMER20_IRQ			VIC3_INT_VEC(30)
#define TIMER19_IRQ			VIC3_INT_VEC(31)

#define NR_IRQS				VIC3_INT_VEC(32)

/* ==========================================================================*/
#elif (CHIP_REV == S3L)
#define TIMER1_IRQ			VIC0_INT_VEC(0)
#define TIMER2_IRQ			VIC0_INT_VEC(1)
#define TIMER3_IRQ			VIC0_INT_VEC(2)
#define TIMER4_IRQ			VIC0_INT_VEC(3)
#define TIMER5_IRQ			VIC0_INT_VEC(4)
#define TIMER6_IRQ			VIC0_INT_VEC(5)
#define TIMER7_IRQ			VIC0_INT_VEC(6)
#define AXI_SOFT_IRQ(x)			VIC0_INT_VEC((x) + 7)	/* 0 <= x <= 13 */
#define AXI_SW_IRQ0			VIC0_INT_VEC(21)
#define AXI_SW_IRQ1			VIC0_INT_VEC(22)
#define PMU_IRQ				VIC0_INT_VEC(23)
#define L2CC_INTR_IRQ			VIC0_INT_VEC(24)
#define L2CC_DECERR_IRQ			VIC0_INT_VEC(25)
#define L2CC_SLVERR_IRQ			VIC0_INT_VEC(26)
#define L2CC_ECNTR_IRQ			VIC0_INT_VEC(27)
#define MD5_IRQ				VIC0_INT_VEC(28)
#define DES_IRQ				VIC0_INT_VEC(29)
#define AES_IRQ				VIC0_INT_VEC(30)
#define SHA1_IRQ			VIC0_INT_VEC(31)

#define IDSP_VIN_MVSYNC_IRQ		VIC1_INT_VEC(0)
#define IDSP_VIN_VSYNC_IRQ		VIC1_INT_VEC(1)
#define IDSP_VIN_SOF_IRQ		VIC1_INT_VEC(2)
#define IDSP_VIN_DVSYNC_IRQ		VIC1_INT_VEC(3)
#define IDSP_VIN_LAST_PIXEL_IRQ		VIC1_INT_VEC(4)
#define GDMA_IRQ			VIC1_INT_VEC(5)
#define IDSP_PIP_MVSYNC_IRQ		VIC1_INT_VEC(6)
#define IDSP_PIP_VSYNC_IRQ		VIC1_INT_VEC(7)
#define IDSP_PIP_SOF_IRQ		VIC1_INT_VEC(8)
#define IDSP_PIP_DVSYNC_IRQ		VIC1_INT_VEC(9)
#define IDSP_PIP_LAST_PIXEL_IRQ		VIC1_INT_VEC(10)
#define VOUT_TV_SYNC_IRQ		VIC1_INT_VEC(11)
#define VOUT_LCD_SYNC_IRQ		VIC1_INT_VEC(12)
#define CODE_VDSP_0_IRQ			VIC1_INT_VEC(13)
#define CODE_VDSP_1_IRQ			VIC1_INT_VEC(14)
#define CODE_VDSP_2_IRQ			VIC1_INT_VEC(15)
#define CODE_VDSP_3_IRQ			VIC1_INT_VEC(16)
#define GPIO3_IRQ			VIC1_INT_VEC(17)
#define GPIO2_IRQ			VIC1_INT_VEC(18)
#define GPIO1_IRQ			VIC1_INT_VEC(19)
#define GPIO0_IRQ			VIC1_INT_VEC(20)
#define VIN_IRQ				VIC1_INT_VEC(21)
#define VOUT_IRQ			VIC1_INT_VEC(22)
#define ORC_VOUT0_IRQ			VIC1_INT_VEC(23)
#define VDSP_PIP_CODING_IRQ		VIC1_INT_VEC(24)
#define ADC_LEVEL_IRQ			VIC1_INT_VEC(25)
#define HDMI_IRQ			VIC1_INT_VEC(26)
#define WDT_IRQ				VIC1_INT_VEC(27)
#define ETH_PMT_IRQ			VIC1_INT_VEC(28)
#define UART0_IRQ			VIC1_INT_VEC(29)
#define MOTOR_IRQ			VIC1_INT_VEC(30)
#define TIMER8_IRQ			VIC1_INT_VEC(31)

#define PWC_ALRAM			VIC2_INT_VEC(0)
#define ETH_IRQ				VIC2_INT_VEC(1)
#define USB_EHCI_IRQ			VIC2_INT_VEC(2)
#define USB_OHCI_IRQ			VIC2_INT_VEC(3)
#define USBC_IRQ			VIC2_INT_VEC(4)
#define DMA_IRQ				VIC2_INT_VEC(5)
#define DMA_FIOS_IRQ			VIC2_INT_VEC(6)
#define FIOS_ECC_IRQ			VIC2_INT_VEC(7)
#define FIOCMD_IRQ			VIC2_INT_VEC(8)
#define FIODMA_IRQ			VIC2_INT_VEC(9)
#define SD2_IRQ				VIC2_INT_VEC(10) /* SDXC rather than SDIO */
#define SD_IRQ				VIC2_INT_VEC(11)
#define NOR_SPI				VIC2_INT_VEC(12)
#define SSI_MASTER1_IRQ			VIC2_INT_VEC(13)
#define SSI_MASTER0_IRQ			VIC2_INT_VEC(14)
#define SSI_SLAVE_IRQ			VIC2_INT_VEC(15)
#define UART1_IRQ			VIC2_INT_VEC(16)
#define IDC3_IRQ			VIC2_INT_VEC(17)
#define IDC2_IRQ			VIC2_INT_VEC(18)
#define IDC_IRQ				VIC2_INT_VEC(19)
#define IRIF_IRQ			VIC2_INT_VEC(20)
#define I2STX_IRQ			VIC2_INT_VEC(21)
#define I2SRX_IRQ			VIC2_INT_VEC(22)
#define USBVBUS_IRQ			VIC2_INT_VEC(23)
#define USB_DIGITAL_ID_CHANGE_IRQ	VIC2_INT_VEC(24)
#define USB_CONNECT_CHANGE_IRQ		VIC2_INT_VEC(25)
#define USB_CHARGE_IRQ			VIC2_INT_VEC(26)
#define SD2CD_IRQ			VIC2_INT_VEC(27) /* SDXC rather than SDIO */
#define SD0CD_IRQ			VIC2_INT_VEC(28)

#define NR_IRQS				VIC2_INT_VEC(29)

/* ==========================================================================*/
#else
#error "Not supported!"
#endif


/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/
/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

