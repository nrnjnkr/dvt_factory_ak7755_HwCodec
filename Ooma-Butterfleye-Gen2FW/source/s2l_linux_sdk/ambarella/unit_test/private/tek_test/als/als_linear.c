#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "als.h"
#include "adc/adc_util.h"

#define MAX_CALIB_FILE_LEN  128
#define MAX_CALIB_ENTRY_NUM 100
#define MIN_LUX_RANGE       30
#define MIN_CALIB_ENTRY_NUM 3

#define CONFIG_ALS_DEFAULT_CALIB_FILE_PATH "/etc/calibration/als_default.ini"

static int linreg(int n, const float x[], const float y[], float* m, float* b);
static int als_read_impl(int *lux, int use_default);

int als_probe(void)
{
    if (access(CONFIG_ALS_DEFAULT_CALIB_FILE_PATH, F_OK) != 0)
    {
        printf("Defaut calib file %s does not exist\n", CONFIG_ALS_DEFAULT_CALIB_FILE_PATH);
        return ALS_RET_FILE_NOT_FOUND;
    }

    return ALS_RET_OK;
}

int als_calib_clear(void)
{
    char cmd[256];
    snprintf(cmd, 256, "rm %s/%s -f", CALIBRATION_DIR, CONFIG_ALS_CALIB_FILE_NAME);
    system(cmd);
    
    return ALS_RET_OK;
}

int als_calib(int lux)
{
    char cmd[256];
    int adc = adc_read(CONFIG_ALS_ADC_CH);
    if (adc < 0)
    {
        printf("ADC value read fail\n");
        return ALS_RET_INVALID_PARAMS;
    }

    snprintf(cmd, 256, "echo \"%d %d\" >> %s/%s", lux, adc, CALIBRATION_DIR, CONFIG_ALS_CALIB_FILE_NAME);
    system(cmd);
    
    return ALS_RET_OK;
}

int als_read(int *lux, int *calibrated)
{
    if (lux == NULL || calibrated == NULL)
    {
        return ALS_RET_INVALID_PARAMS;
    }

    int ret = als_read_impl(lux, 0);

    if (ret == ALS_RET_OK)
    {
        *calibrated = 1;
    }
    else if (ret == ALS_RET_FILE_NOT_FOUND)
    {
        ret = als_read_impl(lux, 1);
        *calibrated = 0;
    }

    return ret;
}

inline static float sqr(float x) {
    return x*x;
}

static int als_read_impl(int *lux, int use_default)
{
    int i;
    float calib_adc[MAX_CALIB_ENTRY_NUM] = {0};
    float calib_lux[MAX_CALIB_ENTRY_NUM] = {0};
    char als_calib_file[MAX_CALIB_FILE_LEN] = {0};
    FILE *p_fp = NULL;
    char line_buf[64] = {0};
    char cmd[256] = {0};
    int adc = 0;

    if ((adc = adc_read(CONFIG_ALS_ADC_CH)) < 0)
    {
        printf("ADC value read fail\n");
        return ALS_RET_INVALID_PARAMS;
    }

    if (use_default)
    {
        sprintf(als_calib_file, "%s", CONFIG_ALS_DEFAULT_CALIB_FILE_PATH);
    }
    else
    {
        sprintf(als_calib_file, "%s/%s", CALIBRATION_DIR, CONFIG_ALS_CALIB_FILE_NAME);
    }

    if (access(als_calib_file, F_OK) != 0)
    {
        printf("Calib file %s does not exist\n", als_calib_file);
        return ALS_RET_FILE_NOT_FOUND;
    }

    //Do calibration
    snprintf(cmd, 256, "cat %s", als_calib_file);
    p_fp = popen(cmd, "r");
    if (p_fp == NULL)
    {
        return ALS_RET_INVALID_PARAMS;
    }
    else
    {
        // Read calib file data to array
        i = 0;
        while (fgets(line_buf, sizeof(line_buf), p_fp) != NULL) {
            sscanf(line_buf, "%f %f", &calib_lux[i], &calib_adc[i]);
            i++;
            if (i >= MAX_CALIB_ENTRY_NUM)
            {
                printf("WARNING: ALS calib entries exceeds maximum (%d). Ignoring remainder.\n", MAX_CALIB_ENTRY_NUM);
                break;
            }
        }

        // Check total calib entry number big enough
        if (i < MIN_CALIB_ENTRY_NUM)
        {
            pclose(p_fp);
            printf("Too few calib entries\n");
            return ALS_RET_SAMPLES_INVALID;
        }

        {
            //Check the lux range in calib file
            float min_lux, max_lux, m, c;
            int j, cur_adc;

            min_lux = max_lux = calib_lux[0];

            for (j = 1; j < i; j++)
            {
                if (calib_lux[j] > max_lux)
                {
                    max_lux = calib_lux[j];
                }
                else if (calib_lux[j] < min_lux)
                {
                    min_lux = calib_lux[j];
                }
            }

            if (max_lux - min_lux < MIN_LUX_RANGE)
            {
                pclose(p_fp);
                printf("Invalid samples\n");
                return ALS_RET_SAMPLES_INVALID;
            }
            
            // get m and c by samples
            linreg(i, calib_adc, calib_lux, &m, &c);
            cur_adc = adc;
            if (cur_adc <= 0)
            {
                pclose(p_fp);
                return ALS_RET_INVALID_PARAMS;
            }

            *lux = (int)(cur_adc*m + c);
        }
    }

    pclose(p_fp);
    return ALS_RET_OK;
}

static int linreg(int n, const float x[], const float y[], float* m, float* b)
{
    float   sumx = 0.0;                        /* sum of x                      */
    float   sumx2 = 0.0;                       /* sum of x**2                   */
    float   sumxy = 0.0;                       /* sum of x * y                  */
    float   sumy = 0.0;                        /* sum of y                      */
    float   sumy2 = 0.0;                       /* sum of y**2                   */

    for (int i=0;i<n;i++)
    {
        sumx  += x[i];
        sumx2 += sqr(x[i]);
        sumxy += x[i] * y[i];
        sumy  += y[i];
        sumy2 += sqr(y[i]);
    }

    float denom = (n * sumx2 - sqr(sumx));
    if (denom == 0) {
        *m = 0;
        *b = 0;
        return 1;
    }

    *m = (n * sumxy  -  sumx * sumy) / denom;
    *b = (sumy * sumx2  -  sumx * sumxy) / denom;

    return 0;
}
