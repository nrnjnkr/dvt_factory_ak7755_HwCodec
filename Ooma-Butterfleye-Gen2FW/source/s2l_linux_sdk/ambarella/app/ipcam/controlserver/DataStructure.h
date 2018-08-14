/**********************************************************************
 * DataStructure.h
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

#ifndef APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H
#define APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H
#include "NetDataStructure.h"
#include "VideoDataStructure.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
/* Struct ConfigData is common type of NetConfigData and VideoConfigData */
struct ConfigData {
  NetCmdType cmd;
  char data[];
};

struct NetworkData {
  int buffer_size;
  struct sockaddr src_addr;
  ConfigData * config_data;

  /* Constructor */
  NetworkData (int protocol) {
    buffer_size = max(sizeof(VideoConfigData), sizeof(NetConfigData));
    memset (&src_addr, 0, sizeof(struct sockaddr));
    config_data = (ConfigData *)malloc(buffer_size*sizeof(char));
    memset (config_data, 0, buffer_size);
    src_addr.sa_family = protocol;
  }
  ~NetworkData () {
    if (config_data) {
      delete config_data;
    }
  }
  /* Methods */
  void * buf_addr () {
    return (void *)config_data;
  }
  size_t buf_size () {
    return buffer_size;
  }
  struct sockaddr * get_src_addr () {
    return &src_addr;
  }
  socklen_t src_addr_size () {
    return (socklen_t)sizeof(struct sockaddr);
  }
  NetCmdType get_data_type () {
    return config_data->cmd;
  }
  NetConfigData * get_net_config_data() {
    return (NetConfigData *)config_data;
  }
  VideoConfigData * get_video_config_data () {
    return (VideoConfigData *)config_data;
  }
};
#endif //APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H

