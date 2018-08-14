/**
 * pca953x.c
 *
 * Author: XianqingZheng <xqzheng@ambarella.com>
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
#include <eth/network.h>

#define BMCR_ANRESTART		0x0200	/* Auto negotiation restart    */
#define BMCR_ANENABLE		0x1000	/* Enable auto negotiation     */


#define MII_KSZPHY_CTRL				0x1F
#define MII_KSZPHY_INTCS			0x1B
#define MII_STAT1000				0x0A
#define MII_KSZ9031RN_MMD_CTRL_REG		0x0D
#define MII_KSZ9031RN_MMD_REGDATA_REG		0x0E

#define OP_DATA					1
#define KSZ9031_PS_TO_REG		60

/* Extended registers */
/* MMD Address 0x0 */
#define MII_KSZ9031RN_FLP_BURST_TX_LO	3
#define MII_KSZ9031RN_FLP_BURST_TX_HI	4

/* MMD Address 0x2 */
#define MII_KSZ9031RN_CONTROL_PAD_SKEW	4
#define MII_KSZ9031RN_RX_DATA_PAD_SKEW	5
#define MII_KSZ9031RN_TX_DATA_PAD_SKEW	6
#define MII_KSZ9031RN_CLK_PAD_SKEW	8

#define KSZ80X1R_CTRL_INT_ACTIVE_HIGH		(1 << 9)
#define KSZ80X1R_RMII_50MHZ_CLK			(1 << 7)

#define PHY_ID_KSZ8081		0x00221560
#define PHY_ID_KSZ9031		0x00221620

typedef struct eth_phy_device
{
	u32 phy_id;
	unsigned int phy_id_mask;
	void (*config_init)(struct bld_eth_dev_s *dev, u8 phy_addr);
} eth_phy;

extern u16 eth_mii_read(struct bld_eth_dev_s *dev, u8 addr, u8 reg);
extern void eth_mii_write(struct bld_eth_dev_s *dev, u8 addr, u8 reg, u16 data);
extern u8 eth_scan_phy_addr(struct bld_eth_dev_s *dev);

void ksz9031_extended_write(struct bld_eth_dev_s *dev, u8 phy_addr,
				  u8 mode, u32 dev_addr, u32 regnum, u16 val)
{
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_CTRL_REG, dev_addr);
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_REGDATA_REG, regnum);
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_CTRL_REG, (mode << 14) | dev_addr);
	eth_mii_write(dev,phy_addr, MII_KSZ9031RN_MMD_REGDATA_REG, val);
}

 int ksz9031_extended_read(struct bld_eth_dev_s *dev, u8 phy_addr,
				 u8 mode, u32 dev_addr, u32 regnum)
{
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_CTRL_REG, dev_addr);
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_REGDATA_REG, regnum);
	eth_mii_write(dev, phy_addr, MII_KSZ9031RN_MMD_CTRL_REG, (mode << 14) | dev_addr);
	return eth_mii_read(dev, phy_addr, MII_KSZ9031RN_MMD_REGDATA_REG);
}

void ksz9031_of_load_skew_values(struct bld_eth_dev_s *dev, u8 phy_addr, u16 reg,
				u32 field_sz, u8 numfields, u16 *skew)
{
	u16 mask, maxval, useless = 0;
	u16 newval = 0;
	int i;

	for (i = 0; i < numfields; i++) {
		if (!skew[i]) {
			newval = ksz9031_extended_read(dev, phy_addr, OP_DATA, 2, reg);
			break;
		}
	}

	for (i = 0; i < numfields; i++)
		if (!skew[i])
			useless++;

	if (useless == numfields)
		return;

	maxval = (field_sz == 4) ? 0xf : 0x1f;
	for (i = 0; i < numfields; i++)
		if (skew[i]) {
			mask = 0xffff;
			mask ^= maxval << (field_sz * i);
			newval = (newval & mask) |
				(((skew[i] / KSZ9031_PS_TO_REG) & maxval)
					<< (field_sz * i));
		}

	return ksz9031_extended_write(dev, phy_addr, OP_DATA, 2, reg, newval);
}

void ksz80x1_config_init(struct bld_eth_dev_s *dev, u8 phy_addr)
{
	u16 phy_reg;

	phy_reg = eth_mii_read(dev, phy_addr, MII_KSZPHY_CTRL);
	phy_reg |= KSZ80X1R_RMII_50MHZ_CLK;
	eth_mii_write(dev, phy_addr, MII_KSZPHY_CTRL, phy_reg);
}

void ksz9031_config_init(struct bld_eth_dev_s *dev, u8 phy_addr)
{
	u16 value;

	/* rxc-skew-ps, txc-skew-ps */
	u16 clk_skew[] = {1320, 0};
	/* txen-skew-ps, rxdv-skew-ps */
	u16 control_skew[] = {0, 140};
	/* rxd0-skew-ps, rxd1-skew-ps, rxd2-skew-ps, rxd3-skew-ps */
	u16 rx_data_skew[] = {140 , 140 , 140 , 140};
	/* txd0-skew-ps, txd1-skew-ps, txd2-skew-ps, txd3-skew-ps */
	u16 tx_data_skew[] = {0 , 0 , 0 , 0};


	ksz9031_of_load_skew_values(dev, phy_addr,
			MII_KSZ9031RN_CLK_PAD_SKEW, 5, 2, clk_skew);
	ksz9031_of_load_skew_values(dev, phy_addr,
			MII_KSZ9031RN_CONTROL_PAD_SKEW, 4, 2, control_skew);
	ksz9031_of_load_skew_values(dev, phy_addr,
			MII_KSZ9031RN_RX_DATA_PAD_SKEW, 4, 4, rx_data_skew);
	ksz9031_of_load_skew_values(dev, phy_addr,
			MII_KSZ9031RN_TX_DATA_PAD_SKEW, 4, 4, tx_data_skew);

	ksz9031_extended_write(dev, phy_addr,
			OP_DATA, 0, MII_KSZ9031RN_FLP_BURST_TX_HI, 0x0006);
	ksz9031_extended_write(dev, phy_addr,
			OP_DATA, 0, MII_KSZ9031RN_FLP_BURST_TX_LO, 0x1A80);

	value = eth_mii_read(dev, phy_addr, 0x00);
	value |= BMCR_ANENABLE | BMCR_ANRESTART;
	eth_mii_write(dev, phy_addr, 0x00, value);

	eth_mii_write(dev, phy_addr, 0x09, 0x0300);
	eth_mii_write(dev, phy_addr, 0x09, 0x1300);
}

struct eth_phy_device ksphy[] = {
	{
		.phy_id 	= PHY_ID_KSZ8081,
		.phy_id_mask	= 0x00fffff0,
		.config_init	= ksz80x1_config_init,
	},
	{
		.phy_id 	= PHY_ID_KSZ9031,
		.phy_id_mask	= 0x00fffff0,
		.config_init	= ksz9031_config_init,
	},
	{}

};

void micrel_phy_init(void *dev) {
	u8 phy_addr;
	u16 phy_reg;
	u32 phy_id;
	int i;

	phy_addr = eth_scan_phy_addr(dev);
	phy_reg = eth_mii_read(dev, phy_addr, 0x02);	//PHYSID1
	phy_id = (phy_reg & 0xffff) << 16;
	phy_reg = eth_mii_read(dev, phy_addr, 0x03);	//PHYSID2
	phy_id |= (phy_reg & 0xffff);

	for(i = 0; i < ARRAY_SIZE(ksphy); i++) {
		if((phy_id & ksphy[i].phy_id_mask) == ksphy[i].phy_id &&
					ksphy[i].config_init != NULL) {
			ksphy[i].config_init(dev, phy_addr);
			break;
		}

	}

}


