/*
 * ambhw/uart.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
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

#ifndef __AMBHW__UART_H__
#define __AMBHW__UART_H__

#include <ambhw/chip.h>

/* ==========================================================================*/
#if (CHIP_REV == S5L)
#define	UART_INSTANCES			3
#elif (CHIP_REV == CV1)
#define	UART_INSTANCES			7
#else
#define	UART_INSTANCES			2
#endif

/* ==========================================================================*/
#if (CHIP_REV == CV1)
#define UART0_OFFSET			0x0000
#else
#define UART0_OFFSET			0x5000
#endif

#if (CHIP_REV == CV1)
#define UART1_OFFSET			0x16000
#else
#define UART1_OFFSET			0x32000
#endif

#if (CHIP_REV == CV1)
#define UART2_OFFSET			0x17000
#else
#define UART2_OFFSET			0x33000
#endif

#if (CHIP_REV == CV1)
#define UART3_OFFSET			0x18000
#else
#define UART3_OFFSET			0x15000
#endif

#define UART4_OFFSET			0x19000
#define UART5_OFFSET			0x1A000
#define UART6_OFFSET			0x1B000

#define UART0_BASE			(APB_N_BASE + UART0_OFFSET)
#define UART1_BASE			(AHB_N_BASE + UART1_OFFSET)
#define UART2_BASE			(AHB_N_BASE + UART2_OFFSET)
#define UART3_BASE			(AHB_N_BASE + UART3_OFFSET)
#define UART4_BASE			(AHB_N_BASE + UART4_OFFSET)
#define UART5_BASE			(AHB_N_BASE + UART5_OFFSET)
#define UART6_BASE			(AHB_N_BASE + UART6_OFFSET)

#define UART0_REG(x)			(UART0_BASE + (x))
#define UART1_REG(x)			(UART1_BASE + (x))
#define UART2_REG(x)			(UART2_BASE + (x))
#define UART3_REG(x)			(UART3_BASE + (x))
#define UART4_REG(x)			(UART4_BASE + (x))
#define UART5_REG(x)			(UART5_BASE + (x))
#define UART6_REG(x)			(UART6_BASE + (x))

/* ==========================================================================*/
#define UART_RB_OFFSET			0x00
#define UART_TH_OFFSET			0x00
#define UART_DLL_OFFSET			0x00
#define UART_IE_OFFSET			0x04
#define UART_DLH_OFFSET			0x04
#define UART_II_OFFSET			0x08
#define UART_FC_OFFSET			0x08
#define UART_LC_OFFSET			0x0c
#define UART_MC_OFFSET			0x10
#define UART_LS_OFFSET			0x14
#define UART_MS_OFFSET			0x18
#define UART_SC_OFFSET			0x1c	/* Byte */
#define UART_DMAE_OFFSET		0x28
#define UART_DMAF_OFFSET		0x40	/* DMA fifo */
#define UART_US_OFFSET			0x7c
#define UART_TFL_OFFSET			0x80
#define UART_RFL_OFFSET			0x84
#define UART_SRR_OFFSET			0x88

/* UART[x]_IE_REG */
#define UART_IE_PTIME			0x80
#define UART_IE_ETOI			0x20
#define UART_IE_EBDI			0x10
#define UART_IE_EDSSI			0x08
#define UART_IE_ELSI			0x04
#define UART_IE_ETBEI			0x02
#define UART_IE_ERBFI			0x01

/* UART[x]_II_REG */
#define UART_II_MODEM_STATUS_CHANGED	0x00
#define UART_II_NO_INT_PENDING		0x01
#define UART_II_THR_EMPTY		0x02
#define UART_II_RCV_DATA_AVAIL		0x04
#define UART_II_RCV_STATUS		0x06
#define UART_II_CHAR_TIMEOUT		0x0c

/* UART[x]_FC_REG */
#define UART_FC_RX_ONECHAR		0x00
#define UART_FC_RX_QUARTER_FULL		0x40
#define UART_FC_RX_HALF_FULL		0x80
#define UART_FC_RX_2_TO_FULL		0xc0
#define UART_FC_TX_EMPTY		0x00
#define UART_FC_TX_2_IN_FIFO		0x10
#define UART_FC_TX_QUATER_IN_FIFO	0x20
#define UART_FC_TX_HALF_IN_FIFO		0x30
#define UART_FC_XMITR			0x04
#define UART_FC_RCVRR			0x02
#define UART_FC_FIFOE			0x01

/* UART[x]_LC_REG */
#define UART_LC_DLAB			0x80
#define UART_LC_BRK			0x40
#define UART_LC_EVEN_PARITY		0x10
#define UART_LC_ODD_PARITY		0x00
#define UART_LC_PEN			0x08
#define UART_LC_STOP_2BIT		0x04
#define UART_LC_STOP_1BIT		0x00
#define UART_LC_CLS_8_BITS		0x03
#define UART_LC_CLS_7_BITS		0x02
#define UART_LC_CLS_6_BITS		0x01
#define UART_LC_CLS_5_BITS		0x00
/*	quick defs */
#define	UART_LC_8N1			0x03
#define	UART_LC_7E1			0x0a

/* UART[x]_MC_REG */
#define UART_MC_SIRE			0x40
#define UART_MC_AFCE			0x20
#define UART_MC_LB			0x10
#define UART_MC_OUT2			0x08
#define UART_MC_OUT1			0x04
#define UART_MC_RTS			0x02
#define UART_MC_DTR			0x01

/* UART[x]_LS_REG */
#define UART_LS_FERR			0x80
#define UART_LS_TEMT			0x40
#define UART_LS_THRE			0x20
#define UART_LS_BI			0x10
#define UART_LS_FE			0x08
#define UART_LS_PE			0x04
#define UART_LS_OE			0x02
#define UART_LS_DR			0x01

/* UART[x]_MS_REG */
#define UART_MS_DCD			0x80
#define UART_MS_RI			0x40
#define UART_MS_DSR			0x20
#define UART_MS_CTS			0x10
#define UART_MS_DDCD			0x08
#define UART_MS_TERI			0x04
#define UART_MS_DDSR			0x02
#define UART_MS_DCTS			0x01

/* UART[x]_US_REG */
#define UART_US_RFF			0x10
#define UART_US_RFNE			0x08
#define UART_US_TFE			0x04
#define UART_US_TFNF			0x02
#define UART_US_BUSY			0x01

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

#if (CHIP_REV == S2L)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{39, 40}
#elif (CHIP_REV == S3)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{36, 37}
#elif (CHIP_REV == S3L)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{40, 41}
#elif (CHIP_REV == S5)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{31, 32}
#elif (CHIP_REV == S5L)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{45, 46}
#elif (CHIP_REV == CV1)
#define MINIPIN_UART_ALTFUNC		1
#define MINIPIN_UART_PIN		{51, 52}
#else
#error "MINIPIN uart: not defined!"
#endif

extern void uart_init(void);
extern void uart_putchar(char c);
extern void uart_getchar(char *c);
extern void uart_puthex(u32 h);
extern void uart_putbin(u32 h, int bits, int show_0);
extern void uart_putbyte(u32 h);
extern void uart_putdec(u32 d);
extern int  uart_putstr(const char *str);
extern int  uart_poll(void);
extern int  uart_read(void);
extern void uart_flush_input(void);
extern int  uart_wait_escape(int time);
extern int  uart_getstr(char *str, int n, int timeout);
extern int  uart_getcmd(char *cmd, int n, int timeout);
extern int  uart_getblock(char *buf, int n, int timeout);

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif /* __AMBHW__UART_H__ */

