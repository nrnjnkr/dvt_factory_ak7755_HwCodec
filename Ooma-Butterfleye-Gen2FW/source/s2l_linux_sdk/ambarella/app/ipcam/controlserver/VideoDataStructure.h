/**********************************************************************
 * VideoDataStructure.h
 *
 * Histroy:
 *  2011年03月22日 - [Yupeng Chang] created file
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

#ifndef APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H
#define APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H

#include "mw_struct.h"
#include "NetCommand.h"
#include <string.h>

struct EncodeInfo {
  mw_encode_format encode_format;               /* Encode format info */
  union {
    mw_h264_config _h264;
    mw_jpeg_config _jpeg;
  } encode_type;                                /* Encode type H.264 or Mjpeg */
  void clean () {
    memset (&encode_type, 0, sizeof(encode_type));
    memset (&encode_format, 0, sizeof(mw_encode_format));
  }
  bool get_encode_info (int iav_fd, unsigned int stream_id);
};

struct EncodeSetting {
  bool stream_state[MW_MAX_STREAM_NUM];         /* STREAM STATUS  */
  EncodeInfo stream_info[MW_MAX_STREAM_NUM];

  void clean () {
    memset (stream_state, 0, sizeof(bool) * MW_MAX_STREAM_NUM);
  }

  bool get_stream_info (int iav_fd);
};

struct VideoConfigData {
  NetCmdType cmd;
  union {
    EncodeSetting _encode;
  }configData;
#define encodeSetting (configData._encode)

  VideoConfigData (NetCmdType command) {
    cmd = command;
    memset (&configData, 0, sizeof(configData));
  }
};
#endif //APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H

