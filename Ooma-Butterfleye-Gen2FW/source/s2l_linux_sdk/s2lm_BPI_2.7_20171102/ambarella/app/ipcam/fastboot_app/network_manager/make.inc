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
ELEKTRA_UTILS_DIR	:= $(LOCAL_PATH)/../utils
COOEE_DIR	             := $(LOCAL_PATH)/cooee
DEVICE_AGENT		:= $(LOCAL_PATH)/../device_agent
BRCM_DIR		:= $(LOCAL_PATH)/../../wowl/brcm/inc
WOWL_DIR		:= $(LOCAL_PATH)/../../wowl/inc

LOCAL_TARGET	:= libbpiNetManager.so
LOCAL_SRCS         := $(wildcard $(LOCAL_PATH)/src/*.c)
LOCAL_SRCS	+= $(wildcard $(COOEE_DIR)/*.c)
LOCAL_SRCS	+= $(wildcard $(COOEE_DIR)/proto/*.c)
LOCAL_SRCS	+= $(wildcard $(COOEE_DIR)/easy_setup/*.c)

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) \
	-I$(BRCM_DIR) \
	-I$(WOWL_DIR) \
	-I$(ELEKTRA_UTILS_DIR) \
	-I$(COOEE_DIR) \
	-I$(COOEE_DIR)/proto \
	-I$(COOEE_DIR)/easy_setup \
	-I$(PREBUILD_3RD_PARTY_DIR)/libpcap/include \
	-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \


LOCAL_LDFLAGS	:= -lpthread -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib/ -liniparser -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_SO_NAME := $(LOCAL_TARGET)

LOCAL_LIBS := libaplayAudio.so libbpiUtils.so libbrcmioc.so


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
