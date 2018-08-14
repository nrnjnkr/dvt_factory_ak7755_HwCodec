/*******************************************************************************
 * am_record_engine_config.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#ifndef ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_
#define ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_

#include "am_engine_config.h"

class AMConfig;
class AMRecordEngine;
class AMRecordEngineConfig
{
    friend class AMRecordEngine;
  public:
    AMRecordEngineConfig();
    virtual ~AMRecordEngineConfig();
    EngineConfig* get_config(const std::string &conf_file);

  private:
    inline bool filter_exists(std::string &filter);
    inline ConnectionConfig* find_connection_by_name(std::string &filter);

  private:
    AMConfig     *m_config;
    EngineConfig *m_engine_config;
};

#endif /* ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_ */