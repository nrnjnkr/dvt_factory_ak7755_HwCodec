/*******************************************************************************
 * am_iq_param.h
 *
 * History:
 *   Dec 29, 2014 - [binwang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/
#ifndef AM_IQ_PARAM_H_
#define AM_IQ_PARAM_H_

#include "am_base_include.h"
#include "am_image_quality_config.h"

#define AE_METERING_TABLE_LEN 96

enum AM_IQ_IAV_STATE {
  IQ_IAV_STATE_IDLE = 0,
  IQ_IAV_STATE_PREVIEW = 1,
  IQ_IAV_STATE_ENCODING = 2,
  IQ_IAV_STATE_STILL_CAPTURE = 3,
  IQ_IAV_STATE_DECODING = 4,
  IQ_IAV_STATE_TRANSCODING = 5,
  IQ_IAV_STATE_DUPLEX = 6,
  IQ_IAV_STATE_EXITING_PREVIEW = 7,
  IQ_IAV_STATE_INIT = 0xFF,
};

enum AM_IQ_LOG_LEVEL
{
  AW_MSG_LEVEL = 0,
  AW_INFO_LEVEL,
  AW_DEBUG_LEVEL,
  AW_LOG_LEVEL_NUM
};

enum AM_AAA_FILE_TYPE
{
  AM_FILE_TYPE_ADJ = 0,
  AM_FILE_TYPE_AEB = 1,
  AM_FILE_TYPE_CFG = 2,
  AM_FILE_TYPE_TOTAL_NUM
};

struct AMAEMeteringTable {
    int32_t metering_weight[96] = {0};
};

struct AMAEParam
{
    AMAEMeteringTable      ae_metering_table;
    AMAELuma               luma;
    int32_t                ae_target_ratio       = 100;
    int32_t                shutter_time          = 0;
    int32_t                sensor_gain           = 0;
    uint32_t               auto_wdr_strength     = 0;
    uint32_t               sensor_gain_max       = 0;
    uint32_t               sensor_shutter_min    = 0;
    uint32_t               sensor_shutter_max    = 0;
    AM_AE_MODE             ae_enable             = AM_AE_ENABLE;
    AM_AE_METERING_MODE    ae_metering_mode      = AM_AE_CENTER_METERING;
    AM_DAY_NIGHT_MODE      day_night_mode        = AM_DAY_MODE;
    AM_SLOW_SHUTTER_MODE   slow_shutter_mode     = AM_SLOW_SHUTTER_ON;
    AM_ANTI_FLICK_MODE     anti_flicker_mode     = AM_ANTI_FLICKER_50HZ;
    AM_BACKLIGHT_COMP_MODE backlight_comp_enable = AM_BACKLIGHT_COMP_OFF;
    AM_DC_IRIS_MODE        dc_iris_enable        = AM_DC_IRIS_DISABLE;
    AM_IR_LED_MODE         ir_led_mode           = AM_IRLED_AUTO;
};

struct AMAWBParam
{
    AM_WB_MODE wb_mode = AM_WB_AUTO;
};

struct AMNFParam
{
    uint32_t mctf_strength = 0;
};

struct AMStyleParam
{
    int32_t saturation         = 0;
    int32_t brightness         = 0;
    int32_t hue                = 0;
    int32_t contrast           = 0;
    int32_t sharpness          = 0;
    int32_t auto_contrast_mode = 0;
};

struct AMIQParam
{
    AMAEParam       ae;
    AMAWBParam      awb;
    AMNFParam       nf;
    AMStyleParam    style;
    AM_IQ_LOG_LEVEL log_level              = AW_MSG_LEVEL;
    uint32_t        img_debug_level        = 0;
    bool            notify_3A_to_media_svc = false;;
    bool            dump_config            = false;;
};

#endif /* AM_IQ_PARAM_H_ */
