/*******************************************************************************
 * am_image_quality.h
 *
 * History:
 *   Dec 29, 2014 - [binwang] created file
 *
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
 */

#ifndef AM_IMAGE_QUALITY_H_
#define AM_IMAGE_QUALITY_H_

#include "am_image_quality_if.h"
#include "am_iq_param.h"
#include <atomic>


enum AM_IQ_STATE
{
  AM_IQ_STATE_NOT_INIT = 0,
  AM_IQ_STATE_INIT_DONE = 1,
  AM_IQ_STATE_STARTING = 2,
  AM_IQ_STATE_RUNNING = 3,
  AM_IQ_STATE_STOPPED = 4,
  AM_IQ_STATE_ERROR = 5,
};

class AMImageQuality;
class AMIQConfig
{
    friend class AMImageQuality;
  public:
    AMIQConfig();
    virtual ~AMIQConfig();
    virtual bool load_config();
    virtual bool save_config();
    virtual bool sync();

  protected:
    int32_t m_aeb_adj_file_flag[AM_FILE_TYPE_TOTAL_NUM] = {0};
    char m_aeb_adj_file_name[AM_FILE_TYPE_TOTAL_NUM][FILE_NAME_LENGTH] = {{0}};
    bool m_changed = false;
    AMIQParam m_loaded;

  protected:
    AMIQParam m_using;
};

class AMTimer;
class AMImageQuality: public AMIImageQuality
{
    friend class AMIImageQuality;
  public:
    virtual bool start();
    virtual bool stop();
    virtual bool load_config();
    virtual bool save_config();
    virtual bool set_config(AM_IQ_CONFIG *config);
    virtual bool get_config(AM_IQ_CONFIG *config);
    virtual bool need_notify();

  protected:
    static AMImageQuality *get_instance();
    virtual bool init();
    virtual void release();
    virtual void inc_ref();

  protected:
    AMImageQuality();
    virtual ~AMImageQuality();
    AMImageQuality(AMImageQuality const &copy) = delete;
    AMImageQuality& operator=(AMImageQuality const &copy) = delete;

  private:
    void print_config(AMIQParam *iq_param);
    static bool prepare_to_run_server(void *data);
    /*log level config*/
    bool set_log_level(AM_IQ_LOG_LEVEL *log_level);
    /*AE config*/
    bool set_ae_metering_mode(AM_AE_METERING_MODE *ae_metering_mode);
    bool set_day_night_mode(AM_DAY_NIGHT_MODE *day_night_mode);
    bool set_slow_shutter_mode(AM_SLOW_SHUTTER_MODE *slow_shutter_mode);
    bool set_anti_flick_mode(AM_ANTI_FLICK_MODE *anti_flicker_mode);
    bool set_ae_target_ratio(int32_t *ae_target_ratio);
    bool set_backlight_comp_enable(AM_BACKLIGHT_COMP_MODE *backlight_comp_enable);
    bool set_auto_wdr_strength(uint32_t *auto_wdr_strength);
    bool set_dc_iris_enable(AM_DC_IRIS_MODE *dc_iris_enable);
    bool set_sensor_gain_max(uint32_t *sensor_gain_max);
    bool set_sensor_shutter_min(uint32_t *sensor_shutter_min);
    bool set_sensor_shutter_max(uint32_t *sensor_shutter_max);
    bool set_ir_led_mode(AM_IR_LED_MODE *ir_led_mode);
    bool set_shutter_time(int32_t *shutter_time);
    bool set_sensor_gain(int32_t *sensor_gain);
    bool set_ae_enable(AM_AE_MODE *ae_enable);
    /*AWB config*/
    bool set_wb_mode(AM_WB_MODE *wb_mode);
    /*NF config*/
    bool set_mctf_strength(uint32_t *mctf_strength);
    /*IQ style config*/
    bool set_saturation(int32_t *saturation);
    bool set_brightness(int32_t *brightness);
    bool set_hue(int32_t *hue);
    bool set_contrast(int32_t *contrast);
    bool set_sharpness(int32_t *sharpness);
    bool set_auto_contrast_mode(int32_t *ac_mode);

    /*log level config*/
    bool get_log_level(AM_IQ_LOG_LEVEL *log_level);
    /*AE config*/
    bool get_ae_metering_mode(AM_AE_METERING_MODE *ae_metering_mode);
    bool get_day_night_mode(AM_DAY_NIGHT_MODE *day_night_mode);
    bool get_slow_shutter_mode(AM_SLOW_SHUTTER_MODE *slow_shutter_mode);
    bool get_anti_flick_mode(AM_ANTI_FLICK_MODE *anti_flicker_mode);
    bool get_ae_target_ratio(int32_t *ae_target_ratio);
    bool get_backlight_comp_enable(AM_BACKLIGHT_COMP_MODE *backlight_comp_enable);
    bool get_auto_wdr_strength(uint32_t *auto_wdr_strength);
    bool get_dc_iris_enable(AM_DC_IRIS_MODE *dc_iris_enable);
    bool get_sensor_gain_max(uint32_t *sensor_gain_max);
    bool get_sensor_shutter_min(uint32_t *sensor_shutter_min);
    bool get_sensor_shutter_max(uint32_t *sensor_shutter_max);
    bool get_ir_led_mode(AM_IR_LED_MODE *ir_led_mode);
    bool get_shutter_time(int32_t *shutter_time);
    bool get_sensor_gain(int32_t *sensor_gain);
    bool get_luma(AMAELuma *luma);
    bool get_ae_enable(AM_AE_MODE *ae_enable);
    /*AWB config*/
    bool get_wb_mode(AM_WB_MODE *wb_mode);
    /*NF config*/
    bool get_mctf_strength(uint32_t *mctf_strength);
    /*IQ style config*/
    bool get_saturation(int32_t *saturation);
    bool get_brightness(int32_t *brightness);
    bool get_hue(int32_t *hue);
    bool get_contrast(int32_t *contrast);
    bool get_sharpness(int32_t *sharpness);
    bool get_auto_contrast_mode(int32_t *ac_mode);
    /*load bin*/
    bool load_adj_bin(const char *file_name);

  protected:
    static AMImageQuality   *m_instance;
    AMIQConfig              *m_iq_config = nullptr;
    AMTimer                 *m_aaa_timer = nullptr;
    std::atomic<int>         m_ref_count = {0};
    std::atomic<AM_IQ_STATE> m_iq_state  = {AM_IQ_STATE_NOT_INIT};

  private:
    int32_t                m_fd_iav    = -1;
};

#endif /* AM_IMAGE_QUALITY_H_ */
