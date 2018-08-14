/*
 * /s2lm_elektra_project/src/am_i2c.c
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

#include "em_i2c.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"

#include "config.h"
#include "am_i2c.h"
#include "am_uart.h"
#include "am_common.h"
#include "am_log.h"

// #define I2C_DEBUG

////////////////////////////////////////////////
void SetupI2C(uint8_t slaveAddr)
{
  CMU_ClockEnable(cmuClock_I2C0, true);
  // Using default settings
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

  // Using PA0 (SDA) and PA1 (SCL)
  GpioSetup(gpioGroup_I2C, true);

  // Enable pins at location 1
  I2C0->ROUTE = I2C_ROUTE_SDAPEN |
                I2C_ROUTE_SCLPEN |
                I2C_ROUTE_LOCATION_LOC0;

  /* Initializing the I2C */
  I2C_Init(I2C0, &i2cInit);

  return;
}

I2C_TransferReturn_TypeDef PerformI2CTransfer(I2C_TransferSeq_TypeDef* i2cTransferPtr)
{
  I2C_TransferReturn_TypeDef ret = i2cTransferInProgress;
  ret = I2C_TransferInit(I2C0, i2cTransferPtr);
  while (ret == i2cTransferInProgress) {
    ret = I2C_Transfer(I2C0);
  }

  return ret;
}

void scan_i2c_bus(void)
{
    uint8_t i, j;
    int res;

    tek_print("    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

    for (i = 0; i < 128; i += 16) {
        	tek_print("%02x  ", i);

        for(j = 0; j < 16; j++) {
            // i+j
            uint8_t slave_addr = (i + j)<<1;
            // needed?
            SetupI2C(slave_addr);
            // Delay(500);

            uint8_t txData = 0x00;
            uint8_t rxData = 0x00;
            I2C_TransferSeq_TypeDef i2cTransferSeqBuf = { 0 };
            i2cTransferSeqBuf.addr = slave_addr;
            i2cTransferSeqBuf.flags = I2C_FLAG_WRITE_READ;
            i2cTransferSeqBuf.buf[0].data = &txData;
            i2cTransferSeqBuf.buf[0].len = sizeof(txData);
            i2cTransferSeqBuf.buf[1].data = &rxData;
            i2cTransferSeqBuf.buf[1].len = sizeof(rxData);

            if (PerformI2CTransfer(&i2cTransferSeqBuf) != i2cTransferDone) {
                res = -1;
            }
            else
            {
                res = 0;
            }

            if (res < 0)
            {
            	tek_print("-- ");
            }
            else
            {
            	tek_print("%02x ", slave_addr);
            }
        }
        tek_print("\r\n");
    }
}

int i2c_write_byte(uint8_t slave_addr, uint8_t addr, uint8_t value)
{
    I2C_TransferSeq_TypeDef transfer = { 0 };
    transfer.addr = slave_addr;
    transfer.flags = I2C_FLAG_WRITE_WRITE;
    transfer.buf[0].data = &addr;
    transfer.buf[0].len = 1;

    transfer.buf[1].data = &value;
    transfer.buf[1].len = sizeof(value);

    if (PerformI2CTransfer(&transfer) != i2cTransferDone) {
#ifdef I2C_DEBUG
        LOG_DEBUG("ERROR: %s write: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
#endif
        return -1;
    }
#ifdef I2C_DEBUG
    LOG_DEBUG("%s write: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
#endif
    return 0;
}

int i2c_read_byte(uint8_t slave_addr, uint8_t addr, uint8_t* p_value)
{
    if (p_value == NULL)
    {
        return -1;
    }
    I2C_TransferSeq_TypeDef transfer = { 0 };
    uint8_t value = 0;
    transfer.addr = slave_addr;
    transfer.flags = I2C_FLAG_WRITE_READ;
    transfer.buf[0].data = &addr;
    transfer.buf[0].len = 1;

    transfer.buf[1].data = &value;
    transfer.buf[1].len = sizeof(value);

    if (PerformI2CTransfer(&transfer) != i2cTransferDone) {
#ifdef I2C_DEBUG
        LOG_DEBUG("ERROR: %s read: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
#endif
        return -2;
    }
#ifdef I2C_DEBUG
    LOG_DEBUG("%s read: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
#endif
    *p_value = value;
    return 0;
}

