LOCAL_PATH  := $(call my-dir)

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_TEK_DAY_NIGHT), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET    := test_day_night
LOCAL_SRCS      := $(LOCAL_PATH)/test_day_night.c
LOCAL_CFLAGS    := -std=c99
LOCAL_LDFLAGS   := -lpthread -lm
LOCAL_LIBS      := libamp.so libamdev.so
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LIBS	+= libimg_algo_s2l.a
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/etc/idsp/cfg
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_TEK_ALS), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET    := test_als
LOCAL_SRCS      := $(LOCAL_PATH)/test_als.c $(LOCAL_PATH)/als/als_linear.c $(LOCAL_PATH)/als/adc/adc_util.c
LOCAL_CFLAGS    := -std=c99

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/etc/idsp/cfg
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_TEK_MCU_OTA), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET    := test_mcu_ota
LOCAL_SRCS      := $(LOCAL_PATH)/mcu_ota/main.cpp $(LOCAL_PATH)/mcu_ota/qbytearray.cpp $(LOCAL_PATH)/mcu_ota/efm32bootloader_client.cpp
LOCAL_CFLAGS    := -Wall
LOCAL_LDFLAGS   := -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/etc/idsp/cfg
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
