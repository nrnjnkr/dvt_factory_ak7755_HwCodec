#
# make.inc
#
# History:
#       2017/03/17 - [jyi] created file
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


LOCAL_PATH		:= $(call my-dir)
include $(CLEAR_VARS)
IPCAM_APP_LOCAL_BIN_DIR	:= $(FAKEROOT_DIR)/usr/local/bin
PROMPT_WAV		:= prompt.wav
LOCAL_MATERIAL	:= $(wildcard $(LOCAL_PATH)/*.wav)

ifeq ($(AMBOOT_AUDIO_48000), y)
	LOCAL_MATERIAL  += $(LOCAL_PATH)/48k/*.wav
else ifeq ($(AMBOOT_AUDIO_16000), y)
	LOCAL_MATERIAL  += $(LOCAL_PATH)/16k/*.wav
else
	LOCAL_MATERIAL  += $(LOCAL_PATH)/8k/*.wav
endif

.PHONY: bpi_audio_material

bpi_audio_material: $(LOCAL_MATERIAL)
	@mkdir -p $(IPCAM_APP_LOCAL_BIN_DIR)
	@cp -dpRf $(LOCAL_MATERIAL) $(IPCAM_APP_LOCAL_BIN_DIR)/
	@echo "Copy BPI audio material Done"

$(call add-target-into-build, bpi_audio_material)
