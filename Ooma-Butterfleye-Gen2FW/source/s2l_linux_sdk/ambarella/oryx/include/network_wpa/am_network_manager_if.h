/*******************************************************************************
 * am_network_manager_if.h
 *
 * History:
 *   Jul 12, 2016 - [longli] created file
 *
 * Copyright (C) 2016, Ambarella Co, Ltd.
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
#ifndef ORYX_NETWORK_NEW_INCLUDE_AM_NETWORK_MANAGER_if_H_
#define ORYX_NETWORK_NEW_INCLUDE_AM_NETWORK_MANAGER_if_H_

#include "am_network_structs.h"

class AMINetworkManger
{
  public:
    static AMINetworkManger *create();

    virtual bool set_wifi_enable(const bool wifi_enable) = 0;
    virtual bool set_sta_enable(const bool sta_enable) = 0;
    virtual bool set_ap_enable(const bool ap_enable) = 0;
    virtual bool set_mode_cfg(const AMModeCfg &mode_cfg) = 0;
    virtual bool set_eth_cfg(const AMEthCfg &eth_cfg) = 0;
    virtual bool set_sta_cfg(const AMStaCfg &sta_cfg) = 0;
    virtual bool set_ap_cfg(const AMAPCfg &ap_cfg) = 0;
    virtual bool network_setup() = 0;
    virtual bool save_network_config() = 0;

    virtual bool list_ap() = 0;
    virtual bool wifi_status(const char *ifname = nullptr) = 0;
    virtual bool eth_status(const char *ifname = nullptr) = 0;
    virtual bool all_status() = 0;

    virtual void get_eth_cfg(AMEthCfg &eth_cfg) = 0;
    virtual void get_wifi_mode_cfg(AMModeCfg &mode_cfg) = 0;
    virtual void get_sta_cfg(AMStaCfg &sta_cfg) = 0;
    virtual void get_ap_cfg(AMAPCfg &ap_cfg) = 0;

    virtual ~AMINetworkManger(){};

};

#endif /* ORYX_NETWORK_NEW_INCLUDE_AM_NETWORK_MANAGER_H_ */
