LOCAL_PATH   := $(call my-dir)

###############################################
####     butterfleye app
################################################
include $(CLEAR_VARS)

BUTTERFLEYE_APP_BIN_DIR	    := $(FAKEROOT_DIR)/usr/bin
BUTTERFLEYE_CONFIG_FILE		:= $(FAKEROOT_DIR)/etc/butterfleye/setting.ini

LOCAL_TARGET	:= butterfleye_app
LOCAL_SRCS	    := $(LOCAL_PATH)/main.c \
		$(LOCAL_PATH)/bluetooth.c \
		$(LOCAL_PATH)/cloud.c \
		$(LOCAL_PATH)/event_manager.c \
		$(LOCAL_PATH)/file_upload.c \
		$(LOCAL_PATH)/keep_alive.c \
		$(LOCAL_PATH)/mcu_logger.c \
		$(LOCAL_PATH)/record_stream.c \
		$(LOCAL_PATH)/uart_tx.c \
		$(LOCAL_PATH)/uart_rx.c \
		$(LOCAL_PATH)/wifi.c \


LOCAL_CFLAGS	:= -I$(LOCAL_PATH) -I$(LOCAL_PATH)/../include \
		-I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include

LOCAL_CFLAGS	+= -I$(ORYX_DIR)/include \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/event \
		-I$(ORYX_DIR)/include/video \
		-I$(ORYX_DIR)/include/stream \
		-I$(ORYX_DIR)/include/utility/jpeg_encoder \
		-I$(ORYX_DIR)/include/configure \
		-I$(ORYX_DIR)/include/audio \
		-I$(ORYX_DIR)/include/utility \
		-I$(ORYX_DIR)/include/image_quality \
		-I$(ORYX_DIR)/include/audio/capture \
		-I$(ORYX_DIR)/include/audio/codec

LOCAL_CFLAGS	+= -Werror -Wno-unused-function
LOCAL_LDFLAGS	:= -lpthread  -lm -lbutterfleye_app_utils \
		-L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
		-Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \

ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
LOCAL_LDFLAGS	+= -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

LOCAL_LIBS := libbutterfleye_app_utils.so librecorded.so

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): add_board_rootfs

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(BUTTERFLEYE_APP_BIN_DIR)/
	@cp -dpRf $< $(BUTTERFLEYE_APP_BIN_DIR)/
	@sed -i '/auto-generated/ d' $(BUTTERFLEYE_CONFIG_FILE)
	@sed -i '/\[audio\]/a sample_rate=$(FAST_AUDIO_RATE); auto-generated for fastboot audio. Non-editable' $(BUTTERFLEYE_CONFIG_FILE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
