/*******************************************************************************
 * bpi_oryx_export.h
 *
 * History:
 *   2017-01-17 - [Jian Liu]      created file
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
#ifndef BPI_ORYX_EXPORT_H_
#define BPI_ORYX_EXPORT_H_

#include <string>
#include "bpi_app_config.h"
#include "bpi_typedefs.h"
#include "am_record_if.h"

class OryxVideoModule;
class CaptureRoutine{
public:
    CaptureRoutine();
    void set_rotation(BPI_ROTATION_TYPE _rotation);
    void enable(const char *storage_folder);
    void disable();
    bool rename_file(std::string &target_name);
    ~CaptureRoutine();
private:
    bool parse_filename(std::string &file_name);
    bool capture(int stream_id, const char* file_name);
    BPI_ROTATION_TYPE m_rotation;
    bool m_enable;
    std::string m_file_name;
    std::string m_file_base;
};

class OryxRecorderWrapper{
public:
    OryxRecorderWrapper();
    ~OryxRecorderWrapper();
    bool init_recorder(app_conf_t *p_user_conf);
    bool start_recorder();
    bool stop_recorder();
    void set_data_handler(void* p_user_data);

    static void file_operation_callback(AMRecordFileInfo &file_info);
    static void* m_data_handler;
    static int s_stop_flag;
private:
    static CaptureRoutine s_capture;
    AMIRecordPtr m_record;
    unsigned int m_muxer_bitmap;
    int m_recording_duration;
    int m_file_number;
    int m_file_duration;
    OryxVideoModule *m_video_module;
};


#endif

