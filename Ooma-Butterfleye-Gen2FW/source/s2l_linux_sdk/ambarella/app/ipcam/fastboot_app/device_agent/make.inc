#
# make.inc
#
# History:
#       2015/03/10 - [jywang] created file
#
# Copyright (c) 2015 Ambarella, Inc.
#
# This file and its contents ("Software") are protected by intellectual
# property rights including, without limitation, U.S. and/or foreign
# copyrights. This Software is also the confidential and proprietary
# information of Ambarella, Inc. and its licensors. You may not use, reproduce,
# disclose, distribute, modify, or otherwise prepare derivative works of this
# Software or any portion thereof except pursuant to a signed license agreement
# or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
# In the absence of such an agreement, you agree to promptly notify and return
# this Software to Ambarella, Inc.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
# MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
########################################################
# libdevice_agent.a
########################################################

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

SRC_DIR = $(LOCAL_PATH)/src
INC_DIR = -I$(LOCAL_PATH)/include \
		-I$(SRC_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/libev/include \
		-I$(PREBUILD_3RD_PARTY_DIR)/json-c/include

LOCAL_TARGET	:= libdevice_agent.a
LOCAL_SRCS := $(SRC_DIR)/defs.c \
		$(SRC_DIR)/queue.c	\
		$(SRC_DIR)/channel.c	\
		$(SRC_DIR)/impl_dev.c	\
		$(SRC_DIR)/agent_dev.c	\
		$(SRC_DIR)/proc_dev.c

LOCAL_CFLAGS   := $(INC_DIR) -Wall -O2 -std=c99 -ffast-math -Wno-unused-parameter -fno-strict-aliasing -Wno-missing-field-initializers
ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_CFLAGS += -DAUDIO_INPUT_ALSA_SUPPORTED=1
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


########################################################
# test_device_agent
########################################################
include $(CLEAR_VARS)

SRC_DIR = $(LOCAL_PATH)/src
INC_DIR = -I$(LOCAL_PATH)/include

IPCAM_APP_BIN_DIR = $(FAKEROOT_DIR)/usr/bin
IPCAM_APP_LOCAL_BIN_DIR := $(FAKEROOT_DIR)/usr/local/bin

LOCAL_TARGET	:= test_device_agent
LOCAL_SRCS	:= $(LOCAL_PATH)/test_device_agent.c
LOCAL_CFLAGS  	:= $(INC_DIR) -Wall -O2 -std=c99 -ffast-math -Wno-unused-parameter -fno-strict-aliasing -Wno-missing-field-initializers
LOCAL_LIBS	:= libdevice_agent.a
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib \
		-L$(PREBUILD_3RD_PARTY_DIR)/libev/usr/lib \
		-lev -ljson-c  -lpthread -lm \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
