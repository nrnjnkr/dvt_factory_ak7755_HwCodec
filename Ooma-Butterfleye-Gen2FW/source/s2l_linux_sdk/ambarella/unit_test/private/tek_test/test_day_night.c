/*
 * test_day_night.c
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mw_struct.h"
#include "mw_api.h"

void usage(void)
{
    printf("Usage:\n");
    printf("\ttest_day_night on - switch on night mode\n");
    printf("\ttest_day_night off - switch off night mode\n");
}

int get_process_pid(const char * process_name)
{
    FILE *p_fp = NULL; 
    char line_buf[256] = {0};
    char cmd[256] = {0};
    int  pid = -1;

    sprintf(cmd, "pgrep -xl \"%s\"", process_name);
    p_fp = popen(cmd, "r");
    if (p_fp == NULL)
    {   
        return -1;
    }
    else
    {
        while (fgets(line_buf, sizeof(line_buf), p_fp) != NULL)
        {
            if (strstr(line_buf, "pgrep") == NULL)
            {
                sscanf(line_buf, "%d", &pid);
            }
        }
    }

    pclose(p_fp);
    return pid;
}

int main(int argc, char ** argv)
{
	int enable, pid;
	
    if (argc < 2) 
    {
        usage();
		return -1;
	}

    if (strcmp(argv[1], "on") == 0)
    {
        enable = 1;
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        enable = 0;
    }
    else
    {
        usage();
        return -1;
    }

    pid = get_process_pid("test_image");

    if (pid < 0)
    {
        system("test_image -i0 &");
        usleep(500);
    }

    if (mw_enable_day_night_mode(enable) < 0)
    {
        printf("mw_enable_day_night_mode error\n");
    }

	return 0;
}
