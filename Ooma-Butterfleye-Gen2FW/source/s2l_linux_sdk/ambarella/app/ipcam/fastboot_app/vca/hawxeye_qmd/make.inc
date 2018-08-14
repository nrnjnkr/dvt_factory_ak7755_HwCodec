#
# make.inc
#
# History:
#       2016/10/25 - [yuan xiaojie] created file
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

ifneq ($(CONFIG_BSP_BOARD_S3LM_DAREDEVIL), y)
LOCAL_PATH   := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../make.common


LOCAL_TARGET	:= libvca_hawxeye_qmd.so
IPCAM_APP_BIN_DIR   := $(FAKEROOT_DIR)/usr/bin
LOCAL_QMD_DIR     := $(LOCAL_PATH)
LOCAL_SRCS	+= $(LOCAL_PATH)/bpi_vca_hawxeye.cpp

LOCAL_CFLAGS    += -I$(LOCAL_PATH)/include

LOCAL_CFLAGS	+= -std=c++11 -Werror -g

LOCAL_LDFLAGS   += -L$(LOCAL_QMD_DIR)/lib -lhawxeyeqmd

HAWXEYE_VCA_SO_TARGET	:= libvca_hawxeye_qmd.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(IPCAM_APP_LIB_DIR)
	@cp -dpRf $< $(IPCAM_APP_LIB_DIR)/$(HAWXEYE_VCA_SO_TARGET)
	@cp -dpRf $(LOCAL_QMD_DIR)/lib/libhawxeyeqmd.so $(IPCAM_APP_LIB_DIR)/
	@cp -dpRf $(LOCAL_QMD_DIR)/configs $(IPCAM_APP_BIN_DIR)/
	@cp -dpRf $(LOCAL_QMD_DIR)/models $(IPCAM_APP_BIN_DIR)/
	@ln -sf $(HAWXEYE_VCA_SO_TARGET) $(IPCAM_APP_LIB_DIR)/$(VCA_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
