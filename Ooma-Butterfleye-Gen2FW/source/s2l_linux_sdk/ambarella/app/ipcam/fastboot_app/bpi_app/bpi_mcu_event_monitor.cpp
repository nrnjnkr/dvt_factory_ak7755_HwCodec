/*
 * bpi_mcu_event_monitor.cpp
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
#include "device.h"
#include "bpi_typedefs.h"
#include "bpi_utils.h"
#include "bpi_app_config.h"
#include "bpi_mcu_event_monitor.h"

McuEventMonitor::McuEventMonitor():
    m_cloud_agent(NULL),
    m_timer_start_point(0){
}

McuEventMonitor::~McuEventMonitor(){
}

void McuEventMonitor::set_cloud_agent(handle_t handle)
{
    m_cloud_agent = handle;
}

unsigned int McuEventMonitor::get_timer_start_point(){
    return m_timer_start_point;
}

void McuEventMonitor::init_timer_start_point(){
    m_timer_start_point = get_current_time();
}

void McuEventMonitor::update(MCU_EVENT_TYPE event_type, int data1 ){
    msg_t msg;
    if (MCU_EVENT_PWR == event_type){
        return;
    }
    switch(event_type){
        case MCU_EVENT_PIR_ON:
        case MCU_EVENT_PIR_OFF:
        {
            msg.what = MSG_DEVICE_PIR;
            if(event_type == MCU_EVENT_PIR_ON){
                msg.para1 = EPIR_ON;
                m_timer_start_point = 0;
            }else{
                msg.para1 = EPIR_OFF;
                m_timer_start_point = get_current_time();
            }
            break;
        }
        case MCU_EVENT_BATTERY_EMPTY:
        case MCU_EVENT_BATTERY_LOW:
        case MCU_EVENT_BATTERY_NORMAL:
        {
            int battery_quantity = (int)data1;
            msg.what = MSG_DEVICE_BATTERY;
            msg.para2 = battery_quantity;
            if(event_type == MCU_EVENT_BATTERY_EMPTY){
                msg.para1 = EBattery_Empty;
                if(battery_quantity > 5){
                    LOG_ERROR("battery quantity(%d) incorrect under EBattery_Empty", battery_quantity);
                    return;
                }
            }else if(event_type == MCU_EVENT_BATTERY_LOW){
                msg.para1 = EBattery_Low;
                if(battery_quantity <= 5 || battery_quantity > 20){
                    LOG_ERROR("battery quantity(%d) incorrect under EBattery_Low", battery_quantity);
                    return;
                }
            }else{
                msg.para1 = EBattery_Normal;
                if(battery_quantity <= 20 || battery_quantity > 100){
                    LOG_ERROR("battery quantity(%d) incorrect under EBattery_Normal", battery_quantity);
                    return;
                }
            }
            break;
        }
        case MCU_EVENT_DC_ON:
        case MCU_EVENT_DC_OFF:
        {
            msg.what = MSG_DEVICE_DC;
            msg.para1 = (event_type == MCU_EVENT_DC_ON) ? EDC_Plugin : EDC_Unplugged;
            break;
        }
        default:
            LOG_ERROR("unhandled mcu event(%d)\n", event_type);
            break;
    }

    if(m_cloud_agent){
        device_msg_request(m_cloud_agent, &msg, NULL);
    }
}
