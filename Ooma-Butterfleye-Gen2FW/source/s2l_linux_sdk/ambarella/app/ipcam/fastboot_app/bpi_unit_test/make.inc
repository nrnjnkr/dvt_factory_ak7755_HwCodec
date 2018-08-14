#
# make.inc
#
# History:
#       2017/04/24 - [jyi] created file
#
# Copyright (c) 2017 Ambarella, Inc.
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
##################################################
# Notice: put any bpi unit test into this folder #
##################################################

ifeq ($(BUILD_AMBARELLA_BPI_UNIT_TEST), y)

LOCAL_PATH   := $(call my-dir)
IPCAM_APP_BIN_DIR	:= $(FAKEROOT_DIR)/usr/bin
IPCAM_SCRIPT_DIR	:= $(FAKEROOT_DIR)/usr/local/bin/
BPI_APP_DIR			:= $(LOCAL_PATH)/../bpi_app
BPI_UTILS_DIR	:= $(LOCAL_PATH)/../utils
BPI_ADC_DIR	:= $(BPI_UTILS_DIR)/adc

ifeq ($(CONFIG_ELEKTRA_FAST_BOOT), y)
D_FASTBOOT_TEST = yes
endif
ifeq ($(CONFIG_ELEKTRAS_FAST_BOOT), y)
D_FASTBOOT_TEST = yes
endif
ifeq ($(CONFIG_ELEKTRA_V_FAST_BOOT), y)
D_FASTBOOT_TEST = yes
endif
ifeq ($(CONFIG_BTFL_FAST_BOOT),y)
D_FASTBOOT_TEST = yes
endif
ifeq ($(D_FASTBOOT_TEST), yes)
##############################################
#bpi_test_streaming
##############################################
include $(CLEAR_VARS)

LOCAL_SRCS		:= $(LOCAL_PATH)/bpi_test_streaming.cpp \
			$(BPI_APP_DIR)/bpi_oryx_config.cpp \
			$(BPI_APP_DIR)/streaming_rtp_over_udp.cpp

LOCAL_TARGET	:= bpi_test_streaming

LOCAL_CFLAGS	:= -std=c++11
LOCAL_CFLAGS	+= -I$(BPI_APP_DIR) \
		-I$(BPI_UTILS_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include

LOCAL_CFLAGS	+= -I$(ORYX_DIR)/include \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/video \
		-I$(ORYX_DIR)/include/audio \
		-I$(ORYX_DIR)/include/image_quality \
		-I$(ORYX_DIR)/include/audio/capture \
		-I$(ORYX_DIR)/include/stream \
		-I$(ORYX_DIR)/include/configure \
		-I$(ORYX_DIR)/include/audio/codec

LOCAL_CFLAGS  += -D$(BOARD_TYPE)

LOCAL_LDFLAGS	:= -lpthread \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib


LOCAL_LIBS		:= libbpiUtils.so
LOCAL_LIBS += libamstream-record.so libamstream-playback.so libamstream-amf.so libamvideo.so libamaudio-capture.so
LOCAL_LIBS += libamvideo-reader.so libamvideo-address.so libamiq.so libamutil.so libamosal.so libamconfig.so


include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): bpi_app
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##############################################
#bpi_test_recorder
##############################################
include $(CLEAR_VARS)

LOCAL_SRCS		:= $(LOCAL_PATH)/bpi_test_recorder.cpp \
			$(BPI_APP_DIR)/bpi_app_config.c \
			$(BPI_APP_DIR)/bpi_oryx_export.cpp \
			$(BPI_APP_DIR)/bpi_oryx_config.cpp \
			$(BPI_APP_DIR)/bpi_uploader.cpp \
			$(BPI_APP_DIR)/bpi_yuv_rotate.c

LOCAL_TARGET	:= bpi_test_recorder

LOCAL_CFLAGS	:= -std=c++11 -Werror
LOCAL_CFLAGS	+= -I$(BPI_APP_DIR) \
		-I$(BPI_UTILS_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include

LOCAL_CFLAGS	+= -I$(ORYX_DIR)/include \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/video \
		-I$(ORYX_DIR)/include/audio \
		-I$(ORYX_DIR)/include/image_quality \
		-I$(ORYX_DIR)/include/audio/capture \
		-I$(ORYX_DIR)/include/stream \
		-I$(ORYX_DIR)/include/configure \
		-I$(ORYX_DIR)/include/utility/jpeg_encoder \
		-I$(ORYX_DIR)/include/audio/codec \
		-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include

LOCAL_CFLAGS  += -D$(BOARD_TYPE)

LOCAL_LDFLAGS	:= -lpthread \
		-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
		-L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnghttp2/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libunistring/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib

LOCAL_LIBS	:= libbpiUtils.so
LOCAL_LIBS	+= libamvideo.so libamaudio-capture.so libamvideo-reader.so \
		libamvideo-address.so libamiq.so libamutil.so libamosal.so \
		libamjpeg-encoder.so libamvideo-utility.so libamconfig.so \
		libamstream-record.so libamstream-amf.so

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): bpi_app
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##############################################
#bpi_test_audio_playback
##############################################
include $(CLEAR_VARS)

LOCAL_SRCS		:= $(LOCAL_PATH)/bpi_test_audio_playback.cpp \
			$(BPI_APP_DIR)/bpi_oryx_config.cpp \
			$(BPI_APP_DIR)/streaming_audio_playback.cpp \
			$(BPI_APP_DIR)/bpi_app_config.c

LOCAL_TARGET	:= bpi_test_audio_playback

LOCAL_CFLAGS	:= -std=c++11 -Werror
LOCAL_CFLAGS	+= -I$(BPI_APP_DIR) \
		-I$(BPI_UTILS_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include

LOCAL_CFLAGS	+= -I$(ORYX_DIR)/include \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/video \
		-I$(ORYX_DIR)/include/audio \
		-I$(ORYX_DIR)/include/image_quality \
		-I$(ORYX_DIR)/include/stream \
		-I$(ORYX_DIR)/include/configure

LOCAL_CFLAGS  += -D$(BOARD_TYPE)

LOCAL_LDFLAGS	:= -lpthread \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
		-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser

LOCAL_LIBS	:= libbpiUtils.so
LOCAL_LIBS	+= libamvideo.so libamstream-amf.so libamutil.so libamstream-playback.so \
		libamvideo-utility.so libamconfig.so libaplayAudio.so libamosal.so libamiq.so

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): bpi_app prebuild_alsa
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##############################################
#bpi_test_adc
##############################################
include $(CLEAR_VARS)

LOCAL_SRCS		:= $(LOCAL_PATH)/bpi_test_adc.cpp \
			$(BPI_APP_DIR)/bpi_app_config.c

LOCAL_TARGET	:= bpi_test_adc

LOCAL_CFLAGS	:= -std=c++11 -Werror
LOCAL_CFLAGS	+= -I$(BPI_APP_DIR) \
		-I$(BPI_UTILS_DIR) \
		-I$(BPI_ADC_DIR) \
		-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include

LOCAL_LDFLAGS	:= -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
		-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser

LOCAL_LIBS	:= libbpiUtils.so libbpiAdc.so

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif  #end of D_FASTBOOT_TEST

endif
