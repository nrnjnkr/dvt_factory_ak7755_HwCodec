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
CFLAGS := -I../include -g -gdwarf-2 -ggdb
#CFLAGS := -I../include -O3

big_number.o: big_number.c
crypt_utils.o: crypt_utils.c
digest_md5.o: digest_md5.c
digest_sha256.o: digest_sha256.c
pk_rsa.o: pk_rsa.c
pk_ecc.o: pk_ecc.c
aes.o: aes.c

all: big_number.o crypt_utils.o digest_md5.o digest_sha256.o pk_rsa.o pk_ecc.o aes.o
	@$(AR) rcus liblwcryptography.a big_number.o crypt_utils.o digest_md5.o digest_sha256.o pk_rsa.o pk_ecc.o aes.o
	@-mkdir -p ../lib/
	@-cp liblwcryptography.a ../lib/
	@echo "Build liblwcryptography.a done."

clean:
	@rm *.a -rf
	@rm ../lib/liblwcryptography.a -rf
	@rm *.o -rf



