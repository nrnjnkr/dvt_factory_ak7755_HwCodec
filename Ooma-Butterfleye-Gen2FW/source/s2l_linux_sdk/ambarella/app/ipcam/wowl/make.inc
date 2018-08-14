##
## app/ipcam/wowl/make.inc
##
## History:
##    2015/04/03 - [Tao Wu] Created file
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

ifeq ($(BUILD_AMBARELLA_APP_WIFI_UTILITY), y)

WOWL_PATH   := $(call my-dir)

WOWL_APP_DIR = $(FAKEROOT_DIR)/usr/bin
WOWL_LIB_DIR = $(FAKEROOT_DIR)/usr/lib

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_LIBRARY_BRCM), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= libbrcmioc.so
LOCAL_SRCS	:= $(WOWL_PATH)/brcm/brcm_ioc.c
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(WOWL_PATH)/brcm/inc

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_LIB_DIR)/
	@cp -dpRf $< $(WOWL_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_LIBRARY_BRCM

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_LIBRARY_MRVL), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= libmrvlioc.so
LOCAL_SRCS	:= $(WOWL_PATH)/mrvl/mrvl_ioc.c
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(WOWL_PATH)/mrvl/inc

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_LIB_DIR)/
	@cp -dpRf $< $(WOWL_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_LIBRARY_MRVL

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= wowl
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(PREBUILD_3RD_PARTY_DIR)/libpcap/include

ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL_BRCM), y)
LOCAL_SRCS	:= $(WOWL_PATH)/wowl.c
LOCAL_LIBS   	:= libbrcmioc.so
LOCAL_CFLAGS 	+= -I$(WOWL_PATH)/brcm/inc
else ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL_MRVL), y)
LOCAL_SRCS	:= $(WOWL_PATH)/wowl_mrvl.c
LOCAL_LIBS   	:= libmrvlioc.so
LOCAL_CFLAGS 	+= -I$(WOWL_PATH)/mrvl/inc
endif

LOCAL_LDFLAGS 	:= -L$(PREBUILD_3RD_PARTY_DIR)/libpcap/usr/lib -lpcap \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib \
		-lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_APP_DIR)/
	@cp -dpRf $< $(WOWL_APP_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_WOWL

endif ## BUILD_AMBARELLA_APP_WIFI_UTILITY
