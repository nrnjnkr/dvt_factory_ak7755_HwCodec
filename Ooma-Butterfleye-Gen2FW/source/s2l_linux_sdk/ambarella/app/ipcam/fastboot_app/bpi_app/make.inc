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


ifeq ($(CONFIG_BSP_BOARD_S3LM_ELEKTRA), y)
BOARD_TYPE = ELEKTRA_S
else ifeq ($(CONFIG_BSP_BOARD_S2LM_ELEKTRA), y)
BOARD_TYPE = ELEKTRA_II
else ifeq ($(CONFIG_BSP_BOARD_S5LM_ELEKTRA_V), y)
BOARD_TYPE = ELEKTRA_V
else ifeq ($(CONFIG_BSP_BOARD_S3LM_DAREDEVIL), y)
BOARD_TYPE = DAREDEVIL
else ifeq ($(CONFIG_BSP_BOARD_BTFL), y)
BOARD_TYPE = BTFL
endif

ifeq ($(CONFIG_BPI_APP_HIBERNATION), y)
###############################################
####     hibernation apps
################################################
include $(CLEAR_VARS)

IPCAM_APP_BIN_DIR	:= $(FAKEROOT_DIR)/usr/bin
UTILS_DIR		:= $(LOCAL_PATH)/../utils
AGENT_DIR		:= $(LOCAL_PATH)/../device_agent
NET_DIR			:= $(LOCAL_PATH)/../network_manager
BPI_CONFIG_FILE		:= $(FAKEROOT_DIR)/etc/bpi/setting.ini
VCA_DIR		:= $(LOCAL_PATH)/../vca

LOCAL_TARGET	:= bpi_app
LOCAL_SRCS	:= $(LOCAL_PATH)/hibernate_main.cpp \
		$(LOCAL_PATH)/cloud_comm.c \
		$(UTILS_DIR)/xmodem/xmodem.c \
		$(LOCAL_PATH)/bpi_app_config.c\
		$(LOCAL_PATH)/bpi_mcu_proxy.cpp\
		$(LOCAL_PATH)/bpi_uploader.cpp\
		$(LOCAL_PATH)/bpi_oryx_export.cpp\
		$(LOCAL_PATH)/bpi_oryx_config.cpp\
		$(LOCAL_PATH)/bpi_yuv_rotate.cpp\
		$(LOCAL_PATH)/streaming_rtp_over_udp.cpp\
		$(LOCAL_PATH)/streaming_audio_playback.cpp\
		$(LOCAL_PATH)/bpi_mcu_event_monitor.cpp

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) \
		-I$(UTILS_DIR) \
		-I$(UTILS_DIR)/xmodem \
		-I$(AGENT_DIR)/include \
		-I$(NET_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include \
		-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include
LOCAL_CFLAGS	+= -I$(VCA_DIR)

LOCAL_CFLAGS	+= -I$(ORYX_DIR)/include \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/event \
		-I$(ORYX_DIR)/include/video \
		-I$(ORYX_DIR)/include/stream \
		-I$(ORYX_DIR)/include/utility/jpeg_encoder \
		-I$(ORYX_DIR)/include/configure \
		-I$(ORYX_DIR)/include/audio \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/image_quality \
		-I$(ORYX_DIR)/include/audio/capture \
		-I$(ORYX_DIR)/include/audio/codec

LOCAL_CFLAGS  += -D$(BOARD_TYPE)

LOCAL_CFLAGS	+= -std=c++11 -Werror
LOCAL_LDFLAGS	:= -lpthread  -lm\
		-L$(PREBUILD_3RD_PARTY_DIR)/libpcap/usr/lib -lpcap \
		-L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c \
		-L$(PREBUILD_3RD_PARTY_DIR)/libev/usr/lib -lev \
		-L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl \
		-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnghttp2/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libunistring/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib

ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_LDFLAGS	+= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

LOCAL_LIBS := libbpiUtils.so libbpiNetManager.so libaplayAudio.so libdevice_agent.a libbpiUbi.so libbpiAdc.so
LOCAL_LIBS += libamstream-record.so libamstream-playback.so libamstream-amf.so libamvideo.so libamaudio-capture.so libamjpeg-encoder.so
LOCAL_LIBS += libamvideo-reader.so libamvideo-address.so libamiq.so libamutil.so libamosal.so libamconfig.so
LOCAL_LIBS += libvca.so

ifeq ($(AMBOOT_AUDIO_16000), y)
FAST_AUDIO_RATE	:= 16000
else ifeq ($(AMBOOT_AUDIO_8000), y)
FAST_AUDIO_RATE	:= 8000
else
FAST_AUDIO_RATE	:= 48000
endif

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): add_board_rootfs

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@sed -i '/auto-generated/ d' $(BPI_CONFIG_FILE)
	@sed -i '/\[audio\]/a sample_rate=$(FAST_AUDIO_RATE); auto-generated for fastboot audio. Non-editable' $(BPI_CONFIG_FILE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

########################################################
# config_reconnect
########################################################

include $(CLEAR_VARS)

LOCAL_TARGET		:= config_reconnect
IPCAM_APP_BIN_DIR	:= $(FAKEROOT_DIR)/usr/bin
IPCAM_APP_LOCAL_BIN_DIR	:= $(FAKEROOT_DIR)/usr/local/bin
DEVICE_AGENT		:=$(LOCAL_PATH)/../device_agent

LOCAL_SRCS	:= $(LOCAL_PATH)/config_reconnect.c \
		$(LOCAL_PATH)/bpi_app_config.c \
		$(LOCAL_PATH)/cloud_comm.c

LOCAL_CFLAGS  	:= -I$(LOCAL_PATH)/../network_manager\
	-I$(LOCAL_PATH)/../utils \
	-I$(DEVICE_AGENT)/include\
	-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
	-I$(LOCAL_PATH)/../record/export

LOCAL_LDFLAGS	:= -lpthread -lm  \
	-lbpiNetManager -laplayAudio -lbpiUtils  \
	-L$(PREBUILD_3RD_PARTY_DIR)/libpcap/usr/lib -lpcap \
	-L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c \
	-L$(PREBUILD_3RD_PARTY_DIR)/libev/usr/lib -lev \
	-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
	-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
	-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_LDFLAGS	+= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

LOCAL_LIBS	:= libbpiNetManager.so libaplayAudio.so libbpiUtils.so libdevice_agent.a


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@mkdir -p $(IPCAM_APP_LOCAL_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
