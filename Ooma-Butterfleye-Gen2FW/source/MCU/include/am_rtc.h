/*
 * /s2lm_elektra_project/include/am_rtc.h
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

#ifndef AM_RTC_H_
#define AM_RTC_H_

void RTCSetup(CMU_Select_TypeDef clockSource, uint32_t clkDiv, uint32_t timer);

#endif /* AM_RTC_H_ */
