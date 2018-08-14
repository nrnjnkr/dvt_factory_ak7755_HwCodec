LOCAL_PATH   := $(call my-dir)

################################################
####    butterfleye app utilities
################################################
include $(CLEAR_VARS)

BUTTERFLEYE_APP_LIB_DIR	    := $(FAKEROOT_DIR)/usr/lib

RTSP_DIR := $(LOCAL_PATH)/../../ipcam/rtsp

LOCAL_TARGET	:= libbutterfleye_app_rtsp.so
LOCAL_SRCS	    :=  $(LOCAL_PATH)/H264StreamParser.cpp \
                    $(LOCAL_PATH)/H264VideoFileServerMediaSubsession.cpp \
                    $(LOCAL_PATH)/MyH264VideoRTPSink.cpp \
                    $(LOCAL_PATH)/MyH264VideoStreamFramer.cpp \
                    $(LOCAL_PATH)/rtsp_server.cpp

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) -I$(LOCAL_PATH)/../include \
		-I$(RTSP_DIR)/BasicUsageEnvironment/include \
		-I$(RTSP_DIR)/groupsock/include \
		-I$(RTSP_DIR)/liveMedia/include \
		-I$(RTSP_DIR)/liveMedia \
		-I$(RTSP_DIR)/UsageEnvironment/include \
		-I$(AMB_TOPDIR)/packages/bsreader

LOCAL_CFLAGS	+= -Werror
LOCAL_LDFLAGS	:= -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS += libliveMedia.a libgroupsock.a \
		libBasicUsageEnvironment.a libUsageEnvironment.a libbsreader.a

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): add_board_rootfs

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(BUTTERFLEYE_APP_LIB_DIR)/
	@cp -dpRf $< $(BUTTERFLEYE_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
