/**
 * ambhw/memory.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __AMBHW__MEMORY_H__
#define __AMBHW__MEMORY_H__

#include <ambhw/chip.h>

/*===========================================================================*/
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L) || \
	(CHIP_REV == S5) || (CHIP_REV == S5L)
#define DRAM_START_ADDR		0x00000000
#define AHB_BASE		0xE0000000
#define APB_BASE		0xE8000000
#define DBGBUS_BASE		0xEC000000
#define AXI_BASE		0xF0000000
#else
#define DRAM_START_ADDR		0x00000000
#define AHB_N_BASE		0xE0000000
#define APB_N_BASE		0xE4000000
#define AHB_S_BASE		0xE8000000
#define APB_S_BASE		0xEC000000
#define DBGBUS_BASE		0xED000000
#define AXI_BASE		0xF2000000
#endif

#ifndef AHB_N_BASE
#define AHB_N_BASE		AHB_BASE
#endif
#ifndef AHB_S_BASE
#define AHB_S_BASE		AHB_BASE
#endif
#ifndef APB_N_BASE
#define APB_N_BASE		APB_BASE
#endif
#ifndef APB_S_BASE
#define APB_S_BASE		APB_BASE
#endif


/*===========================================================================*/
#ifndef DRAM_SIZE
#error "DRAM_SIZE undefined!"
#endif

#if IDSP_RAM_START > DRAM_SIZE
#error "IDSP_RAM_START overflow DRAM_SIZE!"
#endif

#define SIZE_1KB		(1 * 1024)
#define SIZE_1KB_MASK		(SIZE_1KB - 1)
#define SIZE_1MB		(1024 * 1024)
#define SIZE_1MB_MASK		(SIZE_1MB - 1)

#define DRAM_END_ADDR		(DRAM_START_ADDR + DRAM_SIZE - 1)

#define AMBOOT_BLD_RAM_MAX_SIZE	(0x00100000)
#define AMBOOT_BLD_RAM_MAX_END	(AMBOOT_BLD_RAM_START + AMBOOT_BLD_RAM_MAX_SIZE)

#define MEMFWPROG_RAM_START	(DRAM_START_ADDR + (4 * SIZE_1MB))
#define MEMFWPROG_HOOKCMD_SIZE	(0x00010000)

#define AMBOOT_DTB_MAX_SIZE	(0x00008000)
/* ptb buffer is used to store the DTB and PTB, and must be multiple of 2048 */
#define AMBOOT_PTB_BUF_SIZE	(AMBOOT_DTB_MAX_SIZE + 0x00002000)

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

extern unsigned char bld_buf_addr[];
extern unsigned char bld_buf_end[];
extern unsigned char splash_buf_addr[];
extern unsigned char splash_buf_end[];

extern unsigned char bld_hugebuf_addr[];

extern unsigned char __heap_start[];
extern unsigned char __heap_end[];

extern unsigned int ambausb_boot_from[];
extern unsigned int ambausb_boot_usb[];

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

