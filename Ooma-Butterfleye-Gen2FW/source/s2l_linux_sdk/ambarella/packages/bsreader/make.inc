##
## packages/bsreader/make.inc
##
## History:
##    2012/06/01 - [Cao Rongrong] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
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

ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libbsreader.a

LOCAL_CFLAGS	:=

LOCAL_SRCS	:= $(LOCAL_PATH)/fifo.c
ifeq ($(CONFIG_ARCH_A5S),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_a5s.c
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/build/include
else
ifeq ($(CONFIG_ARCH_S2),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2.c
endif
ifeq ($(CONFIG_ARCH_S2E),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2.c
endif
ifeq ($(CONFIG_ARCH_S2L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2l.c
endif
ifeq ($(CONFIG_ARCH_S3L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s3l.c
endif
ifeq ($(CONFIG_ARCH_S5),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s5.c
endif
ifeq ($(CONFIG_ARCH_A7L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_a7l.c
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/build/include
endif
ifeq ($(CONFIG_ARCH_S5L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s5l.c
endif
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

