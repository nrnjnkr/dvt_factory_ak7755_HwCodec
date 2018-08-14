##
## /S2l/oryx/video/utility/make.inc
##
## History:
##   Apr 17, 2017 - [Shupeng Ren] created file
##
## Copyright (c) 2017 Ambarella, Inc.
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
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_TARGET := libamvideo-utility.so
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                          \
                -I$(ORYX_DIR)/include/video                    \
                -I$(ORYX_DIR)/include/utility                  \
                -I$(ORYX_DIR)/video/include                    \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

VUTILITY_SO_NAME   = libamvideo-utility.so.1
VUTILITY_SO_TARGET = libamvideo-utility.so.1.0.0

LOCAL_SO_NAME := $(VUTILITY_SO_NAME)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(VUTILITY_SO_TARGET)
	@ln -sf $(VUTILITY_SO_TARGET) $(ORYX_LIB_DIR)/$(VUTILITY_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
