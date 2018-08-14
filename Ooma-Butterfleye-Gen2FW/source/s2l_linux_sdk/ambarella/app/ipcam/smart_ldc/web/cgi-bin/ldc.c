 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <iav_ioctl.h>

#include <basetypes.h>
#include <iav_ioctl.h>

#include "ClearSilver.h"

#define BUFFER_LEN (256)
#define MAX_FOV (200)
char cmd[BUFFER_LEN];
char tmp[BUFFER_LEN];
static int fd_iav = -1;

int main(int argc,char **agrs,char **env)
{
	char *strength = NULL;
	char *offset_x = NULL;
	char *offset_y = NULL;
	char *zoom_num = NULL;
	char *zoom_denum = NULL;
	char *mode = NULL;
	char *pano_h_fov = NULL;
	CGI *cgi = NULL;
	HDF *hdf = NULL;
	int angel = 1;
	struct vindev_video_info video_info;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info);

	hdf_init(&hdf);
	cgi_init(&cgi, hdf);
	memset(cmd, 0, BUFFER_LEN);

	strength = hdf_get_value(cgi->hdf,"Query.STRENGTH","0");
	offset_x = hdf_get_value(cgi->hdf,"Query.X","0");
	offset_y = hdf_get_value(cgi->hdf,"Query.Y","0");
	zoom_num = hdf_get_value(cgi->hdf,"Query.zoom_num","0");
	zoom_denum = hdf_get_value(cgi->hdf,"Query.zoom_denum","0");
	mode = hdf_get_value(cgi->hdf,"Query.mode","1");
	pano_h_fov = hdf_get_value(cgi->hdf,"Query.pano_h_fov","1");

	angel = (MAX_FOV - 1) * atof(strength) / 20.0 + 1;
	snprintf(cmd, BUFFER_LEN, "/usr/local/bin/test_ldc -F %d -R %d -m %s -h %s -v -C %sx%s -z %s/%s -f /tmp/ldc/ldc >> /tmp/ldc/ldc_config &",\
		angel, video_info.info.width / 2 , mode, pano_h_fov, offset_x, offset_y, zoom_num, zoom_denum );
	snprintf(tmp, BUFFER_LEN, "echo -e \"*****command*****\n/usr/local/bin/test_ldc -F %d -R %d -m %s -h %s -v -C %sx%s -z %s/%s -f /tmp/ldc/ldc\n*****************\n\" > /tmp/ldc/ldc_config", \
		angel, video_info.info.width / 2 , mode, pano_h_fov, offset_x, offset_y, zoom_num, zoom_denum );
	printf("%s",cmd);
	system("/bin/mkdir -p /tmp/ldc");
	system(tmp);
	system(cmd);
	return 0;

}
