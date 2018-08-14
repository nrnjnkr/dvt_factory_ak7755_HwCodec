/*
 * bpi_mcu_proxy.h
 *
 * History:
 *       2016/09/01 - [Zhipeng Dong] created file
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
 */

#ifndef __BPI_MCU_PROXY_H_
#define __BPI_MCU_PROXY_H_
#include <time.h>
#include <thread>
#include <condition_variable>
#include <chrono>
using std::thread;
using std::condition_variable;
using std::mutex;

class IMcuEventObserver;

class BPIMcuProxy{
public:
    BPIMcuProxy();
    ~BPIMcuProxy();
    MCU_TRIGGER_TYPE get_trigger_event();
    MCU_TRIGGER_TYPE get_competition_trigger_event();
    int get_time_cost();
    int get_mcu_version();
    void send_cmd(MCU_CMD_TYPE cmd_type);
    void attach(IMcuEventObserver* observer);
    bool init();
    bool wait_on_event(int event_type, int* data, int timeout_ms);


private:
    bool mcu_connect();
    static void on_mcu_events(void *arg);
    int m_uart_fd;
    int m_pipe_fd[2];
    thread* m_thread;
    IMcuEventObserver* m_observer;
    condition_variable m_cv;
    mutex m_mutex_cv;
    MCU_EVENT_TYPE m_cur_event;
    int m_cur_data1;
};

#define MCU_MAIN_VERSION(x) ((x>>8)&0xFF)
#define MCU_SUB_VERSION(x)  (x&0xFF)

#endif //__BPI_MCU_PROXY_H_
