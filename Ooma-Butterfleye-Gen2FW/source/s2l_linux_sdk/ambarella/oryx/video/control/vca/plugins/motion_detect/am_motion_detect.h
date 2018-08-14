/*******************************************************************************
 * am_motion_detect.h
 *
 * History:
 *   May 3, 2016 - [binwang] created file
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
#ifndef ORYX_VIDEO_CONTROL_VCA_PLUGINS_MOTION_DETECT_AM_MOTION_DETECT_H_
#define ORYX_VIDEO_CONTROL_VCA_PLUGINS_MOTION_DETECT_AM_MOTION_DETECT_H_

#include "am_motion_detect_types.h"
#include "am_vca_plugin_if.h"
#include "am_motion_detect_if.h"
#include "am_platform_if.h"
#include "am_video_camera_if.h"
#include "am_motion_detect_config.h"

class AMVin;
class AMMotionDetect: public AMIVCAPlugin, public AMIMotionDetect
{
  public:
    static AMMotionDetect* create();
    virtual void destroy()                                       override;
    virtual bool start()                                         override;
    virtual bool stop()                                          override;
    virtual std::string& name()                                  override;
    virtual void* get_interface()                                override;
    virtual bool data_loop(const AMQueryFrameDesc &desc)         override;
    virtual bool get_buffer_config(AMVCABuf &buf_config)         override;
  public:
    virtual bool set_config(AMMDConfig *pConfig)                 override;
    virtual bool get_config(AMMDConfig *pConfig)                 override;
    virtual bool set_md_callback(void *this_obj,
                                 AMRecvMD recv_md)               override;
    /* Customers can use this API to get motion info from motion detect
     * It is a C style callback function interface.
     * If you want to set callback function in a C++ function,
     * you need to change it to
     *     virtual bool set_md_callback_customer(void *this_obj,
                                                 AMRecvMD recv_md,
                                                 AMMDMessage *msg);
     */
    virtual bool set_md_callback_customer(void *this_obs,
                                          AMRecvMD recv_md)      override;
    //for config files
    virtual AM_RESULT load_config()                              override;
    virtual AM_RESULT save_config()                              override;

  protected:
    AMMotionDetect(const char *name);
    virtual ~AMMotionDetect();

  private:
    bool init();
    bool sync_config();
    void print_md_config();
    bool check_motion(const AMQueryFrameDesc &buf_frame);
    bool set_md_state(bool enable);
    bool get_md_state(bool *enable);
    bool set_md_buffer_id(AM_SOURCE_BUFFER_ID source_buf_id);
    bool get_md_buffer_id(AM_SOURCE_BUFFER_ID *source_buf_id);
    bool set_md_buffer_type(AM_DATA_FRAME_TYPE source_buf_type);
    bool get_md_buffer_type(AM_DATA_FRAME_TYPE *source_buf_type);
    bool check_md_roi_format(AMMDRoi *roi_info, uint32_t buf_width,
                             uint32_t buf_height, uint32_t buf_pitch);
    bool set_roi_info(AMMDRoi *roi_info);
    bool get_roi_info(AMMDRoi *roi_info);
    bool set_threshold_info(AMMDThreshold *threshold);
    bool get_threshold_info(AMMDThreshold *threshold);
    bool set_level_change_delay_info(
        AMMDLevelChangeDelay *level_change_delay);
    bool get_level_change_delay_info(
        AMMDLevelChangeDelay *level_change_delay);
    bool exec_callback_func(void);
    bool exec_callback_func_customer(void);

  protected:
    std::string           m_name;

  private:
    AMRecvMD              m_callback                    = nullptr;
    AMRecvMD              m_callback_customer           = nullptr;
    mdet_instance        *m_inst                        = nullptr;
    AMMotionDetectConfig *m_config                      = nullptr;
    AMMDRoi              *m_roi_info                    = nullptr;
    AMMDThreshold        *m_threshold                   = nullptr;
    AMMDLevelChangeDelay *m_level_change_delay          = nullptr;
    void                 *m_callback_owner_obj          = nullptr;
    void                 *m_callback_customer_owner_obj = nullptr;
    AMIPlatformPtr        m_platform                    = nullptr;
    AMIVideoReaderPtr     m_video_reader                = nullptr;
    AMIVideoAddressPtr    m_video_address               = nullptr;
    AMMDMessage           m_msg                         = {};
    std::atomic<bool>     m_started                     = {false};
    std::atomic<bool>     m_enable                      = {false};
    std::atomic<bool>     m_init_param                  = {false};
    std::string           m_conf_path;
    mdet_session_t        mdet_session;
    mdet_cfg              mdet_config;
    AMMDBuf               m_md_buffer;
    AMQueryFrameDesc      m_frame_desc;
    MotionDetectParam     m_config_param;
};

#endif /* ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_H_ */
