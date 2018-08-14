/*
 * /s2lm_elektra_project/include/am_uart.h
 *
 *  Created on: May 7, 2015
 *      Author: ChuChen
 *
 *  Copyright (C) 2015-2018, Ambarella, Inc.
 *  All rights reserved. No Part of this file may be reproduced, stored
 *  in a retrieval system, or transmitted, in any form, or by any means,
 *  electronic, mechanical, photocopying, recording, or otherwise,
 *  without the prior consent of Ambarella, Inc.
 */

#ifndef AM_UART_H_
#define AM_UART_H_

#define BUFFERSIZE          64

//include .h here
#include <stdio.h>
#include <stdarg.h>

typedef volatile struct
{
  uint8_t  data[BUFFERSIZE];
  uint32_t rdI;
  uint32_t wrI;
  uint32_t pendingBytes;
  bool     overflow;
} CircularBuffer;

typedef enum {
  uartWorkingMode_Debug = 0,
  uartWorkingMode_Communication,
} UART_Working_Mode_TypeDef;

typedef enum {
	CMD_RED_LED = 'R',
	CMD_GREEN_LED = 'G',
	CMD_BLUE_LED = 'B',
	CMD_PIR = 0,
	CMD_ACCELEROMETER,
	CMD_VOICE_DECT,
	CMD_SWT_DECT,
	CMD_MCU_RESET,
}mycommands_t;

void UartSetup(bool enable);
void UartRxIntEnable(void);
void UartRxIntDisable(void);
void tek_print(const char* format, ...);
void tek_send_msg(uint8_t *buf, int len);
void SendMsg(uint8_t msg);
uint8_t RecvMsg(void);
void SetUartWorkingMode(void);

void SetUartDebugMode();
void SetUartCommunicationMode();

#endif /* AM_UART_H_ */
