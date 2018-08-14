/*******************************************************************************
 * am_vca_plugin_if.h
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
#ifndef ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_PLUGIN_IF_H_
#define ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_PLUGIN_IF_H_

#include <string>

#include "am_vca_framework_types.h"

class AMIVCAPlugin
{
  public:
    virtual void destroy()                                      = 0;
    virtual bool start()                                        = 0;
    virtual bool stop()                                         = 0;
    virtual std::string& name()                                 = 0;
    virtual void* get_interface()                               = 0;
    virtual bool data_loop(const AMQueryFrameDesc &desc)        = 0;
    virtual bool get_buffer_config(AMVCABuf &buf_config)        = 0;
    virtual ~AMIVCAPlugin(){}
};

typedef AMIVCAPlugin* (*CreateVCAPlugin)();

#define CREATE_VCA_PLUGIN ((const char*)"create_vca_plugin")

#ifdef BUILD_AMBARELLA_ORYX_VIDEO_VCA_PLUGIN_DIR
#define VIDEO_VCA_PLUGIN_DIR ((const char*)BUILD_AMBARELLA_ORYX_VIDEO_VCA_PLUGIN_DIR)
#else
#define VCA_PLUGIN_DIR ((const char*)"/usr/lib/oryx/video/vca")
#endif

#endif /* ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_PLUGIN_IF_H_ */
