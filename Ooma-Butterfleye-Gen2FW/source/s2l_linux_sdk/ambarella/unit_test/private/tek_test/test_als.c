#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "als/als.h"

#define ALS_CALIB_MIN 0        //Dummy
#define ALS_CALIB_MAX 0xFFFF   //Dummy

void test_als_calib_init(void)
{
    char cmd[256];
    snprintf(cmd, 256, "mount -o remount,rw %s", CALIBRATION_DIR);
    system(cmd);
}

int test_als_calib_clear(void)
{
    test_als_calib_init();
    return als_calib_clear();
}

int test_als_calib(int lux)
{
    test_als_calib_init();
    return als_calib(lux);
}

int test_als_show(int *lux, int *calibrated)
{
    return als_read(lux, calibrated);
}

void usage(void)
{
    printf("Usage:\n");
    printf("\ttest_als probe\n");
    printf("\ttest_als calib <calib>\n");
    printf("\ttest_als calib reset\n");
    printf("\ttest_als show\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return -1;
    }
    else
    {
        if (argc == 2 && strcmp(argv[1], "probe") == 0)
        {
            printf("%s\n", ALS_RET_OK == als_probe() ? "Success" : "Failed");
        }
        else if (argc == 3 && strcmp(argv[1], "calib") == 0)
        {
            if (strcmp(argv[2], "reset") == 0)
            {
                printf("%s\n", ALS_RET_OK == test_als_calib_clear() ? "Success" : "Failed");
            }
            else
            {
                char *endptr;
                long int value;
            
                value = strtol(argv[2], &endptr, 10);
            
                if (argv[2] == endptr)
                {
                    printf("Invalid param value %s\n", argv[2]);
                    return -1;
                }
                else if (value < (long int)ALS_CALIB_MIN || value > (long int)ALS_CALIB_MAX)
                {
                    printf("Invalid parameter range\n");
                    return -1;
                }
                else
                {
                    printf("%s\n", ALS_RET_OK == test_als_calib((int) value) ? "Success" : "Failed");
                }
            }
        }
        else if (argc == 2 && strcmp(argv[1], "show") == 0)
        {
            int value = -1;
            int calibrated = 0;

            printf("%s\n", ALS_RET_OK == test_als_show(&value, &calibrated) ? "Success" : "Failed");
            printf("\tcalibrated : %d\n", calibrated);
            printf("\tlux : %d\n", value);
        }
        else
        {
            usage();
            return -1;
        }
    }
    
    return 0;   
}
