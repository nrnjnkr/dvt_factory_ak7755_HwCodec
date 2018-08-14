/**
 * bld/dsp/s2l/dsp_aaa.c
 *
 * History:
 *    2017/06/05 - [Hao Qian] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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

#include <dsp/fb_aaa_ctrl.h>

#define MAX_SLICE_NUM 3
#define MIN_SLICE_NUM 1

typedef struct aaa_parse_info_s
{
	u8 is_first_cfa : 1;
	u8 index : 7;
	u8 reserved[3];
	u8 *cfa_slice_addr[MAX_SLICE_NUM];
	u32 cfa_data_size;
	u32 cfa_mem_size;
	u32 cfa_base_addr;
	u32 cfa_limited_addr;
} aaa_parse_info_t;

typedef struct aaa_context_s
{
	aaa_output_t aaa_output;
	unsigned int iso_cfg_addr;
	unsigned int cmd_seq_num;
	aaa_config_info_t aaa_cfg_info;
	aaa_parse_info_t aaa_parse_info;
} aaa_context_t;

int clamp(int x, int min, int max)
{
	if (x < min) { return min; }
	if (x > max) { return max; }

	return x;
}

void convert_dot_12_2_hardware_gain_A7(int mul_dot_12,
	int *gain, int *shift)
{
	*shift = 12;
	while ((*gain = mul_dot_12 >> (12 - *shift)) > 4095)  { (*shift)--; }
}

static void fb_aaa_issue_dsp_cmd(aaa_context_t *p_aaa_ctx)
{
	video_hiso_config_update_t vcap_set_video_iso_proc_ctrl_cmd;

	memset(&vcap_set_video_iso_proc_ctrl_cmd, 0,sizeof(video_hiso_config_update_t));
	vcap_set_video_iso_proc_ctrl_cmd.cmd_code = VIDEO_HISO_CONFIG_UPDATE;
	vcap_set_video_iso_proc_ctrl_cmd.hiso_param_daddr = p_aaa_ctx->iso_cfg_addr;
	vcap_set_video_iso_proc_ctrl_cmd.loadcfg_type.flag.hiso_config_color_update = 1;

	clean_d_cache((void *)p_aaa_ctx->iso_cfg_addr, FB_ISO_CFG_DATA_SIZE);
	clean_d_cache((void *)(&vcap_set_video_iso_proc_ctrl_cmd),
		sizeof(vcap_set_video_iso_proc_ctrl_cmd));

	add_dsp_cmd_seq(&vcap_set_video_iso_proc_ctrl_cmd,
		sizeof(video_hiso_config_update_t), p_aaa_ctx->cmd_seq_num);
	clean_cache_dsp_cmd();
}

static void fb_awb_ctrl(aaa_context_t *p_aaa_ctx)
{
	aaa_output_t *p_aaa_output = &p_aaa_ctx->aaa_output;
	u8 *p_lisocfg = (u8 *)p_aaa_ctx->iso_cfg_addr;

	int gain = 0, shift = 0;
	u32 wb_gain_r_offset = 192+40+4;
	u32 wb_gain_ge_offset = 192+40+8;
	u32 wb_gain_go_offset = 192+40+12;
	u32 wb_gain_b_offset = 192+40+16;
	u32 flag_offset = 192+156;
	u32 dgain = p_aaa_output->dgain;
	u32 gain_r = (p_aaa_output->wb_gain.r_gain * dgain) >> (12-2);
	u32 gain_g = (p_aaa_output->wb_gain.g_gain * dgain) >> (12-2);
	u32 gain_b = (p_aaa_output->wb_gain.b_gain * dgain) >> (12-2);

	convert_dot_12_2_hardware_gain_A7((int)gain_r, &gain, &shift);
	gain_r = gain << (16 - shift);
	convert_dot_12_2_hardware_gain_A7((int)gain_g, &gain, &shift);
	gain_g = gain << (16 - shift);
	convert_dot_12_2_hardware_gain_A7((int)gain_b, &gain, &shift);
	gain_b = gain << (16 - shift);

	memcpy((p_lisocfg+wb_gain_r_offset), &gain_r, sizeof(u32));
	memcpy((p_lisocfg+wb_gain_ge_offset), &gain_g, sizeof(u32));
	memcpy((p_lisocfg+wb_gain_go_offset), &gain_g, sizeof(u32));
	memcpy((p_lisocfg+wb_gain_b_offset), &gain_b, sizeof(u32));
	*(p_lisocfg + flag_offset) |= (1<<1); //wb_gain_update =1
}

static void fb_ae_ctrl(aaa_context_t *p_aaa_ctx)
{
	if (p_aaa_ctx->aaa_output.ae_output_update) {
		/* defined in boards/bsp/iav/sensor_xxx.c */
		fb_sensor_ctrl(&p_aaa_ctx->aaa_output);
		p_aaa_ctx->aaa_output.ae_output_update = 0;
	}
}

static void fb_aaa_ctrl(aaa_context_t *p_aaa_ctx)
{
	if (p_aaa_ctx->aaa_output.awb_output_update ||
		p_aaa_ctx->aaa_output.ae_output_update) {
		fb_awb_ctrl(p_aaa_ctx);
		fb_aaa_issue_dsp_cmd(p_aaa_ctx);
	}
	if (p_aaa_ctx->aaa_output.ae_output_update) {
		fb_ae_ctrl(p_aaa_ctx);
	}
	p_aaa_ctx->aaa_output.awb_output_update = 0;
	p_aaa_ctx->aaa_output.ae_output_update = 0;
}

static void fb_aaa_save(aaa_context_t *p_aaa_ctx)
{
	aaa_fb_aeb_t pre_aeb;
	aaa_output_t *p_aaa_output = &p_aaa_ctx->aaa_output;

	pre_aeb.agc = p_aaa_output->agc_index;
	pre_aeb.b_gain = p_aaa_output->wb_gain.b_gain << 2;
	pre_aeb.d_gain = p_aaa_output->dgain;
	pre_aeb.r_gain = p_aaa_output->wb_gain.r_gain << 2;
	pre_aeb.shutter = p_aaa_output->shutter_row;

	clean_d_cache((void *)(&pre_aeb), sizeof(aaa_fb_aeb_t));
	amboot_update_pandora_aaa((void*)&pre_aeb);
}

static void fb_stream_force_idr(aaa_context_t *p_aaa_ctx, u32 pts)
{
	dsp_issue_force_idr_cmd(p_aaa_ctx->cmd_seq_num, pts);
	clean_cache_dsp_cmd();
}

static int dsp_aaa_ctrl(encode_msg_t *encode_msg, aaa_context_t *aaa_ctx)
{
	u8 i = 0;
	u8 slice_num_cur_vcap = 0;
	u32 cur_cfa_addr = 0;
	int is_aaa_stable = 0;
	u32 aaa_stat = encode_msg->aaa_data_fifo_next;
	aaa_parse_info_t *aaa_parse_info = &aaa_ctx->aaa_parse_info;
	u8 slice_num = ((cfa_aaa_stat_t *)aaa_stat)->header_info.total_slices_x;
	u32 cfa_data_size = aaa_parse_info->cfa_data_size;
	u32 cfa_mem_size = aaa_parse_info->cfa_mem_size;
	u32 cfa_base_limit_addr = aaa_parse_info->cfa_limited_addr;

	if ((slice_num < MIN_SLICE_NUM) || (slice_num > MAX_SLICE_NUM)) {
		putstr("The slice num: ");
		putdec(slice_num);
		putstr("is out of range!\r\n");
		return -1;
	}

	if (aaa_parse_info->is_first_cfa) {
		aaa_parse_info->is_first_cfa = 0;
		slice_num_cur_vcap = (slice_num > 1) ? (slice_num - 1) : 1;
	} else {
		slice_num_cur_vcap = slice_num;
	}

	for (i = 0; i < slice_num_cur_vcap; i++) {
		cur_cfa_addr = aaa_stat + i * cfa_data_size;
		if (cur_cfa_addr >= cfa_base_limit_addr) {
			cur_cfa_addr = cur_cfa_addr - cfa_mem_size;
		}
		flush_d_cache((void *)cur_cfa_addr, CFA_AAA_DATA_BLOCK);
		aaa_parse_info->cfa_slice_addr[aaa_parse_info->index] = (u8 *)cur_cfa_addr;
		if (++aaa_parse_info->index >= slice_num) {
			aaa_parse_info->index = 0;
			is_aaa_stable = fb_aaa_algo(aaa_parse_info->cfa_slice_addr[0],
				aaa_parse_info->cfa_slice_addr[1],
				aaa_parse_info->cfa_slice_addr[2], (u8 *)aaa_ctx->iso_cfg_addr,
				&aaa_ctx->aaa_cfg_info, &aaa_ctx->aaa_output);
		}
	}

	return is_aaa_stable;
}

void dsp_pandora_aaa(void)
{
	/* Change it according to your requirment. More count
	 * mean more stable aaa, but the time can not longer wait_dsp_event */
#define MAX_VCAP_COUNT	(30)
#define PIPE_DEPTH_CNT_DOWN	(2 + 3)

	u8 i, j;
	u8 force_idr_cnt = 0;
	u8 force_idr_flag = 0;
	u8 is_first_frame = 1;
	int is_aaa_stable = 0;
	DSP_MSG *msg = NULL;
	DSP_MSG *curr_msg = NULL;
	encode_msg_t *encode_msg = NULL;
	vdsp_info_t *vdsp_info = NULL;
	aaa_context_t aaa_ctx;
	aaa_config_info_t *aaa_cfg_info = &aaa_ctx.aaa_cfg_info;
	aaa_parse_info_t *aaa_parse_info = &aaa_ctx.aaa_parse_info;

	memset(&aaa_ctx, 0, sizeof(aaa_context_t));
	aaa_parse_info->cfa_data_size = CFA_AAA_DATA_BLOCK;
	aaa_parse_info->is_first_cfa = 1;
	aaa_parse_info->cfa_mem_size = CFA_AAA_DATA_BLOCK * MAX_AAA_DATA_NUM;
	aaa_parse_info->cfa_base_addr = DSP_AAA_STATS_START + RGB_AAA_DATA_BLOCK_ARRAY;
	aaa_parse_info->cfa_limited_addr = aaa_parse_info->cfa_base_addr + aaa_parse_info->cfa_mem_size;

	/* Call this after prepare_vcap_setup_cmd() */
	init_iso_cfg_addr(&aaa_ctx.iso_cfg_addr);
	init_sensor_aaa_param(aaa_cfg_info);
	amboot_init_pandora_aaa((void*)&aaa_ctx.aaa_output);
	//fb_aaa_set_log_level(1);

	for (i = 0; i < MAX_VCAP_COUNT; i++) {
		wait_dsp_vcap();
		flush_cache_dsp_msg();

		msg = (DSP_MSG *)(DSP_MSG_BUF_START);

		encode_msg = (encode_msg_t *)msg;
		curr_msg = msg;
		aaa_ctx.cmd_seq_num = encode_msg->prev_cmd_seq + 1;
		vdsp_info = (vdsp_info_t *)(UCODE_CHIP_ID_START + sizeof(u32));

		/* Need clear cmd_num in every vcap interrupt */
		clear_dsp_cmd_num();

		if (i == 0) {
			/* issue aaa statis setup dsp cmd after the first vcap interrupt */
			dsp_issue_liso_cfg_cmd(aaa_ctx.cmd_seq_num);
			clean_cache_dsp_cmd();
		}
		if (force_idr_flag && force_idr_cnt > 0) {
			force_idr_cnt --;
		}

		for (j = 0; j < vdsp_info->num_dsp_msgs; ++j, ++curr_msg) {
			if (curr_msg->msg_code == DSP_STATUS_MSG_ENCODE) {
				encode_msg = (encode_msg_t *)curr_msg;
				//printf("yuv_aaa_data_fifo_next 0x%x\n", encode_msg->yuv_aaa_data_fifo_next);
				//printf("aaa_data_fifo_next 0x%x\n", encode_msg->aaa_data_fifo_next);
				if (encode_msg->aaa_data_fifo_next && (is_aaa_stable <= 0)) {
					if (encode_msg->main_y_addr && is_first_frame) {
						is_first_frame = 0;
						force_idr_flag = 1;
						force_idr_cnt = PIPE_DEPTH_CNT_DOWN;
					}
					is_aaa_stable = dsp_aaa_ctrl(encode_msg, &aaa_ctx);
					if (is_aaa_stable < 0) {
						putstr("dsp aaa algo failed\r\n");
						break;
					} else if (is_aaa_stable == 0) {
						fb_aaa_ctrl(&aaa_ctx);
					} else if (is_aaa_stable > 0) {
						fb_aaa_save(&aaa_ctx);
						printf("dsp aaa algo done at frame [%d]\n", (i+1));
						if (force_idr_flag) {
							break;
						} else {
							return;
						}
					}
					if (force_idr_flag && force_idr_cnt == 0) {
						fb_stream_force_idr(&aaa_ctx, encode_msg->sw_pts);
						force_idr_flag = 0;
						printf("fb force idr at frame [%d]\n", (i+1));
					}
				}
			}
		}

		if (force_idr_flag && force_idr_cnt == 0) {
			fb_stream_force_idr(&aaa_ctx, encode_msg->sw_pts);
			force_idr_flag = 0;
			printf("fb delay force idr at frame [%d]\n", (i+1));
			return;
		}
	}

	/* Save context if 3A algo still cannot get stable after N frames. */
	if (!is_aaa_stable && (i == MAX_VCAP_COUNT)) {
		fb_aaa_save(&aaa_ctx);
	}
}

