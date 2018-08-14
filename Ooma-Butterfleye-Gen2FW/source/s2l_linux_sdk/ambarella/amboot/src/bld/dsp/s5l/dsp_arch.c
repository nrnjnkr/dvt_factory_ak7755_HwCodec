/**
 * bld/dsp/s5l/dsp_arch.c
 *
 * Author: Xu Liang <xliang@ambarella.com>
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

#include <ambhw/cache.h>
#include <dsp/dsp.h>
#include <bldfunc.h>

#define S5L_MEMD_OFFSET			(0x150000)
#define S5L_CODE_OFFSET			(0x160000)
#define S5L_MEMD_BASE			(DBGBUS_BASE + S5L_MEMD_OFFSET)
#define S5L_CODE_BASE			(DBGBUS_BASE + S5L_CODE_OFFSET)

#define S5L_DSP_DRAM_MAIN_OFFSET	(0x0008)
#define S5L_DSP_DRAM_SUB0_OFFSET	(0x0008)
#define S5L_DSP_DRAM_SUB1_OFFSET	(0x8008)
#define S5L_DSP_CONFIG_MAIN_OFFSET	(0x0000)
#define S5L_DSP_CONFIG_SUB0_OFFSET	(0x0000)
#define S5L_DSP_CONFIG_SUB1_OFFSET	(0x8000)

#define S5L_DSP_DRAM_MAIN_REG		(S5L_CODE_BASE + S5L_DSP_DRAM_MAIN_OFFSET)
#define S5L_DSP_DRAM_SUB0_REG		(S5L_MEMD_BASE + S5L_DSP_DRAM_SUB0_OFFSET)
#define S5L_DSP_DRAM_SUB1_REG		(S5L_MEMD_BASE + S5L_DSP_DRAM_SUB1_OFFSET)
#define S5L_DSP_CONFIG_MAIN_REG		(S5L_CODE_BASE + S5L_DSP_CONFIG_MAIN_OFFSET)
#define S5L_DSP_CONFIG_SUB0_REG		(S5L_MEMD_BASE + S5L_DSP_CONFIG_SUB0_OFFSET)
#define S5L_DSP_CONFIG_SUB1_REG		(S5L_MEMD_BASE + S5L_DSP_CONFIG_SUB1_OFFSET)

/* DO NOT change ucode name */
struct ucode_name_addr ucode_mem[] = {
	{"orccode.bin", UCODE_ORCCODE_START},
	{"orcme.bin", UCODE_ORCME_START},
	{"default_binary.bin", UCODE_DEFAULT_BINARY_START},
};

/*===========================================================================*/
void vin_phy_init(int interface_type)
{
	switch (interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		writel(RCT_REG(0x578), 0x600800FF);
		writel(RCT_REG(0x594), 0x300FF);
		writel(RCT_REG(0x57c), 0x5);
		putstr_debug("SENSOR_PARALLEL_LVCMOS");
		break;
	case SENSOR_MIPI:
		writel(RCT_REG(0x578), 0x60000000);
		writel(RCT_REG(0x57c), 0x3801);
		rct_timer_dly_ms(5);
		writel(RCT_REG(0x57c), 0x1);
		writel(RCT_REG(0x580), 0x300);
		writel(RCT_REG(0x594), 0x3300FF); //force mipi clk to be HS mode
		writel(RCT_REG(0x5a0), 0x1D300E05);
		writel(RCT_REG(0x5a4), 0x123F);
		putstr_debug("SENSOR_MIPI");
		break;
	case SENSOR_SERIAL_LVDS:
		writel(RCT_REG(0x578), 0x60300000);
		writel(RCT_REG(0x57c), 0x801);
		rct_timer_dly_ms(10);
		writel(RCT_REG(0x57c), 0x1);
		writel(RCT_REG(0x594), 0x0);
		putstr_debug("SENSOR_SERIAL_LVDS");
		break;
	default:
		putstr_debug("Unkown sensor interface type");
		break;
	}
}

void vin_phy_init_pre(int interface_type)
{
	switch (interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		writel(RCT_REG(0x578), 0x600800FF);
		writel(RCT_REG(0x594), 0x300FF);
		writel(RCT_REG(0x57c), 0x5);
		putstr_debug("SENSOR_PARALLEL_LVCMOS");
		break;
	case SENSOR_MIPI:
		writel(RCT_REG(0x578), 0x60000000);
		writel(RCT_REG(0x57c), 0x3801);
		putstr_debug("SENSOR_MIPI");
		break;
	case SENSOR_SERIAL_LVDS:
		writel(RCT_REG(0x578), 0x60300000);
		writel(RCT_REG(0x57c), 0x801);
		putstr_debug("SENSOR_SERIAL_LVDS");
		break;
	default:
		putstr_debug("Unkown sensor interface type");
		break;
	}
}

void vin_phy_init_post(int interface_type)
{
	switch (interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		break;
	case SENSOR_MIPI:
//		/* add other code to generate 5 ms delay */
		writel(RCT_REG(0x57c), 0x1);
		writel(RCT_REG(0x580), 0x300);
		writel(RCT_REG(0x594), 0x3300FF); //force mipi clk to be HS mode
		writel(RCT_REG(0x5a0), 0x1D300E05);
		writel(RCT_REG(0x5a4), 0x123F);
		break;
	case SENSOR_SERIAL_LVDS:
		/* add other code to generate 10 ms delay */
		writel(RCT_REG(0x57c), 0x1);
		writel(RCT_REG(0x594), 0x0);
		break;
	default:
		break;
	}
}

static void show_dram_layout_info(void)
{
#ifdef AMBOOT_IAV_STR_DEBUG
	/* The following info should be exactly same as output by "test_encode --show-dram" */
	printf("[DRAM Layout info]:\n");
	printf("     [BSB] Base Address: [0x%08X], Size [0x%08X]\n", DSP_BSB_START, DSP_BSB_SIZE);
	printf(" [OVERLAY] Base Address: [0x%08X], Size [0x%08X]\n", DSP_OVERLAY_START, DSP_OVERLAY_SIZE);
	printf("    [WARP] Base Address: [0x%08X], Size [0x%08X]\n", DSP_WARP_START, DSP_WARP_TAB_MAX_SIZE);
	printf(" [IMGPROC] Base Address: [0x%08X], Size [0x%08X]\n", DSP_IMGPROC_START, DSP_IMGPROC_SIZE);
	printf(" [IAVRSVD] Base Address: [0x%08X], Size [0x%08X]\n", DSP_IAVRSVD_START, DSP_IAVRSVD_SIZE);
	printf(" [FB_DATA] Base Address: [0x%08X], Size [0x%08X]\n", DSP_FASTDATA_START, DSP_FASTDATA_SIZE);
	printf("[FB_AUDIO] Base Address: [0x%08X], Size [0x%08X]\n", DSP_FASTAUDIO_START, DSP_FASTAUDIO_SIZE);
	printf("     [DSP] Base Address: [0x%08X], Size [0x%08X]\n", DSP_BUFFER_START, DSP_BUFFER_SIZE);
	printf("  [DSPLOG] Base Address: [0x%08X], Size [0x%08X]\n", DSP_LOG_START, DSP_LOG_SIZE);
#endif
}

static int dsp_init_data(void)
{
	dsp_init_data_t *extended_init_data = NULL;
	dsp_init_data_common_t *init_data = NULL;
	vdsp_info_t *vdsp_info = NULL;
	DSP_HEADER_CMD *cmd_hdr = NULL;
	u32 *ucode_init_data_ptr = NULL;

	/* initialize struct dsp_init_data */
	extended_init_data = (dsp_init_data_t *)DSP_INIT_DATA_START;
	memset(extended_init_data, 0, sizeof(dsp_init_data_t));
	init_data = &extended_init_data->common_init_data;

	/* setup default binary data pointer */
	init_data->default_binary_data_addr = UCODE_DEFAULT_BINARY_START;
	init_data->default_binary_data_size = UCODE_DEFAULT_BINARY_SIZE;

	/* setup cmd/msg memory for DSP-ARM */
	init_data->cmd_data_gen_daddr = DSP_CMD_BUF_START;
	init_data->cmd_data_gen_size = DSP_PORT_CMD_SIZE;
	init_data->msg_queue_gen_daddr = DSP_MSG_BUF_START;
	init_data->msg_queue_gen_size = DSP_PORT_MSG_SIZE;

	init_data->cmd_data_vcap_daddr = init_data->cmd_data_gen_daddr +
		init_data->cmd_data_gen_size;
	init_data->cmd_data_vcap_size = DSP_PORT_CMD_SIZE;
	init_data->msg_queue_vcap_daddr = init_data->msg_queue_gen_daddr +
		init_data->msg_queue_gen_size;
	init_data->msg_queue_vcap_size = DSP_PORT_MSG_SIZE;

	memset((void *)(u64)init_data->cmd_data_gen_daddr, 0, init_data->cmd_data_gen_size);
	memset((void *)(u64)init_data->cmd_data_vcap_daddr, 0, init_data->cmd_data_vcap_size);
	memset((void *)(u64)init_data->msg_queue_gen_daddr, 0, init_data->msg_queue_gen_size);
	memset((void *)(u64)init_data->msg_queue_vcap_daddr, 0, init_data->msg_queue_vcap_size);

	init_data->cmd_data_3rd_daddr = 0;
	init_data->cmd_data_3rd_size = 0;
	init_data->msg_queue_3rd_daddr = 0;
	init_data->msg_queue_3rd_size = 0;

	init_data->cmd_data_4th_daddr = 0;
	init_data->cmd_data_4th_size = 0;
	init_data->msg_queue_4th_daddr = 0;
	init_data->msg_queue_4th_size = 0;

	init_data->default_config_daddr = DSP_DEF_CMD_BUF_START;
	init_data->default_config_size = DSP_DEF_CMD_BUF_SIZE;

	/* setup buffer for dsp running */
	init_data->dsp_buffer_daddr = DSP_BUFFER_START;
	init_data->dsp_buffer_size = DSP_BUFFER_SIZE;
	init_data->dsp_log_buf_daddr = DSP_LOG_START;
	init_data->product_id = AMBA_PRODUCT_ID_IPCAM;
	extended_init_data->ipcam_init_data.dsp_log_size = DSP_LOG_SIZE;

	/* misc info */
	init_data->chip_id_daddr = (u32)UCODE_CHIP_ID_START;
	vdsp_info = (vdsp_info_t *)(u64)(UCODE_CHIP_ID_START + sizeof(u32));
	vdsp_info->dsp_cmd_rx = 1;
	vdsp_info->dsp_msg_tx = 1;
	init_data->vdsp_info_daddr = (uintptr_t)vdsp_info;

	/* used for valid bsh data check */
	memset((void *)DSP_BSH_START, 0, DSP_BSH_SIZE);

	/* reset IMGPROC buffer in case wild pointers are used by idsp */
	memset((void *)DSP_FASTBOOT_IDSPCFG_START, 0, DSP_ISOCFG_RSVED_SIZE);

	/* used for sync fastboot data from amboot to linux/iav */
	memset((void *)DSP_FASTDATA_START, DSP_FASTDATA_INVALID, DSP_FASTDATA_SIZE);

	/* Default CMD Queue */
	cmd_hdr = (DSP_HEADER_CMD *)DSP_DEF_CMD_BUF_START;
	cmd_hdr->cmd_code = CMD_DSP_HEADER;
	cmd_hdr->cmd_seq_num = 1;
	cmd_hdr->num_cmds = 0;

	/* Gen Port CMD Queue */
	cmd_hdr = (DSP_HEADER_CMD *)DSP_CMD_BUF_START;
	cmd_hdr->cmd_code = CMD_DSP_HEADER;
	cmd_hdr->cmd_seq_num = 1;
	cmd_hdr->num_cmds = 0;

	/* Vcap Port CMD Queue */
	cmd_hdr = (DSP_HEADER_CMD *)(DSP_CMD_BUF_START + DSP_PORT_CMD_SIZE);
	cmd_hdr->cmd_code = CMD_DSP_HEADER;
	cmd_hdr->cmd_seq_num = 1;
	cmd_hdr->num_cmds = 0;

	ucode_init_data_ptr = (u32 *)(u64)UCODE_DSP_INIT_DATA_PTR;
	*ucode_init_data_ptr = DSP_INIT_DATA_START;

	bopt_sync(init_data, (u32*)UCODE_DEFAULT_BINARY_START, (u32*)UCODE_ORCCODE_START, NULL);
#ifdef AMBOOT_DSP_LOG_CAPTURE
	memset((void *)DSP_LOG_START, 0, DSP_LOG_SIZE);
#endif

	show_dram_layout_info();

	putstr_debug("dsp_init_data done\r\n");
	return 0;
}

int dsp_boot(void)
{
	/* reset analog/digital mipi phy */
	writel(DBGBUS_BASE + 0x11801c, 0x30002);
	// rct_timer_dly_ms(5);
	writel(DBGBUS_BASE + 0x11801c, 0x0);

	_drain_write_buffer();
	_clean_flush_all_cache();

	writel(S5L_DSP_DRAM_MAIN_REG, UCODE_ORCCODE_START);
	writel(S5L_DSP_DRAM_SUB0_REG, UCODE_ORCME_START);

	writel(S5L_DSP_CONFIG_SUB0_REG, 0xF);
	writel(S5L_DSP_CONFIG_MAIN_REG, 0xFF);

	putstr_debug("dsp_boot\r\n");
	return 0;
}

int dsp_boot_pre(void)
{
	/* reset analog/digital mipi phy */
	writel(DBGBUS_BASE + 0x11801c, 0x30002);
	// rct_timer_dly_ms(5);
	writel(DBGBUS_BASE + 0x11801c, 0x0);

	_drain_write_buffer();
	_clean_flush_all_cache();

	writel(S5L_DSP_DRAM_MAIN_REG, UCODE_ORCCODE_START);
	writel(S5L_DSP_CONFIG_MAIN_REG, 0xFF);

	putstr_debug("dsp_boot_code\r\n");
	return 0;
}

int dsp_boot_post(void)
{
	_drain_write_buffer();
	_clean_flush_all_cache();

	writel(S5L_DSP_DRAM_SUB0_REG, UCODE_ORCME_START);
	writel(S5L_DSP_CONFIG_SUB0_REG, 0xF);

	putstr_debug("dsp_boot_me\r\n");
	return 0;
}

int dsp_ucode_pre_load(struct dspfw_header *hdr)
{
	int i = 0;
	int rval = 0;
	unsigned int mem = 0;
	struct ucode_file_info *ucode = NULL;

	for (i = 0; i < hdr->total_dsp; i++) {
		ucode = &hdr->ucode[i];
		if (!ucode->post) {
			rval = dsp_get_ucode_mem_by_name(ucode_mem,
				ARRAY_SIZE(ucode_mem), ucode, &mem);
			if (rval >= 0) {
				rval = bld_loader_load_partition_partial(PART_DSP,
					ucode->offset, ucode->size, mem, 0);
			}
		}
	}

	return rval;
}

int dsp_ucode_post_load(struct dspfw_header *hdr)
{
	int i = 0;
	int rval = 0;
	unsigned int mem = 0;
	struct ucode_file_info *ucode = NULL;

	for (i = (hdr->total_dsp - 1); i >= 0; i--) {
		ucode = &hdr->ucode[i];
		if (ucode->post) {
			rval = dsp_get_ucode_mem_by_name(ucode_mem,
				ARRAY_SIZE(ucode_mem), ucode, &mem);
			if (rval >= 0) {
				rval = bld_loader_load_partition_partial(PART_DSP,
					ucode->offset, ucode->size, mem, 0);
			}
		}
	}
	dsp_boot_post();

	return rval;
}

int add_dsp_cmd(void *cmd, unsigned int size)
{
	DSP_HEADER_CMD *cmd_hdr;
	u32 cmd_ptr;

	dsp_print_cmd(cmd, size);

	cmd_hdr = (DSP_HEADER_CMD *)DSP_DEF_CMD_BUF_START;

	cmd_ptr = DSP_DEF_CMD_BUF_START + sizeof(DSP_HEADER_CMD) +
		cmd_hdr->num_cmds * DSP_CMD_SIZE;
	if ((cmd_ptr + DSP_CMD_SIZE) >=
		(DSP_DEF_CMD_BUF_START + DSP_DEF_CMD_BUF_SIZE)) {
		putstr("cmd buffer is too small!!!\n");
		BUG_ON(1);
		return -1;
	}

	memcpy((u8 *)(u64)cmd_ptr, cmd, size);
	memset((u8 *)(u64)(cmd_ptr + size), 0, DSP_CMD_SIZE - size);

	cmd_hdr->num_cmds++;

	return 0;
}

/* The value of port:  0: CMD_PORT_GEN;  1: CMD_PORT_VCAP. */
int add_dsp_cmd_port(void *cmd, unsigned int size, DSP_CMD_PORT_PARAM port,
	unsigned int seq_num)
{
	DSP_HEADER_CMD *cmd_hdr;
	u32 cmd_ptr;
	u32 base;

	if ((port < CMD_PORT_GEN) || (port >= CMD_PORT_TOTAL_NUM)) {
		putstr("Invalid cmd port\r\n");
		return -1;
	}
	dsp_print_cmd(cmd, size);

	base = DSP_CMD_BUF_START + DSP_PORT_CMD_SIZE * port;
	cmd_hdr = (DSP_HEADER_CMD *)(u64)base;

	cmd_ptr = base + sizeof(DSP_HEADER_CMD) +
			cmd_hdr->num_cmds * DSP_CMD_SIZE;
	if ((cmd_ptr + DSP_CMD_SIZE) >=
		(DSP_CMD_BUF_START + DSP_CMD_BUF_SIZE)) {
		putstr("cmd buffer is too small!!!\n");
		BUG_ON(1);
		return -1;
	}

	memcpy((u8 *)(u64)cmd_ptr, cmd, size);
	memset((u8 *)(u64)(cmd_ptr + size), 0, DSP_CMD_SIZE - size);

	cmd_hdr->num_cmds++;
	cmd_hdr->cmd_seq_num = seq_num;

	return 0;
}

void clear_dsp_cmd_port_num(DSP_CMD_PORT_PARAM port)
{
	DSP_HEADER_CMD *cmd_hdr = NULL;

	if ((port < CMD_PORT_GEN) || (port >= CMD_PORT_TOTAL_NUM)) {
		putstr("Clear invalid cmd port\r\n");
		return;
	}
	cmd_hdr = (DSP_HEADER_CMD *)(u64)(DSP_CMD_BUF_START + DSP_PORT_CMD_SIZE * port);
	cmd_hdr->num_cmds = 0;
}

void flush_cache_dsp_chan_fifo(void)
{
	flush_d_cache((void *)(u64)(DSP_CHAN_INFO_FIFO_START), DSP_CHAN_INFO_FIFO_SIZE);
}

/* sync memory to cache before read dsp message */
void flush_cache_dsp_msg_port(DSP_CMD_PORT_PARAM port)
{
	if ((port < CMD_PORT_GEN) || (port >= CMD_PORT_TOTAL_NUM)) {
		putstr("Flush invalid msg port\r\n");
		return;
	}
	flush_d_cache((void *)(u64)(DSP_MSG_BUF_START + DSP_PORT_MSG_SIZE * port),
		DSP_PORT_MSG_SIZE);
}

/* sync cache to memory after issue dsp cmd */
void clean_cache_dsp_cmd_port(DSP_CMD_PORT_PARAM port)
{
	if ((port < CMD_PORT_GEN) || (port >= CMD_PORT_TOTAL_NUM)) {
		putstr("Clean invalid cmd port\r\n");
		return;
	}
	clean_d_cache((void *)(u64)(DSP_CMD_BUF_START + DSP_PORT_CMD_SIZE * port),
		DSP_PORT_CMD_SIZE);
}

void dsp_halt(void)
{
#define DSP_RESET_OFFSET (0x4)
#define DSP_SYNC_START_OFFSET (0x101c00)
#define DSP_SYNC_END_OFFSET (0x101d00)

#define DSP_SYNC_START_REG (DBGBUS_BASE + DSP_SYNC_START_OFFSET)
#define DSP_SYNC_END_REG (DBGBUS_BASE + DSP_SYNC_END_OFFSET)

	u32 addr = 0;

	/* Suspend all code orc threads */
	writel(S5L_CODE_BASE, 0xff00);
	/* Suspend all md orc threads */
	writel(S5L_MEMD_BASE, 0xf0);

	/* Assuming all the orc threads are now waiting to receive on some sync.
	 * counter, debug port writes to all result in wakes to all sync */
	for (addr = DSP_SYNC_START_REG; addr < DSP_SYNC_END_REG; addr += 0x4) {
		writel(addr, 0x1008);
	}
	/* Now, reset sync counters */
	for (addr = DSP_SYNC_START_REG; addr < DSP_SYNC_END_REG; addr += 0x4) {
		writel(addr, 0x0);
	}

	/* Reset code */
	writel((S5L_CODE_BASE + DSP_RESET_OFFSET), 0x1);
	/* Reset me */
	writel((S5L_MEMD_BASE + DSP_RESET_OFFSET), 0x1);
}
