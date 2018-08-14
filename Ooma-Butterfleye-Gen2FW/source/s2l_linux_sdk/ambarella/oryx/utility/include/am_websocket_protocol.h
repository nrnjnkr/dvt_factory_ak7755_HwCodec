/*******************************************************************************
 * am_websocket_protocol.h
 *
 * History:
 *   May 25, 2017 - [Huaiqing Wang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef AM_WEBSOCKET_PROTOCOL_H_
#define AM_WEBSOCKET_PROTOCOL_H_

#include <map>
#include "am_websocket_protocol_if.h"
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

using std::string;
using std::map;

typedef map<string, string> HANDSHAKEDATAMAP;
class AMWebSocket: public AMIWebSocket
{
    friend class AMIWebSocket;

  public:
    void destroy();

    bool handshake();

    int32_t  get_frame_data(AM_WS_FRAME_TYPE &type, uint8_t *data);
    bool send_frame_data(AM_WS_FRAME_TYPE type, uint8_t *data, int32_t size);

    int32_t  get_socket_fd();
    bool query_handshake_info(const string &key, string &value);

  private:
    AMWebSocket();
    virtual ~AMWebSocket();
    static AMWebSocket* create(int32_t fd);

    bool init(int32_t fd);
    bool get_handshake_info();
    bool response_handshake();

  private:
    int32_t m_fd;
    HANDSHAKEDATAMAP  m_handshake_data;
};



#endif /* AM_WEBSOCKET_PROTOCOL_H_ */
