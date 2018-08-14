###############################################################################
## Makefile
##
## History:
##    2015/10/13 - [Zhi He] Create File
##
## Copyright (C) 2015 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
##################################################################################

default: all

.PHONY: all
.PHONY: clean

CC := gcc
##CC := /usr/local/linaro-armv7ahf-2016.02-gcc5.3/bin/arm-linux-gnueabihf-gcc
AR := ar
##AR := /usr/local/linaro-armv7ahf-2016.02-gcc5.3/bin/arm-linux-gnueabihf-ar
STRIP := strip
##STRIP := /usr/local/linaro-armv7ahf-2016.02-gcc5.3/bin/arm-linux-gnueabihf-strip
CFLAGS := -I../include -g -gdwarf-2 -ggdb -Og
LDFLAGS := -L../lib -llwcryptography

all:
	@$(CC) rsa_genkey.c -o rsa_genkey $(CFLAGS) $(LDFLAGS)
	@echo "Build rsa_genkey done."
	@$(CC) rsa_verify.c -o rsa_verify $(CFLAGS) $(LDFLAGS)
	@echo "Build rsa_verify done."
	@$(CC) rsa_sign.c -o rsa_sign $(CFLAGS) $(LDFLAGS)
	@echo "Build rsa_sign done."
	@$(CC) ecc_genkey.c -o ecc_genkey $(CFLAGS) $(LDFLAGS)
	@echo "Build ecc_genkey done."
	@$(CC) ecc_verify.c -o ecc_verify $(CFLAGS) $(LDFLAGS)
	@echo "Build ecc_verify done."
	@$(CC) ecc_sign.c -o ecc_sign $(CFLAGS) $(LDFLAGS)
	@echo "Build ecc_sign done."
	@$(CC) verify_aes.c -o verify_aes $(CFLAGS) $(LDFLAGS)
	@echo "Build test_aes done."
	@$(CC) verify_sha256.c -o verify_sha256 $(CFLAGS) $(LDFLAGS)
	@echo "Build verify_sha256 done."

clean:
	@-rm *.a -rf
	@-rm *.o -rf
	@-rm rsa_genkey -rf
	@-rm rsa_verify -rf
	@-rm rsa_sign -rf
	@-rm ecc_genkey -rf
	@-rm ecc_verify -rf
	@-rm ecc_sign -rf
	@-rm verify_aes -rf
	@-rm verify_sha256 -rf

