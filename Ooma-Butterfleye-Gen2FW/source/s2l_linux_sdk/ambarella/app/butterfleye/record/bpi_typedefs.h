/*
 * bpi_typedef.h
 *
 * History:
 *       2015/01/14 - [Chu Chen] created file
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

#ifndef __BPI_TYPEDEFS_H_
#define __BPI_TYPEDEFS_H_
#include <stdint.h>

typedef enum {
    EChrg_ChrgFull = 0,
    EChrg_Charging,
    EChrg_Discharging,
} EChrg;

typedef enum {
    EDC_Plugin = 0,
    EDC_Unplugged,
} EDC;

typedef enum {
    EPIR_ON = 0,
    EPIR_OFF,
} EPIR;

typedef enum {
    EBattery_Low = 0,
    EBattery_Empty,
    EBattery_Normal,
} EBattery;

//MCU TRIGGER/CMD/MSG typedefs
enum MCU_TRIGGER_TYPE{//max trigger number = 0x0F
    MCU_TRIGGER_BASE = 0x00,

    MCU_TRIGGER_PIR_ON,
    MCU_TRIGGER_WIFI_WAKEUP,
    MCU_TRIGGER_WIFI_RECONNECT,
    MCU_TRIGGER_PWR,
    MCU_TRIGGER_PWR_2S,
    MCU_TRIGGER_POWER_EMPTY,

    //add more trigger here

    MCU_TRIGGER_END = MCU_TRIGGER_BASE + 0x10
};

enum MCU_CMD_TYPE{//max command number = 0x7F
    MCU_CMD_BASE = MCU_TRIGGER_END,

    MCU_CMD_GET_TRIGGER_EVENT,
    MCU_CMD_GET_RACE_TRIGGER_EVENT,

    MCU_CMD_LED_RED_ON,
    MCU_CMD_LED_RED_OFF,
    MCU_CMD_LED_GREEN_ON,
    MCU_CMD_LED_GREEN_OFF,
    MCU_CMD_LED_RED_BLINK,
    MCU_CMD_LED_GREEN_BLINK,

    MCU_CMD_GET_TIME_COST,

    MCU_CMD_SET_STRESS_TEST_MODE,
    MCU_CMD_SET_NORMAL_MODE,

    MCU_CMD_GET_VERSION,//return the MCU_CMD_GET_VERSION + Main version + Subversion
    MCU_CMD_NETWORK_NEED_REBUILD,
    MCU_CMD_POWEROFF_CPU_DRAM,
    MCU_CMD_IMAGE_UPGRADE,

    //add more cmd here

    MCU_CMD_END= MCU_CMD_BASE + 0x8F
};

enum MCU_EVENT_TYPE{//max event number = 0x70
    MCU_EVENT_BASE = MCU_CMD_END,

    MCU_EVENT_PIR_ON,
    MCU_EVENT_PIR_OFF,

    MCU_EVENT_BATTERY_EMPTY,
    MCU_EVENT_BATTERY_LOW,
    MCU_EVENT_BATTERY_NORMAL,
    MCU_EVENT_BATTERY_CHARGE_OFF,
    MCU_EVENT_BATTERY_CHARGE_ON,
    MCU_EVENT_BATTERY_CHARGE_DONE,

    MCU_EVENT_DC_ON,
    MCU_EVENT_DC_OFF,

    //add more event here
    MCU_EVENT_PWR,

    MCU_EVENT_TIME_COST = MCU_CMD_GET_TIME_COST,
    MCU_EVENT_RACE_TRIGGER_EVENT = MCU_CMD_GET_RACE_TRIGGER_EVENT,
    MCU_EVENT_VERSION = MCU_CMD_GET_VERSION,

    MCU_EVENT_END = 0xFF
};

typedef enum {
    AM_BPI_MODE_UNKNOWN = 0,
    AM_BPI_MODE_RECORDING = 1,
    AM_BPI_MODE_STREAMING = 2,
    AM_BPI_MODE_WLAN_CONFIG = 3,
    AM_BPI_MODE_WLAN_RECONNECT = 4,
    AM_BPI_MODE_NOTIFY = 5,
    /*add more mode here*/
    AM_BPI_MODE_DEBUG = AM_BPI_MODE_UNKNOWN + 0x10,
} AM_BPI_MODE;

typedef struct {
   uint8_t index;
   uint8_t bpi_mode;
   uint8_t reserved0;
   uint8_t reserved1;
} AM_BPI_MODE_MAP;

typedef enum
{
    BPI_VIDEO_CODEC_NONE = 0,
    BPI_VIDEO_CODEC_H264,
    BPI_VIDEO_CODEC_H265,
    BPI_VIDEO_CODEC_MJPEG,
} BPI_VIDEO_CODEC_TYPE;

typedef enum
{
    BPI_AUDIO_CODEC_AAC = 0,
    BPI_AUDIO_CODEC_G711,
    BPI_AUDIO_CODEC_G726,
    BPI_AUDIO_CODEC_G711A,
    BPI_AUDIO_CODEC_G711MU,
} BPI_AUDIO_CODEC_TYPE;

typedef enum
{
    BPI_FILE_MUXER_MP4 = 0,
    BPI_FILE_MUXER_TS,
} BPI_FILE_MUXER_TYPE;

typedef enum
{
    BPI_ROTATION_TYPE_NONE = 0,
    BPI_ROTATION_TYPE_90,
    BPI_ROTATION_TYPE_180,
    BPI_ROTATION_TYPE_270,
} BPI_ROTATION_TYPE;

#endif
