#
# make.inc
#
# History:
#       2016/06/01 - [cz Lin] created file
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

LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

IPCAM_APP_LIB_DIR	:= $(FAKEROOT_DIR)/usr/lib
INLCUDE			:= $(AMB_TOPDIR)/include

LOCAL_TARGET	:= libbpiUtils.so
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/*.c*)

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) \
		-I$(INLCUDE)
LOCAL_CFLAGS	+= -std=c++11 -Werror
LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###################
include $(CLEAR_VARS)
IPCAM_APP_LIB_DIR	:= $(FAKEROOT_DIR)/usr/lib
INLCUDE			:= $(AMB_TOPDIR)/include

LOCAL_TARGET	:= libaplayAudio.so
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/aplay/*.c)

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) \
	-I$(INLCUDE)

LOCAL_LDFLAGS	:= -lpthread
ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_CFLAGS	+= -DAUDIO_INPUT_ALSA_SUPPORTED=1 -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include
LOCAL_LDFLAGS	+= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(CONFIG_S2LMELEKTRA_WM8974), y)
LOCAL_CFLAGS  += -DCONFIG_ELEKTRA_CODEC_WM8974
else ifeq ($(CONFIG_S2LMBTFL_AK7755), y)
LOCAL_CFLAGS  += -DCONFIG_BTFL_CODEC_AK7755
endif

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

ifeq ($(CONFIG_BPI_APP_HIBERNATION), y)
###################
## libbpiUbi.so
###################
include $(CLEAR_VARS)
IPCAM_APP_LIB_DIR	:= $(FAKEROOT_DIR)/usr/lib

LOCAL_TARGET	:= libbpiUbi.so
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/ubi/*.c)

LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/ubi
LOCAL_SO_NAME := $(LOCAL_TARGET)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###################
## libbpiAdc.so
###################
include $(CLEAR_VARS)
IPCAM_APP_LIB_DIR	:= $(FAKEROOT_DIR)/usr/lib

LOCAL_TARGET	:= libbpiAdc.so
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/adc/*.c)


LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/adc \
		-I$(AMB_TOPDIR)/amboot/include \
		-I$(AMB_TOPDIR)/app/utility/upgrade_partition

LOCAL_CFLAGS	+= -Werror
LOCAL_SO_NAME := $(LOCAL_TARGET)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

