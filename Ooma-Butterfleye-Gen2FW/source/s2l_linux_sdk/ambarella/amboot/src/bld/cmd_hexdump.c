/**
 * bld/cmd_hexdump.c
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
#include <ambhw/drctl.h>
static void hexdump(unsigned char *p, int len)
{
	int i, j;

	for (i = 0; i < len; ) {
		if (i % 16 == 0) {
			printf("%08x  ", &p[i]);
			for (j = 0; j < 16; j++) {
				printf("%02x ", p[i + j]);
				if (j == 7)
					printf(" ");
			}

			printf(" |");

			for (j = 0; j < 16; j++) {
				if ((p[i + j] < 127)&&(p[i + j] > 32))
					printf("%c", p[i + j]);
				else
					printf(".");
			}

			i += j;
			printf("|\n");
		}
	}
}

static int cmd_hexdump(int argc, char *argv[])
{
	u32	addr;
	u32 len = 16;

	if (argc == 3) {
		if (strtou32(argv[1], &addr) == -1) {
			printf("invalid address!\n");
			return -1;
		}

		if (strtou32(argv[2], &len) == -1) {
			printf("invalid len!\n");
			return -1;
		}
		hexdump((unsigned char *)(uintptr_t)addr, len);

	} else if (argc == 2) {
		if (strtou32(argv[1], &addr) == -1) {
			printf("invalid address!\n");
			return -1;
		}
		hexdump((unsigned char *)(uintptr_t)addr, len);

	} else {
		printf("'help hexdump' for show command help\n");
		return -2;
	}
	return 0;
}


static char help_hexdump[] =
	"hexdump [address] [len]\r\n"
	"dump data from an address\r\n";

__CMDLIST(cmd_hexdump, "hexdump", help_hexdump);
