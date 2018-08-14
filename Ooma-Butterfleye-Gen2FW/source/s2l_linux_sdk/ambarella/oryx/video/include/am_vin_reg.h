/*******************************************************************************
 * am_vin_reg.h
 *
 * History:
 *   Jun 13, 2017 - [ypchang] created file
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
#ifndef AM_VIN_REG_H_
#define AM_VIN_REG_H_

#include "am_video_types.h"
#include "am_platform_if.h"
#include <vector>

struct AMVinRegConfig
{
    AM_VIN_MODE sensor_mode  = AM_VIN_MODE_AUTO;
    std::string sensor_name;
    AMVinRegVector reg_list;
    AMVinRegConfig(){}
    AMVinRegConfig(const AMVinRegConfig &config) :
      sensor_mode(config.sensor_mode),
      sensor_name(config.sensor_name),
      reg_list(config.reg_list)
    {}
    void print()
    {
      for (auto &r : reg_list) {
        NOTICE("0x%08X: 0x%08X", r.reg_addr, r.reg_data);
      }
    }
};
typedef std::vector<AMVinRegConfig> AMVinRegConfigVector;

class AMVin;
class AMVinRegister
{
    friend class AMVin;
  public:
    static AMVinRegister* create(AM_VIN_ID id);

  public:
    void destroy();
    AM_RESULT apply(AM_VIN_MODE mode, std::string &name);

  private:
    AMVinRegister(AM_VIN_ID id);
    virtual ~AMVinRegister();
    AM_RESULT init();

  private:
    AM_RESULT get_vin_reg_list();

  private:
    AM_VIN_ID            m_id       = AM_VIN_ID_INVALID;
    AMIPlatformPtr       m_platform = nullptr;
    AMVinRegConfigVector m_vin_regs_list;
};

#endif /* AM_VIN_REG_H_ */
