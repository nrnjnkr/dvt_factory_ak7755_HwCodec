/*******************************************************************************
 * unit_test/private/idsp_test/arch/test_yuv_af.c
 *
 * History:
 *    2017/04/24 - [Peter Jiao] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>

#include "basetypes.h"
#include "iav_ioctl.h"
#include "ambas_imgproc_arch.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "img_customer_interface_arch.h"
#include <pthread.h>
#include "idsp_netlink.h"


#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
static pthread_t nl_thread;
static int fd_iav =-1;
static amba_img_dsp_mode_cfg_t ik_mode;
static amba_dsp_aaa_statistic_data_t g_stat;
static img_aaa_stat_t aaa_stat_info[MAX_HDR_EXPOSURE_NUM];

static struct rgb_aaa_stat rgb_stat[4];
static struct cfa_aaa_stat cfa_stat[4];
static struct cfa_pre_hdr_stat hdr_cfa_pre_stat[MAX_PRE_HDR_STAT_BLK_NUM];
static u8 sensor_hist_stat[SENSOR_HIST_DATA_BLOCK];
static u8 rgb_stat_valid;
static u8 cfa_stat_valid;
static u8 hdr_cfa_pre_valid;
static u8 sensor_hist_valid;

static aaa_tile_report_t act_tile;
amba_img_dsp_ae_stat_info_t ae_tile_config;
amba_img_dsp_awb_stat_info_t awb_tile_config;
amba_img_dsp_af_stat_info_t af_tile_config;
static amba_img_dsp_hdr_stat_info_t hdr_stat_config;
static amba_img_dsp_hist_stat_info_t hist_stat_config;

static pthread_t id;
static img_config_info_t yuv_work_info;
static raw_offset_config_t cus_offset_cfg;
static hdr_proc_data_t cus_hdr_proc_pipe;
static u8 exit_flag =0;

static statistics_config_t yuv_tile_config ={
                1,
                1,

                32,
                32,
                0,
                0,
                128,
                128,
                128,
                128,
                0,
                0x3fff,

                12,
                8,
                0,
                0,
                340,
                512,

                12,
                8,
                128,
                8,
                320,
                510,
                320,
                510,

                0,
                16383,
};

static struct aaa_statistics_ex af_eng_cof = {
	0,					// af_horizontal_filter1_mode;
	0,					// af_horizontal_filter1_stage1_enb;
	1,					// af_horizontal_filter1_stage2_enb;
	0,					// af_horizontal_filter1_stage3_enb;
	{200, 0, 0, 0, -55, 0, 0},		// af_horizontal_filter1_gain[7];
	{6, 0, 0, 0},		// af_horizontal_filter1_shift[4];
	0,					// af_horizontal_filter1_bias_off;
	0,					// af_horizontal_filter1_thresh;
	0,					// af_vertical_filter1_thresh;
	8,					// af_tile_fv1_horizontal_shift;
	8,					// af_tile_fv1_vertical_shift;
	168,				// af_tile_fv1_horizontal_weight;
	87,					// af_tile_fv1_vertical_weight;
	0,					// af_horizontal_filter2_mode;
	1,					// af_horizontal_filter2_stage1_enb;
	1,					// af_horizontal_filter2_stage2_enb;
	1,					// af_horizontal_filter2_stage3_enb;
	{188, 467, -235, 375, -184, 276, -206},		// af_horizontal_filter2_gain[7];
	{7, 2, 2, 0},		// af_horizontal_filter2_shift[4];
	0,					// af_horizontal_filter2_bias_off;
	0,					// af_horizontal_filter2_thresh;
	0,					// af_vertical_filter2_thresh;
	8,					// af_tile_fv2_horizontal_shift;
	8,					// af_tile_fv2_vertical_shift;
	123,				// af_tile_fv2_horizontal_weight;
	132					// af_tile_fv2_vertical_weight;
};


void yuv_af_loop(void* arg)
{
	int fd_iav = (int)arg;
	int i = 0, tile_cnt = 0;
	hdr_pipeline_t hdr_pipeline = yuv_work_info.hdr_config.pipeline;
	isp_pipeline_t isp_pipeline = yuv_work_info.isp_pipeline;

	usleep(200000);
	while(!exit_flag){
		if(amba_img_dsp_3a_get_aaa_stat(fd_iav,&ik_mode,&g_stat)<0){
			printf("amba_img_dsp_3a_get_aaa_stat fail\n");
			continue;
		}
		if(parse_aaa_data(&g_stat, hdr_pipeline, aaa_stat_info, &act_tile)<0){
			printf("parse_aaa_data fail\n");
			continue;
		}
#if 1
		printf("\nYUV_INPUT_AF:");
		tile_cnt = aaa_stat_info[0].rgb_tile_info.af_tile_num_col * aaa_stat_info[0].rgb_tile_info.af_tile_num_row;
		printf("\nFV1:");
		for(i=0; i<tile_cnt; i++) {
			if(i % aaa_stat_info[0].rgb_tile_info.af_tile_num_col == 0)
				printf("\n");
			printf("%d,", aaa_stat_info[0].rgb_af_info[i].sum_fv1);
		}
		printf("\nFV2:");
		for(i=0; i<tile_cnt; i++) {
			if(i % aaa_stat_info[0].rgb_tile_info.af_tile_num_col == 0)
				printf("\n");
			printf("%d,", aaa_stat_info[0].rgb_af_info[i].sum_fv2);
		}
		printf("\nFY:");
		for(i=0; i<tile_cnt; i++) {
			if(i % aaa_stat_info[0].rgb_tile_info.af_tile_num_col == 0)
				printf("\n");
			printf("%d,", aaa_stat_info[0].rgb_af_info[i].sum_fy);
		}
		printf("\n");
		sleep(1);
#endif
		if((isp_pipeline== ISP_PIPELINE_B_LISO ||isp_pipeline ==ISP_PIPELINE_ADV_LISO)) {
			amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
			ik_mode.ConfigId++;
			ik_mode.ConfigId%=4;
			amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
		}
	}
}

void config_stat_tiles(statistics_config_t* tile_cfg)
{
	hdr_pipeline_t hdr_pipeline = yuv_work_info.hdr_config.pipeline;
	u8 expo_num = yuv_work_info.hdr_config.expo_num;
	int i = 0;

	ae_tile_config.AeTileNumCol   = tile_cfg->ae_tile_num_col;
	ae_tile_config.AeTileNumRow   = tile_cfg->ae_tile_num_row;
	ae_tile_config.AeTileColStart = tile_cfg->ae_tile_col_start;
	ae_tile_config.AeTileRowStart = tile_cfg->ae_tile_row_start;
	ae_tile_config.AeTileWidth    = tile_cfg->ae_tile_width;
	ae_tile_config.AeTileHeight   = tile_cfg->ae_tile_height;
	ae_tile_config.AePixMinValue = tile_cfg->ae_pix_min_value;
	ae_tile_config.AePixMaxValue = tile_cfg->ae_pix_max_value;

	awb_tile_config.AwbTileNumCol = tile_cfg->awb_tile_num_col;
	awb_tile_config.AwbTileNumRow = tile_cfg->awb_tile_num_row;
	awb_tile_config.AwbTileColStart = tile_cfg->awb_tile_col_start;
	awb_tile_config.AwbTileRowStart = tile_cfg->awb_tile_row_start;
	awb_tile_config.AwbTileWidth = tile_cfg->awb_tile_width;
	awb_tile_config.AwbTileHeight = tile_cfg->awb_tile_height;
	awb_tile_config.AwbTileActiveWidth = tile_cfg->awb_tile_active_width;
	awb_tile_config.AwbTileActiveHeight = tile_cfg->awb_tile_active_height;
	awb_tile_config.AwbPixMinValue = tile_cfg->awb_pix_min_value;
	awb_tile_config.AwbPixMaxValue = tile_cfg->awb_pix_max_value;

	af_tile_config.AfTileNumCol = tile_cfg->af_tile_num_col;
	af_tile_config.AfTileNumRow = tile_cfg->af_tile_num_row;
	af_tile_config.AfTileColStart = tile_cfg->af_tile_col_start;
	af_tile_config.AfTileRowStart = tile_cfg->af_tile_row_start;
	af_tile_config.AfTileWidth = tile_cfg->af_tile_width;
	af_tile_config.AfTileHeight = tile_cfg->af_tile_height;
	af_tile_config.AfTileActiveWidth = tile_cfg->af_tile_active_width;
	af_tile_config.AfTileActiveHeight = tile_cfg->af_tile_active_height;
	config_parser_stat_tiles(tile_cfg);

	if(hdr_pipeline != HDR_PIPELINE_OFF){
		hdr_stat_config.VinStatsMainOn =1;
		hdr_stat_config.VinStatsHdrOn = 1;
		hdr_stat_config.TotalExposures = expo_num;
		hdr_stat_config.TotalSliceInX = 1;
	}

	hist_stat_config.HistMode = 2;
	for (i = 0; i < 8; ++i) {
		hist_stat_config.TileMask[i] = 0xFFF;
	}
}

static int start_aaa_init(int fd_iav)
{
	#define	PIXEL_IN_MB			(16)
	img_config_info_t* p_img_config = &yuv_work_info;
	struct vindev_video_info video_info;
	struct iav_enc_mode_cap mode_cap;
	struct iav_system_resource system_resource;
	struct iav_srcbuf_setup	srcbuf_setup;
	struct vindev_aaa_info vin_aaa_info;
	amba_img_dsp_variable_range_t dsp_variable_range;

	// video info
	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return 0;
	}
	p_img_config->raw_width = ROUND_UP(video_info.info.width, PIXEL_IN_MB);
	p_img_config->raw_height = ROUND_UP(video_info.info.height, PIXEL_IN_MB);
	p_img_config->raw_resolution =video_info.info.bits;

	// encode mode capability
	memset(&mode_cap, 0, sizeof(mode_cap));
	mode_cap.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_QUERY_ENC_MODE_CAP, &mode_cap)) {
		perror("IAV_IOC_QUERY_ENC_MODE_CAP");
		return -1;
	}

	// system resource
	memset(&system_resource, 0, sizeof(system_resource));
	system_resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &system_resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE\n");
		return -1;
	}
	p_img_config->hdr_config.expo_num = system_resource.exposure_num;
	p_img_config->hdr_config.pipeline = system_resource.hdr_type;
	p_img_config->isp_pipeline = system_resource.iso_type;
	p_img_config->raw_pitch =system_resource.raw_pitch_in_bytes;

	// source buffer setup
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup) < 0) {
			printf("IAV_IOC_GET_SOURCE_BUFFER_SETUP error\n");
			return -1;
	}
	p_img_config->main_width = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].width, PIXEL_IN_MB);
	p_img_config->main_height = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].height, PIXEL_IN_MB);

	// vin aaa info
	vin_aaa_info.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_AAAINFO, &vin_aaa_info) < 0) {
		perror("IAV_IOC_VIN_GET_AAAINFO error\n");
		return -1;
	}

	p_img_config->raw_bayer = vin_aaa_info.bayer_pattern;
	if(vin_aaa_info.dual_gain_mode){
		p_img_config->hdr_config.method = HDR_DUAL_GAIN_METHOD;
	}else{
		switch (vin_aaa_info.hdr_mode){
			case AMBA_VIDEO_LINEAR_MODE:
				p_img_config->hdr_config.method = HDR_NONE_METHOD;
				break;
			case AMBA_VIDEO_2X_HDR_MODE:
			case AMBA_VIDEO_3X_HDR_MODE:
			case AMBA_VIDEO_4X_HDR_MODE:
				break;
			case AMBA_VIDEO_INT_HDR_MODE:
				p_img_config->hdr_config.method = HDR_BUILD_IN_METHOD;
				break;
			default:
				printf("error: invalid vin HDR sensor info.\n");
				return -1;
		}
	}

	// ik mode configuration
	memset(&ik_mode, 0, sizeof(ik_mode));
	ik_mode.ConfigId = 0;
	ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	if (p_img_config->hdr_config.pipeline !=HDR_PIPELINE_OFF) {
		ik_mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_HDR;
	}
	if(p_img_config->isp_pipeline  ==ISP_PIPELINE_B_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		ik_mode.BatchId = 0xff;
		ik_mode.Reserved1 = 0xAA;
	}
	else if(p_img_config->isp_pipeline  ==ISP_PIPELINE_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
	}
	else if(p_img_config->isp_pipeline  ==ISP_PIPELINE_ADV_LISO||
		p_img_config->isp_pipeline  ==ISP_PIPELINE_MID_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		ik_mode.BatchId = 0xff;
		ik_mode.Reserved1 = 0xAA;
	}
	dsp_variable_range.max_chroma_radius = (1 << (5 + mode_cap.max_chroma_radius));
	dsp_variable_range.max_wide_chroma_radius = (1 << (5 + mode_cap.max_wide_chroma_radius));
	dsp_variable_range.inside_fpn_flag = 0;
	dsp_variable_range.wide_chroma_noise_filter_enable = mode_cap.wcr_possible;
	amba_img_dsp_set_variable_range(&ik_mode, &dsp_variable_range);

	// init functions
	adj_config_work_info(p_img_config);		//for img_runtime_adj

	config_stat_tiles(&yuv_tile_config);
	amba_img_dsp_set_af_statistics_ex(fd_iav, &ik_mode, &af_eng_cof, 1);

	hdr_proc_init(p_img_config, &cus_offset_cfg, &cus_hdr_proc_pipe);

	return 0;
}

int check_iav_work(void)
{
	u32 state;
	memset(&state, 0, sizeof(state));
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}
	if (state == IAV_STATE_PREVIEW || state == IAV_STATE_ENCODING) {
		return 1;
	}

	return 0;
}

void wait_irq_count(int num)
{
	do {
		ioctl(fd_iav, IAV_IOC_WAIT_NEXT_FRAME, 0);
	} while (--num);
}

static int do_init_netlink(void)
{
	init_netlink();
	pthread_create(&nl_thread, NULL, (void *)netlink_loop, (void *)NULL);

	while (1) {
		if (check_iav_work() > 0) {
			break;
		}
		usleep(10000);
	}

	return 0;
}
int do_start_aaa(void)
{
	hdr_pipeline_t hdr_pipeline = yuv_work_info.hdr_config.pipeline;
	isp_pipeline_t isp_pipeline = yuv_work_info.isp_pipeline;
	hdr_method_t	 hdr_method =yuv_work_info.hdr_config.method;
	//prepare for get 3A statistics
	memset(rgb_stat, 0, sizeof(struct rgb_aaa_stat)*4);
	memset(cfa_stat, 0, sizeof(struct cfa_aaa_stat)*4);
	memset(hdr_cfa_pre_stat, 0, sizeof(struct cfa_pre_hdr_stat) * MAX_PRE_HDR_STAT_BLK_NUM);
	memset(sensor_hist_stat, 0, SENSOR_HIST_DATA_BLOCK);
	rgb_stat_valid = 0;
	cfa_stat_valid = 0;
	hdr_cfa_pre_valid = 0;
	sensor_hist_valid =0;

	memset(&g_stat, 0, sizeof(amba_dsp_aaa_statistic_data_t));
	g_stat.CfaAaaDataAddr = (u32)cfa_stat;
	g_stat.RgbAaaDataAddr = (u32)rgb_stat;
	g_stat.CfaPreHdrDataAddr = (u32)hdr_cfa_pre_stat;
	g_stat.SensorDataAddr = (hdr_method == HDR_BUILD_IN_METHOD)? (u32)sensor_hist_stat : 0;

	g_stat.CfaDataValid = (u32)&cfa_stat_valid;
	g_stat.RgbDataValid = (u32)&rgb_stat_valid;
	g_stat.CfaPreHdrDataValid = (u32)&hdr_cfa_pre_valid;
	g_stat.SensorDataValid = (hdr_method == HDR_BUILD_IN_METHOD)? (u32)&sensor_hist_valid : 0;
	//end prepare
	// the operations within brace must be done after entering preview
	amba_img_dsp_3a_config_aaa_stat(fd_iav, 1, &ik_mode, &ae_tile_config, &awb_tile_config, &af_tile_config);
	amba_img_dsp_3a_config_histogram( fd_iav, &ik_mode, &hist_stat_config);
	if(hdr_pipeline != HDR_PIPELINE_OFF){
		amba_img_dsp_3a_config_hdr_stat(fd_iav, &ik_mode, &hdr_stat_config);
	}

	if(isp_pipeline == ISP_PIPELINE_B_LISO||
	isp_pipeline ==ISP_PIPELINE_ADV_LISO||
	isp_pipeline ==ISP_PIPELINE_MID_LISO){
		amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
		ik_mode.ConfigId ^= 1;
		amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
	}else{
		img_adj_init_misc(fd_iav, &ik_mode,&yuv_work_info);
		img_adj_retrieve_filters(fd_iav, &ik_mode);
	}
	exit_flag = 0;
	id = 0;
	pthread_create(&id, NULL, (void*)yuv_af_loop, (void*)fd_iav);

	printf("YUV_Input_AF Start\n");

	return 0;
}

int do_prepare_aaa(void)
{
	isp_pipeline_t isp_pipeline = yuv_work_info.isp_pipeline;
	img_lib_init(fd_iav,0,0);
	start_aaa_init(fd_iav);
	ik_mode.ConfigId = 0;
	img_adj_reset_filters();

	printf("YUV_Input_AF Prepare\n");

	if(isp_pipeline ==ISP_PIPELINE_LISO){
		return 0;
	}

	img_adj_init_misc(fd_iav, &ik_mode,&yuv_work_info);
	img_adj_retrieve_filters(fd_iav, &ik_mode);
	if(isp_pipeline == ISP_PIPELINE_B_LISO||
		isp_pipeline ==ISP_PIPELINE_ADV_LISO||
		isp_pipeline ==ISP_PIPELINE_MID_LISO) {
		if(amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 1)<0)
			return -1;
		ik_mode.ConfigId ^= 1;
		if(amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0)<0)
			return -1;
		ik_mode.Reserved1 = 0x0;
	}

	return 0;
}

int do_stop_aaa(void)
{
	if (id != 0) {
		exit_flag =1;
		pthread_join(id, NULL);
	}

	if (img_lib_deinit() < 0) {
		printf("img_lib_deinit error!\n");
		return -1;
	}

	printf("YUV_Input_AF Stop\n");

	return 0;
}

int main(int argc, char **argv)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (do_init_netlink() < 0) {
		return -1;
	}

	sem_t sem;
	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	return 0;
}

