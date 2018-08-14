/**
 * dsp.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __IAV_DSP_H__
#define __IAV_DSP_H__

#include <dsp/dspfw.h>

#if (CHIP_REV == S2L)
#include "s2l_cmd_msg.h"
#include "s2l_mem.h"
#define CODE_VCAP_IRQ (CODE_VDSP_0_IRQ)
#elif (CHIP_REV == S3L)
#include "s3l_cmd_msg.h"
#include "s3l_mem.h"
#define CODE_VCAP_IRQ (CODE_VDSP_1_IRQ)
#elif (CHIP_REV == S5L)
#include "s5l_cmd_msg.h"
#include "s5l_mem.h"
#define CODE_VCAP_IRQ (CODE_VDSP_1_IRQ)
#else
/* S2E still need this file, even if no fast boot feature now. */
#endif

#if defined(CONFIG_PANDORA_RTOS)
#include <pandora.h>
typedef struct dsp_context_s {
	completion_t dsp_comp;
	completion_t vcap_comp;
	unsigned int dsp_state;
} dsp_context_t;
#endif

/* Interface type sync from kernel/private/include/vin_sensors.h */
typedef enum {
	SENSOR_PARALLEL_LVCMOS = 0,
	SENSOR_SERIAL_LVDS = 1,
	SENSOR_PARALLEL_LVDS = 2,
	SENSOR_MIPI = 3,
} SENSOR_IF;

/* DSP status sync from include/iav_ioctl.h */
enum iav_state {
	IAV_STATE_IDLE = 0,
	IAV_STATE_PREVIEW = 1,
	IAV_STATE_ENCODING = 2,
	IAV_STATE_STILL_CAPTURE = 3,
	IAV_STATE_DECODING = 4,
	IAV_STATE_TRANSCODING = 5,
	IAV_STATE_DUPLEX = 6,
	IAV_STATE_EXITING_PREVIEW = 7,
	IAV_STATE_INIT = 0xFF,
};

struct ucode_name_addr
{
	char name[UCODE_FILE_NAME_SIZE];
	u32 addr; // Physical memory address
};

/* ==========================================================================*/
/* Used for dump dsp cmd with Hex format */
#define AMBOOT_IAV_DEBUG_DSP_CMD
#undef AMBOOT_IAV_DEBUG_DSP_CMD

#ifdef AMBOOT_IAV_DEBUG_DSP_CMD
#define dsp_print_cmd(cmd, cmd_size) 	\
	do { 					\
		int _i_; 				\
		int _j_;				\
		u32 *pcmd_raw = (u32 *)cmd; \
		putstr(#cmd);		\
		putstr("[");			\
		putdec(cmd_size);	\
		putstr("] = {\r\n");	\
		for (_i_ = 0, _j_= 1 ; _i_ < ((cmd_size + 3) >> 2); _i_++, _j_++) {	\
			putstr("0x");			\
			puthex(pcmd_raw[_i_]);\
			putstr(", ");			\
			if (_j_== 4) {			\
				putstr("\r\n");	\
				_j_ = 0;		\
			}				\
		} 					\
		putstr("\r\n}\r\n");	\
	} while (0)
#else
#define dsp_print_cmd(cmd, cmd_size)
#endif

#define AMBOOT_IAV_STR_DEBUG
#undef AMBOOT_IAV_STR_DEBUG

#ifdef AMBOOT_IAV_STR_DEBUG
#define putstr_debug(msg)	\
	do {						\
		putstr(msg);		\
		putstr("\r\n");		\
	} while (0)
#else
#define putstr_debug(debug_msg)
#endif

/* ==========================================================================*/
extern void vin_phy_init(int interface_type);
extern void vin_phy_init_pre(int interface_type);
extern void vin_phy_init_post(int interface_type);

extern int dsp_init_pre(flpart_table_t *pptb);
extern int dsp_init_post(flpart_table_t *pptb);
extern int dsp_boot(void);
extern int dsp_boot_pre(void);
extern int dsp_boot_post(void);
extern int add_dsp_cmd(void *cmd, unsigned int size);
extern void dsp_halt(void);
#if (CHIP_REV == S3L) || (CHIP_REV == S5L)
extern int add_dsp_cmd_port(void *cmd, unsigned int size,
	DSP_CMD_PORT_PARAM port, unsigned int seq_num);
extern void flush_cache_dsp_msg_port(DSP_CMD_PORT_PARAM port);
extern void clean_cache_dsp_cmd_port(DSP_CMD_PORT_PARAM port);
extern void clear_dsp_cmd_port_num(DSP_CMD_PORT_PARAM port);
#elif (CHIP_REV == S2L)
extern int add_dsp_cmd_seq(void *cmd, unsigned int size,
	unsigned int seq_num);
extern void flush_cache_dsp_msg(void);
extern void clean_cache_dsp_cmd(void);
#endif

extern void audio_set_play_size(unsigned int addr, unsigned int size);
extern void audio_init(void);
extern void audio_start(void);

extern int dsp_get_bin_by_name(struct dspfw_header *hdr, const char *name,
		unsigned int *addr, unsigned int *size);
extern int dsp_get_ucode_mem_by_name(const struct ucode_name_addr *map,
	int num, const struct ucode_file_info *ucode, unsigned int *mem);

#if defined(CONFIG_PANDORA_RTOS)
extern void dsp_pandora_aaa(void);
extern void wait_dsp_vcap(void);
extern void set_dsp_state(unsigned int dsp_state);
#endif

#endif
