/*
 * /s2lm_elektra_project/include/am_timer.h
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

#ifndef AM_TIMER_H_
#define AM_TIMER_H_

void Timer0Setup(uint32_t preScale, uint32_t num, uint32_t den);
void Timer1Setup(uint32_t preScale, uint32_t num, uint32_t den);
void ResetTimer0();
void ResetTimer1();

#endif /* AM_TIMER_H_ */
