/**
 * bld/dsp/dsp.c
 *
 * History:
 *    2014/08/04 - [Cao Rongrong] created file
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
#include <bldfunc.h>
#include <ambhw/rct.h>
#include <dsp/dsp.h>

#if (CHIP_REV == S2L)
#include "s2l/dsp_arch.c"
#elif (CHIP_REV == S3L)
#include "s3l/dsp_arch.c"
#elif (CHIP_REV == S5L)
#include "s5l/dsp_arch.c"
#else
#error "Not implemented yet"
#endif

#if defined(CONFIG_PANDORA_RTOS)
#include "dsp_pandora.c"
#endif

int dsp_get_bin_by_name(struct dspfw_header *hdr, const char *name,
		unsigned int *addr, unsigned int *size)
{
	int i;

	for (i = 0; i < hdr->total_bin; i++) {
		if (strcmp(name, hdr->bin[i].name))
			continue;

		if (addr)
			*addr = hdr->bin[i].offset;
		if (size)
			*size = hdr->bin[i].size;
		return 0;
	}

	return -1;
}

int dsp_get_ucode_mem_by_name(const struct ucode_name_addr *map,
	int num, const struct ucode_file_info *ucode, unsigned int *mem)
{
	int i = 0;

	for (i = 0; i < num; i++) {
		if (!strcmp(ucode->name, map[i].name)) {
			if (ucode->post) {
				*mem = (map[i].addr + ucode->bin_split_offset);
			} else {
				*mem = map[i].addr;
			}
			return 0;
		}
	}

	putstrstr("Unknown ucode name: ", ucode->name);
	return -1;
}

static int dsp_get_logo_by_id(struct dspfw_header *hdr, int id,
		unsigned int *addr, unsigned int *size)
{
	if (id >= hdr->total_logo) {
		if (hdr->total_logo > 0)
			putstrdec("dsp_get_logo_by_id: Invalid id! ", hdr->total_logo);
		return -1;
	}

	if (hdr->logo[id].offset == 0 || hdr->logo[id].size == 0) {
		putstr("dsp_get_logo_by_id: no logo file\r\n");
		return -1;
	}

	if (hdr->logo[id].size > SIZE_1MB) {
		putstr("dsp_get_logo_by_id: the logo size if too large\r\n");
		return -1;
	}

	if (addr)
		*addr = hdr->logo[id].offset;
	if (size)
		*size = hdr->logo[id].size;

	return 0;
}

static int dsp_load_splash_logo(flpart_table_t *pptb, struct dspfw_header *hdr)
{
	int rval = -1;
	u32 offset = 0, size = 0;

	rval = dsp_get_logo_by_id(hdr, pptb->dev.splash_id, &offset, &size);
	if (rval >= 0) {
		memcpy(splash_buf_addr, &hdr->logo[pptb->dev.splash_id],
				sizeof(struct splash_file_info));
		bld_loader_load_partition_partial(PART_DSP, offset, size,
			(uintptr_t)(splash_buf_addr + sizeof(struct splash_file_info)), 0);
	}

	return rval;
}

static int dsp_load_audio(struct dspfw_header *hdr)
{
	int rval = -1;
	u32 offset = 0, size = 0;
	u8 *audio_play;

	rval = dsp_get_bin_by_name(hdr, "start.bin", &offset, &size);
	if (rval >= 0) {
		size = min(size, AUDIO_PLAY_MAX_SIZE);
		audio_play = malloc(size);
		if (audio_play == NULL) {
			putstr("audio_play malloc failed!\r\n");
			return -1;
		}
		bld_loader_load_partition_partial(PART_DSP, offset, size, (uintptr_t) audio_play, 0);
		audio_set_play_size((uintptr_t)audio_play, size);
	}

	return rval;
}

int dsp_init_pre(flpart_table_t *pptb)
{
	struct dspfw_header *hdr = NULL;
	int rval = -1;

	/* sanity check */
	if (IDSP_RAM_START + DSP_BSB_SIZE + DSP_IAVRSVD_SIZE
		+ DSP_FASTAUDIO_SIZE
		+ DSP_LOG_SIZE + DSP_UCODE_SIZE > DRAM_SIZE
		|| DSP_BUFFER_SIZE < 64 * 1024 * 1024) {
		putstr("dsp_init: Invalid buffer size for DSP!\r\n");
		return -1;
	}

	rval = bld_loader_load_partition(PART_DSP, pptb, sizeof(struct dspfw_header), 0);
	if (rval < 0) {
		putstr("DSP: Load(PART_DSP) fail!\r\n");
		return rval;
	}

	hdr = (struct dspfw_header *)(unsigned long)pptb->part[PART_DSP].mem_addr;
	if (hdr->magic != DSPFW_IMG_MAGIC
		|| hdr->size != pptb->part[PART_DSP].img_len) {
		putstr("DSP: Invalid(PART_DSP)!\r\n");
		return -1;
	}

	rval = dsp_ucode_pre_load(hdr);
	rval = dsp_load_splash_logo(pptb, hdr);
	rval = dsp_load_audio(hdr);
	rval = dsp_init_data();

	return rval;
}

int dsp_init_post(flpart_table_t *pptb)
{
	struct dspfw_header *hdr = NULL;
	int rval = -1;

	hdr = (struct dspfw_header *)(unsigned long)pptb->part[PART_DSP].mem_addr;
	if (hdr->magic != DSPFW_IMG_MAGIC
		|| hdr->size != pptb->part[PART_DSP].img_len) {
		putstr("DSP: Invalid(PART_DSP)!\r\n");
		return -1;
	}

	rval = dsp_ucode_post_load(hdr);
	if (rval < 0) {
		putstr("Load ucode me failed\r\n");
	}

	return rval;
}

