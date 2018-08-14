/**
 * bld/cmd_sd.c
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

#include <bldfunc.h>
#include <ambhw/sd.h>
#include <sdmmc.h>

/*===========================================================================*/
#define DIAG_SD_BUF_START	bld_hugebuf_addr
#define SECTORS_PER_OP		128
#define SECTORS_PER_SHMOO	4096
#if defined(CONFIG_SD_SHMOO_SD_CTL) || defined(CONFIG_SD_SHMOO_PHY_TURNING)
static u32 send_tuning = 0;
#endif
/*===========================================================================*/
static int cmd_sd_init(int argc, char *argv[], int verbose)
{
	u32 slot, mode, clock;
	int ret_val;

	if (argc < 1)
		return -1;

	if (verbose) {
		putstr("running SD test ...\r\n");
		putstr("press any key to terminate!\r\n");
	}

	ret_val = strtou32(argv[0], &slot);
	if (ret_val < 0) {
		putstr("Invalid slot id!\r\n");
		return -1;
	}

	if (argc >= 2) {
		if (strcmp(argv[1], "ddr50") == 0)
			mode = SDMMC_MODE_DDR50;
		else if (strcmp(argv[1], "sdr104") == 0)
			mode = SDMMC_MODE_SDR104;
		else if (strcmp(argv[1], "sdr50") == 0)
			mode = SDMMC_MODE_SDR50;
		else if (strcmp(argv[1], "sdr25") == 0)
			mode = SDMMC_MODE_SDR25;
		else if (strcmp(argv[1], "sdr12") == 0)
			mode = SDMMC_MODE_SDR12;
		else if (strcmp(argv[1], "hs") == 0)
			mode = SDMMC_MODE_HS;
		else if (strcmp(argv[1], "ds") == 0)
			mode = SDMMC_MODE_DS;
		else
			mode = SDMMC_MODE_AUTO;
	} else {
		mode = SDMMC_MODE_AUTO;
	}

	if (argc >= 3) {
		ret_val = strtou32(argv[2], &clock);
		if (ret_val < 0) {
			putstr("Invalid clock\r\n");
			return -1;
		}
		clock *= 1000000;
	} else {
		clock = -1;
	}

	ret_val = sdmmc_init_sd(slot, mode, clock, verbose);
	if (ret_val < 0)
		ret_val = sdmmc_init_mmc(slot, mode, clock, verbose, 0);

	if (verbose) {
		putstr("total_secs: ");
		putdec(sdmmc_get_total_sectors());
		putstr("\r\n");
	}

	return ret_val;
}

static int cmd_sd_show(int argc, char *argv[])
{
	int i, ret_val = -1;

	if (argc < 1)
		return -1;

	if (strcmp(argv[0], "partition") == 0) {
		putstr("total_sectors: ");
		putdec(sdmmc_get_total_sectors());
		putstr("\r\n");
		putstr("sector_size: ");
		putdec(sdmmc.sector_size);
		putstr("\r\n");
		for (i = 0; i < HAS_IMG_PARTS; i++) {
			putstr(get_part_str(i));
			putstr(" partition blocks: ");
			putdec(sdmmc.ssec[i]);
			putstr(" - ");
			putdec(sdmmc.ssec[i] + sdmmc.nsec[i]);
			putstr("\r\n");
		}

		ret_val = 0;
	} else if (strcmp(argv[0], "info") == 0) {
		ret_val = sdmmc_show_card_info();
		if (ret_val < 0)
			putstr("Please init card first\r\n");
	}

	return ret_val;
}

static int cmd_sd_read(int argc, char *argv[])
{
	int ret_val = 0;
	int i = 0;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		ret_val = sdmmc_read_sector(sector, sectors, buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_write(int argc, char *argv[])
{
	int ret_val = 0;
	int i, j;
	int total_secs;
	int sector, sectors;
	u8 *buf = (u8 *)DIAG_SD_BUF_START;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	for (i = 0; i < SECTORS_PER_OP * SDMMC_SEC_SIZE / 16; i++) {
		for (j = 0; j < 16; j++) {
			buf[(i * 16) + j] = i;
		}
	}

	total_secs = sdmmc_get_total_sectors();
	for (sector = 0, i = 0; sector < total_secs;
		sector += SECTORS_PER_OP, i++) {

		if (uart_poll())
			break;

		if ((total_secs - sector) < SECTORS_PER_OP)
			sectors = total_secs - sector;
		else
			sectors = SECTORS_PER_OP;

		ret_val = sdmmc_write_sector(sector, sectors, buf);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(sector);
			putchar('/');
			putdec(total_secs);
			putstr(" (");
			putdec(sector * 100 / total_secs);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_verify_data(u8* origin, u8* data, u32 len, u32 sector, u8 verbose)
{
	int i, rval = 0;

	for (i = 0; i < len; i++) {
		if (origin[i] != data[i]) {
			rval = -1;;
			if (verbose) {
				putstr("\r\nSector ");
				putdec(i / SDMMC_SEC_SIZE + sector);
				putstr(" byte ");
				putdec(i % SDMMC_SEC_SIZE);
				putstr(" failed data origin: 0x");
				puthex(origin[i]);
				putstr(" data: 0x");
				puthex(data[i]);
				putstr("\r\n");
			}
		}
	}

	return rval;
}

static int cmd_sd_verify(int argc, char *argv[], u8 shmoo, u8 verbose)
{
	u32 i, total_secs, sector, sectors;
	u8 *wbuf = (u8 *)DIAG_SD_BUF_START;
	u8 *rbuf = (u8 *)DIAG_SD_BUF_START + 0x100000;
	int ret_val = 0;

	if(!shmoo) {
		ret_val = cmd_sd_init(argc, argv, 1);
		if (ret_val < 0)
			return ret_val;

	}

	for (i = 0; i < SECTORS_PER_OP * SDMMC_SEC_SIZE; i++)
		wbuf[i] = rand() / SECTORS_PER_OP;

	if (shmoo)
		total_secs = SECTORS_PER_SHMOO;
	else
		total_secs = sdmmc_get_total_sectors();

	for (i = 0, sector = 0, sectors = 0; sector < total_secs;
		i++, sector += SECTORS_PER_OP) {

		if (uart_poll())
			break;

		sectors++;
		if (sectors > SECTORS_PER_OP)
			sectors = 1;

		ret_val = sdmmc_write_sector(sector, sectors, wbuf);
		if (ret_val != 0) {
			if (verbose) {
				putstr("Write sector fail ");
				putdec(ret_val);
				putstr(" @ ");
				putdec(sector);
				putstr(" : ");
				putdec(sectors);
				putstr("\r\n");
			}
			return -1;
		}

		ret_val = sdmmc_read_sector(sector, sectors, rbuf);
		if (ret_val != 0) {
			if (verbose) {
				putstr("Read sector fail ");
				putdec(ret_val);
				putstr(" @ ");
				putdec(sector);
				putstr(" : ");
				putdec(sectors);
				putstr("\r\n");
			}
			return -1;
		}

		ret_val = cmd_sd_verify_data(wbuf, rbuf,
				(sectors * SDMMC_SEC_SIZE), sector, verbose);
		if (ret_val < 0)
			return -1;

		if(verbose) {
			putchar('.');

			if ((i & 0xf) == 0xf) {
				putchar(' ');
				putdec(sector);
				putchar('/');
				putdec(total_secs);
				putstr(" (");
				putdec(sector * 100 / total_secs);
				putstr("%)\t\t\r");
			}
		}

		if ((sector + SECTORS_PER_OP) >= total_secs)
			break;
	}

	if (verbose)
		putstr("\r\ndone!\r\n");

	return ret_val;
}

static int cmd_sd_erase(int argc, char *argv[])
{
	u32 sector, sector_end, sector_num;
	u32 total_secs, sector_erase;
	int i, ret_val = 0;

	ret_val = cmd_sd_init(argc, argv, 1);
	if (ret_val < 0)
		return ret_val;

	total_secs = sdmmc_get_total_sectors();

	if (argc > 1) {
		ret_val = strtou32(argv[0], &sector);
		if (ret_val < 0 || sector >= total_secs) {
			putstr("Invalid sector address!\r\n");
			return -1;
		}
	} else {
		sector = 0;
	}

	if (argc > 2) {
		ret_val = strtou32(argv[0], &sector_num);
		if (ret_val < 0) {
			putstr("Invalid sector number!\r\n");
			return -1;
		}
	} else {
		sector_num = sdmmc_get_total_sectors();
	}

	if (sector + sector_num > total_secs)
		sector_num = total_secs - sector;

	sector_end = sector + sector_num;

	for (i = 0; sector < sector_end; i++, sector += sector_erase) {
		if (sector_end - sector > SECTORS_PER_OP)
			sector_erase = SECTORS_PER_OP;
		else
			sector_erase = sector_end - sector_num;

		ret_val = sdmmc_erase_sector(sector, sector_erase);

		putchar('.');

		if ((i & 0xf) == 0xf) {
			putchar(' ');
			putdec(i * SECTORS_PER_OP);
			putchar('/');
			putdec(sector_num);
			putstr(" (");
			putdec(i * SECTORS_PER_OP * 100 / sector_num);
			putstr("%)\t\t\r");
		}

		if (ret_val < 0) {
			putstr("\r\nfailed at sector ");
			putdec(sector);
			putstr("\r\n");
			break;
		}

		if (uart_poll())
			break;
	}

	putstr("\r\ndone!\r\n");

	return ret_val;
}

#if defined(CONFIG_SD_SHMOO_SD_CTL) || defined(CONFIG_SD_SHMOO_PHY_TURNING)
static int cmd_sd_tuning(int argc, char *argv[])
{
	int ret_val;
	ret_val = strtou32(argv[0], &send_tuning);
	if (ret_val < 0) {
		putstr("Unknown tuning switch!\r\n");
		return -1;
	}

	send_tuning = !!send_tuning;
	if (send_tuning)
		printf("send-tuning opened\n");
	else
		printf("send-tuning closed\n");
	return 0;
}
#endif

#if defined(CONFIG_SD_SHMOO_SD_CTL)
struct sd_timing_reg {
	u32 clk_output_mode : 2;
	u32 clk_output_delay : 3;
	u32 data_output_delay : 3;
	u32 sd_delay_sel0;
	u32 sd_delay_sel1;
};

struct sd_timing_count {
	u32 value;
	int count;
};

static u32 sd_shmoo_find_best_value(struct sd_timing_count *timing_count, int num)
{
	int i;
	u32 best_value, best_count;

	best_count = timing_count[0].count;
	best_value = timing_count[0].value;
	for(i = 0; i < num; i++) {
		if(timing_count[i].count > best_count) {
			best_count = timing_count[i].count;
			best_value = timing_count[i].value;
		}
	}

	return best_value;
}

static int cmd_sd_shmoo(int argc, char *argv[])
{
	struct sd_timing_reg timing[256];
	struct sd_timing_count data_output[8];
	u32 slot, i, j, num, total_num, sd_delay_sel0, sd_delay_sel1;
	const char *mode[] = {"ddr50", "sdr104", "sdr50", "sdr25", "sdr12", "hs", "ds"};
	int ret_val, d0_out, d0_in, cmd_out, cmd_in, clk_mode, clk_delay;

	ret_val = strtou32(argv[0], &slot);
	if (ret_val < 0) {
		putstr("Invalid slot id!\r\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(mode); i++) {
		if (strcmp(argv[1], mode[i]) == 0)
			break;
	}

	if (i >= ARRAY_SIZE(mode)) {
		putstr("Unknown mode!\r\n");
		return -1;
	}

	putstr("\r\nRunning SD Shmoo for ");
	putstr(mode[i]);
	putstr(" ...\r\n");

	i = num = 0;
	cmd_out = cmd_in = d0_in = 0;
	total_num = 4 * 8 * 8;
	memset(timing, 0, sizeof(struct sd_timing_reg) * total_num);
	memset(data_output, 0, sizeof(struct sd_timing_count) * 8);

	for(clk_mode = 0; clk_mode < 4; clk_mode++) {
		for(clk_delay = 0; clk_delay < 8; clk_delay++) {
			for(d0_out = 0; d0_out < 8; d0_out++) {
				/*user abort*/
				if (uart_poll()) {
					putstr("\r\n!!! Abort by User !!!\r\n");
					goto __done;
				}

				ret_val = cmd_sd_init(argc, argv, 0);
				if(ret_val < 0) {
					break;
				}
				sd_delay_sel0 = cmd_in | (cmd_out << 3) | (d0_out << 9) |
						(d0_out << 15) | (d0_out << 21) |
						(d0_out << 27) | (d0_in << 6) |
						(d0_in << 12) | (d0_in << 18) |
						(d0_in << 24) | ((d0_in & 0x3) << 30);
				sd_delay_sel1 = (clk_mode << 25) | (clk_delay << 22) |
						(d0_out << 1) | (d0_out << 7) |
						(d0_out << 13) | (d0_out << 19) |
						(d0_in >> 2) | (d0_in << 4) |
						(d0_in << 10) | (d0_in << 16);
				writel(SD_BASE(slot) + SD_DELAY_SEL_L, sd_delay_sel0);
				writel(SD_BASE(slot) + SD_DELAY_SEL_H, sd_delay_sel1);

				if (send_tuning)
					ret_val = sdmmc_send_tuning();
				/* Except DDR50, SDR50 and SDR104, all other modes are using
				 * write-and-read to verify the device */
				if(!send_tuning || ret_val == 1)
					ret_val = cmd_sd_verify(argc, argv, 1, 0);

				if(ret_val == 0) {
					/* Shmoo successfully */
					timing[num].data_output_delay = d0_out;
					timing[num].clk_output_delay = clk_delay;
					timing[num].clk_output_mode = clk_mode;
					timing[num].sd_delay_sel0 = sd_delay_sel0;
					timing[num].sd_delay_sel1 = sd_delay_sel1;
					num++;
				}

				putchar('.');

				if ((i++ & 0xf) == 0xf) {
					putchar(' ');
					putdec(i);
					putchar('/');
					putdec(total_num);
					putstr(" (");
					putdec(i * 100 / total_num);
					putstr("%) [");
					putdec(num);
					putstr("]\r");
				}

			}
		}
	}
__done:

	putstr("\r\nValid delay timing: \r\n");
	for(i =0; i < num; i++) {
		putstr("\r\nclk_mode:");
		putdec(timing[i].clk_output_mode);
		putstr(" clk_delay:");
		putdec(timing[i].clk_output_delay);
		putstr(" data_out:");
		putdec(timing[i].data_output_delay);
	}

	putstr("\r\n\r\nValid value for SD Controller 0xD8 and 0xDC:\r\n");
	for(i =0; i < num; i++) {
		putstr("CTRL_L: 0x");
		puthex(timing[i].sd_delay_sel0);
		putstrhex(" CTRL_H: 0x", timing[i].sd_delay_sel1);
	}

	putstrdec("\r\nThe num of sucessful: ", num);

	if(num) {
		/*Try to find the best value for every timing*/
		for (i = 0; i < 4; i++) {
			data_output[i].value = i;
			data_output[i].count = 0;
			for (j = 0; j < num; j++) {
				if (timing[j].clk_output_mode == i)
					data_output[i].count++;
			}
		}

		clk_mode = sd_shmoo_find_best_value(data_output, 4);

		for (i = 0; i < 8; i++) {
			data_output[i].value = i;
			data_output[i].count = 0;
			for (j = 0; j < num; j++) {
				if (timing[j].clk_output_delay == i)
					data_output[i].count++;
			}
		}

		clk_delay = sd_shmoo_find_best_value(data_output, 8);

		for (i = 0; i < 8; i++) {
			data_output[i].value = i;
			data_output[i].count = 0;
			for (j = 0; j < num; j++) {
				if (timing[j].data_output_delay == i)
					data_output[i].count++;
			}
		}

		d0_out = sd_shmoo_find_best_value(data_output, 8);

		/*Calculate the best value*/
		sd_delay_sel0 = cmd_in | (cmd_out << 3) | (d0_out << 9) | (d0_out << 15) |
				(d0_out << 21) | (d0_out << 27) | (d0_in << 6) | (d0_in << 12) |
				(d0_in << 18) | (d0_in << 24) | ((d0_in & 0x3) << 30);
		sd_delay_sel1 = (clk_mode << 25) | (clk_delay << 22) | (d0_out << 1) |
				(d0_out << 7) | (d0_out << 13) | (d0_out << 19) | (d0_in >> 2) |
				(d0_in << 4) | (d0_in << 10) | (d0_in << 16);


		putstr("\r\nThe best timing:");
		putstr("\r\nCTRL_L: 0x");
		puthex(sd_delay_sel0);
		putstrhex(" CTRL_H: 0x", sd_delay_sel1);
	}

	return 0;
}

#elif defined(CONFIG_SD_SHMOO_PHY_TURNING)

struct sd_timing_reg {
	u32 sd_phy0_reg;
	u32 sd_phy1_reg;
	u32 sd_ncr_reg;
} timing[32*3*3];

static int cmd_sd_shmoo(int argc, char *argv[])
{
	u32 phy0, phy1, ncr_reg, reg;
	u32 slot, i, num, phy_ctrl_0_reg, phy_ctrl_1_reg;
	const char *mode[] = {"ddr50", "sdr104", "sdr50", "sdr25", "sdr12", "hs", "ds"};
	int ret_val;

	ret_val = strtou32(argv[0], &slot);
	if (ret_val < 0) {
		putstr("Invalid slot id!\r\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(mode); i++) {
		if (strcmp(argv[1], mode[i]) == 0)
			break;
	}

	if (i >= ARRAY_SIZE(mode)) {
		putstr("Unknown mode!\r\n");
		return -1;
	}

	/*select the timing contorller based on different slot*/
	if(slot == 2) {
#if (SDXC_SOFT_PHY_SUPPORT == 1)
		phy_ctrl_0_reg = SDXC_PHY_CTRL_0_REG;
		phy_ctrl_1_reg = SDXC_PHY_CTRL_1_REG;
#else
		putstr("Now our chip can not support slot[2] timing tuning.");
		putstr("\r\n");

		return -1;
#endif
	} else if(slot == 1) {
		/*reserved*/
		putstr("Now our chip can not support slot[1] timing tuning.");
		putstr("\r\n");
		return -1;
	} else {
		phy_ctrl_0_reg = SD_PHY_CTRL_0_REG;
		phy_ctrl_1_reg = SD_PHY_CTRL_1_REG;
	}

	putstr("\r\nRunning SD Shmoo for ");
	putstr(mode[i]);
	putstr(" ...\r\n");

	memset(timing, 0, sizeof(timing));
	i = num = 0;

	/* [sd_clkout_bypass, sd_rx_clk_pol, sd_data_cmd_bypass, sbc_2, sbc_1]
	 * = 0000b to 1111b, [bit26, bit19, bit18, bit[2:1]] in SD_PHY_CTRL_0_REG */
	for(phy0 = 0; phy0 < 32; phy0++) {
		for (phy1 = 0x00000000; phy1 < 0x20202020; phy1 += 0x0f0f0f0f) {
			for (ncr_reg = 0x0000; ncr_reg < 0x3333; ncr_reg += 0x1111) {
				ret_val = cmd_sd_init(argc, argv, 0);
				if(ret_val < 0) {
					break;
				}
				reg = 0x1;
				reg |= ((phy0 >> 0) & 0x3) << 1;   /* sbc */
				reg |= ((phy0 >> 2) & 0x1) << 18; /* sd_data_cmd_bypass */
				reg |= ((phy0 >> 3) & 0x1) << 19; /* sd_rx_clk_pol */
				reg |= ((phy0 >> 4) & 0x1) << 26; /* sd_clkout_bypass */

				writel(phy_ctrl_0_reg, reg | 0x02000000);
				rct_timer_dly_ms(2);
				writel(phy_ctrl_0_reg, reg);
				rct_timer_dly_ms(2);
				writel(phy_ctrl_1_reg, phy1);

				writel(SD_BASE(slot) + SD_LAT_CTRL_OFFSET, ncr_reg);

				if (send_tuning)
					ret_val = sdmmc_send_tuning();
				/* Except DDR50, SDR50 and SDR104, all other modes are using
				 * write-and-read to verify the device */
				if(!send_tuning || ret_val == 1)
					ret_val = cmd_sd_verify(argc, argv, 1, 0);

				if(ret_val == 0) {
					timing[num].sd_phy0_reg =
						readl(phy_ctrl_0_reg);
					timing[num].sd_phy1_reg =
						readl(phy_ctrl_1_reg);
					timing[num].sd_ncr_reg =
						readl(SD_BASE(slot) + SD_LAT_CTRL_OFFSET);
					num++;
				}

				/*user abort*/
				if (uart_poll()) {
					putstr("\r\n!!! Abort by User !!!\r\n");
					goto __done;
				}

				putchar('.');

				if ((i++ & 0xf) == 0xf) {
					putchar(' ');
					putdec(i);
					putchar('/');
					putdec(ARRAY_SIZE(timing));
					putstr(" (");
					putdec(i * 100 / ARRAY_SIZE(timing));
					putstr("%) [");
					putdec(num);
					putstr("]\r");
				}
			}
		}
	}

	putstr("\r\nDone.\r\n\r\n");

__done:
	putstr("Shmoo Result: \r\n");

	for(i = 0; i < num; i++) {
		putstr("CTRL_0:0x");
		puthex(timing[i].sd_phy0_reg);
		putstr(", CTRL_1:0x");
		puthex(timing[i].sd_phy1_reg);
		putstr(", ncr_reg:");
		puthex(timing[i].sd_ncr_reg);
		putstr("\r\n");

	}

	putstrdec("\r\nThe num of sucessful: ", num);

	return 0;
}
#endif
/*===========================================================================*/
static int cmd_sd(int argc, char *argv[])
{
	int ret_val = -1;

	if (argc < 3)
		return ret_val;

	if (strcmp(argv[1], "init") == 0) {
		ret_val = cmd_sd_init(argc - 2, &argv[2], 1);
	} else if (strcmp(argv[1], "read") == 0) {
		ret_val = cmd_sd_read(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "write") == 0) {
		ret_val = cmd_sd_write(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "verify") == 0) {
		ret_val = cmd_sd_verify(argc - 2, &argv[2], 0, 1);
	} else if (strcmp(argv[1], "erase") == 0) {
		ret_val = cmd_sd_erase(argc - 2, &argv[2]);
#if defined(CONFIG_SD_SHMOO_SD_CTL) || defined(CONFIG_SD_SHMOO_PHY_TURNING)
	} else if (strcmp(argv[1], "shmoo") == 0) {
		ret_val = cmd_sd_shmoo(argc - 2, &argv[2]);
	} else if (strcmp(argv[1], "tuning") == 0) {
		ret_val = cmd_sd_tuning(argc - 2, &argv[2]);
#endif
	} else if (strcmp(argv[1], "show") == 0) {
		ret_val = cmd_sd_show(argc - 2, &argv[2]);
	}

	return ret_val;
}

/*===========================================================================*/
static char help_sd[] =
	"\t[slot]: 0/1/2/..\r\n"
	"\t[mode]: ds/hs/sdr12/sdr25/sdr50/sdr104/ddr50\r\n"
	"\t[clock]: clock in MHz\r\n"
	"\tsd init slot [mode] [clock]\r\n"
	"\tsd read slot [mode] [clock]\r\n"
	"\tsd write slot [mode] [clock]\r\n"
	"\tsd verify slot [mode] [clock]\r\n"
	"\tsd erase slot [ssec] [nsec]\r\n"
#if defined(CONFIG_SD_SHMOO_SD_CTL) || defined(CONFIG_SD_SHMOO_PHY_TURNING)
	"\tsd tuning [mode] ; # shmoo test with send-tuning\r\n"
	"\tsd shmoo slot [mode] [clock] \r\n"
#endif
	"\tsd show partition/info\r\n"
	"Test SD.\r\n";
__CMDLIST(cmd_sd, "sd", help_sd);

