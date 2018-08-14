/*
 * bpi_mcu_event_monitor.h
 *
 * History:
 *       2016/09/19 - [CZ Lin] created file
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
#ifndef __BPI_MCU_EVENT_MONITOR_H_
#define __BPI_MCU_EVENT_MONITOR_H_

#include "bpi_typedefs.h"

#define DAREDEVIL_SOUND_PATH  "/usr/local/bin/doorbell.wav"
#define ELEKTRA_SOUND_PATH  "/usr/local/bin/elektra-s.wav"

class IMcuEventObserver{
public:
    virtual void update(MCU_EVENT_TYPE event_type, int data1 )= 0;
};

class BPIRecorder;

class McuEventMonitor: public IMcuEventObserver{
public:
    McuEventMonitor();
    McuEventMonitor & operator=(const McuEventMonitor&) = delete;
    McuEventMonitor(const McuEventMonitor&) = delete;
    ~McuEventMonitor();
    void set_cloud_agent(void* handle);
    unsigned int get_timer_start_point();
    void init_timer_start_point();
    virtual void update(MCU_EVENT_TYPE event_type, int data1);

private:
    void* m_cloud_agent;
    //m_timer_start_point -- used in record mode 1: record for a fixed period of time after motion stops(no "PIR on" singal)
    unsigned int m_timer_start_point;
};
#endif
