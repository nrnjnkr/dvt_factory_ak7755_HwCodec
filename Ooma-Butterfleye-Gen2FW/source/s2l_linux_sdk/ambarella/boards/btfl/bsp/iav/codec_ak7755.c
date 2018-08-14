/*******************************************************************************
 * codec_ak7755.c
 *
 * History:
 *   Dec 30, 2016 - [Shupeng Ren] created file
 *   Jan 4, 2017 - [Xianqing Zheng] init codec
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
 ******************************************************************************/

#include <ambhw/spi.h>

// #define ak7755_debug

enum AK7755_SF_REG_VALUE
{
    SF_8K  = 0x00,
    SF_12K = 0x01,
    SF_16K = 0x02,
    SF_24K = 0x03,
    SF_32K = 0x04,
    SF_48K = 0x05,
};

static u32 ak7755_reg_read(u8 spi_id, u32 reg)
{
  unsigned char tx[1], rx[1];
  int wlen, rlen, ret;
  u32 rdata = 0;

  wlen = 1;
  rlen = 1;
  tx[0] = (unsigned char)(0x7F & reg);
  ret = spi_bld_write_then_read(spi_id, tx, wlen, rx, rlen);
  if(ret < 0) {
    printf("read ak7755 register[0x%x] fail\n", reg);
    return ret;
  } else {
    rdata = (unsigned int)rx[0];
  }

  return rdata;
}

static int ak7755_reg_write(u8 spi_id, u32 reg, u32 value)
{
  unsigned char tx[3];
  int wlen, ret;

  wlen = 2;
  tx[0] = reg;
  tx[1] = value;
  ret = spi_bld_write_then_read(spi_id, tx, wlen, NULL, 0);

  return ret;
}

static int ak7755_update_bits(u8 spi_id, u32 reg, u32 mask, u32 value)
{
  u32 old, new;
  int ret;

  ret = ak7755_reg_read(spi_id, reg);
  if (ret < 0) {
    printf("ak7755 update register[0x%x] bits:read fail\n", reg);
    return ret;
  }
  old = ret;
  new = (old & ~mask) | (value & mask);
  if (old != new) {
    ret = ak7755_reg_write(spi_id, reg, new);
  }

  if (ret < 0) {
    printf("ak7755 update register[0x%x] bits:write fail\n", reg);
    return ret;
  }

  return ret;
}

static int codec_init()
{
  int pdn_pin = GPIO(25);
  int amp_pin = GPIO(100);

  amba_spi_cfg_t spi_cfg = {
    .spi_mode = SPI_MODE_3,
    .cfs_dfs = 8,
    .cs_gpio = GPIO(49),
    .cs_change = 0,
    .baud_rate = 1000000,
  };
  spi_bld_init(SPI_MASTER2, &spi_cfg);

  if(0)
  {
    int power_pin = GPIO(0);
    gpio_config_sw_out(power_pin);
    gpio_set(power_pin);
  }

  gpio_config_sw_out(pdn_pin);
  gpio_clr(pdn_pin);
  rct_timer_dly_ms(2);
  gpio_set(pdn_pin);

  gpio_config_sw_out(amp_pin);
  gpio_set(amp_pin);

  ak7755_update_bits(SPI_MASTER2, 0xC1, 0x1, 0x1);
  ak7755_update_bits(SPI_MASTER2, 0xCF, 0xc, 0x0);
  ak7755_update_bits(SPI_MASTER2, 0xD4, 0xFF, 0xFF);
  ak7755_update_bits(SPI_MASTER2, 0xD3, 0x0F, 0x0F);
  ak7755_update_bits(SPI_MASTER2, 0xD2, 0xFF, 0xCC);
  ak7755_update_bits(SPI_MASTER2, 0xD0, 0x40, 0x40);
  ak7755_update_bits(SPI_MASTER2, 0xC2, 0x40, (0 << 6));
  ak7755_update_bits(SPI_MASTER2, 0xD0, 0x10, (0 << 4));
  // Set digital audio input (DMIC1)
  ak7755_update_bits(SPI_MASTER2, 0xDE, 0xE0, 0xA0);

  ak7755_reg_write(SPI_MASTER2, 0xCA, 0x01);
  ak7755_reg_write(SPI_MASTER2, 0xDA, 0x10);
  ak7755_reg_write(SPI_MASTER2, 0xCD, 0xC0);
  ak7755_reg_write(SPI_MASTER2, 0xE6, 0x01);
  ak7755_reg_write(SPI_MASTER2, 0xEA, 0x80);
  /* Set DAC digital volume
   * default value is 0x18(0.0dB)
   * 00h ~ FFh, +12.0dB ~ Mute
   * 36h is -15dB
   */
  // ak7755_reg_write(SPI_MASTER2, 0xD8, 0x10); /* left  */
  // ak7755_reg_write(SPI_MASTER2, 0xD9, 0x10); /* right */
  // ak7755_reg_write(SPI_MASTER2, 0xD8, 0x36); /* left  */
  // ak7755_reg_write(SPI_MASTER2, 0xD9, 0x36); /* right */

  /*Bypass mode*/
  ak7755_update_bits(SPI_MASTER2, 0xC8, 0xC0, 0xC0);
  ak7755_update_bits(SPI_MASTER2, 0xCE, 0xC7, 0xC7);
  ak7755_update_bits(SPI_MASTER2, 0xCC, 0x07, 0x03);

  /*Set fmt*/
  ak7755_update_bits(SPI_MASTER2, 0xC0, 0x70, 0x20);
  ak7755_update_bits(SPI_MASTER2, 0xCA, 0x60, 0x00);
#if defined(AMBOOT_AUDIO_48000)
  ak7755_update_bits(SPI_MASTER2, 0xC0, 0x07, SF_48K);
#elif defined(AMBOOT_AUDIO_16000)
  ak7755_update_bits(SPI_MASTER2, 0xC0, 0x07, SF_16K);
#elif defined(AMBOOT_AUDIO_8000)
  ak7755_update_bits(SPI_MASTER2, 0xC0, 0x07, SF_8K);
#endif
  ak7755_update_bits(SPI_MASTER2, 0xC1, 0x31, 0x21);
  ak7755_update_bits(SPI_MASTER2, 0xCF, 0xC, 0xC);
  ak7755_update_bits(SPI_MASTER2, 0xC2, 0xb0, 0x10);
  ak7755_update_bits(SPI_MASTER2, 0xC3, 0xF0, 0xf0);
  ak7755_update_bits(SPI_MASTER2, 0xC6, 0x37, 0x33);
  ak7755_update_bits(SPI_MASTER2, 0xC7, 0xFF, 0xf3);

#ifdef ak7755_debug
  u32 i, val;
  for(i = 0xc0; i <= 0xde; i++) {
    val = ak7755_reg_read(SPI_MASTER2, i);
    printf("0x%x: 0x%x\n", i, val);
  }
#endif
  return 0;
}

