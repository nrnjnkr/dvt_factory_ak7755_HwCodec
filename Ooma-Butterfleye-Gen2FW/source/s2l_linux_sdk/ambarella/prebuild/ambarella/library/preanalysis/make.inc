##
## prebuild/ambarella/library/preanalysis/make.inc
##
## History:
##    2016/08/23 - [Hao Qian] Created file
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

ifeq ($(CONFIG_AMBARELLA_PREANALYSIS_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

.PHONY: pre_analysis_lib

pre_analysis_lib: PRIVATE_PATH:=$(LOCAL_PATH)
pre_analysis_lib:
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $(PRIVATE_PATH)/lib/arch_$(AMBARELLA_ARCH)/*.so* $(FAKEROOT_DIR)/usr/lib/
	@echo "Copy $@ Done."

$(call add-target-into-build, pre_analysis_lib)

###
ifeq ($(BUILD_AMBARELLA_PREANALYSIS_PACKAGE_UT), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= patest
LOCAL_SRCS	:= $(wildcard ${LOCAL_PATH}/unit_test_priv/*.c)
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/include_priv
LOCAL_LDFLAGS	+= -L$(LOCAL_PATH)/lib/arch_$(AMBARELLA_ARCH) -lpreanalysis -lpreanalysis_core

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

endif