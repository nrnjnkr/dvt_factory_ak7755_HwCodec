
/**
 * fb_aaa_ctl.h
 *
 * History:
 *    2017/03/27 - [Teng Huang] created file
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

#ifndef	__FB_AAA_CTRL_H__
#define	__FB_AAA_CTRL_H__

#include <amboot.h>

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

//stats
typedef struct awb_data_s {
	u32 r_avg;
	u32 g_avg;
	u32 b_avg;
	u32 lin_y;
	u32 cr_avg;
	u32 cb_avg;
	u32 non_lin_y;
}awb_data_t;

typedef struct ae_data_s {
	u32 lin_y;
	u32 non_lin_y;
}ae_data_t;

typedef struct af_data_s {
	u16 sum_fy;
	u16 sum_fv1;
	u16 sum_fv2;
}af_data_t;

typedef struct aaa_data_header_info_s
{
    // 1 u32
	u16 awb_tile_col_start;
	u16 awb_tile_row_start;
    // 2 u32
	u16 awb_tile_width;
	u16 awb_tile_height;
    // 3 u32
	u16 awb_tile_active_width;
	u16 awb_tile_active_height;
    // 4 u32
	u16 awb_rgb_shift;
	u16 awb_y_shift;
    //5 u32
	u16 awb_min_max_shift;
	u16 ae_tile_col_start;
    //6 u32
	u16 ae_tile_row_start;
	u16 ae_tile_width;
    //7 u32
	u16 ae_tile_height;
	u16 ae_y_shift;
    //8 u32
	u16 ae_linear_y_shift;
	u16 ae_min_max_shift;

    //9 u32
	u16 af_tile_col_start;
	u16 af_tile_row_start;

    //10 u32
	u16 af_tile_width;
	u16 af_tile_height;

    //11 u32
	u16 af_tile_active_width;
	u16 af_tile_active_height;

    //12 u32
	u16 af_y_shift;
	u16 af_cfa_y_shift;

    //13 u32
	u8  awb_tile_num_col;
	u8  awb_tile_num_row;
	u8  ae_tile_num_col;
	u8  ae_tile_num_row;

    //14 u32
	u8  af_tile_num_col;
	u8  af_tile_num_row;
	u8 total_exposures;
	u8 exposure_index;


   //15 u32
	u8 total_slices_x;
	u8 total_slices_y;
	u8 slice_index_x;
	u8 slice_index_y;


   //16 u32
	u16 slice_width;
	u16 slice_height;


   //17 u32
	u16 slice_start_x;
	u16 slice_start_y;
	u16 black_red;
	u16 black_green;
	u16 black_blue;

	u16 reserved[27];
}aaa_data_header_info_t;

typedef struct aaa_data_s
{
	awb_data_t awb_info[1024];
	ae_data_t ae_info[96];
	af_data_t af_info[96];
}aaa_data_t;

struct cfa_awb_stat {
	u16	sum_r;
	u16	sum_g;
	u16	sum_b;
	u16	count_min;
	u16	count_max;
};

struct cfa_ae_stat {
	u16	lin_y;
	u16	count_min;
	u16	count_max;
};

struct cfa_af_stat {
	u16	sum_fy;
	u16	sum_fv1;
	u16	sum_fv2;
};

typedef struct cfa_aaa_stat_s {
	aaa_data_header_info_t	header_info;
	u16       frame_id;
	struct cfa_awb_stat	awb_stat[1024];
	struct cfa_ae_stat		ae_stat[96];
	struct cfa_af_stat		af_stat[96];
//	struct cfa_histogram_stat	histogram_stat;
	u8			reserved[121];
}cfa_aaa_stat_t;

//awb
#define AWB_UNIT_GAIN (1024)
typedef	struct wb_gain_s {
	u32	r_gain;
	u32	g_gain;
	u32	b_gain;
} wb_gain_t;

typedef struct awb_lut_unit_s {
	u16 gr_min;
	u16 gr_max;
	u16 gb_min;
	u16 gb_max;
	s16 y_a_min_slope;
	s16 y_a_min;
	s16 y_a_max_slope;
	s16 y_a_max;
	s16 y_b_min_slope;
	s16 y_b_min;
	s16 y_b_max_slope;
	s16 y_b_max;
	s8  weight;
} awb_lut_unit_t;

typedef struct awb_lut_s {
	u8		lut_no;
	awb_lut_unit_t	awb_lut[20];
} awb_lut_t;

typedef struct awb_lut_idx_s{
	u8 start;
	u8 num;
}awb_lut_idx_t;

typedef struct img_awb_param_s {
	wb_gain_t	menu_gain[12];
	awb_lut_t		wr_table;
	awb_lut_idx_t awb_lut_idx[20];
}img_awb_param_t;

//ae
#define AE_GAIN_PRECISION		(12)
#define AE_GAIN_UNIT			(1<<12)
enum {
	CHG_SHUTTER = 0,
	CHG_GAIN,
	CHG_APERTURE,
	CHG_FACTOR_NUM,
};

typedef struct joint_s {
/*	s16	shutter;		// shall be values in ae_shutter_mode
	s16	gain;		//shall be values in ae_iso_mode
	s16	aperture;	//shall be values in ae_aperture_mode
*/
	s32	factor[3];	//0-shutter, 1-gain, 2-iris
}joint_t;

typedef struct line_s {
	joint_t	start;
	joint_t	end;
}line_t;

typedef struct aaa_config_info_s
{
	u32* p_ae_gain_table;
	line_t* p_ae_lines;
	img_awb_param_t* p_awb_param;
	u32 max_agc_index;
}aaa_config_info_t;

typedef struct aaa_output_s{
	u32 shutter_row;
	u32 agc_index;
	u32 dgain;
	u32 ae_output_update;
	wb_gain_t wb_gain;
	u32 awb_output_update;
}aaa_output_t;

typedef struct aaa_fb_aeb_s {
	/* AWB */
	unsigned int	r_gain;
	unsigned int	b_gain;
	/* AE */
	unsigned int	d_gain;
	unsigned int	shutter;
	unsigned int	agc;
}aaa_fb_aeb_t;

int fb_aaa_algo(u8* p_cfa_stats_addr_a,
				u8* p_cfa_stats_addr_b,
				u8* p_cfa_stats_addr_c,
				u8* p_lisocfg,aaa_config_info_t* p_aaa_cfg,aaa_output_t* p_aaa_output);
void fb_aaa_set_log_level(u8 log_level);
void fb_aaa_get_version(u32 version);

/* defined in amboot/src/bld/dsp/<arch>/dsp_aaa.c */
extern int clamp(int x, int min, int max);
extern void convert_dot_12_2_hardware_gain_A7(int mul_dot_12,
	int *gain, int *shift);

/* defined in boards/bsp/iav/sensor_xxx_aaa.c */
#if (CHIP_REV == S5L)
extern void dsp_issue_liso_cfg_batch_cmd(unsigned int seq_num, u8 is_first);
#else
extern void dsp_issue_liso_cfg_cmd(unsigned int seq_num);
#endif
extern void dsp_issue_force_idr_cmd(unsigned int seq_num, u32 pts);
extern void init_iso_cfg_addr(u32 *p_iso_cfg_addr);
extern void init_sensor_aaa_param(aaa_config_info_t *p_aaa_cfg_info);
extern void fb_sensor_ctrl(aaa_output_t* p_aaa_output);
extern u8 get_force_idr_cnt(void);
#endif	// __FB_AAA_CTRL_H__

