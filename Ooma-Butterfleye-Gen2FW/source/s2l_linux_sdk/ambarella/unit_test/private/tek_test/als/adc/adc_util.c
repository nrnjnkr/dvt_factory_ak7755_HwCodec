// Teknique SDK. COPYRIGHT (C) 2016 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "adc_util.h"

int adc_read_impl(int channel);

int adc_read(int channel)
{
    // double read to get latest value
    adc_read_impl(channel);
    usleep(100000);
    return adc_read_impl(channel);
}

int adc_read_impl(int channel)
{
#ifdef CONFIG_ADC_DEVICE
	FILE* fp;
	char line[80];
	int i;
	int value;

	fp = fopen(CONFIG_ADC_DEVICE, "r");

	if (!fp) {
		return -1;
	}

	while (fgets(line, 80, fp) != NULL)
	{
		if (sscanf(line, "adc%d=%x\n", &i, &value) != 2)
		{
			printf("adc read error\n");
			break;
		}

		if (i == channel)
		{
			fclose(fp);
			return value;
		}
	}

	fclose(fp);
#else
	(void) channel;
#endif
	return -1;
}

