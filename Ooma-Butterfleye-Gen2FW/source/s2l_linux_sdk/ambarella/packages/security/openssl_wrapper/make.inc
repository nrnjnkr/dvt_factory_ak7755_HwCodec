###############################################################################
## packages/security/openssl_wrapper/make.inc
##
## History:
##    2015/05/19 - [Zhi He] Created file
##
## Copyright (C) 2015 Ambarella, Inc.
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
##################################################################################

ifeq ($(BUILD_OPENSSL_WRAPPER_LIBRARY), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)
SSL_WRAPPER_VER_FILE = $(LOCAL_PATH)/openssl_wrapper.h
SSL_WRAPPER_VER_PREFIX = SSL_WRAPPER
SSL_WRAPPER_TARGET = libopensslwrapper.so

LOCAL_TARGET  := $(SSL_WRAPPER_TARGET)
LOCAL_SRCS    := $(LOCAL_PATH)/openssl_wrapper.cpp
LOCAL_CFLAGS  := -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include \
                 -fvisibility=hidden                         \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto

SSL_WRAPPER_SO_VER_MAJOR  := $(shell awk '/define\ $(SSL_WRAPPER_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(SSL_WRAPPER_VER_FILE))
SSL_WRAPPER_SO_VER_MINOR  := $(shell awk '/define\ $(SSL_WRAPPER_VER_PREFIX)_LIB_MINOR/{print $$3}' $(SSL_WRAPPER_VER_FILE))
SSL_WRAPPER_SO_VER_PATCH  := $(shell awk '/define\ $(SSL_WRAPPER_VER_PREFIX)_LIB_PATCH/{print $$3}' $(SSL_WRAPPER_VER_FILE))
SSL_WRAPPER_SO_VER_STRING := $(SSL_WRAPPER_SO_VER_MAJOR).$(SSL_WRAPPER_SO_VER_MINOR).$(SSL_WRAPPER_SO_VER_PATCH)

SSL_WRAPPER_SO_NAME   = $(patsubst %.so,%.so.$(SSL_WRAPPER_SO_VER_MAJOR), $(SSL_WRAPPER_TARGET))
SSL_WRAPPER_SO_TARGET = $(patsubst %.so,%.so.$(SSL_WRAPPER_SO_VER_STRING), $(SSL_WRAPPER_TARGET))

LOCAL_SO_NAME := $(SSL_WRAPPER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(SSL_WRAPPER_SO_TARGET)
	@ln -sf $(SSL_WRAPPER_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(SSL_WRAPPER_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
