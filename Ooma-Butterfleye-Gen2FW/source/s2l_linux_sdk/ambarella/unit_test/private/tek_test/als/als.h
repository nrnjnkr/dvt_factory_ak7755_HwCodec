// Teknique SDK. COPYRIGHT (C) 2016 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

#ifndef __ALS_H__
#define __ALS_H__

// INCLUDES //////////////////////////////////////////////////////////////////

#include <stdint.h>

#define ALS_RET_OK 0
#define ALS_RET_NOT_SUPPORTED -1
#define ALS_RET_FILE_NOT_FOUND -2
#define ALS_RET_INVALID_PARAMS -3
#define ALS_RET_SAMPLES_INVALID -4

#define CALIBRATION_DIR "/mnt/provision"
#define CONFIG_ALS_CALIB_FILE_NAME "als.ini"
#define CONFIG_ALS_ADC_CH 2

// probe als, returns < 0 on error

int als_probe(void);

/// clear any als calibration, returns < 0 on error

int als_calib_clear(void);

/// calibrate the als with the given lux, return < 0 on error

int als_calib(int lux);

/// show the current als value, returns < 0 on error

int als_read(int *lux, int *calibrated);

#endif // __ALS_H__
