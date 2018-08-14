/*
 * /s2lm_elektra_project/src/am_battery.c
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
//#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_adc.h"
#include "em_i2c.h"

#include "am_base.h"
#include "am_log.h"
#include "am_common.h"
#include "am_i2c.h"
#include "am_adc.h"
#include "am_battery.h"
#include "bq27410_driver.h"

static uint8_t powerManagerType = powerManager_I2C;

int SetupPowerManager(uint8_t pm_type)
{
  int ret = 0;
  powerManagerType = pm_type;
  if (powerManagerType == powerManager_I2C) {
    SetupI2C(I2C_SLAVE_ADDRESS);
    Delay(500);
    uint8_t opConfig = OPCFG_BIT_SLEEP | OPCFG_BIT_RMFCC;
    if (BQ27410_Setup(Battery_Design_Capacity, Battery_Design_Energy, opConfig, Battery_Terminate_Voltage) != bqProcess_OK) {
      LOG_ERROR("BQ27410_Setup fail\n");
      ret = -1;
    }else{
    	LOG_PRINT("powerManager_I2C --- BQ27410_Setup success\n");
    }
  } else if (powerManagerType == powerManager_ADC) {
    ADCSetup(AMBA_ADC_RESOLUTION);
    ADC_Start(ADC0, adcStartSingle);
  }

  return ret;
}

int GetBatVoltage(void)
{
  int voltage = 0;
  if (powerManagerType == powerManager_I2C) {
    uint8_t ret = BQ27410_Vol_Read((uint16_t*)&voltage);
    if (ret != bqProcess_OK) {
      LOG_ERROR("BQ27410_Vol_Read fail");
      return -1;
    }
  } else if (powerManagerType == powerManager_ADC) {
    voltage = ADCReadSingleData() * 5000 / (1 << AMBA_ADC_RESOLUTION);
  }

  LOG_DEBUG("GetBatVoltage, voltage %d\n", voltage);

  return voltage;
}

int GetBatRmCapacity(void)
{
  if (powerManagerType == powerManager_I2C) {
	  int rm_capacity = 0;
	  uint8_t ret = BQ27410_RM_Read((uint16_t*)&rm_capacity);
	  if (ret != bqProcess_OK) {
		LOG_ERROR("BQ27410_RM_Read fail\n");
		return -1;
	  }
	  return rm_capacity;

  }else if (powerManagerType == powerManager_ADC) {
	  //add here
	  LOG_ERROR("ADC not support remain capacity\n");
	  return -1;

  }

  return -1;
}

int GetBatSoC(void)
{
	if (powerManagerType == powerManager_I2C) {
		int soc = 0;
		uint8_t ret = BQ27410_SOC_Read((uint16_t*)&soc);
		if (ret != bqProcess_OK) {
			// LOG_ERROR("BQ27410_SOC_Read fail\n");
			return -1;
		}
		LOG_DEBUG("GetBatSoC, soc %d\n", soc);
		return soc;
	}else if (powerManagerType == powerManager_ADC) {
		//add here
		LOG_DEBUG("ADC not support SOC\n");
		return -1;
	}

	return -1;
}

uint8_t UpdateBatStatus(uint8_t status)
{
	if (powerManagerType == powerManager_I2C) {
		int soc = 0;
		int rm_capacity = 0;
		uint8_t ret = BQ27410_RM_Read((uint16_t*)&rm_capacity);
		if (ret != bqProcess_OK) {
			// LOG_ERROR("BQ27410_RM_Read fail\n");
			return status;
		}
		ret = BQ27410_SOC_Read((uint16_t*)&soc);//read the rate
		if (ret != bqProcess_OK) {
			LOG_ERROR("BQ27410_SOC_Read fail\n");
			return status;
		}
		LOG_DEBUG("remain capacity %d\n", rm_capacity);
		if (soc <= Capacity_Threshold_Min_Rate) {
			status |= batteryStatus_Low;
		} else if (soc <= Capacity_Threshold_Mid_Rate) {
			status |= batteryStatus_NeedChrg;
		} else if ((status & batteryStatus_Low) && (soc >= (Capacity_Threshold_Min_Rate + Capacity_Threshold_Sliding_Rate))) {
			status &= 0xFF & (~batteryStatus_Low);
		} else if ((status & batteryStatus_NeedChrg) && (soc >= (Capacity_Threshold_Mid_Rate + Capacity_Threshold_Sliding_Rate))) {
			status &= 0xFF & (~batteryStatus_NeedChrg);
		}

		return status;
	 }
	 else if (powerManagerType == powerManager_ADC) {
		//add here
		LOG_DEBUG("ADC not support UpdateBatStatus\n");
		return -1;
	 }

	return -1;
}

