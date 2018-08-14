/*
 * /s2lm_elektra_project/include/am_common.h
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

#ifndef AM_COMMON_H_
#define AM_COMMON_H_

void SetupSysTick(void);
void Delay(uint32_t dlyTicks);
bool DelayCheckGpio(uint8_t gpio, uint32_t dlyTicks, uint8_t status);
uint8_t DelayCheckPwrMode(uint8_t mcuMode);
bool DelayCheckGpioPIR(uint32_t dlyTicks);
uint32_t GetCurTicks();

#endif /* AM_COMMON_H_ */
