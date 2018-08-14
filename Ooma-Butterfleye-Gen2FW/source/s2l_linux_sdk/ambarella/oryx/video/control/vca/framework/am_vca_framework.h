/*******************************************************************************
 * am_vca_framework.h
 *
 * History:
 *   May 8, 2017 - [nzhang] created file
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
#ifndef ORYX_VIDEO_CONTROL_VCA_FRAMEWORK_AM_VCA_FRAMEWORK_H_
#define ORYX_VIDEO_CONTROL_VCA_FRAMEWORK_AM_VCA_FRAMEWORK_H_

#include "am_vca_framework_if.h"
#include "am_vca_framework_types.h"
#include "am_thread.h"
#include "am_platform_if.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_video_camera_if.h"
#include "am_encode_plugin_if.h"

using std::map;
using std::string;
using std::vector;

class AMVin;
class AMIEncodePlugin;
class AMPlugin;
class AMEncodeStream;
class AMIVCAPlugin;

struct AMVCAPlugin
{
    AMPlugin                  *so = nullptr;
    AMIVCAPlugin              *plugin = nullptr;
    AMVCAPlugin();
    AMVCAPlugin(const AMVCAPlugin &vcaplugin);
    ~AMVCAPlugin();
};

typedef std::map<std::string, AMVCAPlugin*> AMVCAPluginMap;

typedef map<string, AMIVCAPlugin*> AMIVCAPluginMap;

class AMVCA: public AMIEncodePlugin, public AMIVCA
{
  public:
    static AMVCA* create(AMVin *vin);
    virtual void destroy() override;
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX) override;
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX) override;
    virtual std::string& name() override;
    virtual void* get_interface() override;

  public:
    virtual void* get_plugin_interface(const std::string &plugin_name) override;

  private:
    AM_RESULT init();
    void load_all_plugins();
    AMVCAPlugin* load_plugin(const char *name);
    static void frame_capture_entry(void *arg);
    AM_RESULT frame_capture();
    bool load_vca_feature();

  private:
    AMVCA(const char *name);
    virtual ~AMVCA();

  protected:
    std::string                 m_name;

  private:
    AMThread*             m_thread = nullptr;
    AMVCAPluginMap        m_plugin_map;
    AMIVCAPluginMap       m_started_plugin_map;
    vector<string>        m_vca_feature;
    std::atomic<bool>     m_capture_loop_run;
    std::atomic<bool>     m_started;
};

#endif /* ORYX_VIDEO_CONTROL_VCA_FRAMEWORK_AM_VCA_FRAMEWORK_H_ */
