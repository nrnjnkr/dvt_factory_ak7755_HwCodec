/*******************************************************************************
 * am_vca_framework_if.h
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
#ifndef ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_FRAMEWORK_IF_H_
#define ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_FRAMEWORK_IF_H_

#include "am_video_types.h"

class AMIVCA
{
  public:
    virtual void destroy()                                              = 0;
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX)              = 0;
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX)               = 0;
    virtual void* get_plugin_interface(const std::string &plugin_name)  = 0;

  protected:
    virtual ~AMIVCA(){}
};

#define VIDEO_PLUGIN_VCA    ("video-vca")
#define VIDEO_PLUGIN_VCA_SO ("video-vca.so")

#endif /* ORYX_VIDEO_CONTROL_VCA_INCLUDE_AM_VCA_FRAMEWORK_IF_H_ */
