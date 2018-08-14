/*******************************************************************************
 * am_websocket_protocol_if.h
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
#ifndef AM_WEBSOCKET_PROTOCOL_IF_H_
#define AM_WEBSOCKET_PROTOCOL_IF_H_

#include <string>

#define MAX_HTTP_HEAD_SIZE 1024
//max upload payload data size
#define MAX_PAYLOAD_DATA_SIZE 2048
//Payload len <= 126 here
#define MAX_PAYLOAD_SIZE (MAX_PAYLOAD_DATA_SIZE + 8)
/*
 *   0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-------+-+-------------+-------------------------------+
 *  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *  | |1|2|3|       |K|             |                               |
 *  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *  |     Extended payload length continued, if payload len == 127  |
 *  + - - - - - - - - - - - - - - - +-------------------------------+
 *  |                               |Masking-key, if MASK set to 1  |
 *  +-------------------------------+-------------------------------+
 *  | Masking-key (continued)       |          Payload Data         |
 *  +-------------------------------- - - - - - - - - - - - - - - - +
 *  :                     Payload Data continued ...                :
 *  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *  |                     Payload Data continued ...                |
 *  +---------------------------------------------------------------+
 */

enum AM_WS_FRAME_TYPE
{
  AM_WS_FRAME_INVALID = -1,
  AM_WS_FRAME_TEXT = 0,
  AM_WS_FRAME_BINARY,
  AM_WS_FRAME_CLOSE,
};

class AMIWebSocket
{
  public:
    static AMIWebSocket* create(int32_t fd);
    virtual void destroy() = 0;

    virtual bool handshake() = 0;
    virtual int32_t  get_frame_data(AM_WS_FRAME_TYPE &type, uint8_t *data) = 0;
    virtual bool send_frame_data(AM_WS_FRAME_TYPE type, uint8_t *data, int32_t size) = 0;

    virtual int32_t  get_socket_fd() = 0;
    virtual bool query_handshake_info(const std::string &key, std::string &value) = 0;

  protected:
    virtual ~AMIWebSocket(){};
};



#endif /* AM_WEBSOCKET_PROTOCOL_IF_H_ */
