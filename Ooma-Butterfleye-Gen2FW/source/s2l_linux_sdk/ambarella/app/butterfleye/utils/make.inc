LOCAL_PATH   := $(call my-dir)

################################################
####    butterfleye app utilities
################################################
include $(CLEAR_VARS)

BUTTERFLEYE_APP_LIB_DIR	    := $(FAKEROOT_DIR)/usr/lib

LOCAL_TARGET	:= libbutterfleye_app_utils.so
LOCAL_SRCS	    := $(LOCAL_PATH)/boot.c \
                    $(LOCAL_PATH)/gpio.c \
                    $(LOCAL_PATH)/log.c \
                    $(LOCAL_PATH)/messageq.c \
                    $(LOCAL_PATH)/av_utils.c \
                    $(LOCAL_PATH)/enum_print.c \

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) -I$(LOCAL_PATH)/../include \
		-I$(AMB_TOPDIR)/packages/bsreader

LOCAL_CFLAGS	+= -Werror
LOCAL_LDFLAGS	:= -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS += libbutterfleye_app_rtsp.so

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): add_board_rootfs

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(BUTTERFLEYE_APP_LIB_DIR)/
	@cp -dpRf $< $(BUTTERFLEYE_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
