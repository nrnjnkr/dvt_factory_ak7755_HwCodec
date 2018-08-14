/*
 * /s2lm_elektra_project/include/am_utility.h
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

#ifndef AM_UTILITY_H_
#define AM_UTILITY_H_

void PowerWifiModule(bool on);
bool UpdateWifiModuleStatus(void);
uint8_t UpdateDCIRQStatus(uint8_t dcStatus);
uint8_t CheckWlanMode(void);
void before_sleep_do(void);
void after_wakeup_do(void);
void mcu_enter_sleep(void);

#endif /* AM_UTILITY_H_ */
