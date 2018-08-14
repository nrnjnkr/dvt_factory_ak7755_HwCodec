// Teknique SDK. COPYRIGHT (C) 2016 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

#ifndef __ADC_UTIL_H__
#define __ADC_UTIL_H__

#define CONFIG_ADC_DEVICE "/sys/devices/e8000000.apb/e801d000.adc/adcsys"

#ifdef __cplusplus
extern "C" {
#endif

int adc_read(int channel);

#ifdef __cplusplus
};
#endif

#endif // __ADC_UTIL_H__
