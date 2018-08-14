/*******************************************************************************
 * sensor_imx322_aaa.c
 *
 * History:
 *   2017/05/23 - [Tao Wu] created file
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
 ******************************************************************************/

#include "dsp/fb_aaa_ctrl.h"
#include "dsp/fb_aaa_imx322_param.c"

static ipcam_real_time_encode_param_setup_t stream_rt_update = {
	.cmd_code = IPCAM_REAL_TIME_ENCODE_PARAM_SETUP,
	.enable_flags = 0x00002005, /* Force IDR, QP, Bitrate */
	.cbr_modify = 4000000,
	.custom_encoder_frame_rate = 0,
	.frame_rate_division_factor = 0,
	.qp_min_on_I = 1,
	.qp_max_on_I = 35,
	.qp_min_on_P = 1,
	.qp_max_on_P = 51,
	.qp_min_on_B = 1,
	.qp_max_on_B = 51,
	.aqp = 2,
	.frame_rate_multiplication_factor = 0,
	.i_qp_reduce = 6,
	.skip_flags = 0x00000000,
	.M = 0,
	.N = 0,
	.p_qp_reduce = 3,
	.intra_refresh_num_mb_row = 0,
	.preview_A_frame_rate_divison_factor = 0,
	.idr_interval = 0,
	.custom_vin_frame_rate = 0,
	.roi_daddr = 0x00000000,
	.roi_delta[0][0] = 0,
	.roi_delta[0][1] = 0,
	.roi_delta[0][2] = 0,
	.roi_delta[0][3] = 0,
	.roi_delta[1][0] = 0,
	.roi_delta[1][1] = 0,
	.roi_delta[1][2] = 0,
	.roi_delta[1][3] = 0,
	.roi_delta[2][0] = 0,
	.roi_delta[2][1] = 0,
	.roi_delta[2][2] = 0,
	.roi_delta[2][3] = 0,
	.panic_div = 0,
	.is_monochrome = 0,
	.scene_change_detect_on = 0,
	.flat_area_improvement_on = 0,
	.drop_frame = 0,
	.pic_size_control = 0,
	.quant_matrix_addr = 0x00000000,
	.P_IntraBiasAdd = 0,
	.B_IntraBiasAdd = 0,
	.zmv_threshold = 0,
	.N_msb = 0,
	.idsp_frame_rate_M = 0,
	.idsp_frame_rate_N = 0,
	.roi_daddr_p = 0,
	.roi_daddr_b = 0,
	.user1_intra_bias = 0,
	.user1_direct_bias = 0,
	.user2_intra_bias = 0,
	.user2_direct_bias = 0,
	.user3_intra_bias = 0,
	.user3_direct_bias = 0,
	.force_pskip_num_plus1 = 0,
	.set_I_size = 0,
	.q_qp_reduce = 6,
	.qp_min_on_Q = 21,
	.qp_max_on_Q = 51,
	.log_q_num_per_gop_plus_1 = 0,
};

static video_hiso_config_update_t iso_proc_control = {
	.cmd_code = VIDEO_HISO_CONFIG_UPDATE,
	.hiso_param_daddr = 0,
#if 0
	.loadcfg_type.flag.hiso_config_common_update = 1,
	.loadcfg_type.flag.hiso_config_color_update = 1,
	.loadcfg_type.flag.hiso_config_mctf_update = 1,
	.loadcfg_type.flag.hiso_config_step1_update = 1,
	.loadcfg_type.flag.hiso_config_step2_update = 1,
	.loadcfg_type.flag.hiso_config_step3_update = 1,
	.loadcfg_type.flag.hiso_config_step4_update = 1,
	.loadcfg_type.flag.hiso_config_step5_update = 1,
	.loadcfg_type.flag.hiso_config_step6_update = 1,
	.loadcfg_type.flag.hiso_config_step7_update = 1,
	.loadcfg_type.flag.hiso_config_step8_update = 1,
	.loadcfg_type.flag.hiso_config_step9_update = 1,
	.loadcfg_type.flag.hiso_config_step10_update = 1,
	.loadcfg_type.flag.hiso_config_step11_update = 1,
	.loadcfg_type.flag.hiso_config_step15_update = 1,
	.loadcfg_type.flag.hiso_config_step16_update = 1,
	.loadcfg_type.flag.hiso_config_vwarp_update = 1,
	.loadcfg_type.flag.hiso_config_flow_update = 1,
	.loadcfg_type.word = 0xc0000,
#endif
	.loadcfg_type.flag.hiso_config_aaa_update = 1,
};

static aaa_statistics_setup_t aaa_statis_setup = {
	.cmd_code = AAA_STATISTICS_SETUP,
	.on = 1,
	.auto_shift = 1,
	.data_fifo_no_reset = 1,
	.data_fifo2_no_reset = 1,
	.reserved = 0,
	.data_fifo_base = DSP_AAA_STATS_RGB_START,
	.data_fifo_limit = DSP_AAA_STATS_CFA_START,
	.data_fifo2_base = DSP_AAA_STATS_CFA_START,
	.data_fifo2_limit = DSP_AAA_STATS_CFA_START + CFA_AAA_DATA_BLOCK_ARRAY,
	.awb_tile_num_col = 32,
	.awb_tile_num_row = 32,
	.awb_tile_col_start = 0,
	.awb_tile_row_start = 0,
	.awb_tile_width = 128,
	.awb_tile_height = 128,
	.awb_tile_active_width = 128,
	.awb_tile_active_height = 128,
	.awb_pix_min_value = 0,
	.awb_pix_max_value = 16383,
	.ae_tile_num_col = 12,
	.ae_tile_num_row = 8,
	.ae_tile_col_start = 0,
	.ae_tile_row_start = 0,
	.ae_tile_width = 340,
	.ae_tile_height = 512,
	.ae_pix_min_value = 1,
	.ae_pix_max_value = 16383,
	.af_tile_num_col = 12,
	.af_tile_num_row = 8,
	.af_tile_col_start = 0,
	.af_tile_row_start = 0,
	.af_tile_width = 340,
	.af_tile_height = 512,
	.af_tile_active_width = 340,
	.af_tile_active_height = 512,
};

static aaa_histogram_t aaa_histogram_setup = {
	.cmd_code = AAA_HISTORGRAM_SETUP,
	.mode = 2,
	.ae_file_mask[0] = 0xfff,
	.ae_file_mask[1] = 0xfff,
	.ae_file_mask[2] = 0xfff,
	.ae_file_mask[3] = 0xfff,
	.ae_file_mask[4] = 0xfff,
	.ae_file_mask[5] = 0xfff,
	.ae_file_mask[6] = 0xfff,
	.ae_file_mask[7] = 0xfff,
};

static enc_sync_cmd_t enc_sync_cmd = {
	.cmd_code = IPCAM_ENC_SYNC_CMD,
	.target_pts = 0,
	.cmd_daddr = 0,
	.enable_flag = 1,
	.force_update_flag = 0,
};

void prepare_aaa_statistics_setup_cmd(void)
{
	iso_proc_control.hiso_param_daddr =  video_preprocessing.still_iso_config_daddr;
}

void dsp_issue_force_idr_cmd(unsigned int seq_num, u32 pts)
{
	enc_sync_cmd.target_pts = pts;
	enc_sync_cmd.cmd_daddr = (u32)&stream_rt_update;
	add_dsp_cmd_seq(&enc_sync_cmd, sizeof(enc_sync_cmd), seq_num);
}

void dsp_issue_liso_cfg_cmd(unsigned int seq_num)
{
	add_dsp_cmd_seq(&aaa_statis_setup, sizeof(aaa_statis_setup), seq_num);
	add_dsp_cmd_seq(&aaa_histogram_setup, sizeof(aaa_histogram_setup),seq_num);
	add_dsp_cmd_seq(&iso_proc_control, sizeof(iso_proc_control), seq_num);
}

void init_iso_cfg_addr(u32 *p_iso_cfg_addr)
{
	*p_iso_cfg_addr = video_preprocessing.still_iso_config_daddr;
}

void init_sensor_aaa_param(aaa_config_info_t *p_aaa_cfg_info)
{
	p_aaa_cfg_info->p_ae_gain_table = fb_imx322_aeb_gain_table;
	p_aaa_cfg_info->p_ae_lines = fb_imx322_60hz_lines;
	p_aaa_cfg_info->p_awb_param = &fb_imx322_awb_param;
	p_aaa_cfg_info->max_agc_index = (sizeof(fb_imx322_aeb_gain_table)/sizeof(u32) -1);
}

void fb_sensor_ctrl(aaa_output_t* p_aaa_output)
{
	u32 agc_index=p_aaa_output->agc_index;
	//write agc
	idc_bld_write_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR, IMX322_REG_GAIN, agc_index);
	u32 frame_length_lines = 0;
	u32 val_high = 0, val_low = 0;
	u32 num_line = 0, min_line = 0, max_line = 0;
	u32 blank_lines = 0;

	val_high=idc_bld_read_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR, IMX322_VMAX_MSB);
	val_low=idc_bld_read_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR, IMX322_VMAX_LSB);

	frame_length_lines = (val_high << 8) + val_low;
	num_line = p_aaa_output->shutter_row;
	/* FIXME: shutter width: 1 ~ Frame format(V) */
	min_line = 1;
	max_line = frame_length_lines;
	num_line = clamp(num_line, min_line, max_line);
	blank_lines = frame_length_lines - num_line; /* get the shutter sweep time */

	//write shutter
	idc_bld_write_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR, IMX322_SHS1_MSB, blank_lines >> 8);
	idc_bld_write_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR, IMX322_SHS1_LSB, blank_lines & 0xff);
}

