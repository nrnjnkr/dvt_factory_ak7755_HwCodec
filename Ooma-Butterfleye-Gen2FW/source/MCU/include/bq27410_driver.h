/*
 * /s2lm_elektra_project/include/bq27410_driver.h
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

#ifndef BQ27410_DRIVER_H_
#define BQ27410_DRIVER_H_

#define Taper_Current                   (100)//mA
#define Taper_Voltage                   (100)//mV

#define SOCI_Delta                      1
#define Sleep_Current                   10
#define Hibernate_I                     8
#define Hibernate_V                     2550

//I2C slave addr
#define I2C_SLAVE_ADDRESS (0xAA) //0x1010101x

//Bits
#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)
#define BIT4 (1 << 4)
#define BIT5 (1 << 5)
#define BIT6 (1 << 6)
#define BIT7 (1 << 7)
#define BIT8 (1 << 8)
#define BIT9 (1 << 9)
#define BITA (1 << 10)
#define BITB (1 << 11)
#define BITC (1 << 12)
#define BITD (1 << 13)
#define BITE (1 << 14)
#define BITF (1 << 15)

//Standard Data Commands
#define CMD_CNTL_LSB                    0x00
#define CMD_CNTL_MSB                    0x01
#define CMD_TEMP_LSB                    0x02
#define CMD_TEMP_MSB                    0x03
#define CMD_VOLT_LSB                    0x04
#define CMD_VOLT_MSB                    0x05
#define CMD_FLAGS_LSB                   0x06
#define CMD_FLAGS_MSB                   0x07
#define CMD_NAC_LSB                     0x08
#define CMD_NAC_MSB                     0x09
#define CMD_FAC_LSB                     0x0A
#define CMD_FAC_MSB                     0x0B
#define CMD_RM_LSB                      0x0C
#define CMD_RM_MSB                      0x0D
#define CMD_FCC_LSB                     0x0E
#define CMD_FCC_MSB                     0x0F
#define CMD_AI_LSB                      0x10
#define CMD_AI_MSB                      0x11
#define CMD_SI_LSB                      0x12
#define CMD_SI_MSB                      0x13
#define CMD_MLI_LSB                     0x14
#define CMD_MLI_MSB                     0x15
#define CMD_AE_LSB                      0x16
#define CMD_AE_MSB                      0x17
#define CMD_AP_LSB                      0x18
#define CMD_AP_MSB                      0x19
#define CMD_SOC_LSB                     0x1C
#define CMD_SOC_MSB                     0x1D
#define CMD_ITEMP_LSB                   0x1E
#define CMD_ITEMP_MSB                   0x1F
#define CMD_SCH_LSB                     0x20
#define CMD_SCH_MSB                     0x21

//Extended Commands
#define CMD_OPCFG_LSB                   0x3A
#define CMD_OPCFG_MSB                   0x3B
#define CMD_DCAP_LSB                    0x3C
#define CMD_DCAP_MSB                    0x3D
#define CMD_DFCLS                       0x3E
#define CMD_DFBLK                       0x3F
#define CMD_DFD_LSB                     0x40
#define CMD_DFD_MSB                     0x5F
#define CMD_DFDCKS                      0x60
#define CMD_DFDCNTL                     0x61
#define CMD_DNAMELEN                    0x62
#define CMD_DNAME_LSB                   0x63
#define CMD_DNAME_MSB                   0x69
#define CMD_RSVD_LSB                    0x6A
#define CMD_RSVD_MSB                    0x7F

//Control Sub-commands
#define CNTL_SUB_CMD_HIGH               0x00
#define CNTL_SUB_CMD_CONTROL_STATUS     0x00
#define CNTL_SUB_CMD_DEVICE_TYPE        0x01
#define CNTL_SUB_CMD_FW_VERSION         0x02
#define CNTL_SUB_CMD_HW_VERSION         0x03
#define CNTL_SUB_CMD_PREV_MACWRITE      0x07
#define CNTL_SUB_CMD_BAT_INSERT         0x0C
#define CNTL_SUB_CMD_BAT_REMOVE         0x0D
#define CNTL_SUB_CMD_SET_FULLSLEEP      0x10
#define CNTL_SUB_CMD_SET_HIBERNATE      0x11
#define CNTL_SUB_CMD_CLEAR_HIBERNATE    0x12
#define CNTL_SUB_CMD_FACTORY_RESTORE    0x15
#define CNTL_SUB_CMD_SEALED             0x20
#define CNTL_SUB_CMD_RESET              0x41

//cmd cntl Sub-cmd control bits map
#define CNTL_CONTROL_STATUS_BIT_HIBE         BITF
#define CNTL_CONTROL_STATUS_BIT_FAS          BITE
#define CNTL_CONTROL_STATUS_BIT_SS           BITD
#define CNTL_CONTROL_STATUS_BIT_CCA          BITC
#define CNTL_CONTROL_STATUS_BIT_QMAXU        BIT9
#define CNTL_CONTROL_STATUS_BIT_RESU         BIT8
#define CNTL_CONTROL_STATUS_BIT_INITCOMP     BIT7
#define CNTL_CONTROL_STATUS_BIT_HIBERNATE    BIT6
#define CNTL_CONTROL_STATUS_BIT_FULLSLEEP    BIT5
#define CNTL_CONTROL_STATUS_BIT_SLEEP        BIT4
#define CNTL_CONTROL_STATUS_BIT_RUP_DIS      BIT2
#define CNTL_CONTROL_STATUS_BIT_VOK          BIT1

//cmd flags bits map
#define FLAGS_BIT_OTC                        BITF
#define FLAGS_BIT_OTD                        BITE
#define FLAGS_BIT_CHG_INH                    BITB
#define FLAGS_BIT_FC                         BIT9
#define FLAGS_BIT_CHG                        BIT8
#define FLAGS_BIT_OCVTAKEN                   BIT7
#define FLAGS_BIT_BAT_DET                    BIT3
#define FLAGS_BIT_SOC1                       BIT2
#define FLAGS_BIT_SOCF                       BIT1
#define FLAGS_BIT_DSG                        BIT0

//external cmd opcfg bits map
#define OPCFG_BIT_RESCAP                     BIT7
#define OPCFG_BIT_BATLOWEN                   BIT5
#define OPCFG_BIT_SLEEP                      BIT4
#define OPCFG_BIT_RMFCC                      BIT3
#define OPCFG_BIT_BIE                        BIT2
#define OPCFG_BIT_GPIO_POL                   BIT1
#define OPCFG_BIT_WRTEMP                     BIT0

//data falsh class id
#define CONFIGURATION_SUBCLASS_DATACLASS_ID  48
#define DESIGN_CAPACITY_DATA_FLASH_OFFSET    19
#define DESIGN_CAPACITY_DATA_FLASH_BLOCK_ID  (DESIGN_CAPACITY_DATA_FLASH_OFFSET/32)

#define GASGAUGIN_SUBCLASS_ITCFGCLASS_ID     80
#define TERM_VOLTAGE_DATA_FLASH_OFFSET       45
#define TERM_VOLTAGE_DATA_FLASH_BLOCK_ID     (TERM_VOLTAGE_DATA_FLASH_OFFSET/32)

#define CONFIGURATION_SUBCLASS_REGISTERCLASS_ID 64
#define OP_CONFIG_DATA_FLASH_OFFSET          0
#define OP_CONFIG_DATA_FLASH_BLOCK_ID        (OP_CONFIG_DATA_FLASH_OFFSET/32)

typedef enum {
  bqProcess_OK = 0,
  bqProcess_Error = 1,
  bqProcess_Result_Error = 2,
} Bq_Process_Error_Code_Typedef;

uint8_t BQ27410_Read(uint8_t cmd, uint8_t bytes, uint8_t* rxData);
uint8_t BQ27410_Write(uint8_t cmd, uint8_t bytes, uint8_t* txData);

uint8_t BQ27410_ReadV2(uint8_t txBytes, uint8_t* txData, uint8_t rxBytes, uint8_t* rxData);

uint8_t BQ27410_CNTL_SubCmd_Read(uint8_t subCmd, uint8_t bytes, uint16_t* rxData);
uint8_t BQ27410_CNTL_SubCmd_Write(uint8_t subCmd, uint8_t bytes, uint16_t* txData);

uint8_t BQ27410_Temp_Read(uint16_t* temp);
uint8_t BQ27410_Vol_Read(uint16_t* voltage);
uint8_t BQ27410_FCC_Read(uint16_t* fcc);
uint8_t BQ27410_FAC_Read(uint16_t* fac);
uint8_t BQ27410_RM_Read(uint16_t* rm);
uint8_t BQ27410_SOC_Read(uint16_t* soc);
uint8_t BQ27410_AI_Read(int16_t* ai);
uint8_t BQ27410_SI_Read(int16_t* si);
uint8_t BQ27410_Flags_Read(uint16_t* flags);

uint8_t BQ27410_DataBlock_Read(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* rxData, uint8_t* checkSum);
uint8_t BQ27410_DataBlock_Write(uint8_t classId, uint8_t blockId, uint8_t offset, uint8_t bytes, uint8_t* txData, uint8_t checkSum);
uint8_t Update_Checksum(uint8_t oldSum, uint8_t bytes, uint8_t* oldData, uint8_t* newData);

uint8_t BQ27410_Update_DesignCapacity(uint16_t capacity);
uint8_t BQ27410_Update_TermVoltage(uint16_t voltage);
uint8_t BQ27410_Update_OpConfig(uint8_t config);

uint8_t BQ27410_Setup(uint16_t designCapacity, uint16_t designEnergy, uint8_t opConfig, uint16_t termVoltage);
uint8_t BQ27410_Set_Fullsleep();

#endif /* BQ27410_DRIVER_H_ */
