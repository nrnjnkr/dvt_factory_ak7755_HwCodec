/*******************************************************************************
 * am_face_detect.cpp
 *
 * History:
 *   Sep 19, 2017 - [Guohua Zheng] created file
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

#ifndef ORYX_INCLUDE_HAWXEYE_TYPES_H_
#define ORYX_INCLUDE_HAWXEYE_TYPES_H_
#include "am_video_types.h"

struct AMHxeBuf
{
    enum AM_SOURCE_BUFFER_ID buf_id;
    enum AM_DATA_FRAME_TYPE buf_type;
};

enum AMHxeObjectType
{
    AM_HXE_TYPE_UNCLASSIFIED,
    AM_HXE_TYPE_HUMAN,
    AM_HXE_TYPE_FACE,
    AM_HXE_TYPE_CAR,
    AM_HXE_TYPE_PET,
    AM_HXE_TYPE_PACKAGE_ADDED,
    AM_HXE_TYPE_PACKAGE_REMOVED,
    AM_HXE_TYPE_PACKAGE_MOVED,
    AM_HXE_TYPE_PACKAGE_SWAPPED,
    AM_HXE_TYPE_PACKAGE_EVENT
};

enum AMHxeObjectDirection
{
  AM_HXE_DIRECTION_NONE,
  AM_HXE_DIRECTION_TOWARDS,
  AM_HXE_DIRECTION_AWAY,
  AM_HXE_DIRECTION_LEFT,
  AM_HXE_DIRECTION_RIGHT,
  AM_HXE_DIRECTION_TOWARDS_LEFT,
  AM_HXE_DIRECTION_TOWARDS_RIGHT,
  AM_HXE_DIRECTION_AWAY_LEFT,
  AM_HXE_DIRECTION_AWAY_RIGHT
};
struct AMHxeObjectInfo
{
    int32_t id;
    int32_t engine_id;
    time_t timestamp;
    AMHxeObjectType type;
    uint32_t object_lt_x; //left top x
    uint32_t object_lt_y; //left top y
    uint32_t object_rb_x; //right bottom x
    uint32_t object_rb_y; //right bottom y
};

typedef std::vector<AMHxeObjectInfo> AMHxeMsg;
typedef int32_t (*AMHxeCallback)(void *owner, AMHxeMsg *event_msg);
#endif
