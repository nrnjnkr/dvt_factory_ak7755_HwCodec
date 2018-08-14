##
## app/ipcam/autorun_services/make.inc
##
## History:
##    2014/12/12 - [Alex ZHU] Create
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

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_SERVICES), y)

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

.PHONY: add_autorun_scripts
.PHONY: del_autorun_services
.PHONY: add_autorun_services

LOCAL_SCPS	:=
LOCAL_SRVS	:=
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_DEMO), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_example.service
LOCAL_SCPS  +=  $(LOCAL_PATH)/autorun_example.sh
endif
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_ATR_SHMOO), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_shmoo.service
endif
ifeq ($(BUILD_AMBARELLA_APP_CLOUD_AGENT), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_ambacloud_agent.service
LOCAL_SCPS  +=  $(LOCAL_PATH)/autorun_ambacloud_agent.sh
endif
LOCAL_SRVS_NODIR	:= $(notdir $(LOCAL_SRVS))
LOCAL_ALLSRVS	:=
LOCAL_ALLSRVS	+= $(wildcard $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/autorun*.service)
LOCAL_ALLSRVS_NODIR	:=	$(notdir $(LOCAL_ALLSRVS))

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_DEMO), y)
add_autorun_scripts: $(LOCAL_SCPS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(LOCAL_SCPS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh
endif

ifeq ($(BUILD_AMBARELLA_APP_CLOUD_AGENT), y)
add_autorun_scripts: $(LOCAL_SCPS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(LOCAL_SCPS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh
endif

## delete the autorun service in the folder of multi-user.target.wants when user switch config
del_autorun_services:
	@for n in $(LOCAL_ALLSRVS_NODIR);\
	do \
	rm $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/$$n; \
	done

add_autorun_services: $(LOCAL_SRVS)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/systemd/system/
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/
	@cp -dpRf $(LOCAL_SRVS) $(FAKEROOT_DIR)/usr/lib/systemd/system/
	@ln -s ../$(LOCAL_SRVS_NODIR) $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/$(LOCAL_SRVS_NODIR)

$(call add-target-into-build, add_autorun_scripts)
$(call add-target-into-build, del_autorun_services)
$(call add-target-into-build, add_autorun_services)

endif
