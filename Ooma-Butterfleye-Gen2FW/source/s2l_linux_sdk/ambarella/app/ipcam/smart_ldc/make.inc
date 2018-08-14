##
##
## History:
##    2012/06/25 - [Jian Tang] Created file
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
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_SMART_LDC), y)

LOCAL_PATH	:= $(call my-dir)
WEB_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= smart_ldc
LOCAL_SRCS	:= $(LOCAL_PATH)/smart_ldc.c $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -lpthread -lm -ljpeg

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/
	@cp -dpRf $(WEB_PATH)/web/css $(WEB_PATH)/web/js $(WEB_PATH)/web/index.html $(FAKEROOT_DIR)/webSvr/web
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= ldc.cgi
LOCAL_SRCS	:= $(LOCAL_PATH)/web/cgi-bin/ldc.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/clearsilver/include/ClearSilver
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/clearsilver/usr/lib -lneo_cgi -lneo_cs -lneo_utl -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= get_vin.cgi
LOCAL_SRCS	:= $(LOCAL_PATH)/web/cgi-bin/get_vin.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/clearsilver/include/ClearSilver
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/clearsilver/usr/lib -lneo_cgi -lneo_cs -lneo_utl -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif

