/**********************************************************************
 * boards/btfl/devices.h
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __DEVICES_H__
#define __DEVICES_H__

#define SUPPORT_DC_IRIS         0
#define SUPPORT_IR_LED          1
#define SUPPORT_IR_CUT          0
#define SUPPORT_DAY_NIGHT_MODE_SWITCH   0

/* DC IRIS */
#define PWM_CHANNEL_DC_IRIS     -1

/* IR LED */

#define PWM_CHANNEL_IR_LED      0
#define IR_LED_NO_GPIO_CONTROL

#define GPIO_ID_LED_POWER       -1
#define GPIO_ID_LED_IR_ENABLE   -1
#define GPIO_ID_LED_NETWORK     -1
#define GPIO_ID_LED_SD_CARD     -1

#define IR_LED_ADC_CHANNEL      2

/* IR CUT */
#define GPIO_ID_IR_CUT_CTRL     111

typedef enum {
	GPIO_UNEXPORT = 0,
	GPIO_EXPORT = 1
} gpio_ex;

typedef enum {
	GPIO_IN = 0,
	GPIO_OUT = 1
} gpio_direction;

typedef enum {
	GPIO_LOW = 0,
	GPIO_HIGH = 1
} gpio_state;

#if defined TSDK
/* These defines are used by tsdk-drv, tsdk-mfg, tsdk-sdk and tsdk-vue-tek */

#include "device_types.h"

// Calibration dir is common to all
// x calibration directory
#define CALIBRATION_DIR  "/mnt/provision"

// adc device on your system
#define CONFIG_ADC_DEVICE "/sys/devices/e8000000.apb/e801d000.adc/adcsys"


// Sensor related items are common to all board types, all use OV4689

// video modes and buffer setup, currently only used for calibration
static vin_mode_t vin_modes[] = {
	{ 1920, 1080, 30, 0, 0, 0, 1, 1920, 1080, 0, 0, "off", 0, 0, "off", 0, 0, "off" }
};

/* IR CUT */
#define GPIO_ID_IR_CUT_CTRL     26
#define GPIO_ID_IR_CUT_CTRL_A    26

// Ambient light sensor
#define CONFIG_ADC_SUPPORT
#define CONFIG_ADC_DEVICE "/sys/devices/e8000000.apb/e801d000.adc/adcsys"
#define CONFIG_ALS_ADC_CH 2
#define CONFIG_ALS_CALIB_FILE_NAME "als.ini"

// CAPTURE

//Short name for CAPTURE_PARAMS
#define CAPTURE_MODES   {"1920x1080"}

//Params passed straight to test_encode
#define CAPTURE_PARAMS  {"-i 0 -Y --btype off -J --btype off \
	-K --btype off -X --bsize 1920x1080 \
	--bmaxsize 1920x1080 --enc-mode 0 --raw-capture 1"}

//X video
#define VIDEO_MODES
static venc_mode_t video_modes[] = {
	{ 30, 1920, 1080, 1920, 1080, ENC_H264 },
	{ 30, 1280, 720, 1280, 720, ENC_H264 },
};

// Sensor test
#define CAMERA_INIT             "init.sh --imx322"
#define CAMERA_SENSOR_NAME      {"IMX322"}
#define CAMERA_SENSOR_EFFECTIVE {1920, 1080}
#define CAMERA_SENSOR_CELL      {2.0, 2.0}


// WIFI
#define WIFI_MODULE_CMD "bcmdhd"

// Using AP6255
#define WIFI_FW_CONFIG_PATH "/tmp/bcmdhd.cal"
#define WIFI_FW_PATH_SCAN_STA "/lib/firmware/broadcom/ap6255/fw_bcm43455c0_ag.bin"
#define WIFI_FW_PATH_AP_STA "/lib/firmware/broadcom/ap6255/fw_bcm43455c0_ag_apsta.bin"
#define WIFI_FW_PATH_MFG "/lib/firmware/broadcom/ap6255/fw_bcm43455c0_ag_mfg.bin"

#define WIFI_MFG_2GHZ_SUPPORTED 1
#define WIFI_MFG_5GHZ_SUPPORTED 0

// IR Cut related (IRcut driver and ALS)

// No IRCUT or ALS

// x sd
#define SD_DEVICE_DIR "/sys/class/mmc_host/mmc1/"
#define SD_DEVICE_PAT "mmc.:*"
#define SD_DEVICE "/dev/sda1"
#define SD_DCIM_DIR "/sdcard/DCIM/100MEDIA"
#define SD_MOUNT_DIR "/sdcard"

// GPIO count for read all function
#define GPIO_COUNT 120

// LED map, PWM definition, and Button inputs,

const static bsp_device_output_map_t g_bsp_pwm_map[] =
{
    { "IR_LED", BSP_DEVICE_OUTPUT_TYPE_LED_PWM, {.pwm.channel = "0.pwm_bl"} }
};

const static bsp_device_output_map_t g_bsp_led_map[] =
{
    { "IR_LED", BSP_DEVICE_OUTPUT_TYPE_LED_PWM, {.pwm.channel = "0.pwm_bl"} }
};

// mount point for user partition to store downloaded firmware
#define USER_PARTITION_MOUNT_POINT      "/mnt/upgrade"
#define USER_PARTITION_USE_UBIFS            1

// Volume number of OTA partition when mounting with UBIFS.
// Must not clash with existing UBIFS volume
// (e.g. have 0=rootfs; 1=calib and settings)
#define USER_UBIFS_VOL_NO           2

// command line for pba reboot to enable enough ram etc
#define FW_UPGRADE_PBA_CMD_LINE "console=ttyS0 rootfs=ramfs root=/dev/ram rw rdinit=/linuxrc mem=200M dsp=0xC0000000,0x00000000 bsb=0xC0000000,0x00000000"

//X ble mfg
#define BLE_PWRON_GPIO 4
#define BLE_RESET_GPIO 101
#define BLE_TTY_DEVICE  "/dev/ttyS1"
#define BLE_FIRMWARE  "/lib/firmware/broadcom/ap6255/BCM4345C0.hcd"
#define BLE_DRIVER  "bcmdhd"
#define USE_BT_MAC_FROM_TEXT_FILE "/mnt/provision/btmac.txt"

/* BROADCOM BT MAC */
#define OGF_VENDOR_CMD  0x3f
#define OCF_BCM_WRITE_BD_ADDR 0x0001

#define MFI_I2C_DEV "/dev/i2c-2"
#define MFI_I2C_ADDR 0x20

/* IR LED */
#define IR_LED_NO_GPIO_CONTROL
#define PWM_CHANNEL_IR_LED      0


#endif // TSDK
#endif
