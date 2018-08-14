/*******************************************************************************
 * bpi_oryx_export.h
 *
 * History:
 *   2017-01-17 - [Jian Liu]      created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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
#ifndef BPI_ORYX_CONFIG_H_
#define BPI_ORYX_CONFIG_H_

#include "bpi_app_config.h"
#include "am_image_quality_if.h"
#include "am_video_camera_if.h"
#include "am_vca_framework_if.h"

void read_sole_config(const char *filename, void *content, size_t length);
void write_sole_config(const char *filename, void *content, size_t length);
bool config_oryx_engine(app_conf_t *p_user_conf);


class AMILBRControl;
class AMIMotionDetect;
class AMIEncodeOverlay;
struct AMMDMessage;
struct AMOverlayAreaAttr;
struct AMOverlayAreaData;

class OryxVideoModule{
public:
    bool start_camera();
    bool start_linux_aaa(bool enable = true);
    bool start_overlay(const char *label);
    bool start_smart_avc(bool enable);
    bool set_bitrate(int target_bitrate);
    bool stop_camera();
    bool stop_linux_aaa();
    bool stop_overlay();
    bool stop_smart_avc();
    static OryxVideoModule* get_instance();
    static int receive_data_from_md(void *owner, AMMDMessage *info);
private:
    OryxVideoModule();
    ~OryxVideoModule();
    OryxVideoModule(const OryxVideoModule &module) = delete;
    OryxVideoModule& operator=(const OryxVideoModule module) = delete;

    AMIImageQualityPtr m_image;
    AMIVideoCameraPtr m_video_camera; //surrogate of AMEncodeDevice
    AMILBRControl* m_lbr;
    AMIMotionDetect* m_md;
    AMIEncodeOverlay* m_ol;
    AMOverlayAreaAttr *m_stream0_overlay_area0;
    AMOverlayAreaData *m_area0_data0;
    AMIVCA *m_am_vca;
};


#endif
