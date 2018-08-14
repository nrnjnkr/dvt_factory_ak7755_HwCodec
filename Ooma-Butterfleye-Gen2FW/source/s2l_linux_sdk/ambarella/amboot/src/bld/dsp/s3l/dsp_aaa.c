/**
 * bld/dsp/s3l/dsp_aaa.c
 *
 * History:
 *    2017/03/27 - [Tao Wu] created file
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

#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#define MAX_SLICE_NUM 3
#define MIN_SLICE_NUM 1

typedef struct aaa_parse_info_s
{
	u8 is_first_cfa : 1;
	u8 index : 7;
	u8 reserved[3];
	u8 *cfa_slice_addr[MAX_SLICE_NUM];
	u32 cfa_data_size;
	u32 cur_entry_addr;
	u32 next_entry_addr;
} aaa_parse_info_t;

typedef struct aaa_context_s
{
	aaa_output_t aaa_output;
	u32 iso_cfg_addr;
	u32 gen_cmd_seq_num;
	u32 vcap_cmd_seq_num;
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
	VCAP_SET_VIDEO_ISO_PROC_CONTROL_CMD vcap_set_video_iso_proc_ctrl_cmd;

	memset(&vcap_set_video_iso_proc_ctrl_cmd, 0,
		sizeof(VCAP_SET_VIDEO_ISO_PROC_CONTROL_CMD));
	vcap_set_video_iso_proc_ctrl_cmd.cmd_code = CMD_VCAP_SET_VIDEO_ISO_PROC_CONTROL;
	vcap_set_video_iso_proc_ctrl_cmd.iso_cfg_daddr = p_aaa_ctx->iso_cfg_addr;
	vcap_set_video_iso_proc_ctrl_cmd.flag.iso_config_color_update = 1;

	clean_d_cache((void *)p_aaa_ctx->iso_cfg_addr, FB_ISO_CFG_DATA_SIZE);
	clean_d_cache((void *)(&vcap_set_video_iso_proc_ctrl_cmd),
		sizeof(vcap_set_video_iso_proc_ctrl_cmd));

	/* Need clear cmd_num in every vcap interrupt */
	clear_dsp_cmd_port_num(CMD_PORT_VCAP);
	add_dsp_cmd_port(&vcap_set_video_iso_proc_ctrl_cmd,
		sizeof(VCAP_SET_VIDEO_ISO_PROC_CONTROL_CMD), CMD_PORT_VCAP,
		p_aaa_ctx->vcap_cmd_seq_num);
	clean_cache_dsp_cmd_port(CMD_PORT_VCAP);
}

static void fb_awb_ctrl(aaa_context_t *p_aaa_ctx)
{
	aaa_output_t *p_aaa_output = &p_aaa_ctx->aaa_output;
	u8 *p_lisocfg = (u8 *)p_aaa_ctx->iso_cfg_addr;

	int gain = 0, shift = 0;
	u32 wb_gain_r_offset = 184+40+4;
	u32 wb_gain_ge_offset = 184+40+8;
	u32 wb_gain_go_offset = 184+40+12;
	u32 wb_gain_b_offset = 184+40+16;
	u32 flag_offset = 184+164;
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

	amboot_update_pandora_aaa((void*)&pre_aeb);
}

static void fb_stream_force_idr(aaa_context_t *p_aaa_ctx, u32 pts)
{
	dsp_issue_force_idr_cmd(p_aaa_ctx->vcap_cmd_seq_num, pts);
	clean_cache_dsp_cmd_port(CMD_PORT_VCAP);
}


static int pandora_aaa_ctrl( VCAP_STRM_REPORT *stm_rpt, aaa_context_t *aaa_ctx)
{
	u8 i = 0;
	u8 slice_num_cur_vcap = 0;
	u32 cur_cfa_addr = 0;
	int is_aaa_stable = 0;
	aaa_parse_info_t *aaa_parse_info = &aaa_ctx->aaa_parse_info;
	u32 cfa_base_addr = stm_rpt->cfa_3a_stat_dram_base_addr;
	u32 cfa_base_limit_addr = stm_rpt->cfa_3a_stat_dram_limit_addr;
	u8 slice_num = ((aaa_data_header_info_t *)cfa_base_addr)->total_slices_x;
	u32 cfa_data_size = aaa_parse_info->cfa_data_size;

	if ((slice_num < MIN_SLICE_NUM) || (slice_num > MAX_SLICE_NUM)) {
		putstr("The slice num: ");
		putdec(slice_num);
		putstr("is out of range!\r\n");
		return -1;
	}

	aaa_parse_info->next_entry_addr = (stm_rpt->cfa_3a_stat_dram_addr + cfa_data_size);
	if (aaa_parse_info->next_entry_addr >= cfa_base_limit_addr) {
		aaa_parse_info->next_entry_addr = cfa_base_addr;
	}

	if (aaa_parse_info->is_first_cfa) {
		aaa_parse_info->is_first_cfa = 0;
		aaa_parse_info->cur_entry_addr = cfa_base_addr;
		slice_num_cur_vcap = (aaa_parse_info->next_entry_addr -
			aaa_parse_info->cur_entry_addr) / cfa_data_size;
	} else {
		slice_num_cur_vcap = slice_num;
	}

	for (i = 0; i < slice_num_cur_vcap; i++) {
		cur_cfa_addr = aaa_parse_info->cur_entry_addr + i * cfa_data_size;
		if (cur_cfa_addr >= cfa_base_limit_addr) {
			cur_cfa_addr = cur_cfa_addr - (cfa_base_limit_addr - cfa_base_addr);
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

	aaa_parse_info->cur_entry_addr = aaa_parse_info->next_entry_addr;

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
	DSP_STATUS_MSG * gen_msg_hdr = NULL;
	DSP_STATUS_MSG * vcap_msg_hdr = NULL;
	DSP_MSG *vcap_curr_msg = NULL;
	VCAP_STATUS_MSG * vcap_msg = NULL;
	VCAP_STRM_REPORT * stm_rpt =  NULL;
	aaa_context_t aaa_ctx;
	aaa_config_info_t *aaa_cfg_info = &aaa_ctx.aaa_cfg_info;
	aaa_parse_info_t *aaa_parse_info = &aaa_ctx.aaa_parse_info;

	memset(&aaa_ctx, 0, sizeof(aaa_context_t));
	aaa_parse_info->cfa_data_size = ROUND_UP(CFA_AAA_DATA_BLOCK, 256);
	aaa_parse_info->is_first_cfa = 1;

	/* Call this after prepare_vcap_setup_cmd() */
	init_iso_cfg_addr(&aaa_ctx.iso_cfg_addr);
	init_sensor_aaa_param(aaa_cfg_info);
	amboot_init_pandora_aaa((void*)&aaa_ctx.aaa_output);

	for (i = 0; i < MAX_VCAP_COUNT; ++i) {
		wait_dsp_vcap();
		flush_cache_dsp_msg_port(CMD_PORT_VCAP);

		gen_msg_hdr = (DSP_STATUS_MSG *)(DSP_MSG_BUF_START);
		vcap_msg_hdr = (DSP_STATUS_MSG *)(DSP_MSG_BUF_START + DSP_PORT_MSG_SIZE);
		vcap_curr_msg = (DSP_MSG *)vcap_msg_hdr + 1;

		aaa_ctx.vcap_cmd_seq_num = vcap_msg_hdr->prev_cmd_seq + 1;
		aaa_ctx.gen_cmd_seq_num = gen_msg_hdr->prev_cmd_seq + 1;
		if (i == 0) {
			/* issue aaa statis setup dsp cmd after the first vcap interrupt */
			dsp_issue_liso_cfg_cmd(aaa_ctx.vcap_cmd_seq_num);
			clean_cache_dsp_cmd_port(CMD_PORT_VCAP);
		}
		if (force_idr_flag && force_idr_cnt > 0) {
			force_idr_cnt --;
		}
		for (j = 0; j < vcap_msg_hdr->num_msgs; ++j, ++vcap_curr_msg) {
			if (vcap_curr_msg->msg_code == MSG_VCAP_STATUS) {
				vcap_msg = (VCAP_STATUS_MSG *) vcap_curr_msg;
				stm_rpt = &vcap_msg->strm_reports;
				if (stm_rpt->cfa_3a_stat_dram_addr && (is_aaa_stable <= 0)) {
					if (stm_rpt->main_pic_luma_addr && is_first_frame) {
						is_first_frame = 0;
						force_idr_flag = 1;
						force_idr_cnt = PIPE_DEPTH_CNT_DOWN;
					}

					is_aaa_stable = pandora_aaa_ctrl(stm_rpt, &aaa_ctx);

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
						fb_stream_force_idr(&aaa_ctx, stm_rpt->main_pic_sw_pts);
						force_idr_flag = 0;
						printf("fb force idr at frame [%d]\n", (i+1));
					}
				}
			}
		}

		if (force_idr_flag && force_idr_cnt == 0) {
			fb_stream_force_idr(&aaa_ctx, stm_rpt->main_pic_sw_pts);
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

