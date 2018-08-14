/*
 * bpi_vca_hawxeye.cpp
 *
 * History:
 *       2016/09/13 - [niu zhifeng] created file
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

 //hawxeye library is built with gcc4.x, the abi format is different when use c++11, so use the older abi format here
#define _GLIBCXX_USE_CXX11_ABI 0

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "bpi_utils.h"
#include "bpi_vca_hawxeye.h"
#include "hreturn.hpp"
#include "QuickMotionDetector.h"

VCA *VCA::get_instance()
{
    static HawxeyeVCA instance;
    LOG_PRINT("HawxeyeVCA instance\n");
    return &instance;
}

HawxeyeVCA::HawxeyeVCA() {
        LOG_DEBUG("%s entered\n", __FUNCTION__);
}

HawxeyeVCA::~HawxeyeVCA() {
    LOG_DEBUG("%s entered \n", __FUNCTION__);
}

int HawxeyeVCA::pre_vca()
{
    // Initialize
    if (QuickMotionDetectorInitialize("/usr/bin/configs/client.xml") != RETURN_SUCCESS) {
        LOG_ERROR("QuickMotionDetector initialize failed\n");
        return -1;
    }
    LOG_PRINT("VCA initialized \n");
    return 0;
}
#include <sys/time.h>

int HawxeyeVCA::do_vca()
{
    struct timeval tm, tm1;
    gettimeofday(&tm, NULL);

    bool detections;
    if (QuickMotionDetectorExecute(&detections, m_buffer_y, m_buffer_width, m_buffer_height) != RETURN_SUCCESS) {
        LOG_ERROR("QuickMotionDetector execute failed\n");
        return -1;
    }
    gettimeofday(&tm1, NULL);
    LOG_DEBUG("hawxeye do vca done, cost-> %ld ms\n", tm1.tv_sec*1000+tm1.tv_usec/1000-tm.tv_sec*1000-tm.tv_usec/1000);

    if (detections) {
        LOG_PRINT("object detected.");
        return 0;
    }

    return 1;
}

int HawxeyeVCA::post_vca()
{
    // Destroy VideoAnalysisManager regardless of whether object was detected
    if (RETURN_SUCCESS != QuickMotionDetectorDestroy()) {
        LOG_ERROR("QuickMotionDetector destroy failed\n");
        return -1;
    }

    return 0;
}
