/*******************************************************************************
 * am_network_structs.h
 *
 * History:
 *   Jun 28, 2016 - [longli] created file
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
#ifndef ORYX_NETWORK_NEW_INCLUDE_AM_NETWORK_STRUCTS_H_
#define ORYX_NETWORK_NEW_INCLUDE_AM_NETWORK_STRUCTS_H_


enum AM_WIFI_MODE
{
  AM_NONE = 0,
  AM_STATION_MODE,
  AM_AP_MODE,
  AM_CONCURRENT_MODE
};

struct AMIfInfo
{
    std::string ifname;
    std::string mac;
    std::string ip;
    std::string mask;
    std::string gw;
    bool link_connected;
    bool if_up;
    bool wireless;
    AM_WIFI_MODE mode;
    std::string ssid;
    std::string bssid;
    std::string pairwise_cipher;
    std::string key_mgmt;
    std::string group_cipher;
};

struct AMEthCfg
{
    std::string ifname;
    std::string ip;
    std::string mask;
    std::string gw;
    bool dhcp_enable = false;
    bool enable = false;
};

struct AMStaCfg
{
    //to design
    std::string ssid;
    std::string passwd;
    std::string ip;
    std::string mask;
    std::string gw;
    bool dhcp_enable = true;
    bool hidden_ssid = false;
    bool enable = false;
};

struct AMAPCfg
{
    //to design
    std::string ssid;
    std::string passwd;
    //std::string encrypt;
    std::string ip;
    std::string mask;
    std::string gw;
    std::string dhcp_range;
    std::string lease;
    int32_t channel;
    bool hidden = false;
    bool enable = false;
};

struct AMModeCfg
{
    AM_WIFI_MODE mode = AM_NONE;
    std::string ifname;
    std::string con_if;
    std::string cfg;
};

struct AMWifiDriver
{
    std::string name;
    std::string ap_fw;
    std::string sta_fw;
    std::string con_fw;
    bool reload = false;
};

struct AMWifiCfg
{
    AMModeCfg mode_cfg;
    AMStaCfg sta_cfg;
    AMAPCfg ap_cfg;
    AMWifiDriver drv_cfg;
    bool enable = false;
};

#endif
