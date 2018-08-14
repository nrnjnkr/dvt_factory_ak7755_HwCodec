/**
 * boards/hawthorn/bsp/iav/codec_wm8974.c
 *
 * History:
 *    2014/11/18 - [Cao Rongrong] created file
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
 enum WM8974_SF_REG_VALUE
 {
	 SF_8K	= 0x00,
	 SF_16K = 0x01,
	 SF_48K = 0x02,
 };
 struct codec_reg_8_16 {
	 u8 addr;
	 u16 data;
 };

static int wm8974_bld_write(u8 idc_id, u8 addr, u8 reg, u16 value)
{
	reg = (reg <<1) + (value>>8);
	value = (u8)(value & 0x00ff);
	idc_bld_write_8_8(idc_id,addr,reg,value);

	return 0;
}

static const struct codec_reg_8_16 wm8974_regs[] = {
		{0x02, 0x015},
		{0x03, 0x065},
#if defined (AMBOOT_AUDIO_16BIT)
		{0x04, 0x010},
#else
		{0x04, 0x050},
#endif
		{0x0a, 0x04c},
		{0x0b, 0x0dd},		// 0 dB  -0.5dB step
		{0x0e, 0x1c8},
		{0x0f, 0x0ff},		// 0 dB  -0.5dB step
		{0x20, 0x134},
		{0x21, 0x03c},
		{0x22, 0x098},
		{0x2c, 0x103},
		{0x2d, 0x020},
		{0x2f, 0x100},
		{0x31, 0x006},
		{0x32, 0x001},
};


static int codec_setclk(u8 idc_id, u8 addr, u8 fs)
{
	switch(fs){
		case SF_8K:
		wm8974_bld_write(idc_id,addr,0x24, 0x017);			/* Turn PLL off for configuration... */
		wm8974_bld_write(idc_id,addr,0x25, 0x0d0);
		wm8974_bld_write(idc_id,addr,0x26, 0x000);
		wm8974_bld_write(idc_id,addr,0x27, 0x000);
#if defined	(AMBOOT_AUDIO_MODE_SLAVE)
		wm8974_bld_write(idc_id,addr,0x06, 0x0ad);
#else
		wm8974_bld_write(idc_id,addr,0x06, 0x0ac);
#endif
		wm8974_bld_write(idc_id,addr,0x07, 0x00a);
		wm8974_bld_write(idc_id,addr,0x01, 0x03f);			/* ...and on again */
	break;

	case SF_16K:
		wm8974_bld_write(idc_id,addr,0x24, 0x017); 		/* Turn PLL off for configuration... */
		wm8974_bld_write(idc_id,addr,0x25, 0x0d0);
		wm8974_bld_write(idc_id,addr,0x26, 0x000);
		wm8974_bld_write(idc_id,addr,0x27, 0x000);
#if defined	(AMBOOT_AUDIO_MODE_SLAVE)
		wm8974_bld_write(idc_id,addr,0x06, 0x06d);
#else
		wm8974_bld_write(idc_id,addr,0x06, 0x06c);
#endif
		wm8974_bld_write(idc_id,addr,0x07, 0x006);
		wm8974_bld_write(idc_id,addr,0x01, 0x03f); 		/* ...and on again */
	break;

	case SF_48K:
#if defined	(AMBOOT_AUDIO_MODE_SLAVE)
		wm8974_bld_write(idc_id,addr,0x06, 0x10d);
#else
		wm8974_bld_write(idc_id,addr,0x06, 0x00c);
#endif
		wm8974_bld_write(idc_id,addr,0x07, 0x000);
		wm8974_bld_write(idc_id,addr,0x01, 0x01f); 		/* ...and on again */
	break;
	default:
		putstr("WM8974 clock config error\r\n");
	}

        return 0;
}

static int do_codec_init(u8 idc_id, u8 addr)
{
	int i = 0;

	idc_bld_init(idc_id, 400000);
	wm8974_bld_write(idc_id, addr, 0x00, 0x000);
	wm8974_bld_write(idc_id, addr, 0x01, 0x01f);

	for (i = 0; i < ARRAY_SIZE(wm8974_regs); i++)
		wm8974_bld_write(idc_id, addr, wm8974_regs[i].addr, wm8974_regs[i].data);

#if defined(AMBOOT_AUDIO_48000)
	codec_setclk(idc_id, addr, SF_48K);
#elif defined(AMBOOT_AUDIO_16000)
	codec_setclk(idc_id, addr, SF_16K);
#elif defined(AMBOOT_AUDIO_8000)
	codec_setclk(idc_id, addr, SF_8K);
#endif
	wm8974_bld_write(idc_id,addr,0x0a, 0x00c);

	return 0;
}

#if defined(CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMBTFL_IMX322_S2L55M)
static int codec_init(){
      return do_codec_init(IDC_MASTER3,0x34);
}
#endif


