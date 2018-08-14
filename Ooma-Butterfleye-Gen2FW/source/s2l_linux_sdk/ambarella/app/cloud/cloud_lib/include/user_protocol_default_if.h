 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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

/**
 * user_protocol_default.h
 */

#ifndef __USER_PROTOCOL_DEFAULT_H__
#define __USER_PROTOCOL_DEFAULT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DUPDefaultCameraZoomCmdTLVPayloadLength 16
#define DUPDefaultCameraZoomCmdLength 18
#define DUPDefaultCameraZoomV2CmdTLVPayloadLength 12
#define DUPDefaultCameraZoomV2CmdLength 14
#define DUPDefaultCameraBitrateCmdTLVPayloadLength 4
#define DUPDefaultCameraBitrateCmdLength 6
#define DUPDefaultCameraFramerateCmdTLVPayloadLength 4
#define DUPDefaultCameraFramerateCmdLength 6

//camera related
typedef enum {
    EUPDefaultCameraTLV8Type_Invalid = 0x00,

    //
    EUPDefaultCameraTLV8Type_Standby = 0x01,
    EUPDefaultCameraTLV8Type_Streaming = 0x02,
    EUPDefaultCameraTLV8Type_DisableAudio = 0x03,
    EUPDefaultCameraTLV8Type_EnableAudio = 0x04,

    //
    EUPDefaultCameraTLV8Type_Zoom = 0x20,
    EUPDefaultCameraTLV8Type_ZoomV2 = 0x21,
    EUPDefaultCameraTLV8Type_UpdateBitrate = 0x22,
    EUPDefaultCameraTLV8Type_UpdateFramerate = 0x23,

} EUPDefaultCameraTLV8Type;

TU32 gfUPDefaultCamera_BuildZoomCmd(TU8 *payload, TU32 buffer_size, TU32 offset_x, TU32 offset_y, TU32 width, TU32 height);
EECode gfUPDefaultCamera_ParseZoomCmd(TU8 *payload, TU32 &offset_x, TU32 &offset_y, TU32 &width, TU32 &height);

TU32 gfUPDefaultCamera_BuildZoomV2Cmd(TU8 *payload, TU32 buffer_size, TU32 zoom_factor, TU32 zoom_offset_x, TU32 zoom_offset_y);
EECode gfUPDefaultCamera_ParseZoomV2Cmd(TU8 *payload, TU32 &zoom_factor, TU32 &zoom_offset_x, TU32 &zoom_offset_y);

TU32 gfUPDefaultCamera_BuildBitrateCmd(TU8 *payload, TU32 buffer_size, TU32 bitrateKbps);
EECode gfUPDefaultCamera_ParseBitrateCmd(TU8 *payload, TU32 &bitrateKbps);

TU32 gfUPDefaultCamera_BuildFramerateCmd(TU8 *payload, TU32 buffer_size, TU32 framerate);
EECode gfUPDefaultCamera_ParseFramerateCmd(TU8 *payload, TU32 &framerate);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

