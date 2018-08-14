/*******************************************************************************
 * am_rtp_client_if.h
 *
 * History:
 *   Jun 28, 2017 - [ypchang] created file
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
#ifndef AM_RTP_CLIENT_IF_H_
#define AM_RTP_CLIENT_IF_H_

#include "am_rtp_msg.h"

class AMMuxerRtp;
class AMIRtpSession;
class AMIRtpClient
{
  public:
    virtual bool is_alive()                                                 = 0;
    virtual bool is_abort()                                                 = 0;
    virtual bool is_enable()                                                = 0;
    virtual bool is_new_client()                                            = 0;
    virtual bool start()                                                    = 0;
    virtual void stop()                                                     = 0;
    virtual void destroy(bool send_notify = true)                           = 0;
    virtual void send_ack()                                                 = 0;
    virtual void inc_ref()                                                  = 0;
    virtual void set_enable(bool enable)                                    = 0;
    virtual void set_new_client(bool new_client)                            = 0;
    virtual void set_session(AMIRtpSession *session)                        = 0;
    virtual void set_start_clock_90k(uint64_t clock)                        = 0;
    virtual uint64_t get_start_clock_90k()                                  = 0;
    virtual uint32_t ssrc()                                                 = 0;

  protected:
    virtual ~AMIRtpClient(){}
};

AMIRtpClient* create_rtp_client(const std::string &config,
                                int proto_fd,
                                int tcp_fd,
                                int ctrl_fd,
                                uint32_t session,
                                AMMuxerRtp *muxer,
                                const AMRtpClientInfo &clientInfo);

#endif /* AM_RTP_CLIENT_IF_H_ */
