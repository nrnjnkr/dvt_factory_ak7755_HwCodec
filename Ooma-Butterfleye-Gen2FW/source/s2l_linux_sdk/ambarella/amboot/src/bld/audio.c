/**
 * system/src/bld/audio.c
 *
 * History:
 *    2014/11/17 - [Cao Rongrong] creat
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
#include <dsp/dsp.h>
#include <ambhw/dma.h>
#include <ambhw/i2s.h>
#include <ambhw/cache.h>
#include <ambhw/timer.h>

/* 4 desc for record, and 1 desc for playback */
#define DMA_DESC_NUM		4
static struct dma_desc pcm_dma_desc[DMA_DESC_NUM + 1] __attribute__((aligned(32)));

static u32 audio_play_size = 0;
static u32 audio_play_addr = 0;

/* i2s format: 12.288MHz, 48KHz, 16bit&32bit, stereo, master mode */
static void i2s_init(void)
{
	u32 rate, bclk, clk_div, clk_reg;

	rate = AUDIO_SAMPLE_RATE;
#if defined(AMBOOT_AUDIO_16BIT)
	bclk = 2 * 16 * rate;
#elif defined(AMBOOT_AUDIO_32BIT)
	bclk = 2 * 32 * rate;
#endif
	clk_div = ((12288000 / ( 2 * bclk)) - 1) & 0x1f;

#if defined(AMBOOT_AUDIO_MODE_SLAVE)
	clk_reg = 0x040 | clk_div;
#else
	clk_reg = 0x3c0 | clk_div;
#endif

	rct_set_audio_pll();		/* 12.288MHz */
	writel(I2S_CLOCK_REG, clk_reg); /* set bclk for i2s */
	setbitsl(I2S_INIT_REG, 0x1);	/* i2s fifo reset */
	clrbitsl(I2S_INIT_REG, 0x4);	/* i2s tx fifo disable */
	clrbitsl(I2S_INIT_REG, 0x2);	/* i2s rx fifo disable */

#if defined(AMBOOT_AUDIO_MODE_SLAVE)
	writel(I2S_RX_CTRL_REG, 0x00);
	writel(I2S_TX_CTRL_REG, 0x00);
#else
	writel(I2S_RX_CTRL_REG, 0x02);
#if defined(AMBOOT_AUDIO_16BIT)
	writel(I2S_TX_CTRL_REG, 0x28);
#else
	writel(I2S_TX_CTRL_REG, 0x20);
#endif
#endif

#if defined(AMBOOT_AUDIO_16BIT)
	writel(I2S_TX_FIFO_LTH_REG, 0x10);
#elif defined(AMBOOT_AUDIO_32BIT)
	writel(I2S_TX_FIFO_LTH_REG, 0x20);
#endif
	writel(I2S_RX_FIFO_GTH_REG, 0x20);

	writel(I2S_CHANNEL_SELECT_REG, I2S_2CHANNELS_ENB);
	writel(I2S_MODE_REG, 0x4);
#if defined(AMBOOT_AUDIO_16BIT)
	writel(I2S_WLEN_REG, 0x0f);
#elif defined(AMBOOT_AUDIO_32BIT)
	writel(I2S_WLEN_REG, 0x1f);
#endif
	writel(I2S_WPOS_REG, 0x0);
	writel(I2S_SLOT_REG, 0x0);
#if defined(AMBOOT_AUDIO_16BIT)
	writel(I2S_24BITMUX_MODE_REG, 0x0);
#elif defined(AMBOOT_AUDIO_32BIT)
	writel(I2S_24BITMUX_MODE_REG, 0x1);
#endif
}

static void pcm_dma_init(void)
{
	struct dma_desc *desc, *next_desc;
	u32 i, *data;

	/* used for valid audio data check */
	data = (u32 *)DSP_FASTAUDIO_START;
	for(i = 0; i < DSP_FASTAUDIO_SIZE/4; i++)
		data[i] = 0xdeadbeaf;

	/* dma desc init for record */
	for (i = 0; i < DMA_DESC_NUM; i++) {
		desc = &pcm_dma_desc[i];
		next_desc = &pcm_dma_desc[(i + 1) % DMA_DESC_NUM];

		desc->attr = DMA_DESC_WM | DMA_DESC_NI |
				DMA_DESC_IE | DMA_DESC_ST |
				DMA_DESC_ID | DMA_DESC_BLK_32B;
#if defined(AMBOOT_AUDIO_16BIT)
		desc->attr |= DMA_DESC_TS_2B;
#elif defined(AMBOOT_AUDIO_32BIT)
		desc->attr |= DMA_DESC_TS_4B;
#endif

		desc->xfr_count = DSP_FASTAUDIO_SIZE / DMA_DESC_NUM;
		desc->src = I2S_RX_DATA_DMA_REG;
		desc->dst = DSP_FASTAUDIO_START + i * desc->xfr_count;
		/* rpt_addr points to desc->rpt field */
		desc->rpt_addr = (uintptr_t)desc + sizeof(struct dma_desc) - 4;
		desc->next_desc = (uintptr_t)next_desc;
	}

	clean_d_cache(pcm_dma_desc, sizeof(pcm_dma_desc));
	writel(DMA0_CHAN_STA_REG(I2S_RX_DMA_CHAN), 0x0);
	writel(DMA0_CHAN_DA_REG(I2S_RX_DMA_CHAN), (uintptr_t)pcm_dma_desc);

	/* dma desc init for playback */
	desc = &pcm_dma_desc[DMA_DESC_NUM];
	desc->attr = DMA_DESC_RM | DMA_DESC_NI |
			DMA_DESC_IE | DMA_DESC_ST |
			DMA_DESC_BLK_32B |
			DMA_DESC_ID | DMA_DESC_EOC;
#if defined(AMBOOT_AUDIO_16BIT)
	desc->attr |= DMA_DESC_TS_2B;
#elif defined(AMBOOT_AUDIO_32BIT)
	desc->attr |= DMA_DESC_TS_4B;
#endif
	desc->src = audio_play_addr;
	desc->dst = I2S_TX_LEFT_DATA_DMA_REG;
	BUG_ON(audio_play_size >= 0x00400000);
	desc->xfr_count = audio_play_size;
	/* rpt_addr points to desc->rpt field */
	desc->rpt_addr = (uintptr_t)desc + sizeof(struct dma_desc) - 4;
	desc->next_desc = (uintptr_t)desc;

	clean_d_cache(desc, sizeof(*desc));
	writel(DMA0_CHAN_STA_REG(I2S_TX_DMA_CHAN), 0x0);
	writel(DMA0_CHAN_DA_REG(I2S_TX_DMA_CHAN), (uintptr_t)desc);
}

void audio_set_play_size(u32 addr, u32 size)
{
	audio_play_size = size;
	audio_play_addr = addr;
}

static void timer4_reset(void)
{
	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) & (~0xF000)));
	writel(TIMER4_STATUS_REG, 0xFFFFFFFF);
	writel(TIMER4_RELOAD_REG, 0x00000000);
	writel(TIMER4_MATCH1_REG, 0x00000000);
	writel(TIMER4_MATCH2_REG, 0x00000000);

	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) | 0x1000));
}

void audio_init(void)
{
	i2s_init();
	pcm_dma_init();
}

void audio_start(void)
{
	setbitsl(I2S_INIT_REG, 0x4);	/* i2s tx fifo enable */
	setbitsl(I2S_INIT_REG, 0x2);	/* i2s rx fifo enable */

	writel(DMA0_CHAN_CTR_REG(I2S_TX_DMA_CHAN), DMA_CHANX_CTR_EN | DMA_CHANX_CTR_D);
	writel(DMA0_CHAN_CTR_REG(I2S_RX_DMA_CHAN), DMA_CHANX_CTR_EN | DMA_CHANX_CTR_D);

	timer4_reset();

	putstr("DSP_FASTAUDIO_START = 0x");
	puthex(DSP_FASTAUDIO_START);
	putstr("\r\n");

	putstr("DSP_FASTAUDIO_SIZE = 0x");
	puthex(DSP_FASTAUDIO_SIZE);
	putstr("\r\n");
}

