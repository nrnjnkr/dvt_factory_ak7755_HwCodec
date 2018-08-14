# make.inc
#
# Butterfleye : Building custom app for butterfleye
#


###############################
#Building publisher client
################################
LOCAL_PATH := $(call my-dir)

ifeq ($(CONFIG_BPI_APP_HIBERNATION),y)
include $(CLEAR_VARS)
#Building mosquitto clients 

LOCAL_TARGET := publisher_client

MOSQUITTO := $(LOCAL_PATH)

LOCAL_SRCS := $(LOCAL_PATH)/cloud_publisher.c  \
	      $(LOCAL_PATH)/client_shared.c  \
	      $(LOCAL_PATH)/config.c \
          $(LOCAL_PATH)/../utils/log.c

LOCAL_CFLAGS := -I$(LOCAL_PATH)/mosquitto/include \
		-I$(LOCAL_PATH)/ -I$(LOCAL_PATH)/../include

LOCAL_LDFLAGS := -lmosquitto  \
		 -L$(LOCAL_PATH)/mosquitto/lib/


include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE) 
	@echo "BUild $@ path $(LOCAL_PATH)"
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@mkdir -p $(IPCAM_APP_LOCAL_BIN_DIR)/
	@echo "BUild $@ path $(LOCAL_PATH)"
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@cp $(MOSQUITTO)/mosquitto/lib/libmosquitto.so.1 $(IPCAM_APP_LIB_DIR)/
	@cp $(MOSQUITTO)/mosquitto/sbin/mosquitto $(IPCAM_APP_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###################################################
## Building subscriber client 
###################################################

include $(CLEAR_VARS)

LOCAL_TARGET := subscriber_client

MOSQUITTO := $(LOCAL_PATH)

LOCAL_SRCS := $(LOCAL_PATH)/cloud_subscriber.c  \
	      $(LOCAL_PATH)/client_shared.c  \
	      $(LOCAL_PATH)/config.c \
          $(LOCAL_PATH)/../utils/log.c

LOCAL_CFLAGS := -I$(LOCAL_PATH)/mosquitto/include \
		-I$(LOCAL_PATH)/ -I$(LOCAL_PATH)/../include

LOCAL_LDFLAGS := -lmosquitto  \
		 -L$(LOCAL_PATH)/mosquitto/lib/


include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE) 
	@echo "BUild $@ path $(LOCAL_PATH)"
	@mkdir -p $(IPCAM_APP_BIN_DIR)/
	@mkdir -p $(IPCAM_APP_LOCAL_BIN_DIR)/
	@echo "BUild $@ path $(LOCAL_PATH)"
	@cp -dpRf $< $(IPCAM_APP_BIN_DIR)/
	@cp $(MOSQUITTO)/mosquitto/lib/libmosquitto.so.1 $(IPCAM_APP_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
