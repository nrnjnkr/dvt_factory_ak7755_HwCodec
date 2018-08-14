# make.inc
#
# Diagnostic tool for know boot reason and inform mcu to shut off s2lm 
#


LOCAL_PATH := $(call my-dir)

ifeq ($(CONFIG_BSP_BOARD_BTFL),y)
include $(CLEAR_VARS)

LOCAL_TARGET := send_powerOff


LOCAL_SRCS := $(LOCAL_PATH)/gpio.c \
		$(LOCAL_PATH)/main.c

LOCAL_CFLAGS := -I$(LOCAL_PATH)/.

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE) 
	@echo "BUild $@ path $(LOCAL_PATH)"
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@mkdir -p $(IPCAM_APP_LOCAL_BIN_DIR)/
	@echo "BUild $@ path $(LOCAL_PATH)"
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/

$(call add-target-into-build, $(LOCAL_TARGET))
endif
