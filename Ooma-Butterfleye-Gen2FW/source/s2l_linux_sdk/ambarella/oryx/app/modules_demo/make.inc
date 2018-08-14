##
## oryx/app/modules_demo/make.inc
##
## History:
##   Feb 9, 2017 - [Huaiqing Wang] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_MODULES_DEMO), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET :=	test_oryx_modules

LOCAL_SRCS := $(LOCAL_PATH)/modules.cpp \
              $(LOCAL_PATH)/netlink.cpp \
              $(LOCAL_PATH)/test_modules.cpp

LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/image_quality  \
                 -I$(ORYX_DIR)/image_quality/include \
                 -I$(ORYX_DIR)/include/stream \
                 -I$(ORYX_DIR)/include/video  \
                 -I$(ORYX_DIR)/include/audio  \
                 -I$(ORYX_DIR)/include/audio/device  \
                 -I$(ORYX_DIR)/include/audio/capture  \
                 -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                 -I$(LOCAL_PATH)              \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib	\
                 -Wl,-rpath-link=$(AMB_TOPDIR)/prebuild/imgproc/img_lib

LOCAL_LIBS    :=  libamutil.so          \
                  libamosal.so          \
                  libamiq.so            \
                  libamstream-amf.so    \
                  libamstream-record.so \
                  libamaudio-capture.so \
                  libamvideo.so         \
                  libamvideo-reader.so  \
                  libamvideo-address.so \
                  libamvideo-utility.so

ifeq ($(BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE), y)
LOCAL_LDFLAGS += -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libsndfile/usr/lib   \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/xz/usr/lib           \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgcrypt/usr/lib    \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgpg-error/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/systemd/usr/lib      \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib   \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib/pulseaudio
LOCAL_LIBS    +=  libamaudio-device.so
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
