#
# make.inc
#
# History:
#       2016/09/13 - [j Yi] created file
#
# Copyright (c) 2016 Ambarella, Inc.
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

##############################################
#set_mcu_mode
##############################################
include $(CLEAR_VARS)

IPCAM_APP_BIN_DIR	:= $(FAKEROOT_DIR)/usr/bin

LOCAL_TARGET		:= set_mcu_mode
LOCAL_SRCS  		:= $(LOCAL_PATH)/set_mcu_mode.cpp\
			$(LOCAL_PATH)/../bpi_app/bpi_mcu_proxy.cpp

LOCAL_CFLAGS		:= -I$(LOCAL_PATH)/../bpi_app\
			-I$(LOCAL_PATH)/../utils
LOCAL_CFLAGS		+= -g -std=c++11 -Werror

LOCAL_LDFLAGS		:= -lpthread
LOCAL_LIBS          := libbpiUtils.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

ifeq ($(CONFIG_BPI_APP_HIBERNATION), y)
SKIP_STRESS_TEST := yes
endif

ifeq ($(CONFIG_BSP_BOARD_S3LM_DAREDEVIL), y)
SKIP_STRESS_TEST := yes
endif

ifeq ($(SKIP_STRESS_TEST), yes)
else
##############################################
#stress_test
##############################################


include $(CLEAR_VARS)

IPCAM_APP_BIN_DIR	:= $(FAKEROOT_DIR)/usr/bin
UTILS_DIR		:= $(LOCAL_PATH)/../utils
AGENT_DIR		:= $(LOCAL_PATH)/../device_agent
NET_DIR			:= $(LOCAL_PATH)/../network_manager
APP_DIR		:= $(LOCAL_PATH)/../bpi_app

LOCAL_TARGET		:= stress_test
LOCAL_SRCS  		:= $(LOCAL_PATH)/stress_test.cpp\
			$(APP_DIR)/bpi_mcu_proxy.cpp\
			$(APP_DIR)/bpi_app_config.c\
			$(APP_DIR)/bpi_uploader.cpp\
			$(APP_DIR)/bpi_recorder.cpp\
			$(APP_DIR)/bpi_yuv_rotate.cpp\
			$(APP_DIR)/cloud_comm.c

LOCAL_CFLAGS		:= -I$(APP_DIR) \
			-I$(UTILS_DIR) \
			-I$(AGENT_DIR)/include \
			-I$(NET_DIR) \
			-I$(NET_DIR)/cooee \
			-I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include \
			-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include

LOCAL_CFLAGS  		+= -Werror -g -std=c++11

LOCAL_CFLAGS 		+= -I$(ORYX_DIR)/include/utility \
			-I$(ORYX_DIR)/event/include \
			-I$(ORYX_DIR)/video/include \
			-I$(ORYX_DIR)/include/event \
			-I$(ORYX_DIR)/include/video \
			-I$(ORYX_DIR)/include/services \
			-I$(ORYX_DIR)/include/services/commands \
			-I$(ORYX_DIR)/include/ipc        \
			-I$(ORYX_DIR)/include/utility/jpeg_encoder \
			-I$(ORYX_DIR)/include \
			-I$(ORYX_DIR)/include/configure

LOCAL_LDFLAGS		:= -lpthread \
			-L$(AGENT_DIR)/deps/lib -lev -lm\
			-L$(PREBUILD_3RD_PARTY_DIR)/libpcap/usr/lib -lpcap \
			-L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c \
			-L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl \
			-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib  \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
			-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib \
			-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_LDFLAGS		+= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(CONFIG_BSP_BOARD_S3LM_ELEKTRA), y)
LOCAL_CFLAGS  += -DELEKTRA_S
endif

LOCAL_LIBS := libbpiUtils.so libbpiNetManager.so \
			libdevice_agent.a libamvideo-reader.so libamvideo-address.so \
			libamutil.so libamosal.so libamjpeg-encoder.so libamapi.so libamipc.so \
			libammedia-data-struct.so libamconfig.so

include $(BUILD_APP)

LOCAL_SCRIPT	:= $(wildcard $(LOCAL_PATH)/*.sh)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): bpi_app

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@mkdir -p  $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(LOCAL_SCRIPT) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
