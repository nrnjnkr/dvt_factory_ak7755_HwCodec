/*******************************************************************************
 * am_face_detect_types.h
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
#ifndef ORYX_INCLUDE_VIDEO_AM_FACE_DETECT_TYPES_H_
#define ORYX_INCLUDE_VIDEO_AM_FACE_DETECT_TYPES_H_

#include "am_video_types.h"

#define DMAX_FACE_NUMBER 16

struct AMFDINFO {
    uint8_t *after_transform_addr; // rgb
    uint8_t *y_addr;
    uint8_t *uv_addr;
    uint8_t after_transform_w; // rgb
    uint8_t after_transform_h; // rgb
    uint32_t pitch;
    int32_t face_rect_x;
    int32_t face_rect_y;
    int32_t face_rect_w;
    int32_t face_rect_h;
    AMPoint face_ldmk[5];
    std::pair<bool, std::vector<float>> eigenvalues;
};

struct AMFDYUVINFO {
    uint8_t *y_addr;
    uint8_t *uv_addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

struct AMFDMessage
{
    int face_num;
    AMFDINFO face_info[DMAX_FACE_NUMBER];
    AMFDYUVINFO yuv_info;
    AMRect capture_window;
};

typedef int32_t (*AMFDCallback)(void *owner, AMFDMessage *event_msg);

#endif /* ORYX_INCLUDE_VIDEO_AM_FACE_DETECT_TYPES_H_ */
