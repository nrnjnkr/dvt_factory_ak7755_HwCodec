/*
 * /s2lm_elektra_project/include/am_log.h
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

#ifndef AM_LOG_H_
#define AM_LOG_H_

#include "am_uart.h"

#define AM_DEBUG

#define LOG_ERROR(format, args...) do { \
  tek_print("ERROR %s, %d: "format, __FILE__, __LINE__, ##args); \
  tek_print("\r"); \
} while (0)

#define LOG_PRINT(format, args...) do { \
  tek_print("Info: "format, ##args); \
  tek_print("\r"); \
} while (0)

#ifdef AM_DEBUG
#define LOG_DEBUG(format, args...) do { \
  tek_print("Debug: %s:%d " format "\r\n", __FILE__, __LINE__, ##args); \
} while (0)
#else
#define LOG_DEBUG(format, args...)
#endif

#endif /* AM_LOG_H_ */
