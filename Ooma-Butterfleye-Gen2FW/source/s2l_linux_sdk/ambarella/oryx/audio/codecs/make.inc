##
## /S2l/oryx/stream/common/codecs/make.inc
##
## History:
##   2014-11-3 - [ypchang] created file
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

LOCAL_PATH := $(call my-dir)

### libamcodec-base.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_BASE), y)
include $(CLEAR_VARS)

CODEC_BASE = libamcodec-base.so

LOCAL_TARGET := $(CODEC_BASE)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/base/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base              \
                -I$(ORYX_DIR)/include             \
                -I$(ORYX_DIR)/include/audio       \
                -I$(ORYX_DIR)/include/audio/codec \
                -I$(ORYX_DIR)/include/utility     \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

CODEC_BASE_NAME   = $(CODEC_BASE)
CODEC_BASE_TARGET = $(CODEC_BASE)

LOCAL_SO_NAME := $(CODEC_BASE)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(CODEC_BASE_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-aac.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_AAC), y)
include $(CLEAR_VARS)

CODEC_AAC  = codec-aac.so
CODEC_AAC_48K = codec-aac-48k.so
CODEC_AAC_16K = codec-aac-16k.so
CODEC_AAC_8K  = codec-aac-8k.so

AAC_CONFIG_48K = codec-aac-48k.acs
AAC_CONFIG_16K = codec-aac-16k.acs
AAC_CONFIG_8K  = codec-aac-8k.acs
AAC_CONFIG     = codec-aac.acs

LOCAL_TARGET := $(CODEC_AAC)
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_AAC_OLD), y)
AAC_SRC_DIR := $(LOCAL_PATH)/codec-aac
AAC_LIB_DIR := $(PREBUILD_3RD_PARTY_DIR)/aac
else
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_AAC_NEW), y)
AAC_SRC_DIR := $(LOCAL_PATH)/codec-aac-new
AAC_LIB_DIR := $(PREBUILD_3RD_PARTY_DIR)/aac-new
endif
endif

LOCAL_SRCS   := $(wildcard $(AAC_SRC_DIR)/*.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                \
                -I$(AAC_SRC_DIR)                    \
                -I$(ORYX_DIR)/include               \
                -I$(ORYX_DIR)/include/audio         \
                -I$(ORYX_DIR)/include/audio/codec   \
                -I$(ORYX_DIR)/include/utility       \
                -I$(ORYX_DIR)/include/audio/utility \
                -I$(ORYX_DIR)/include/configure     \
                -I$(AAC_LIB_DIR)/include            \
                -fvisibility=hidden                 \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(AAC_LIB_DIR)/lib -laacenc -laacdec

CODEC_AAC_NAME   = $(CODEC_AAC)
CODEC_AAC_TARGET = $(CODEC_AAC)

LOCAL_SO_NAME := $(CODEC_AAC)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): AAC_SRC_PATH := $(AAC_SRC_DIR)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(AAC_SRC_PATH)/$(AAC_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG_48K)
	@cp -dpRf $(AAC_SRC_PATH)/$(AAC_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG_16K)
	@cp -dpRf $(AAC_SRC_PATH)/$(AAC_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG_8K)
	@ln -sf $(AAC_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(AAC_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_AAC_TARGET)
	@ln -sf $(CODEC_AAC_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_AAC_48K)
	@ln -sf $(CODEC_AAC_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_AAC_16K)
	@ln -sf $(CODEC_AAC_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_AAC_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-g711.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_G711), y)
include $(CLEAR_VARS)

CODEC_G711  = codec-g711.so

CODEC_G711_16K = codec-g711-16k.so
CODEC_G711_8K  = codec-g711-8k.so

G711_CONFIG_16K = codec-g711-16k.acs
G711_CONFIG_8K  = codec-g711-8k.acs
G711_CONFIG     = codec-g711.acs

LOCAL_TARGET := $(CODEC_G711)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-g711/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                        \
                -I$(LOCAL_PATH)/codec-g711                  \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/audio                 \
                -I$(ORYX_DIR)/include/audio/codec           \
                -I$(ORYX_DIR)/include/audio/utility         \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/configure             \
                -I$(PREBUILD_3RD_PARTY_DIR)/libg7xx/include \
                -fvisibility=hidden                         \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/libg7xx/usr/lib -lg7xx

CODEC_G711_NAME   = $(CODEC_G711)
CODEC_G711_TARGET = $(CODEC_G711)

LOCAL_SO_NAME := $(CODEC_G711)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): G711_SRC_PATH := $(LOCAL_PATH)/codec-g711
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(G711_SRC_PATH)/$(G711_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(G711_CONFIG_16K)
	@cp -dpRf $(G711_SRC_PATH)/$(G711_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(G711_CONFIG_8K)
	@ln -sf $(G711_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(G711_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_G711_TARGET)
	@ln -sf $(CODEC_G711_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_G711_8K)
	@ln -sf $(CODEC_G711_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_G711_16K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-g726.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_G726), y)
include $(CLEAR_VARS)

CODEC_G726  = codec-g726.so

CODEC_G726_8K  = codec-g726-8k.so
G726_CONFIG_8K = codec-g726-8k.acs
G726_CONFIG    = codec-g726.acs

LOCAL_TARGET := $(CODEC_G726)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-g726/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                        \
                -I$(LOCAL_PATH)/codec-g726                  \
                -I$(ORYX_DIR)/include                       \
                -I$(ORYX_DIR)/include/audio                 \
                -I$(ORYX_DIR)/include/audio/codec           \
                -I$(ORYX_DIR)/include/audio/utility         \
                -I$(ORYX_DIR)/include/utility               \
                -I$(ORYX_DIR)/include/configure             \
                -I$(PREBUILD_3RD_PARTY_DIR)/libg7xx/include \
                -fvisibility=hidden                         \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/libg7xx/usr/lib -lg7xx

CODEC_G726_NAME   = $(CODEC_G726)
CODEC_G726_TARGET = $(CODEC_G726)

LOCAL_SO_NAME := $(CODEC_G726)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): libamcodec-base.so
$(LOCAL_TARGET): G726_SRC_PATH := $(LOCAL_PATH)/codec-g726
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(G726_SRC_PATH)/$(G726_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(G726_CONFIG_8K)
	@ln -sf $(G726_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(G726_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_G726_TARGET)
	@ln -sf $(CODEC_G726_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_G726_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-mp3.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_MP3), y)
include $(CLEAR_VARS)

CODEC_MP3  = codec-mp3.so

CODEC_MP3_48K  = codec-mp3-48k.so
CODEC_MP3_16K  = codec-mp3-16k.so
CODEC_MP3_8K   = codec-mp3-8k.so

MP3_CONFIG_48K = codec-mp3-48k.acs
MP3_CONFIG_16K = codec-mp3-16k.acs
MP3_CONFIG_8K  = codec-mp3-8k.acs
MP3_CONFIG     = codec-mp3.acs

LOCAL_TARGET := $(CODEC_MP3)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-mp3/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                       \
                -I$(LOCAL_PATH)/codec-mp3                  \
                -I$(ORYX_DIR)/include                      \
                -I$(ORYX_DIR)/include/audio                \
                -I$(ORYX_DIR)/include/audio/codec          \
                -I$(ORYX_DIR)/include/audio/utility        \
                -I$(ORYX_DIR)/include/utility              \
                -I$(ORYX_DIR)/include/configure            \
                -I$(ORYX_DIR)/stream/include/common/media  \
                -I$(PREBUILD_3RD_PARTY_DIR)/mpg123/include \
                -fvisibility=hidden                        \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/mpg123/usr/lib -lmpg123 -lm

CODEC_MP3_NAME   = $(CODEC_MP3)
CODEC_MP3_TARGET = $(CODEC_MP3)

LOCAL_SO_NAME := $(CODEC_MP3)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): MP3_SRC_PATH := $(LOCAL_PATH)/codec-mp3
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
#	@cp -dpRf $(MP3_SRC_PATH)/$(MP3_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(MP3_CONFIG_48K)
#	@cp -dpRf $(MP3_SRC_PATH)/$(MP3_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(MP3_CONFIG_16K)
#	@cp -dpRf $(MP3_SRC_PATH)/$(MP3_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(MP3_CONFIG_8K)
#	@ln -sf $(MP3_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(MP3_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_MP3_TARGET)
	@ln -sf $(CODEC_MP3_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_MP3_48K)
	@ln -sf $(CODEC_MP3_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_MP3_16K)
	@ln -sf $(CODEC_MP3_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_MP3_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-opus.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_OPUS), y)
include $(CLEAR_VARS)

CODEC_OPUS  = codec-opus.so

CODEC_OPUS_48K  = codec-opus-48k.so
CODEC_OPUS_16K  = codec-opus-16k.so
CODEC_OPUS_8K   = codec-opus-8k.so

OPUS_CONFIG_48K = codec-opus-48k.acs
OPUS_CONFIG_16K = codec-opus-16k.acs
OPUS_CONFIG_8K  = codec-opus-8k.acs
OPUS_CONFIG     = codec-opus.acs

LOCAL_TARGET := $(CODEC_OPUS)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-opus/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                     \
                -I$(LOCAL_PATH)/codec-opus               \
                -I$(ORYX_DIR)/include                    \
                -I$(ORYX_DIR)/include/audio              \
                -I$(ORYX_DIR)/include/audio/codec        \
                -I$(ORYX_DIR)/include/audio/utility      \
                -I$(ORYX_DIR)/include/utility            \
                -I$(ORYX_DIR)/include/configure          \
                -I$(PREBUILD_3RD_PARTY_DIR)/opus/include \
                -fvisibility=hidden                      \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/opus/usr/lib -lopus -lm

CODEC_OPUS_NAME   = $(CODEC_OPUS)
CODEC_OPUS_TARGET = $(CODEC_OPUS)

LOCAL_SO_NAME := $(CODEC_OPUS)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): OPUS_SRC_PATH := $(LOCAL_PATH)/codec-opus
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(OPUS_SRC_PATH)/$(OPUS_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG_48K)
	@cp -dpRf $(OPUS_SRC_PATH)/$(OPUS_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG_16K)
	@cp -dpRf $(OPUS_SRC_PATH)/$(OPUS_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG_8K)
	@ln -sf $(OPUS_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(OPUS_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_OPUS_TARGET)
	@ln -sf $(CODEC_OPUS_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_OPUS_48K)
	@ln -sf $(CODEC_OPUS_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_OPUS_16K)
	@ln -sf $(CODEC_OPUS_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_OPUS_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

### codec-speex.so
ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CODEC_SPEEX), y)
include $(CLEAR_VARS)

CODEC_SPEEX  = codec-speex.so

CODEC_SPEEX_48K = codec-speex-48k.so
CODEC_SPEEX_16K = codec-speex-16k.so
CODEC_SPEEX_8K  = codec-speex-8k.so

SPEEX_CONFIG_48K = codec-speex-48k.acs
SPEEX_CONFIG_16K = codec-speex-16k.acs
SPEEX_CONFIG_8K  = codec-speex-8k.acs
SPEEX_CONFIG     = codec-speex.acs

LOCAL_TARGET := $(CODEC_SPEEX)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/codec-speex/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)/base                            \
                -I$(LOCAL_PATH)/codec-speex                     \
                -I$(ORYX_DIR)/include                           \
                -I$(ORYX_DIR)/include/audio                     \
                -I$(ORYX_DIR)/include/audio/codec               \
                -I$(ORYX_DIR)/include/audio/utility             \
                -I$(ORYX_DIR)/include/utility                   \
                -I$(ORYX_DIR)/include/configure                 \
                -I$(PREBUILD_3RD_PARTY_DIR)/speex/include/speex \
                -fvisibility=hidden                             \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so       \
              libamconfig.so     \
              libamcodec-base.so \
              libamaudio-utility.so

LOCAL_LDFLAGS = -L$(PREBUILD_3RD_PARTY_DIR)/speex/usr/lib -lspeex

CODEC_SPEEX_NAME   = $(CODEC_SPEEX)
CODEC_SPEEX_TARGET = $(CODEC_SPEEX)

LOCAL_SO_NAME := $(CODEC_SPEEX)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): SPEEX_SRC_PATH := $(LOCAL_PATH)/codec-speex
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_CODEC_DIR)
	@mkdir -p $(ORYX_CONF_DIR)/stream/codec
	@cp -dpRf $(SPEEX_SRC_PATH)/$(SPEEX_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG_48K)
	@cp -dpRf $(SPEEX_SRC_PATH)/$(SPEEX_CONFIG_16K) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG_16K)
	@cp -dpRf $(SPEEX_SRC_PATH)/$(SPEEX_CONFIG_8K) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG_8K)
	@ln -sf $(SPEEX_CONFIG_48K) $(ORYX_CONF_DIR)/stream/codec/$(SPEEX_CONFIG)
	@cp -dpRf $< $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_TARGET)
	@ln -sf $(CODEC_SPEEX_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_48K)
	@ln -sf $(CODEC_SPEEX_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_16K)
	@ln -sf $(CODEC_SPEEX_TARGET) $(ORYX_CODEC_DIR)/$(CODEC_SPEEX_8K)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
