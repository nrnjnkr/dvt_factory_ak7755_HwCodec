/*******************************************************************************
 * am_websocket_protocol.cpp
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
#include <arpa/inet.h>
#include <unistd.h>
#include "openssl/sha.h"
#include "am_websocket_protocol.h"

#define RFC6545_MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
using std::getline;
//connection close status code:1000
static uint8_t default_close_code[] = {0x03, 0xe8};

AMWebSocket::AMWebSocket():
  m_fd(-1)
{
}

AMWebSocket::~AMWebSocket()
{
}

AMIWebSocket* AMIWebSocket::create(int32_t fd)
{
  return ((AMIWebSocket*)AMWebSocket::create(fd));
}

AMWebSocket* AMWebSocket::create(int32_t fd)
{
  AMWebSocket* ret = new AMWebSocket();
  if (ret && !(ret->init(fd))) {
    delete ret;
    ret = nullptr;
  }
  return ret;
}

bool AMWebSocket::init(int32_t fd)
{
  bool ret = true;
  do {
    if (fd < 0) {
      ERROR("Invalid fd: %d\n",fd);
      ret = false;
      break;
    }
    m_fd = fd;
  } while (0);

  return ret;
}

void AMWebSocket::destroy()
{
  delete this;
}

bool AMWebSocket::handshake()
{
  bool ret = true;
  do {
    if (!(ret = get_handshake_info())) {
      break;
    }
    if (!(ret = response_handshake())) {
      break;
    }
    INFO("HandShake successed!");
  } while (0);

  return ret;
}

int32_t AMWebSocket::get_frame_data(AM_WS_FRAME_TYPE &type, uint8_t *data)
{
  int32_t len = -1;
  uint8_t *buf;
  do {
    buf = new uint8_t[MAX_PAYLOAD_SIZE];
    if (!buf) {
      ERROR("Allocate memory failed!\n");
      break;
    }
    if(read(m_fd, buf, MAX_PAYLOAD_SIZE) < 3) {
      ERROR("Invalid frame data!\n");
      break;
    }

    uint8_t opcode = buf[0] & 0x0F;
    uint8_t fin = (buf[0] >> 7) & 0x01;
    if ((fin == 1) && (opcode == 1)) {
      type = AM_WS_FRAME_TEXT;
    } else if ((fin == 1) && (opcode == 2)) {
      type = AM_WS_FRAME_BINARY;
    } else if ((fin == 1) && (opcode == 8)) {
      type = AM_WS_FRAME_CLOSE;
      break;
    }
    uint8_t masked = (buf[1] >> 7) & 0x01;
    int32_t pos = 2;
    int32_t len_field = buf[1] & (~0x80);
    uint32_t mask = 0;

    if(len_field <= 125) {
      len = len_field;
    } else if(len_field == 126) {
      len = (buf[2] << 8) + buf[3];
      if (len > MAX_PAYLOAD_DATA_SIZE) {
        WARN("Just support max data size = %d!\n", MAX_PAYLOAD_DATA_SIZE);
        len = MAX_PAYLOAD_DATA_SIZE;
      }
      pos += 2;
    } else if(len_field == 127) {
      ERROR("Don't support 64bit size data!\n");
      break;
    }

    if(masked) {
      mask = *((uint32_t*)(buf + pos));
      pos += 4;
      uint8_t* c = buf + pos;
      for(int32_t i = 0; i < len; ++i) {
        c[i] = c[i] ^ ((uint8_t*)(&mask))[i % 4];
      }
    }

    if (data) {
      memcpy(data, buf + pos, len);
    } else {
      len = 0;
    }
  } while (0);

  delete[] buf;
  return len;
}

bool AMWebSocket::send_frame_data(AM_WS_FRAME_TYPE type,
                                  uint8_t *data, int32_t size)
{
  bool ret = true;
  uint8_t *buf = nullptr;
  do {
    int32_t pos = 0;
    uint8_t opcode = 0;
    uint8_t fin = 0;

    if (type == AM_WS_FRAME_TEXT) {
      fin = 1 << 7;
      opcode = 1;
    } else if (type == AM_WS_FRAME_BINARY) {
      fin = 1 << 7;
      opcode = 2;
    } else if (type == AM_WS_FRAME_CLOSE) {
      fin = 1 << 7;
      opcode = 8;
      if (!data) {
        size = sizeof(default_close_code);
        data = default_close_code;
      }
    }
    int32_t buf_size = (size <= 125 ? size + 2 :
        size <= 65535 ? size + 4 : size + 10);
    buf = new uint8_t[buf_size];
    if (!buf) {
      ERROR("Allocate memory failed!\n");
      ret =false;
      break;
    }
    buf[pos++] = fin + opcode;

    if (size <= 125) {
      buf[pos++] = size;
    } else if (size <= 65535) {
      buf[pos++] = 126;
      buf[pos++] = (size >> 8) & 0xFF;
      buf[pos++] = size & 0xFF;
    } else {
      buf[pos++] = 127;
      // write 8 bytes length (significant first)
      // since size is int it can be no longer than 4 bytes = 2^32-1
      // padd zeroes for the first 4 bytes
      for(int32_t i = 0; i < 4; ++i) {
        buf[pos++] = 0;
      }
      // write the actual 32bit msg_length in the next 4 bytes
      for(int32_t i = 3; i >= 0; i--) {
        buf[pos++] = ((size >> 8 * i) & 0xFF);
      }
    }
    if (data) {
      memcpy(buf+pos, data, size);
    }

    ret = (write(m_fd, buf, buf_size) == -1) ? false : true;
  } while (0);

  delete[] buf;
  return ret;
}

bool AMWebSocket::query_handshake_info(const string &key, string &value)
{
  bool ret = true;
  HANDSHAKEDATAMAP::iterator itr = m_handshake_data.find(key);
  if( itr != m_handshake_data.end()) {
    value = itr->second;
  } else {
    ret = false;
  }
  return ret;
}

int32_t AMWebSocket::get_socket_fd()
{
  return m_fd;
}

bool AMWebSocket::get_handshake_info()
{
  bool ret = true;
  do {
    string deli;
    string key;
    string value;
    uint8_t buf[MAX_HTTP_HEAD_SIZE];
    size_t start = 0;
    size_t end = 0;
    int32_t len = 0;
    if((len = read(m_fd, buf, MAX_HTTP_HEAD_SIZE)) <= 0) {
      ret = false;
      break;
    }


    string http_content((char*)buf, len);
    uint32_t content_end = http_content.find("\r\n\r\n");
    if(content_end == string::npos) {
      ERROR("No http header info during handshake!\n");
      ret = false;
      break;
    }

    //parse http header info
    deli = "\r\n";
    while ( end != string::npos ) {
      end = http_content.find(deli, start );
      len = (end == string::npos) ? string::npos : end - start;

      if ((len > 0) && (start < http_content.size())) {
        string h = http_content.substr(start, len);
        string::size_type pos = h.find(": ");
        if(pos != string::npos) {
          key = h.substr(0, pos);
          value = h.substr(pos + 2);
          m_handshake_data[key] = value;
        }
      }
      start = ((end > (string::npos - deli.size()))
          ? string::npos : end + deli.size());
    }
    start = end = 0;

    //parse query info
    deli = "&";
    if(http_content.find("GET") == 0) {
      string::size_type pos = 0;
      if ((pos = http_content.find("?")) != string::npos) {
        string query_str = http_content.substr(pos + 1, http_content.find("HTTP")-7);
        string sub_qstr;
        while((end = query_str.find(deli, start)) != string::npos) {
          len = end - start;
          sub_qstr = query_str.substr(start, len);
          key = sub_qstr.substr(0, sub_qstr.find("="));
          value = sub_qstr.substr(sub_qstr.find("=") + 1, end);
          start = end + deli.size();
          m_handshake_data[key] = value;
        }
        sub_qstr = query_str.substr(start, end);
        key = sub_qstr.substr(0, sub_qstr.find("="));
        value = sub_qstr.substr(sub_qstr.find("=") + 1);
        m_handshake_data[key] = value;
      }
    }

    for (auto &m : m_handshake_data) {
      INFO("%s: %s\n",m.first.c_str(), m.second.c_str());
    }
  } while (0);

  return ret;
}

bool AMWebSocket::response_handshake()
{
  bool ret = true;
  string response;
  do {
    response += "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Connection: upgrade\r\n";
    std::string svr_key = m_handshake_data["Sec-WebSocket-Key"];
    svr_key += RFC6545_MAGIC_KEY;

    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, svr_key.c_str(), svr_key.length());
    uint8_t message_digest[SHA_DIGEST_LENGTH];
    SHA1_Final(message_digest, &ctx);

    svr_key = base64_encode(message_digest, SHA_DIGEST_LENGTH);
    response += "Sec-WebSocket-Accept: " + (svr_key) + "\r\n";
    response += "Upgrade: websocket\r\n\r\n";
    INFO("\n%s\n",response.c_str());
    ret = (write(m_fd, response.c_str(), response.size()) == -1) ? false : true;
  } while (0);

  return ret;
}
