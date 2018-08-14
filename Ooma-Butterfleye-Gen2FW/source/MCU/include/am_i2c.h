/*
 * /s2lm_elektra_project/include/am_i2c.h
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

#ifndef AM_I2C_H_
#define AM_I2C_H_

void SetupI2C(uint8_t slaveAddr);
//I2C_TransferReturn_TypeDef I2C_SendData(I2C_TransferSeq_TypeDef* i2cTransferPtr);
//I2C_TransferReturn_TypeDef I2C_GetData(I2C_TransferSeq_TypeDef* i2cTransferPtr);
I2C_TransferReturn_TypeDef PerformI2CTransfer(I2C_TransferSeq_TypeDef* i2cTransferPtr);

int i2c_write_byte(uint8_t slave_addr, uint8_t addr, uint8_t value);
int i2c_read_byte(uint8_t slave_addr, uint8_t addr, uint8_t* p_value);


// Scan and show which address is active.
// Return value: 0 for success
void scan_i2c_bus(void);

#endif /* AM_I2C_H_ */
