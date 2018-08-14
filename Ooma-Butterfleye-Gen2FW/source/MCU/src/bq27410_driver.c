/*
 * /s2lm_elektra_project/src/bq27410_driver.c
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

#include "am_common.h"
#include "bq27410_driver.h"
#include "am_i2c.h"

static I2C_TransferSeq_TypeDef i2cTransferSeqBuf;

///////////////////////////////////////////////////////////////////
static uint8_t _bq27410_read(uint8_t cmd, uint8_t bytes, uint8_t* rxData)
{
  uint8_t tx = cmd;
  i2cTransferSeqBuf.addr = I2C_SLAVE_ADDRESS;
  i2cTransferSeqBuf.flags = I2C_FLAG_WRITE_READ;
  i2cTransferSeqBuf.buf[0].data = &tx;
  i2cTransferSeqBuf.buf[0].len = 1;

  i2cTransferSeqBuf.buf[1].data = rxData;
  i2cTransferSeqBuf.buf[1].len = bytes;

  if (PerformI2CTransfer(&i2cTransferSeqBuf) != i2cTransferDone) {
    return bqProcess_Error;
  }

  Delay(50);

  return bqProcess_OK;
}

static uint8_t _bq27410_write(uint8_t cmd, uint8_t bytes, uint8_t* txData)
{
  uint8_t tx = cmd;
  i2cTransferSeqBuf.addr = I2C_SLAVE_ADDRESS;
  i2cTransferSeqBuf.flags = I2C_FLAG_WRITE_WRITE;
  i2cTransferSeqBuf.buf[0].data = &tx;
  i2cTransferSeqBuf.buf[0].len = 1;

  i2cTransferSeqBuf.buf[1].data = txData;
  i2cTransferSeqBuf.buf[1].len = bytes;

  if (PerformI2CTransfer(&i2cTransferSeqBuf) != i2cTransferDone) {
    return bqProcess_Error;
  }

  Delay(50);

  return bqProcess_OK;
}

static uint8_t _bq27410_datablock_read(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* rxData)
{
  uint8_t ret = bqProcess_OK;
  uint8_t txBuf = 0;
  ret = _bq27410_write(CMD_DFDCNTL, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf = classId;
  ret = _bq27410_write(CMD_DFCLS, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf = blockId;
  ret = _bq27410_write(CMD_DFBLK, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_read(CMD_DFD_LSB + offset, bytes, rxData);
  if (ret != bqProcess_OK) {
    return ret;
  }

  return bqProcess_OK;
}

static uint8_t _bq27410_datablock_write(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* txData)
{
  uint8_t ret = bqProcess_OK;
  uint8_t txBuf = 0;
  ret = _bq27410_write(CMD_DFDCNTL, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf = classId;
  ret = _bq27410_write(CMD_DFCLS, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf = blockId;
  ret = _bq27410_write(CMD_DFBLK, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_write(CMD_DFD_LSB + offset, bytes, txData);
  if (ret != bqProcess_OK) {
    return ret;
  }

  return bqProcess_OK;
}

static uint8_t _bq27410_checksum_rw(uint8_t rw, uint8_t* checkSum)
{
  uint8_t ret = bqProcess_OK;
  if (rw == 0) {
    ret = _bq27410_read(CMD_DFDCKS, 1, checkSum);
  } else {
    ret = _bq27410_write(CMD_DFDCKS, 1, checkSum);
  }

  return ret;
}

static uint8_t _update_checksum(uint8_t oldSum, uint8_t bytes, uint8_t* oldData, uint8_t* newData)
{
  uint8_t index = 0;
  oldSum = 0xFF - oldSum;
  for (index = 0; index < bytes; index++) {
    oldSum += newData[index] - oldData[index];
  }
  return (0xFF - oldSum);
}
//////////////////////////////////////////////////////////////////
uint8_t BQ27410_Read(uint8_t cmd, uint8_t bytes, uint8_t* rxData)
{
  return _bq27410_read(cmd, bytes, rxData);
}

uint8_t BQ27410_Write(uint8_t cmd, uint8_t bytes, uint8_t* txData)
{
  return _bq27410_write(cmd, bytes, txData);
}

uint8_t BQ27410_ReadV2(uint8_t txBytes, uint8_t* txData, uint8_t rxBytes, uint8_t* rxData)
{
  i2cTransferSeqBuf.addr = I2C_SLAVE_ADDRESS;
  i2cTransferSeqBuf.flags = I2C_FLAG_WRITE_READ;
  i2cTransferSeqBuf.buf[0].data = txData;
  i2cTransferSeqBuf.buf[0].len = txBytes;

  i2cTransferSeqBuf.buf[1].data = rxData;
  i2cTransferSeqBuf.buf[1].len = rxBytes;

  if (PerformI2CTransfer(&i2cTransferSeqBuf) != i2cTransferDone) {
    return bqProcess_Error;
  }

  return bqProcess_OK;
}

uint8_t BQ27410_CNTL_SubCmd_Read(uint8_t subcmd, uint8_t bytes, uint16_t* rxData)
{
  uint8_t txBuf[2] = {0};
  txBuf[0] = subcmd;
  txBuf[1] = CNTL_SUB_CMD_HIGH;
  uint8_t ret = _bq27410_write(CMD_CNTL_LSB, 2, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_read(CMD_CNTL_LSB, bytes, (uint8_t*)rxData);
  return ret;
}

uint8_t BQ27410_CNTL_SubCmd_Write(uint8_t subcmd, uint8_t bytes, uint16_t* txData)
{
  uint8_t txBuf[2] = {0};
  txBuf[0] = subcmd;
  txBuf[1] = CNTL_SUB_CMD_HIGH;
  uint8_t ret = _bq27410_write(CMD_CNTL_LSB, 2, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  if (bytes) ret = _bq27410_write(CMD_CNTL_LSB, bytes, (uint8_t*)txData);
  return ret;
}

uint8_t BQ27410_Temp_Read(uint16_t* temp)
{
  return _bq27410_read(CMD_TEMP_LSB, 2, (uint8_t*)temp);
}

uint8_t BQ27410_Vol_Read(uint16_t* voltage)
{
  return _bq27410_read(CMD_VOLT_LSB, 2, (uint8_t*)voltage);
}

uint8_t BQ27410_FCC_Read(uint16_t* fcc)
{
  return _bq27410_read(CMD_FCC_LSB, 2, (uint8_t*)fcc);
}

uint8_t BQ27410_FAC_Read(uint16_t* fac)
{
  return _bq27410_read(CMD_FAC_LSB, 2, (uint8_t*)fac);
}

uint8_t BQ27410_RM_Read(uint16_t* rm)
{
  return _bq27410_read(CMD_RM_LSB, 2, (uint8_t*)rm);
}

uint8_t BQ27410_SOC_Read(uint16_t* soc)
{
  return _bq27410_read(CMD_SOC_LSB, 2, (uint8_t*)soc);
}

uint8_t BQ27410_AI_Read(int16_t* ai)
{
  return _bq27410_read(CMD_AI_LSB, 2, (uint8_t*)ai);
}

uint8_t BQ27410_SI_Read(int16_t* si)
{
  return _bq27410_read(CMD_SI_LSB, 2, (uint8_t*)si);
}

uint8_t BQ27410_Flags_Read(uint16_t* flags)
{
  return _bq27410_read(CMD_FLAGS_LSB, 2, (uint8_t*)flags);
}

uint8_t BQ27410_DataBlock_Read(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* rxData, uint8_t* checkSum)
{
  uint8_t ret = bqProcess_OK;
  ret = _bq27410_datablock_read(classId, blockId, offset, bytes, rxData);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(0, checkSum);
  return ret;
}

uint8_t BQ27410_DataBlock_Write(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* txData, uint8_t checkSum)
{
  uint8_t ret = bqProcess_OK;
  ret = _bq27410_datablock_write(classId, blockId, offset, bytes, txData);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  return ret;
}

uint8_t Update_Checksum(uint8_t oldSum, uint8_t bytes, uint8_t* oldData, uint8_t* newData)
{
  return _update_checksum(oldSum, bytes, oldData, newData);
}

uint8_t BQ27410_Update_DesignCapacity(uint16_t capacity)
{
  uint8_t ret = bqProcess_OK;
  uint8_t rxBuf[2] = {0};
  uint8_t txBuf[2] = {0};
  ret = _bq27410_datablock_read(CONFIGURATION_SUBCLASS_DATACLASS_ID, DESIGN_CAPACITY_DATA_FLASH_BLOCK_ID, DESIGN_CAPACITY_DATA_FLASH_OFFSET%32, 2, rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  //DebugPrintUint((uint32_t)rxBuf[0]);
  //DebugPrintUint((uint32_t)rxBuf[1]);

  uint8_t checkSum = 0;
  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf[0] = (capacity >> 8) & 0xFF;
  txBuf[1] = capacity & 0xFF;
  checkSum = _update_checksum(checkSum, 2, rxBuf, txBuf);

  //DebugPrintUint((uint32_t)txBuf[0]);
  //DebugPrintUint((uint32_t)txBuf[1]);
  //DebugPrintUint((uint32_t)checkSum);

  ret = _bq27410_datablock_write(CONFIGURATION_SUBCLASS_DATACLASS_ID, DESIGN_CAPACITY_DATA_FLASH_BLOCK_ID, DESIGN_CAPACITY_DATA_FLASH_OFFSET%32, 2, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  return ret;
}

uint8_t BQ27410_Update_TermVoltage(uint16_t voltage)
{
  uint8_t ret = bqProcess_OK;
  uint8_t rxBuf[2] = {0};
  uint8_t txBuf[2] = {0};
  ret = _bq27410_datablock_read(GASGAUGIN_SUBCLASS_ITCFGCLASS_ID, TERM_VOLTAGE_DATA_FLASH_BLOCK_ID, TERM_VOLTAGE_DATA_FLASH_OFFSET%32, 2, rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  uint8_t checkSum = 0;
  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf[0] = (voltage >> 8) & 0xFF;
  txBuf[1] = voltage & 0xFF;
  checkSum = _update_checksum(checkSum, 2, rxBuf, txBuf);

  ret = _bq27410_datablock_write(GASGAUGIN_SUBCLASS_ITCFGCLASS_ID, TERM_VOLTAGE_DATA_FLASH_BLOCK_ID, TERM_VOLTAGE_DATA_FLASH_OFFSET%32, 2, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  return ret;
}

uint8_t BQ27410_Update_OpConfig(uint8_t config)
{
  uint8_t ret = bqProcess_OK;
  uint8_t rxBuf = 0;
  uint8_t txBuf = 0;
  ret = _bq27410_datablock_read(CONFIGURATION_SUBCLASS_REGISTERCLASS_ID, OP_CONFIG_DATA_FLASH_BLOCK_ID, OP_CONFIG_DATA_FLASH_OFFSET%32, 1, &rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  uint8_t checkSum = 0;
  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf = config;
  checkSum = _update_checksum(checkSum, 1, &rxBuf, &txBuf);

  ret = _bq27410_datablock_write(CONFIGURATION_SUBCLASS_REGISTERCLASS_ID, OP_CONFIG_DATA_FLASH_BLOCK_ID, OP_CONFIG_DATA_FLASH_OFFSET%32, 1, &txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  return ret;
}

uint8_t BQ27410_Setup(uint16_t designCapacity, uint16_t designEnergy, uint8_t opConfig, uint16_t termVoltage)
{
  uint8_t ret = bqProcess_OK;
  uint8_t rxBuf[4] = {0};
  uint8_t txBuf[4] = {0};

  //update capacity and energy
  ret = _bq27410_datablock_read(CONFIGURATION_SUBCLASS_DATACLASS_ID, DESIGN_CAPACITY_DATA_FLASH_BLOCK_ID, DESIGN_CAPACITY_DATA_FLASH_OFFSET%32, 4, rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  uint8_t checkSum = 0;
  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf[0] = (designCapacity >> 8) & 0xFF;
  txBuf[1] = designCapacity & 0xFF;
  txBuf[2] = (designEnergy >> 8) & 0xFF;
  txBuf[3] = designEnergy & 0xFF;
  checkSum = _update_checksum(checkSum, 4, rxBuf, txBuf);

  ret = _bq27410_datablock_write(CONFIGURATION_SUBCLASS_DATACLASS_ID, DESIGN_CAPACITY_DATA_FLASH_BLOCK_ID, DESIGN_CAPACITY_DATA_FLASH_OFFSET%32, 4, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  Delay(200);

  //update terminate voltage
  rxBuf[0] = rxBuf[1] = 0;
  ret = _bq27410_datablock_read(GASGAUGIN_SUBCLASS_ITCFGCLASS_ID, TERM_VOLTAGE_DATA_FLASH_BLOCK_ID, TERM_VOLTAGE_DATA_FLASH_OFFSET%32, 2, rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf[0] = (termVoltage >> 8) & 0xFF;
  txBuf[1] = termVoltage & 0xFF;
  checkSum = _update_checksum(checkSum, 2, rxBuf, txBuf);

  ret = _bq27410_datablock_write(GASGAUGIN_SUBCLASS_ITCFGCLASS_ID, TERM_VOLTAGE_DATA_FLASH_BLOCK_ID, TERM_VOLTAGE_DATA_FLASH_OFFSET%32, 2, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  Delay(200);
  //update op config
  rxBuf[0] = txBuf[0] = 0;
  ret = _bq27410_datablock_read(CONFIGURATION_SUBCLASS_REGISTERCLASS_ID, OP_CONFIG_DATA_FLASH_BLOCK_ID, OP_CONFIG_DATA_FLASH_OFFSET%32, 1, rxBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(0, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  txBuf[0] = opConfig;
  checkSum = _update_checksum(checkSum, 1, rxBuf, txBuf);

  ret = _bq27410_datablock_write(CONFIGURATION_SUBCLASS_REGISTERCLASS_ID, OP_CONFIG_DATA_FLASH_BLOCK_ID, OP_CONFIG_DATA_FLASH_OFFSET%32, 1, txBuf);
  if (ret != bqProcess_OK) {
    return ret;
  }

  ret = _bq27410_checksum_rw(1, &checkSum);
  if (ret != bqProcess_OK) {
    return ret;
  }

  Delay(200);
  //bat insert
  txBuf[0] = CNTL_SUB_CMD_BAT_INSERT;
  txBuf[1] = CNTL_SUB_CMD_HIGH;
  ret = _bq27410_write(CMD_CNTL_LSB, 2, txBuf);

  if (ret != bqProcess_OK) {
    return ret;
  }

  Delay(200);
  while (1) {
    txBuf[0] = CNTL_SUB_CMD_CONTROL_STATUS;
    txBuf[1] = CNTL_SUB_CMD_HIGH;
    ret = _bq27410_write(CMD_CNTL_LSB, 2, txBuf);
    if (ret != bqProcess_OK) {
      break;
    }
    Delay(10);
    rxBuf[0] = rxBuf[1] = 0;
    ret = _bq27410_read(CMD_CNTL_LSB, 2, rxBuf);
    if (ret != bqProcess_OK) {
      break;
    }
    if (rxBuf[0] & 0x80) {
      break;
    }
    Delay(50);
  }

  return ret;
}

uint8_t BQ27410_Set_Fullsleep()
{
  uint8_t txBuf[2] = {0};
  uint8_t ret = bqProcess_OK;
  txBuf[0] = CNTL_SUB_CMD_SET_FULLSLEEP;
  txBuf[1] = CNTL_SUB_CMD_HIGH;
  ret = _bq27410_write(CMD_CNTL_LSB, 2, txBuf);
  return ret;
}
