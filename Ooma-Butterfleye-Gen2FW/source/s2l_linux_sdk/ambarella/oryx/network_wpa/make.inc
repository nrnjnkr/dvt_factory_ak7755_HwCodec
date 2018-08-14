##
## oryx/network_wpa/make.inc
##
## History:
##    2016/04/18 - [Long Li] Created file
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

LOCAL_PATH   := $(call my-dir)

## libamarchive.so
ifeq ($(BUILD_AMBARELLA_ORYX_NETWORK_WPA), y)

include $(CLEAR_VARS)

LIBAMIFCTRL_VER_FILE   = $(ORYX_DIR)/include/network_wpa/version.h
LIBAMIFCTRL_VER_PREFIX = NETWORK

LIBAMIFCTRL_TARGET   = libamifctrl.so
LOCAL_TARGET := $(LIBAMIFCTRL_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/manager/am_ifctrl.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/network_wpa/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so

LIBAMIFCTRL_SO_VER_MAJOR  := $(shell awk '/define\ $(LIBAMIFCTRL_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBAMIFCTRL_VER_FILE))
LIBAMIFCTRL_SO_VER_MINOR  := $(shell awk '/define\ $(LIBAMIFCTRL_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBAMIFCTRL_VER_FILE))
LIBAMIFCTRL_SO_VER_PATCH  := $(shell awk '/define\ $(LIBAMIFCTRL_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBAMIFCTRL_VER_FILE))
LIBAMIFCTRL_SO_VER_STRING := $(LIBAMIFCTRL_SO_VER_MAJOR).$(LIBAMIFCTRL_SO_VER_MINOR).$(LIBAMIFCTRL_SO_VER_PATCH)

LIBAMIFCTRL_SO_NAME   = $(patsubst %.so,%.so.$(LIBAMIFCTRL_SO_VER_MAJOR), $(LIBAMIFCTRL_TARGET))
LIBAMIFCTRL_SO_TARGET = $(patsubst %.so,%.so.$(LIBAMIFCTRL_SO_VER_STRING), $(LIBAMIFCTRL_TARGET))

LOCAL_SO_NAME := $(LIBAMIFCTRL_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBAMIFCTRL_SO_TARGET)
	@ln -sf $(LIBAMIFCTRL_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBAMIFCTRL_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

LIBAMDHCLIENT_VER_FILE   = $(ORYX_DIR)/include/network_wpa/version.h
LIBAMDHCLIENT_VER_PREFIX = NETWORK

LIBAMDHCLIENT_TARGET   = libamdhclient.so
LOCAL_TARGET := $(LIBAMDHCLIENT_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/manager/am_dhclient.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/network_wpa/manager \
                -I$(ORYX_DIR)/network_wpa/include \
                -std=c++11 -Werror

LOCAL_LIBS := libamutil.so libamifctrl.so

LIBAMDHCLIENT_SO_VER_MAJOR  := $(shell awk '/define\ $(LIBAMDHCLIENT_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBAMDHCLIENT_VER_FILE))
LIBAMDHCLIENT_SO_VER_MINOR  := $(shell awk '/define\ $(LIBAMDHCLIENT_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBAMDHCLIENT_VER_FILE))
LIBAMDHCLIENT_SO_VER_PATCH  := $(shell awk '/define\ $(LIBAMDHCLIENT_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBAMDHCLIENT_VER_FILE))
LIBAMDHCLIENT_SO_VER_STRING := $(LIBAMDHCLIENT_SO_VER_MAJOR).$(LIBAMDHCLIENT_SO_VER_MINOR).$(LIBAMDHCLIENT_SO_VER_PATCH)

LIBAMDHCLIENT_SO_NAME   = $(patsubst %.so,%.so.$(LIBAMDHCLIENT_SO_VER_MAJOR), $(LIBAMDHCLIENT_TARGET))
LIBAMDHCLIENT_SO_TARGET = $(patsubst %.so,%.so.$(LIBAMDHCLIENT_SO_VER_STRING), $(LIBAMDHCLIENT_TARGET))

LOCAL_SO_NAME := $(LIBAMDHCLIENT_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBAMDHCLIENT_SO_TARGET)
	@ln -sf $(LIBAMDHCLIENT_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBAMDHCLIENT_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

LIBAMWPACTRL_VER_FILE   = $(ORYX_DIR)/include/network_wpa/version.h
LIBAMWPACTRL_VER_PREFIX = NETWORK

LIBAMWPACTRL_TARGET   = libamwpactrl.so
LOCAL_TARGET := $(LIBAMWPACTRL_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/manager/am_wifi_manager.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network_wpa \
                -I$(ORYX_DIR)/network_wpa/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/wpa_supplicant/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/wpa_supplicant/usr/lib -lwpa_client

LOCAL_LIBS := libamutil.so libamifctrl.so libamdhclient.so

LIBAMWPACTRL_SO_VER_MAJOR  := $(shell awk '/define\ $(LIBAMWPACTRL_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBAMWPACTRL_VER_FILE))
LIBAMWPACTRL_SO_VER_MINOR  := $(shell awk '/define\ $(LIBAMWPACTRL_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBAMWPACTRL_VER_FILE))
LIBAMWPACTRL_SO_VER_PATCH  := $(shell awk '/define\ $(LIBAMWPACTRL_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBAMWPACTRL_VER_FILE))
LIBAMWPACTRL_SO_VER_STRING := $(LIBAMWPACTRL_SO_VER_MAJOR).$(LIBAMWPACTRL_SO_VER_MINOR).$(LIBAMWPACTRL_SO_VER_PATCH)

LIBAMWPACTRL_SO_NAME   = $(patsubst %.so,%.so.$(LIBAMWPACTRL_SO_VER_MAJOR), $(LIBAMWPACTRL_TARGET))
LIBAMWPACTRL_SO_TARGET = $(patsubst %.so,%.so.$(LIBAMWPACTRL_SO_VER_STRING), $(LIBAMWPACTRL_TARGET))

LOCAL_SO_NAME := $(LIBAMWPACTRL_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBAMWPACTRL_SO_TARGET)
	@ln -sf $(LIBAMWPACTRL_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBAMWPACTRL_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

LIBAMNETWORKMANAGER_VER_FILE   = $(ORYX_DIR)/include/network_wpa/version.h
LIBAMNETWORKMANAGER_VER_PREFIX = NETWORK
NETWORKMANAGER_CONFIG := $(wildcard $(LOCAL_PATH)/manager/*.acs)

LIBAMNETWORKMANAGER_TARGET   = libamnetworkmanager.so
LOCAL_TARGET := $(LIBAMNETWORKMANAGER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/manager/am_network_manager.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/network_wpa \
                -I$(ORYX_DIR)/network_wpa/include \
                -fvisibility=hidden -std=c++11 -Werror

LOCAL_LIBS := libamutil.so \
              libamconfig.so \
              libamifctrl.so \
              libamdhclient.so \
              libamwpactrl.so

LIBAMNETWORKMANAGER_SO_VER_MAJOR  := $(shell awk '/define\ $(LIBAMNETWORKMANAGER_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(LIBAMNETWORKMANAGER_VER_FILE))
LIBAMNETWORKMANAGER_SO_VER_MINOR  := $(shell awk '/define\ $(LIBAMNETWORKMANAGER_VER_PREFIX)_LIB_MINOR/{print $$3}' $(LIBAMNETWORKMANAGER_VER_FILE))
LIBAMNETWORKMANAGER_SO_VER_PATCH  := $(shell awk '/define\ $(LIBAMNETWORKMANAGER_VER_PREFIX)_LIB_PATCH/{print $$3}' $(LIBAMNETWORKMANAGER_VER_FILE))
LIBAMNETWORKMANAGER_SO_VER_STRING := $(LIBAMNETWORKMANAGER_SO_VER_MAJOR).$(LIBAMNETWORKMANAGER_SO_VER_MINOR).$(LIBAMNETWORKMANAGER_SO_VER_PATCH)

LIBAMNETWORKMANAGER_SO_NAME   = $(patsubst %.so,%.so.$(LIBAMNETWORKMANAGER_SO_VER_MAJOR), $(LIBAMNETWORKMANAGER_TARGET))
LIBAMNETWORKMANAGER_SO_TARGET = $(patsubst %.so,%.so.$(LIBAMNETWORKMANAGER_SO_VER_STRING), $(LIBAMNETWORKMANAGER_TARGET))

LOCAL_SO_NAME := $(LIBAMNETWORKMANAGER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(LIBAMNETWORKMANAGER_SO_TARGET)
	@ln -sf $(LIBAMNETWORKMANAGER_SO_TARGET) $(ORYX_LIB_DIR)/$(LIBAMNETWORKMANAGER_SO_NAME)
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/network
	@cp -dpRf $(NETWORKMANAGER_CONFIG) $(FAKEROOT_DIR)/etc/oryx/network/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


include $(LOCAL_PATH)/unit_test/make.inc

endif