/*
 * /s2lm_elektra_project/src/am_uart.c
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

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_leuart.h"

#include "config.h"
#include "am_base.h"
#include "am_uart.h"
#include "am_log.h"

// DEFINES ///////////////////////////////////////////////////////////////////

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////

static LEUART_Init_TypeDef leuartInit = LEUART_INIT_DEFAULT;


//static volatile uint8_t _uartWorkingMode = uartWorkingMode_Debug;
static volatile uint8_t _uartWorkingMode = uartWorkingMode_Communication;


// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

static void _put_char(const char ch);
static void SetUartMode(uint8_t mode);

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//functions be used when multi-threads access uart
#if 0
uint8_t UartGetChar(CircularBuffer* rxBuf)
{
  uint8_t ch;

  /* Check if there is a byte that is ready to be fetched. If no byte is ready, wait for incoming data */
  if (rxBuf->pendingBytes < 1)
  {
    while (rxBuf->pendingBytes < 1) ;
  }

  /* Copy data from buffer */
  ch        = rxBuf->data[rxBuf->rdI];
  rxBuf->rdI = (rxBuf->rdI + 1) % BUFFERSIZE;

  /* Decrement pending byte counter */
  rxBuf->pendingBytes--;

  return ch;
}

void UartPutChar(CircularBuffer* txBuf, uint8_t ch)
{
  /* Check if Tx queue has room for new data */
  if ((txBuf->pendingBytes + 1) > BUFFERSIZE)
  {
    /* Wait until there is room in queue */
    while ((txBuf->pendingBytes + 1) > BUFFERSIZE) ;
  }

  /* Copy ch into txBuffer */
  txBuf->data[txBuf->wrI] = ch;
  txBuf->wrI             = (txBuf->wrI + 1) % BUFFERSIZE;

  /* Increment pending byte counter */
  txBuf->pendingBytes++;

  /* Enable interrupt on LEUART TX Buffer*/
  LEUART_IntEnable(LEUART0, LEUART_IEN_TXBL);
}

void UartPutData(CircularBuffer* txBuf, uint8_t* dataPtr, uint32_t dataLen)
{
  uint32_t i = 0;

  /* Check if buffer is large enough for data */
  if (dataLen > BUFFERSIZE) {
    /* Buffer can never fit the requested amount of data */
    return;
  }

  /* Check if buffer has room for new data */
  if ((txBuf->pendingBytes + dataLen) > BUFFERSIZE) {
    /* Wait until room */
    while ((txBuf->pendingBytes + dataLen) > BUFFERSIZE) ;
  }

  /* Fill dataPtr[0:dataLen-1] into txBuffer */
  while (i < dataLen) {
    txBuf->data[txBuf->wrI] = *(dataPtr + i);
    txBuf->wrI             = (txBuf->wrI + 1) % BUFFERSIZE;
    i++;
  }

  /* Increment pending byte counter */
  txBuf->pendingBytes += dataLen;

  /* Enable interrupt on LEUART TX Buffer*/
  LEUART_IntEnable(LEUART0, LEUART_IEN_TXBL);
}

uint32_t UartGetData(CircularBuffer* rxBuf, uint8_t* dataPtr, uint32_t dataLen)
{
  uint32_t i = 0;

  /* Wait until the requested number of bytes are available */
  if (rxBuf->pendingBytes < dataLen) {
    while (rxBuf->pendingBytes < dataLen) ;
  }

  if (dataLen == 0) {
    dataLen = rxBuf->pendingBytes;
  }

  /* Copy data from Rx buffer to dataPtr */
  while (i < dataLen) {
    *(dataPtr + i) = rxBuf->data[rxBuf->rdI];
    rxBuf->rdI      = (rxBuf->rdI + 1) % BUFFERSIZE;
    i++;
  }

  /* Decrement pending byte counter */
  rxBuf->pendingBytes -= dataLen;

  return i;
}
#endif
////////////////////////////////////////////////////////
void UartSetup(bool enable)
{
  /* Enable clock for HF peripherals */
  //CMU_ClockEnable(cmuClock_CORELE, true);
  //CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);

  /* Enable clock for LEUART module */
  //CMU_ClockEnable(cmuClock_LEUART0, true);
  //CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_HFCLKLE);

  //CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_8);


  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_HFCLKLE);

  CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_8);

  /* Enable clock for LEUART module */
  CMU_ClockEnable(cmuClock_LEUART0, true);
  //leuartInit.baudrate = 300;

  /* Initialize LEUART with leuartInit struct */
  LEUART_Reset(LEUART0);
  LEUART_Init(LEUART0, &leuartInit);

  /* Configure GPIO pins */
  GpioSetup(gpioGroup_UART, true);

  /* Prepare UART Rx and Tx interrupts */
  LEUART_IntClear(LEUART0, _LEUART_IFC_MASK);
  if (enable) LEUART_IntEnable(LEUART0, LEUART_IEN_RXDATAV);
  NVIC_ClearPendingIRQ(LEUART0_IRQn);
  NVIC_EnableIRQ(LEUART0_IRQn);

  /* Enable I/O pins at UART1 location #2 */
  LEUART0->ROUTE = LEUART_ROUTE_RXPEN | LEUART_ROUTE_TXPEN | LEUART_ROUTE_LOCATION_LOC0;

  /* Enable LEUART */
  LEUART_Enable(LEUART0, leuartEnable);
}

void UartRxIntEnable(void)
{
  LEUART_IntClear(LEUART0, LEUART_IF_RXDATAV);
  LEUART_IntEnable(LEUART0, LEUART_IEN_RXDATAV);
}

void UartRxIntDisable(void)
{
  LEUART_IntDisable(LEUART0, LEUART_IEN_RXDATAV);
}

void tek_print(const char* format, ...)
{
	// MCU has limited resources(RAM),
#if DEBUG == 1
  // TODO?
#else
  // TODO?
#endif

    char dest[256];
    va_list argptr;
    va_start(argptr, format);
    // TODO: check buffer size
    vsprintf(dest, format, argptr);
    va_end(argptr);
    for (int i = 0; i< strlen(dest); i++)
    {
    	_put_char(dest[i]);
    }
    _put_char('n');
    _put_char('a');
    _put_char('r');
    _put_char('e');
    _put_char('s');
    _put_char('h');
}

void tek_send_msg(uint8_t *buf, int len)
{
    int i;

    for (i=0; i<len; i++) {
        LEUART_Tx(LEUART0, (uint8_t)(buf[i]));
    }
}

void SendMsg(uint8_t msg)
{
#if 0
//#ifndef ENABLE_UART_PRINT
	if (_uartWorkingMode == uartWorkingMode_Communication) {
		_put_char(msg);
	}
//#endif
#endif
}

uint8_t RecvMsg(void)
{
  return LEUART_Rx(LEUART0);
}

void SetUartWorkingMode()
{
	if (_uartWorkingMode == uartWorkingMode_Communication) {
	  _uartWorkingMode = uartWorkingMode_Debug;
	  LOG_DEBUG("****Change UART Working Mode to Debug Mode****\n");
	} else {
	  LOG_DEBUG("****Change UART Working Mode to Communication Mode****\n");
	  _uartWorkingMode = uartWorkingMode_Communication;
	  SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');
	  SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');
	  SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');SendMsg('*');
	}
	return;
}

void SetUartDebugMode()
{
    SetUartMode(uartWorkingMode_Debug);
}

void SetUartCommunicationMode()
{
    SetUartMode(uartWorkingMode_Communication);
}

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////

static void _put_char(const char ch)
{
    if (_uartWorkingMode == uartWorkingMode_Debug)
    {
        LEUART_Tx(LEUART0, (uint8_t)ch);
    }
}

static void SetUartMode(uint8_t mode)
{
    if (mode == uartWorkingMode_Communication) {
        LOG_DEBUG("****Change UART Working Mode to Communication Mode****\n");
        _uartWorkingMode = uartWorkingMode_Communication;
    }
    else if(mode == uartWorkingMode_Debug)
    {
        _uartWorkingMode = uartWorkingMode_Debug;
        LOG_DEBUG("****Change UART Working Mode to Debug Mode****\n");
    }
    else {
        //
    }
}
