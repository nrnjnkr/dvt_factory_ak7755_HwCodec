/**
 * boards/hawthorn/bsp/iav/sensor_imx322.c
 *
 * History:
 *    2015/06/15 - [Chu Chen] created file
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


#define BOOT_VIDEO_MODE 0x00020013
#define BOOT_HDR_MODE 0     // 0 = LINEAR

struct vin_reg_16_8 {
	u16 addr;
	u8 data;
};

#define IMX322_IDC_SUB_ADDR 	(0x34)
#define IMX322_REG_GAIN		(0x301E)
#define IMX322_SHS1_MSB		(0x0202)
#define IMX322_SHS1_LSB		(0x0203)
#define IMX322_VMAX_MSB		(0x0340)
#define IMX322_VMAX_LSB		(0x0341)

static const struct vin_reg_16_8 imx322_1080p_regs[] = {
	{0x301f, 0x73},
	{0x3027, 0x20},
	{0x3117, 0x0d},
	{0x304f, 0x47}, /* SYNC2_EN */
	{0x3054, 0x13}, /* SYNCSEL */
	{0x3011, 0x00}, /* FRSEL/OPORTSEL/M12BEN */
	{0x3002, 0x0f}, /* HD1080p mode */
	{0x3016, 0x3c}, /* WINPV_LSB */
	{0x3022, 0x40}, /* 720PMODE */
	{0x302D, 0x40}, /* DCKDLY_BITSEL */
	{0x309a, 0x26}, /* 12B1080P_LSB */
	{0x309b, 0x02}, /* 12B1080P_MSB */
	{0x30ce, 0x16}, /* PRES */
	{0x30cf, 0x82}, /* DRES_LSB */
	{0x30d0, 0x00}, /* DRES_MSB */
	{0x0009, 0xf0}, /* I2C BLKLEVEL */
	{0x0112, 0x0c}, /* I2C ADRES1 */
	{0x0113, 0x0c}, /* I2C ADRES2 */
	{0x0340, 0x04}, /* VMAX_MSB */
	{0x0341, 0x65}, /* VMAX_LSB */
	{0x0342, 0x04}, /* HMAX_MSB */
	{0x0343, 0x4c}, /* HMAX_LSB */
	{0x3012, 0x82},
	{0x0100, 0x01}, /* cancel standby */
	{0x302C, 0x00}, /* master mode */
	{0x0202, 0x02},
	{0x0203, 0x34},
	{0x301e, 0x00},
};

static u32 set_vin_config_dram[] = {
	0xc0100017, 0x321052b6, 0x00987654, 0x00000000,
	0x00300000, 0x07af0019, 0x00000450, 0x000001cc,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00010000, 0x00010000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000,
};

static struct vin_video_format vin_video_boot_format = {
	.video_mode = BOOT_VIDEO_MODE,
	.device_mode = 0,
	.pll_idx = 0,
	.width = 0,
	.height = 0,

	.def_start_x = 0,
	.def_start_y = 0,
	.def_width = 0,
	.def_height = 0,
	.format = 0,
	.type = 0,
	.bits = 0,
	.ratio = 0,
	.bayer_pattern = 3,//pattern
	.hdr_mode = BOOT_HDR_MODE,
	.readout_mode = 0,

	.max_fps = 0,
	.default_fps = 0,
	.default_agc = 0,
	.default_shutter_time = 0,
	.default_bayer_pattern = 0,

	/* hdr mode related */
	.act_start_x = 0,
	.act_start_y = 0,
	.act_width = 0,
	.act_height = 0,
	//.max_act_width = 0,
	//.max_act_height = 0,
	.hdr_long_offset = 0,
	.hdr_short1_offset = 0,
	.hdr_short2_offset = 0,
	.hdr_short3_offset = 0,
	.dual_gain_mode = 0
};

static chip_select_t chip_selection = {
	.cmd_code = CHIP_SELECTION,
	.chip_type = 3
};

static system_setup_info_t system_setup_info = {
	.cmd_code = SYSTEM_SETUP_INFO,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
	.preview_A_type = 0,
	.preview_B_type = 0,
	.voutA_osd_blend_enabled = 1,
	.voutB_osd_blend_enabled = 0,
	/* End */
	.pip_vin_enabled = 0,
	.raw_encode_enabled = 0,
#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE
	.adv_iso_disabled = 1,
#else
	.adv_iso_disabled = 0,
#endif
	.sub_mode_sel.multiple_stream = 2,
	.sub_mode_sel.power_mode = 0,
	.lcd_3d = 0,
	.iv_360 = 0,
#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE
	.mode_flags = 4,
#else
	.mode_flags = 0,
#endif
	.audio_clk_freq = 0,
	.idsp_freq = 0,
	.sensor_HB_pixel = 0,
	.sensor_VB_pixel = 0,
	.hdr_preblend_from_vin = 0,
	.hdr_num_exposures_minus_1 = 0,
	.vout_swap = 0,
	//.boot_mode = NORMAL_BOOT
};

static set_vin_global_config_t set_vin_global_clk = {
	.cmd_code = SET_VIN_GLOBAL_CLK,
	.main_or_pip = 0,
	.global_cfg_reg_word0 = 9
};

static set_vin_config_t set_vin_config = {
	.cmd_code = SET_VIN_CONFIG,
	.vin_width = 1920,
	.vin_height = 1080,
	.vin_config_dram_addr = (u32)set_vin_config_dram,
	.config_data_size = 0x00000072,
	.sensor_resolution = 0,
	.sensor_bayer_pattern = 0,
	.vin_set_dly = 0,
	.vin_enhance_mode = 0,
	.vin_panasonic_mode = 0
};

static set_vin_master_t set_vin_master_clk = {
	.cmd_code = SET_VIN_MASTER_CLK,
	.master_sync_reg_word0 = 0,
	.master_sync_reg_word1 = 0,
	.master_sync_reg_word2 = 0,
	.master_sync_reg_word3 = 0,
	.master_sync_reg_word4 = 0,
	.master_sync_reg_word5 = 0,
	.master_sync_reg_word6 = 0,
	.master_sync_reg_word7 = 0
};

static sensor_input_setup_t sensor_input_setup = {
	.cmd_code = SENSOR_INPUT_SETUP,
	.sensor_id = 127,
	.field_format = 1,
	.sensor_resolution = 12,
	.sensor_pattern = 3,
	.first_line_field_0 = 0,
	.first_line_field_1 = 0,
	.first_line_field_2 = 0,
	.first_line_field_3 = 0,
	.first_line_field_4 = 0,
	.first_line_field_5 = 0,
	.first_line_field_6 = 0,
	.first_line_field_7 = 0,
#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE
	.sensor_readout_mode = 0
#endif
};

static video_preproc_t video_preprocessing = {
	.cmd_code = VIDEO_PREPROCESSING,
	.input_format = 0,
	.sensor_id = 127,
	.keep_states = 0,
	.vin_frame_rate = 1,
	.vidcap_w = 1920,
	.vidcap_h = 1080,
	.main_w = 1920,
	.main_h = 1080,
	.encode_w = 1920,
	.encode_h = 1080,
	.encode_x = 0,
	.encode_y = 0,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
	.preview_w_A = 0,
	.preview_h_A = 0,
	.input_center_x = 0,
	.input_center_y = 0,
	.zoom_factor_x = 0,
	.zoom_factor_y = 0,
	.aaa_data_fifo_start = 0,
	.sensor_readout_mode = 0,
	.noise_filter_strength = 0,
	.bit_resolution = 12,
	.bayer_pattern = 3,
	/* END */
	.preview_format_A = 6,
	.preview_format_B = 1,
	.no_pipelineflush = 0,
	.preview_frame_rate_A = 0,
	.preview_w_B = 720,
	.preview_h_B = 480,
	.preview_frame_rate_B = 0,
	.preview_A_en = 0,
	.preview_B_en = 1,
	.vin_frame_rate_ext = 0,
	.vdsp_int_factor = 0,
	.main_out_frame_rate = 1,
	.main_out_frame_rate_ext = 0,
	.vid_skip = 0,
	.EIS_delay_count = 0,
#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE
	.Vert_WARP_enable = 1,
#else
	.Vert_WARP_enable = 0,
#endif
	.force_stitching = 1,
	.no_vin_reset_exiting = 0,
	.cmdReadDly = 0,
	.preview_src_w_A = 0,
	.preview_src_h_A = 0,
	.preview_src_x_offset_A = 0,
	.preview_src_y_offset_A = 0,
	.preview_src_w_B = 0,
	.preview_src_h_B = 0,
	.preview_src_x_offset_B = 0,
	.preview_src_y_offset_B = 0,
#ifdef CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE
	.still_iso_config_daddr = (u32)DSP_STILL_ISO_CONFIG_START
#else
	.still_iso_config_daddr = 0x00000000
#endif
};
#if defined( CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE)
#include "ldc_warp_ctrl_tables.h"
static set_warp_control_t set_warp_control_mode4 = {
	.cmd_code = SET_WARP_CONTROL,
	.zoom_x = 0,
	.zoom_y = 0,
	.x_center_offset = 0,
	.y_center_offset = 0,
	.actual_left_top_x = 0,
	.actual_left_top_y = 0,
	.actual_right_bot_x = 92274688,
	.actual_right_bot_y = 62914560,
	.dummy_window_x_left = 256,
	.dummy_window_y_top = 60,
	.dummy_window_width = 1408,
	.dummy_window_height = 960,
	.cfa_output_width = 1408,
	.cfa_output_height = 960,
	.warp_control = 1,
	.warp_horizontal_table_address = (u32)warp_ctrl_h_table,
	.warp_vertical_table_address = (u32)warp_ctrl_v_table,
	.grid_array_width = 31,
	.grid_array_height = 31,
	.horz_grid_spacing_exponent = 2,
	.vert_grid_spacing_exponent = 1,
	.vert_warp_enable = 1,
	.vert_warp_grid_array_width = 31,
	.vert_warp_grid_array_height = 35,
	.vert_warp_horz_grid_spacing_exponent = 2,
	.vert_warp_vert_grid_spacing_exponent = 1,
	.ME1_vwarp_grid_array_width = 16,
	.ME1_vwarp_grid_array_height = 18,
	.ME1_vwarp_horz_grid_spacing_exponent = 1,
	.ME1_vwarp_vert_grid_spacing_exponent = 0,
	.ME1_vwarp_table_address = (u32)warp_ctrl_me1_table
};
#endif
static set_warp_control_t set_warp_control = {
	.cmd_code = SET_WARP_CONTROL,
	.zoom_x = 0,
	.zoom_y = 0,
	.x_center_offset = 0,
	.y_center_offset = 0,
	.actual_left_top_x = 0,
	.actual_left_top_y = 0,
	.actual_right_bot_x = 125829120,
	.actual_right_bot_y = 70778880,
	.dummy_window_x_left = 0,
	.dummy_window_y_top = 0,
	.dummy_window_width = 1920,
	.dummy_window_height = 1080,
	.cfa_output_width = 1920,
	.cfa_output_height = 1080,
	.warp_control = 0,
	.warp_horizontal_table_address = 0x00000000,
	.warp_vertical_table_address = 0x00000000,
	.grid_array_width = 0,
	.grid_array_height = 0,
	.horz_grid_spacing_exponent = 0,
	.vert_grid_spacing_exponent = 0,
	.vert_warp_enable = 0,
	.vert_warp_grid_array_width = 0,
	.vert_warp_grid_array_height = 0,
	.vert_warp_horz_grid_spacing_exponent = 0,
	.vert_warp_vert_grid_spacing_exponent = 0,
	.ME1_vwarp_grid_array_width = 0,
	.ME1_vwarp_grid_array_height = 0,
	.ME1_vwarp_horz_grid_spacing_exponent = 0,
	.ME1_vwarp_vert_grid_spacing_exponent = 0,
	.ME1_vwarp_table_address = 0x00000000
};


static ipcam_video_system_setup_t ipcam_video_system_setup = {
	.cmd_code = IPCAM_VIDEO_SYSTEM_SETUP,
	.main_max_width = 1920,
	.main_max_height = 1088,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
	.preview_A_max_width = 0,
	.preview_A_max_height = 0,
	.preview_B_max_width = 0,
	.preview_B_max_height = 0,
	/* END */
	.preview_C_max_width = 720,
	.preview_C_max_height = 576,
	.stream_0_max_GOP_M = 1,
	.stream_1_max_GOP_M = 1,
	.stream_2_max_GOP_M = 1,
	.stream_3_max_GOP_M = 0,
	.stream_0_max_width = 1920,
	.stream_0_max_height = 1088,
	.stream_1_max_width = 1280,
	.stream_1_max_height = 720,
	.stream_2_max_width = 1280,
	.stream_2_max_height = 720,
	.stream_3_max_width = 0,
	.stream_3_max_height = 0,
	.MCTF_possible = 1,
	.max_num_streams = 4,
	.max_num_cap_sources = 2,
	.use_1Gb_DRAM_config = 0,
	.raw_compression_disabled = 0,
	.vwarp_blk_h = 28,
	.extra_buf_main = 0,
	.extra_buf_preview_a = 0,
	.extra_buf_preview_b = 0,
	.extra_buf_preview_c = 0,
	.raw_max_width = 1920,
	.raw_max_height = 1080,
	.warped_main_max_width = 0,
	.warped_main_max_height = 0,
	.v_warped_main_max_width = 0,
	.v_warped_main_max_height = 0,
	.max_warp_region_input_width = 0,
	.max_warp_region_output_width = 0,
	.max_padding_width = 256,
	.enc_rotation_possible = 1,
	.enc_dummy_latency = 2,
	.stream_0_LT_enable = 0,
	.stream_1_LT_enable = 0,
	.stream_2_LT_enable = 0,
	.stream_3_LT_enable = 0,
	.enc_buf_extra_MB_row_at_top = 1
};

static ipcam_capture_preview_size_setup_t ipcam_video_capture_preview_size_setup = {
	.cmd_code = IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP,
	.preview_id = 3,
	.output_scan_format = 0,
	.deinterlace_mode = 0,
	.disabled = 0,
	.cap_width = 720,
	.cap_height = 480,
	.input_win_offset_x = 0,
	.input_win_offset_y = 0,
	.input_win_width = 1920,
	.input_win_height = 1080
};

static ipcam_capture_preview_size_setup_t ipcam_video_capture_preview_b_size_setup = {
    .cmd_code = IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP,
    .preview_id = 2,
    .output_scan_format = 0,
    .deinterlace_mode = 0,
    .disabled = 1,
    .cap_width = 0,
    .cap_height = 0,
    .input_win_offset_x = 0,
    .input_win_offset_y = 0,
    .input_win_width = 1920,
    .input_win_height = 1080
};

#define STREAM0      0
#define STREAM1      1

static ipcam_video_encode_size_setup_t ipcam_video_encode_size_setup = {
	.cmd_code = IPCAM_VIDEO_ENCODE_SIZE_SETUP,
	.capture_source = 0,
	.enc_x = 0,
	.enc_y = 0,
	.enc_width = 1920,
	.enc_height = 1088
};

static ipcam_video_encode_size_setup_t ipcam_video_encode_size_setup_2nd = {
	.cmd_code = (IPCAM_VIDEO_ENCODE_SIZE_SETUP | ((STREAM1) & 0x3) << 30),
	.capture_source = 3,
	.enc_x = 0,
	.enc_y = 0,
	.enc_width = 720,
	.enc_height = 480
};

static h264_encode_setup_t h264_encoding_setup = {
	.cmd_code = H264_ENCODING_SETUP,
	.mode = 1,
	.M = 1,
	.N = 30,
	.quality = 0,
	.average_bitrate = 4000000,
	.vbr_cntl = 0,
	.vbr_setting = 4,
	.allow_I_adv = 0,
	.cpb_buf_idc = 0,
	.en_panic_rc = 0,
	.cpb_cmp_idc = 0,
	.fast_rc_idc = 0,
	.target_storage_space = 0,
	.bits_fifo_base = 0x00000000,
	.bits_fifo_limit = 0x00000000,
	.info_fifo_base = 0x00000000,
	.info_fifo_limit = 0x00000000,
	.audio_in_freq = 2,
	.vin_frame_rate = 0,
	.encoder_frame_rate = 0,
	.frame_sync = 0,
	.initial_fade_in_gain = 0,
	.final_fade_out_gain = 0,
	.idr_interval = 1,
	.cpb_user_size = 0,
	.numRef_P = 0,
	.numRef_B = 0,
	.vin_frame_rate_ext = 0,
	.encoder_frame_rate_ext = 0,
	.N_msb = 0,
	.fast_seek_interval = 0,
	.vbr_cbp_rate = 0,
	.frame_rate_division_factor = 1,
	.force_intlc_tb_iframe = 0,
	.session_id = 0,
	.frame_rate_multiplication_factor = 1,
	.hflip = 0,
	.vflip = 0,
	.rotate = 0,
	.chroma_format = 0,
	.custom_encoder_frame_rate = 0x7530
};


static h264_encode_setup_t h264_encoding_setup_2nd = {
	.cmd_code = (H264_ENCODING_SETUP | ((STREAM1) & 0x3) << 30),
	.mode = 1,
	.M = 1,
	.N = 30,
	.quality = 0,
	.average_bitrate = 1000000,
	.vbr_cntl = 0,
	.vbr_setting = 4,
	.allow_I_adv = 0,
	.cpb_buf_idc = 0,
	.en_panic_rc = 0,
	.cpb_cmp_idc = 0,
	.fast_rc_idc = 0,
	.target_storage_space = 0,
	.bits_fifo_base = 0x00000000,
	.bits_fifo_limit = 0x00000000,
	.info_fifo_base = 0x00000000,
	.info_fifo_limit = 0x00000000,
	.audio_in_freq = 2,
	.vin_frame_rate = 0,
	.encoder_frame_rate = 0,
	.frame_sync = 0,
	.initial_fade_in_gain = 0,
	.final_fade_out_gain = 0,
	.idr_interval = 1,
	.cpb_user_size = 0,
	.numRef_P = 0,
	.numRef_B = 0,
	.vin_frame_rate_ext = 0,
	.encoder_frame_rate_ext = 0,
	.N_msb = 0,
	.fast_seek_interval = 0,
	.vbr_cbp_rate = 0,
	.frame_rate_division_factor = 1,
	.force_intlc_tb_iframe = 0,
	.session_id = 0,
	.frame_rate_multiplication_factor = 1,
	.hflip = 0,
	.vflip = 0,
	.rotate = 0,
	.chroma_format = 0,
	.custom_encoder_frame_rate = 0x7530
};

static h264_encode_t h264_encode = {
	.cmd_code = H264_ENCODE,
	.bits_fifo_next = 0x00000000,
	.info_fifo_next = 0x00000000,
	.start_encode_frame_no = -1,
	.encode_duration = -1,
	.is_flush = 1,
	.enable_slow_shutter = 0,
	.res_rate_min = 40,
	.alpha = 0,
	.beta = 0,
	.en_loop_filter = 2,
	.max_upsampling_rate = 1,
	.slow_shutter_upsampling_rate = 0,
	.frame_cropping_flag = 1,
	.profile = 0,
	.frame_crop_left_offset = 0,
	.frame_crop_right_offset = 0,
	.frame_crop_top_offset = 0,
	.frame_crop_bottom_offset = 4,
	.num_ref_frame = 0,
	.log2_max_frame_num_minus4 = 0,
	.log2_max_pic_order_cnt_lsb_minus4 = 0,
	.gaps_in_frame_num_value_allowed_flag = 0,
	.sony_avc = 0,
	.height_mjpeg_h264_simultaneous = 0,
	.width_mjpeg_h264_simultaneous = 0,
	.vui_enable = 1,
	.aspect_ratio_info_present_flag = 1,
	.overscan_info_present_flag = 0,
	.overscan_appropriate_flag = 0,
	.video_signal_type_present_flag = 1,
	.video_full_range_flag = 1,
	.colour_description_present_flag = 1,
	.chroma_loc_info_present_flag = 0,
	.timing_info_present_flag = 1,
	.fixed_frame_rate_flag = 1,
	.nal_hrd_parameters_present_flag = 1,
	.vcl_hrd_parameters_present_flag = 1,
	.low_delay_hrd_flag = 0,
	.pic_struct_present_flag = 1,
	.bitstream_restriction_flag = 0,
	.motion_vectors_over_pic_boundaries_flag = 0,
	.SAR_width = 0,
	.SAR_height = 0,
	.video_format = 5,
	.colour_primaries = 1,
	.transfer_characteristics = 1,
	.matrix_coefficients = 1,
	.chroma_sample_loc_type_top_field = 0,
	.chroma_sample_loc_type_bottom_field = 0,
	.aspect_ratio_idc = 1,
	.max_bytes_per_pic_denom = 0,
	.max_bits_per_mb_denom = 0,
	.log2_max_mv_length_horizontal = 0,
	.log2_max_mv_length_vertical = 0,
	.num_reorder_frames = 0,
	.max_dec_frame_buffering = 0,
	.I_IDR_sp_rc_handle_mask = 0,
	.IDR_QP_adj_str = 0,
	.IDR_class_adj_limit = 0,
	.I_QP_adj_str = 0,
	.I_class_adj_limit = 0,
	.firstGOPstartB = 0,
	.au_type = 1
};


static h264_encode_t h264_encode_2nd = {
	.cmd_code = (H264_ENCODE | ((STREAM1) & 0x3) << 30),
	.bits_fifo_next = 0x00000000,
	.info_fifo_next = 0x00000000,
	.start_encode_frame_no = -1,
	.encode_duration = -1,
	.is_flush = 1,
	.enable_slow_shutter = 0,
	.res_rate_min = 40,
	.alpha = 0,
	.beta = 0,
	.en_loop_filter = 2,
	.max_upsampling_rate = 1,
	.slow_shutter_upsampling_rate = 0,
	.frame_cropping_flag = 1,
	.profile = 0,
	.frame_crop_left_offset = 0,
	.frame_crop_right_offset = 0,
	.frame_crop_top_offset = 0,
	.frame_crop_bottom_offset = 0,
	.num_ref_frame = 0,
	.log2_max_frame_num_minus4 = 0,
	.log2_max_pic_order_cnt_lsb_minus4 = 0,
	.sony_avc = 0,
	.height_mjpeg_h264_simultaneous = 0,
	.width_mjpeg_h264_simultaneous = 0,
	.vui_enable = 1,
	.aspect_ratio_info_present_flag = 1,
	.overscan_info_present_flag = 0,
	.overscan_appropriate_flag = 0,
	.video_signal_type_present_flag = 1,
	.video_full_range_flag = 1,
	.colour_description_present_flag = 1,
	.chroma_loc_info_present_flag = 0,
	.timing_info_present_flag = 1,
	.fixed_frame_rate_flag = 1,
	.nal_hrd_parameters_present_flag = 1,
	.vcl_hrd_parameters_present_flag = 1,
	.low_delay_hrd_flag = 0,
	.pic_struct_present_flag = 1,
	.bitstream_restriction_flag = 0,
	.motion_vectors_over_pic_boundaries_flag = 0,
	.SAR_width = 32,
	.SAR_height = 27,
	.video_format = 5,
	.colour_primaries = 6,
	.transfer_characteristics = 6,
	.matrix_coefficients = 6,
	.chroma_sample_loc_type_top_field = 0,
	.chroma_sample_loc_type_bottom_field = 0,
	.aspect_ratio_idc = 255,
	.max_bytes_per_pic_denom = 0,
	.max_bits_per_mb_denom = 0,
	.log2_max_mv_length_horizontal = 0,
	.log2_max_mv_length_vertical = 0,
	.num_reorder_frames = 0,
	.max_dec_frame_buffering = 0,
	.I_IDR_sp_rc_handle_mask = 0,
	.IDR_QP_adj_str = 0,
	.IDR_class_adj_limit = 0,
	.I_QP_adj_str = 0,
	.I_class_adj_limit = 0,
	.firstGOPstartB = 0,
	.au_type = 1
};

static ipcam_real_time_encode_param_setup_t ipcam_real_time_encode_param_setup = {
	.cmd_code = IPCAM_REAL_TIME_ENCODE_PARAM_SETUP,
	.enable_flags = 0x0000c835,
	.cbr_modify = 4000000,
	.custom_encoder_frame_rate = 0,
	.frame_rate_division_factor = 0,
	.qp_min_on_I = 1,
	.qp_max_on_I = 30,
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
	.custom_vin_frame_rate = 1073771824,
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
	.P_IntraBiasAdd = 1,
	.B_IntraBiasAdd = 1,
	.zmv_threshold = 0,
	.N_msb = 0,
	.idsp_frame_rate_M = 30,
	.idsp_frame_rate_N = 30,
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
	.log_q_num_per_gop_plus_1 = 0
};


static ipcam_real_time_encode_param_setup_t ipcam_real_time_encode_param_setup_2nd = {
	.cmd_code = (IPCAM_REAL_TIME_ENCODE_PARAM_SETUP | ((STREAM1) & 0x3) << 30),
	.enable_flags = 0x0000c835,
	.cbr_modify = 1000000,
	.custom_encoder_frame_rate = 0,
	.frame_rate_division_factor = 0,
	.qp_min_on_I = 1,
	.qp_max_on_I = 51,
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
	.custom_vin_frame_rate = 1073771824,
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
	.P_IntraBiasAdd = 1,
	.B_IntraBiasAdd = 1,
	.zmv_threshold = 0,
	.N_msb = 0,
	.idsp_frame_rate_M = 30,
	.idsp_frame_rate_N = 30,
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
	.log_q_num_per_gop_plus_1 = 0
};

#if defined(CONFIG_S2LMELEKTRA_DSP_LOG_CAPTURE)
static dsp_debug_level_setup_t dsp_debug_level_setup =  {
	.cmd_code = DSP_DEBUG_LEVEL_SETUP,
	.module = 0,
	.debug_level = 3,
	.coding_thread_printf_disable_mask = 0
};
#endif

#if defined(CONFIG_PANDORA_RTOS)
#include "sensor_imx322_aaa.c"
#endif
static void rct_set_so_pll_imx322(void)
{
	writel(CLK_SI_INPUT_MODE_REG, 0x0);
	writel(PLL_SENSOR_FRAC_REG, 0x040000b3);
	writel(PLL_SENSOR_CTRL_REG, 0x101a1108);
	writel(PLL_SENSOR_CTRL_REG, 0x101a1109);
	writel(PLL_SENSOR_CTRL_REG, 0x101a1108);
	writel(SCALER_SENSOR_POST_REG, 0x00000011);
	writel(SCALER_SENSOR_POST_REG, 0x00000010);
	writel(PLL_SENSOR_CTRL2_REG, 0x3f770000);
	writel(PLL_SENSOR_CTRL3_REG, 0x00069300);
}

static int imx322_init(void)
{
	int i;

	/* set pll sent to sensor */
	rct_set_so_pll_imx322();

	/* imx322 can work with 800K I2C, so we use it to reduce timing */
	idc_bld_init(IDC_MASTER1, 800000);

	for (i = 0; i < ARRAY_SIZE(imx322_1080p_regs); i++) {
		idc_bld_write_16_8(IDC_MASTER1, IMX322_IDC_SUB_ADDR,
				imx322_1080p_regs[i].addr,
				imx322_1080p_regs[i].data);
	}
	putstr_debug("imx322_init");
	return 0;
}
static void store_vin_format_config(void){
	struct iav_fb_hdr *fb_hdr =
		(struct iav_fb_hdr *)DSP_FASTDATA_START;

	fb_hdr->bin[IAV_FB_VIN_VIDEO_FORMAT].offset = sizeof(struct iav_fb_hdr);
	fb_hdr->bin[IAV_FB_VIN_VIDEO_FORMAT].size = sizeof(struct vin_video_format);

	fb_hdr->bin[IAV_FB_VIN_DSP_CONFIG].offset = fb_hdr->bin[IAV_FB_VIN_VIDEO_FORMAT].offset +
		fb_hdr->bin[IAV_FB_VIN_VIDEO_FORMAT].size;
	fb_hdr->bin[IAV_FB_VIN_DSP_CONFIG].size = set_vin_config.config_data_size;
	/* store vin video format in amboot, restore it after enter Linux IAV */
	memcpy((void *)((u8 *)fb_hdr + fb_hdr->bin[IAV_FB_VIN_VIDEO_FORMAT].offset),
		&vin_video_boot_format, sizeof(vin_video_boot_format));

	/* store dsp vin config in amboot, restore it after enter Linux IAV*/
	memcpy((void *)((u8 *)fb_hdr + fb_hdr->bin[IAV_FB_VIN_DSP_CONFIG].offset),
		set_vin_config_dram, set_vin_config.config_data_size);
}

static inline void prepare_vin_vout_dsp_cmd(void)
{
	system_setup_info.preview_A_type = 0;
	system_setup_info.preview_B_type = 0;

	video_preprocessing.preview_w_A = 0;
	video_preprocessing.preview_h_A = 0;
	video_preprocessing.preview_format_A = 0;
	video_preprocessing.preview_format_B = 0;
	video_preprocessing.no_pipelineflush = 0;
	video_preprocessing.preview_frame_rate_A = 0;
	video_preprocessing.preview_w_B = 0;
	video_preprocessing.preview_h_B = 0;
	video_preprocessing.preview_frame_rate_B = 0;
	video_preprocessing.preview_A_en = 0;
	video_preprocessing.preview_B_en = 0;

	ipcam_video_system_setup.preview_A_max_width = 0;
	ipcam_video_system_setup.preview_A_max_height = 0;
	ipcam_video_system_setup.preview_B_max_width = 0;
	ipcam_video_system_setup.preview_B_max_height = 0;

	// system_setup_info.voutA_osd_blend_enabled = 0;
	// system_setup_info.voutB_osd_blend_enabled = 0;
}

static int setup_preview(unsigned int enable_ldc)
{
	//prepare_vin_vout_dsp_cmd();

	/* preview setup */
	add_dsp_cmd(&chip_selection, sizeof(chip_selection));
	add_dsp_cmd(&system_setup_info, sizeof(system_setup_info));
	add_dsp_cmd(&set_vin_global_clk, sizeof(set_vin_global_clk));
	add_dsp_cmd(&set_vin_config, sizeof(set_vin_config));
	add_dsp_cmd(&set_vin_master_clk, sizeof(set_vin_master_clk));
	add_dsp_cmd(&sensor_input_setup, sizeof(sensor_input_setup));
	add_dsp_cmd(&video_preprocessing, sizeof(video_preprocessing));
	if (enable_ldc){
#if defined( CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE)
		add_dsp_cmd(&set_warp_control_mode4, sizeof(set_warp_control));
#else
		add_dsp_cmd(&set_warp_control, sizeof(set_warp_control));
#endif
	}else{
		add_dsp_cmd(&set_warp_control, sizeof(set_warp_control));
	}
	add_dsp_cmd(&ipcam_video_system_setup, sizeof(ipcam_video_system_setup));
	add_dsp_cmd(&ipcam_video_capture_preview_size_setup,
		sizeof(ipcam_video_capture_preview_size_setup));
	add_dsp_cmd(&ipcam_video_capture_preview_b_size_setup,
		sizeof(ipcam_video_capture_preview_b_size_setup));

	putstr_debug("setup_preview");
	return 0;
}

static int setup_encoding(unsigned int stream1_enable)
{
	/* encode setup */
	add_dsp_cmd(&ipcam_video_encode_size_setup,
		sizeof(ipcam_video_encode_size_setup));
	add_dsp_cmd(&h264_encoding_setup, sizeof(h264_encoding_setup));
	add_dsp_cmd(&h264_encode, sizeof(h264_encode));
	add_dsp_cmd(&ipcam_real_time_encode_param_setup,
		sizeof(ipcam_real_time_encode_param_setup));
	putstr_debug("stream0 setup_encoding");

	if(stream1_enable) {
		add_dsp_cmd(&ipcam_video_encode_size_setup_2nd,
			sizeof(ipcam_video_encode_size_setup_2nd));
		add_dsp_cmd(&h264_encoding_setup_2nd, sizeof(h264_encoding_setup_2nd));
		add_dsp_cmd(&h264_encode_2nd, sizeof(h264_encode_2nd));
		add_dsp_cmd(&ipcam_real_time_encode_param_setup_2nd,
			sizeof(ipcam_real_time_encode_param_setup_2nd));
		putstr_debug("stream1 setup_encoding");
	}

	return 0;
}

/*streaming mode*/
#define VIDEO_MODE_H264_1080P     0
#define VIDEO_MODE_H264_720P       1

/*params_in_amboot.rotation_mode*/
#define ROTATION_MODE_CLOCKWISE_0     0
#define ROTATION_MODE_CLOCKWISE_90   1
#define ROTATION_MODE_CLOCKWISE_180 2
#define ROTATION_MODE_CLOCKWISE_270 3

static void on_boot_preview_rotation(struct adcfw_header *hdr, int boot_mode){
#if !defined(CONFIG_S2LMELEKTRA_TWO_REFERENCE)
	if(!hdr->params_in_amboot.rotation_mode){
		return;
	}

	switch(hdr->params_in_amboot.rotation_mode){
	case ROTATION_MODE_CLOCKWISE_90:
	case ROTATION_MODE_CLOCKWISE_270:
		if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P){
			ipcam_video_system_setup.stream_0_max_width = 1088;
			ipcam_video_system_setup.stream_0_max_height = 1920;
			ipcam_video_system_setup.enc_rotation_possible = 1;
		}else if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P){
			ipcam_video_system_setup.stream_0_max_width = 720;
			ipcam_video_system_setup.stream_0_max_height = 1280;
			ipcam_video_system_setup.enc_rotation_possible = 1;
		}
		break;
	default:
		break;
	}
#endif
}

static void prepare_preview_resolution(struct adcfw_header *hdr)
{
	if(hdr->params_in_amboot.stream0_enable){
		if (hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P) {
			// use preview B buffer (3rd source buffer) for 720p encoding
			system_setup_info.preview_B_type = 3;
			ipcam_video_system_setup.preview_B_max_width = 1280;
			ipcam_video_system_setup.preview_B_max_height =720;
			ipcam_video_capture_preview_b_size_setup.disabled = 0;
			ipcam_video_capture_preview_b_size_setup.cap_width = 1280;
			ipcam_video_capture_preview_b_size_setup.cap_height = 720;
		}else if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P){
			//use default settting
		}else{
			putstr("stream0 resolution unsupported!\r\n");
		}
	}
}

static void on_boot_encode_rotation(struct adcfw_header *hdr, int boot_mode)
{
#if !defined(CONFIG_S2LMELEKTRA_TWO_REFERENCE)
	if(!hdr->params_in_amboot.rotation_mode){
		return;
	}

	int rotation_mode = hdr->params_in_amboot.rotation_mode;
	switch(rotation_mode){
	case ROTATION_MODE_CLOCKWISE_90:
	case ROTATION_MODE_CLOCKWISE_270:
		if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P){
			ipcam_video_encode_size_setup.enc_x = 0;
			ipcam_video_encode_size_setup.enc_y = -8;
			ipcam_video_encode_size_setup.enc_width = 1088;
			ipcam_video_encode_size_setup.enc_height = 1920;
			h264_encoding_setup.rotate = 1;
			h264_encode.frame_crop_right_offset = 4;
			h264_encode.frame_crop_bottom_offset = 0;
		}else if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P){
			ipcam_video_encode_size_setup.enc_x = 0;
			ipcam_video_encode_size_setup.enc_y = 0;
			ipcam_video_encode_size_setup.enc_width = 720;
			ipcam_video_encode_size_setup.enc_height = 1280;
			h264_encoding_setup.rotate = 1;
		}
		if(rotation_mode == ROTATION_MODE_CLOCKWISE_270){
			if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P || hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P){
				ipcam_video_encode_size_setup.enc_y = 0;
				h264_encoding_setup.hflip = 1;
				h264_encoding_setup.vflip = 1;
			}
		}
		break;
	case ROTATION_MODE_CLOCKWISE_180:
		if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P){
			ipcam_video_encode_size_setup.enc_x = 0;
			ipcam_video_encode_size_setup.enc_y = -8;
		}

		if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P || hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P){
			h264_encoding_setup.hflip = 1;
			h264_encoding_setup.vflip = 1;
		}
		break;
	default:break;
	}
#endif
}

/* Advanced Functionality */
#if defined(CONFIG_S2LMELEKTRA_GOP_LENGTH)
#define GOP_N CONFIG_S2LMELEKTRA_GOP_LENGTH
#else
#define GOP_N 30
#endif

#if defined(CONFIG_S2LMELEKTRA_ONE_B_FRAME)
#define GOP_M 2
#elif defined(CONFIG_S2LMELEKTRA_TWO_B_FRAME)
#define GOP_M 3
#else
#define GOP_M 2
#endif

static inline void setup_two_ref()
{
#if defined(CONFIG_S2LMELEKTRA_TWO_REFERENCE)
	/* test_encode -A -h 1080p --gop 8 --max-gop-M 2 --long-ref-enable 1
	 * --multi-ref-p 1 --enc-rotate-possible 0 */
	ipcam_video_system_setup.stream_0_max_GOP_M = GOP_M;
	ipcam_video_system_setup.stream_1_max_GOP_M = GOP_M;
	ipcam_video_system_setup.enc_rotation_possible = 0;
	ipcam_video_system_setup.stream_0_LT_enable = 1;
	ipcam_video_system_setup.stream_1_LT_enable = 1;
	h264_encoding_setup.quality = 40;
	h264_encoding_setup_2nd.quality = 40;
#endif
}

static inline void setup_B_frame()
{
#if defined(CONFIG_S2LMELEKTRA_B_FRAME)
	/* test_encode -A -h 1080p --max-gop-M 3 -M 2 */
	ipcam_video_system_setup.stream_0_max_GOP_M = GOP_M;
	ipcam_video_system_setup.stream_1_max_GOP_M = GOP_M;
	h264_encoding_setup.M = GOP_M;
	h264_encoding_setup_2nd.M = GOP_M;
#if defined(CONFIG_S2LMELEKTRA_TWO_REFERENCE)
	ipcam_video_system_setup.B_frame_enable_in_LT_gop = 1;
#endif
#endif
}

static inline void setup_gop_length()
{
	h264_encoding_setup.N = GOP_N;
	h264_encoding_setup_2nd.N = GOP_N;
}
/* End of Advanced Functionality */

static int iav_boot_preview(struct adcfw_header *hdr, int boot_mode)
{
	// prepare_vin_vout_dsp_cmd();
#if defined(CONFIG_PANDORA_RTOS)
	prepare_aaa_statistics_setup_cmd();
#endif
	unsigned int enable_ldc = hdr->params_in_amboot.enable_ldc;
	prepare_preview_resolution(hdr);
	on_boot_preview_rotation(hdr, boot_mode);
	setup_two_ref();
	setup_B_frame();
#if defined(CONFIG_S2LMELEKTRA_VCA)
    if(boot_mode == 4 && hdr->params_in_amboot.enable_vca){
        ipcam_video_system_setup.preview_C_max_width = 720;
        ipcam_video_system_setup.preview_C_max_height = 480;
        ipcam_video_system_setup.vca_preview_id = 3;
        ipcam_video_system_setup.vca_frame_num = hdr->params_in_amboot.vca_frame_num;
        ipcam_video_system_setup.vca_daddr_base = DSP_VCA_START;
        ipcam_video_system_setup.vca_daddr_size = DSP_VCA_SIZE;
        ipcam_video_capture_preview_size_setup.skip_interval = 0;
        ipcam_video_capture_preview_size_setup.cap_width = 720;
        ipcam_video_capture_preview_size_setup.cap_height = 480;
        //putstrhex("added vca preview cmd, vca buffer addr\n", DSP_VCA_START);
        //putstrhex("added vca preview cmd, vca buffer size\n", DSP_VCA_SIZE);
    }
#endif
    return setup_preview(enable_ldc);
}

static int iav_boot_encode(struct adcfw_header *hdr, int boot_mode)
{
	unsigned int multiplication_factor=30;
	unsigned int division_factor=30;
	if (hdr->params_in_amboot.stream0_fps > 30){
		multiplication_factor=30;
		putstr("Floor to 30fps\r\n");
	}else if (hdr->params_in_amboot.stream0_fps < 15){
		multiplication_factor=15;
		putstr("Ceil to 15fps\r\n");
	}else{
		multiplication_factor=hdr->params_in_amboot.stream0_fps;
	}
	if(hdr->params_in_amboot.stream0_enable){
		if (hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_720P) {
			ipcam_video_encode_size_setup.enc_width = 1280;
			ipcam_video_encode_size_setup.enc_height = 720;
			ipcam_video_encode_size_setup.capture_source = 2;
			h264_encode.frame_crop_bottom_offset = 0;
		}else if(hdr->params_in_amboot.stream0_resolution == VIDEO_MODE_H264_1080P){
			//use default setting
		}else{
			putstr("stream0 resolution unsupported!\r\n");
		}
		h264_encoding_setup.average_bitrate = hdr->params_in_amboot.stream0_bitrate;
		h264_encoding_setup.frame_rate_multiplication_factor = multiplication_factor;
		h264_encoding_setup.frame_rate_division_factor = division_factor;
		ipcam_real_time_encode_param_setup.cbr_modify = hdr->params_in_amboot.stream0_bitrate;
#if defined(CONFIG_PANDORA_RTOS)
		stream_rt_update.cbr_modify = hdr->params_in_amboot.stream0_bitrate;
#endif
	}

	//TODO: rotation handle
	on_boot_encode_rotation(hdr, boot_mode);
	setup_gop_length();

	putstr_debug("setup_encoding");
	add_dsp_cmd(&ipcam_video_encode_size_setup, sizeof(ipcam_video_encode_size_setup));
	add_dsp_cmd(&h264_encoding_setup, sizeof(h264_encoding_setup));
	add_dsp_cmd(&h264_encode, sizeof(h264_encode));
	add_dsp_cmd(&ipcam_real_time_encode_param_setup, sizeof(ipcam_real_time_encode_param_setup));
	return setup_encoding(hdr->params_in_amboot.stream1_enable);
}


static inline int iav_boot_dsplog(void)
{
#if defined(CONFIG_S2LMELEKTRA_DSP_LOG_CAPTURE)
	add_dsp_cmd(&dsp_debug_level_setup, sizeof(dsp_debug_level_setup));
#endif
	return 0;
}

