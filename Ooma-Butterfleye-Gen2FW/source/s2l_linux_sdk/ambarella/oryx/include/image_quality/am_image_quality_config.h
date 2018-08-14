/*******************************************************************************
 * am_image_quality_config.h
 *
 * History:
 *   Apr 17, 2017 - [ypchang] created file
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
 ******************************************************************************/
#ifndef AM_IMAGE_QUALITY_CONFIG_H_
#define AM_IMAGE_QUALITY_CONFIG_H_

enum AM_IQ_CONFIG_KEY
{
  /*log level config*/
  AM_IQ_LOGLEVEL = 0,
  /*AE config*/
  AM_IQ_AE_METERING_MODE,
  AM_IQ_AE_DAY_NIGHT_MODE,
  AM_IQ_AE_SLOW_SHUTTER_MODE,
  AM_IQ_AE_ANTI_FLICKER_MODE,
  AM_IQ_AE_TARGET_RATIO,
  AM_IQ_AE_BACKLIGHT_COMP_ENABLE,
  AM_IQ_AE_AUTO_WDR_STRENGTH,
  AM_IQ_AE_DC_IRIS_ENABLE,
  AM_IQ_AE_SENSOR_GAIN_MAX,
  AM_IQ_AE_SENSOR_SHUTTER_MIN,
  AM_IQ_AE_SENSOR_SHUTTER_MAX,
  AM_IQ_AE_IR_LED_MODE,
  AM_IQ_AE_SHUTTER_TIME,
  AM_IQ_AE_SENSOR_GAIN,
  AM_IQ_AE_LUMA,
  AM_IQ_AE_AE_ENABLE,
  /*AWB config*/
  AM_IQ_AWB_WB_MODE,
  /*NF config*/
  AM_IQ_NF_MCTF_STRENGTH,
  /*IQ style config*/
  AM_IQ_STYLE_SATURATION,
  AM_IQ_STYLE_BRIGHTNESS,
  AM_IQ_STYLE_HUE,
  AM_IQ_STYLE_CONTRAST,
  AM_IQ_STYLE_SHARPNESS,
  AM_IQ_STYLE_AUTO_CONTRAST_MODE,
  /*AEB,ADJ load bin*/
  AM_IQ_AEB_ADJ_BIN_LOAD,
  /*total config key number*/
  AM_IQ_KEY_NUM
};

enum AM_AE_METERING_MODE
{
  AM_AE_SPOT_METERING = 0,
  AM_AE_CENTER_METERING,
  AM_AE_AVERAGE_METERING,
  AM_AE_CUSTOM_METERING,
  AM_AE_METERING_TYPE_NUMBER,
};

enum AM_ANTI_FLICK_MODE
{
  AM_ANTI_FLICKER_50HZ = 0,
  AM_ANTI_FLICKER_60HZ = 1
};

enum AM_DAY_NIGHT_MODE
{
  AM_DAY_NIGHT_MODE_INVALID = -1,
  AM_DAY_MODE = 0,
  AM_NIGHT_MODE = 1
};

enum AM_SLOW_SHUTTER_MODE
{
  AM_SLOW_SHUTTER_OFF = 0,
  AM_SLOW_SHUTTER_ON = 1
};

enum AM_BACKLIGHT_COMP_MODE
{
  AM_BACKLIGHT_COMP_OFF = 0,
  AM_BACKLIGHT_COMP_ON = 1
};

enum AM_DC_IRIS_MODE
{
  AM_DC_IRIS_DISABLE = 0,
  AM_DC_IRIS_ENABLE = 1
};

enum AM_IR_LED_MODE
{
  AM_IR_LED_OFF = 0,
  AM_IR_LED_ON = 1,
  AM_IRLED_AUTO = 2
};

enum AM_AE_MODE
{
  AM_AE_DISABLE = 0,
  AM_AE_ENABLE = 1
};

enum AM_WB_MODE
{
  AM_WB_AUTO = 0,
  AM_WB_INCANDESCENT, // 2800
  AM_WB_D4000,
  AM_WB_D5000,
  AM_WB_SUNNY, // 6500K
  AM_WB_CLOUDY, // 7500K
  AM_WB_FLASH,
  AM_WB_FLUORESCENT,
  AM_WB_FLUORESCENT_H,
  AM_WB_UNDERWATER,
  AM_WB_CUSTOM, // custom
  AM_WB_MODE_NUM
};

struct AM_IQ_CONFIG
{
  void            *value;
  AM_IQ_CONFIG_KEY key;
};

struct AMAELuma
{
    uint16_t rgb_luma = 0;
    uint16_t cfa_luma = 0;
};

#endif /* AM_IMAGE_QUALITY_CONFIG_H_ */
