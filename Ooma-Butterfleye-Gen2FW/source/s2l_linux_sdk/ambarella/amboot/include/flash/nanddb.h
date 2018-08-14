/**
 * system/include/flash/slcnanddb.h
 *
 * History:
 *    2007/10/03 - [Charles Chiou] created file
 *    2007/12/18 - [Charles Chiou] changed the header to be compatible with
 *			AMBoot
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

#ifndef __FLASH__SLCNANDDB_H__
#define __FLASH__SLCNANDDB_H__

#include <basedef.h>
#include <ambhw/nand.h>

#ifndef NAND_ID5
#define NAND_ID5	0x00
#endif

#ifndef NAND_TRHW
#define NAND_TRHW	0
#endif

#ifndef NAND_TADL
#define NAND_TADL	0
#endif

#ifndef NAND_TCRL
#define NAND_TCRL	0
#endif

typedef struct nand_db_s {
	const char *name;		/**< Name */
	u32	id;			/**< ID code */
	u32	id5;			/**< ID code from 5th cycle */
	u16	main_size;		/**< Main area size */
	u16	spare_size;		/**< Spare area size */
	u32	pages_per_block;	/**< Pages per block */
	u32	blocks_per_bank;	/**< Blocks per bank */

	/** Chip(s) timing parameters */
	u32	timing0;
	u32	timing1;
	u32	timing2;
	u32	timing3;
	u32	timing4;
	u32	timing5;
	u32	timing6;
} nand_db_t;

#define __NANDDB_SECTION __attribute__((unused, __section__(".nanddb")))

extern nand_db_t __nanddb_start[];
extern nand_db_t __nanddb_end[];

#define IMPLEMENT_NAND_DB_DEV(x)		\
	const nand_db_t x __NANDDB_SECTION = {		\
		NAND_NAME,			\
		NAND_MANID << 24 | NAND_DEVID << 16 | NAND_ID3 << 8 | NAND_ID4, \
		NAND_ID5,			\
		NAND_MAIN_SIZE,			\
		NAND_SPARE_SIZE,		\
		NAND_PAGES_PER_BLOCK,		\
		NAND_BLOCKS_PER_BANK,		\
		/* timing */			\
		NAND_TCLS << 24 | NAND_TALS << 16 | NAND_TCS << 8 | NAND_TDS, \
		NAND_TCLH << 24 | NAND_TALH << 16 | NAND_TCH << 8 | NAND_TDH, \
		NAND_TWP << 24 | NAND_TWH << 16 | NAND_TWB << 8 | NAND_TRR, \
		NAND_TRP << 24 | NAND_TREH << 16 | NAND_TRB << 8 | NAND_TCEH, \
		(NAND_TRP + NAND_TREH) << 24 | NAND_TCLR << 16 | NAND_TWHR << 8 | NAND_TIR, \
		NAND_TWW << 16 | NAND_TRHZ << 8 | NAND_TAR, \
		NAND_TRHW << 16 | NAND_TADL << 8 | NAND_TCRL, \
	}

/* ==========================================================================*/

#define NAND_BLACKLIST_ID	0xabcdefabcd

#if (NAND_ID5 << 32 | NAND_MANID << 24 | NAND_DEVID << 16 | NAND_ID3 << 8 | NAND_ID4)	\
	== NAND_BLACKLIST_ID
#error "This NAND is in blacklist!"
#endif

#endif

