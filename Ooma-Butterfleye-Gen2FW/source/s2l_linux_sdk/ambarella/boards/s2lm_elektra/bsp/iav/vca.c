/**
 * boards/s2lm_elektra/bsp/iav/vca.c
 *
 * History:
 *    2016/03/31 - [Chu Chen] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include <bldfunc.h>
#include <ambhw/cache.h>
#include <dsp/dsp.h>

#define VCA_MODE_DISABLE            (0)
#define VCA_MODE_IN_AMBOOT      (1)
#define VCA_MODE_IN_LINUX          (2)

typedef struct {
	int   left;
	int   top;
	int   right;
	int   bottom;
} vca_object_region_t;

#if 0
static void shut_down(void)
{
	gpio_config_sw_out(112);
	gpio_set(112);
	while (1) {
		//
		rct_timer_dly_ms(500);//wait 500ms
	}
}

 static int get_yuv_data(unsigned int *y_addr, unsigned int *uv_addr, unsigned int *y_pitch)
{
	DSP_MSG			*msg = (DSP_MSG *)DSP_MSG_BUF_START;
	vdsp_info_t		*info = (vdsp_info_t *)(UCODE_CHIP_ID_START + sizeof(u32));
	encode_msg_t		*e_msg = NULL;
	encode_msg_t		esg;
	int i = 0;
	while (!e_msg) {
		_clean_flush_all_cache();

		for (i = 0; i < info->num_dsp_msgs; i++) {
			if (msg[i].msg_code == DSP_STATUS_MSG_ENCODE) {
				e_msg = (encode_msg_t *)&msg[i];

				if (e_msg->encode_operation_mode == VIDEO_MODE &&
					e_msg->main_y_addr != 0x0) {
					esg = *e_msg;
					break;
				} else {
					e_msg = NULL;
				}
			}
		}
	}
	if (!e_msg) {
		return -1;
	}

	*y_addr = esg.preview_C_y_addr;
	*uv_addr = esg.preview_C_uv_addr;
	*y_pitch = (unsigned int)esg.preview_C_y_pitch;
	return 0;
}
#endif