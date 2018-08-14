/*
 * /s2lm_elektra_project/include/am_battery.h
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

#ifndef AM_BATTERY_H_
#define AM_BATTERY_H_

#define Voltage_Max                     (4200) //mV
#define Voltage_Threshold_Min           (Voltage_Max/10)
#define Voltage_Threshold_Mid           (Voltage_Threshold_Min*2)
#define Voltage_Threshold_Sliding       (10)

#define Battery_Design_Capacity         (2600) //mAh
#define Capacity_Threshold_Min          (Battery_Design_Capacity/20)
#define Capacity_Threshold_Mid          (Capacity_Threshold_Min*2)
#define Capacity_Threshold_Sliding      (50) //mAh

#define Capacity_Threshold_Min_Rate			 5 //5% is the min rate
#define Capacity_Threshold_Mid_Rate          (Capacity_Threshold_Min_Rate*2)
#define Capacity_Threshold_Sliding_Rate      3 //5%

#define Battery_Design_Energy           (9620) //2600*3.7 mW
#define Battery_Terminate_Voltage       (3300)

int SetupPowerManager(uint8_t pm_type);
int GetBatVoltage(void);
int GetBatRmCapacity(void);
int GetBatSoC(void);
uint8_t UpdateBatStatus(uint8_t status);

#endif /* AM_BATTERY_H_ */
