/*******************************************************************************
 * bpi_oryx_export.cpp
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
#include <string>
#include "bpi_utils.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_oryx_export.h"
#include "bpi_oryx_config.h"
#include "bpi_yuv_rotate.h"
#include "am_define.h"
#include "am_file.h"
#include "am_jpeg_encoder_if.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_record_msg.h"

#define THUMBNAIL_BUFFER_ID 1//use the same buffer that vca use, configed to 720x480

using std::string;

CaptureRoutine::CaptureRoutine():
    m_rotation(BPI_ROTATION_TYPE_NONE),
    m_enable(false),
    m_file_name(""),
    m_file_base("")
{
}

CaptureRoutine::~CaptureRoutine()
{
}

void CaptureRoutine::set_rotation(BPI_ROTATION_TYPE _rotation)
{
    m_rotation = _rotation;
}

void CaptureRoutine::enable(const char *storage_folder)
{
    m_enable = true;
    m_file_base = storage_folder;
    if(m_file_base.back() != '/'){
        m_file_base.append("/");
    }
    m_file_name = m_file_base + "BPI_snap.jpg";

    capture(THUMBNAIL_BUFFER_ID, m_file_name.c_str());
}

void CaptureRoutine::disable()
{
    m_enable = false;
}

bool CaptureRoutine::rename_file(string &target_name)
{
    if(m_enable){
        if(parse_filename(target_name) &&
            !rename(m_file_name.c_str(), target_name.c_str())){
            return true;
        }else{
            LOG_ERROR("m_file_name: %s, target_name: %s. "
                "Rename snapshot file name failed: %s\n", m_file_name.c_str(),
                target_name.c_str(), strerror(errno));
        }
    }

    return false;
}

bool CaptureRoutine::parse_filename(string &file_name)
{
    bool ret = false;
    string::size_type pos_tmp;
    do{
        pos_tmp = file_name.find_last_of('.');
        if(pos_tmp != string::npos){
            file_name.replace(file_name.begin() + pos_tmp,
                              file_name.end(), ".jpg");
        }else{
            LOG_ERROR("unrecognized file event(%s)!\n", file_name.c_str());
            break;
        }

        pos_tmp = file_name.find_last_of('/');
        if(pos_tmp != string::npos){
            file_name.replace(file_name.begin(),
                              file_name.begin() + pos_tmp + 1,
                              m_file_base);
        }else{
            LOG_ERROR("no valid name found(%s)!\n", file_name.c_str());
            break;
        }

        ret = true;
    }while(0);

    return ret;
}

bool CaptureRoutine::capture(int stream_id, const char* file_name)
{
    AMQueryFrameDesc frame_desc;
    AM_RESULT result = AM_RESULT_OK;
    AMIVideoReaderPtr video_reader = nullptr;
    AMIVideoAddressPtr video_address = nullptr;
    AMIJpegEncoderPtr jpeg_encoder = nullptr;
    AMJpegData* jpeg = nullptr;
    AMYUVData *yuv = nullptr;
    AMFile f(file_name);

    uint8_t *rotate_addr = nullptr;
    do {
        AMAddress yaddr;
        AMAddress uvaddr;
        video_reader = AMIVideoReader::get_instance();
        video_address = AMIVideoAddress::get_instance();
        jpeg_encoder = AMIJpegEncoder::get_instance();
        yuv = (AMYUVData *)calloc(1, sizeof(AMYUVData));
        if (!yuv) {
            LOG_ERROR("malloc yuv failed!\n");
            result = AM_RESULT_ERR_INVALID;
            break;
        }

        if (AM_RESULT_OK != jpeg_encoder->create()) {
            LOG_ERROR("jpeg_encoder create failed!\n");
            result = AM_RESULT_ERR_INVALID;
            break;
        }

        if (AM_RESULT_OK !=
                video_reader->query_yuv_frame(frame_desc,
                                              AM_SOURCE_BUFFER_ID(stream_id),
                                              false)) {
            result = AM_RESULT_ERR_INVALID;
            LOG_ERROR("query yuv frame failed \n");
            break;
        }

        if (AM_RESULT_OK != video_address->yuv_y_addr_get(frame_desc, yaddr)) {
            result = AM_RESULT_ERR_INVALID;
            LOG_ERROR("Failed to get y address!");
            break;
        }

        if (AM_RESULT_OK != video_address->yuv_uv_addr_get(frame_desc,
                                                           uvaddr)) {
            result = AM_RESULT_ERR_INVALID;
            LOG_ERROR("Failed to get uv address!");
            break;
        }
        LOG_DEBUG("yaddr: %p, max_size: %u, offset: %u\n",
                  (void*) yaddr.data, yaddr.max_size, yaddr.offset);
        LOG_DEBUG("uvaddr: %p, max_size: %u, offset: %u\n",
                  (void*) uvaddr.data, uvaddr.max_size, uvaddr.offset);
        LOG_DEBUG("frame desc, height: %u, width: %u, "
                  "y_offset: %u, uv_offset: %u, "
                  "pitch: %u, seq_num: %u\n",
                  frame_desc.yuv.height, frame_desc.yuv.width,
                  frame_desc.yuv.y_offset, frame_desc.yuv.uv_offset,
                  frame_desc.yuv.pitch, frame_desc.yuv.seq_num);

        if((frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV420)
            && (BPI_ROTATION_TYPE_NONE != m_rotation)){
            rotate_addr = (uint8_t*)malloc(
                frame_desc.yuv.width * frame_desc.yuv.height*3/2);
            if(!rotate_addr){
                LOG_ERROR("malloc rotate_addr failed!\n");
                result = AM_RESULT_ERR_INVALID;
                break;
            }
            if((m_rotation == BPI_ROTATION_TYPE_90)
                || (m_rotation == BPI_ROTATION_TYPE_270)){
                yuv->width = frame_desc.yuv.height;
                yuv->height = frame_desc.yuv.width;
                yuv->pitch = frame_desc.yuv.height;
                yuv->format = frame_desc.yuv.format;
            }else{
                yuv->width = frame_desc.yuv.width;
                yuv->height = frame_desc.yuv.height;
                yuv->pitch = frame_desc.yuv.width;
                yuv->format = frame_desc.yuv.format;
            }
            BPIYUVRotate::BPIYUV420Rotate(rotate_addr,
                                          yaddr.data,
                                          uvaddr.data,
                                          frame_desc.yuv.width,
                                          frame_desc.yuv.height,
                                          frame_desc.yuv.pitch,
                                          m_rotation);
            yuv->y_addr = rotate_addr;
            yuv->uv_addr = rotate_addr
                           + frame_desc.yuv.width * frame_desc.yuv.height;
        }else{
            yuv->width = frame_desc.yuv.width;
            yuv->height = frame_desc.yuv.height;
            yuv->pitch = frame_desc.yuv.pitch;
            yuv->format = frame_desc.yuv.format;
            yuv->y_addr = yaddr.data;
            yuv->uv_addr = uvaddr.data;
        }

        jpeg = jpeg_encoder->new_jpeg_data(yuv->width, yuv->height);

        if (!jpeg) {
            LOG_ERROR("new_jpeg_buf failed!\n");
            result = AM_RESULT_ERR_INVALID;
            break;
        }

        if (AM_RESULT_OK != jpeg_encoder->encode_yuv(yuv, jpeg)) {
            LOG_ERROR("jpeg encode failed!\n");
            result = AM_RESULT_ERR_INVALID;
            break;
        }

        if (!f.open(AMFile::AM_FILE_CREATE)) {
            LOG_ERROR("Failed to open file");
            result = AM_RESULT_ERR_INVALID;
            break;
        }

        if (f.write_reliable(jpeg->data.iov_base, jpeg->data.iov_len) < 0) {
            LOG_ERROR("Failed to sava data into file\n");
            result = AM_RESULT_ERR_INVALID;
            break;
        }
    } while (0);

    if (rotate_addr) {
        free(rotate_addr);
        rotate_addr = nullptr;
    }

    if (yuv) {
        free(yuv);
        yuv = nullptr;
    }

    if(jpeg){
        jpeg_encoder->free_jpeg_data(jpeg);
        jpeg = nullptr;
    }

    if(nullptr != jpeg_encoder){
        AM_DESTROY(jpeg_encoder);
    }

    f.close();

    return (result == AM_RESULT_OK);
}

void* OryxRecorderWrapper::m_data_handler = nullptr;
CaptureRoutine OryxRecorderWrapper::s_capture;
int OryxRecorderWrapper::s_stop_flag = 0;

OryxRecorderWrapper::OryxRecorderWrapper():
    m_record(nullptr),
    m_muxer_bitmap(0),
    m_recording_duration(0),
    m_file_number(0),
    m_file_duration(0),
    m_video_module(nullptr)
{
}

OryxRecorderWrapper::~OryxRecorderWrapper()
{
}

bool OryxRecorderWrapper::init_recorder(app_conf_t *p_user_conf)
{
    AMMuxerParam muxer_param;

    m_video_module = OryxVideoModule::get_instance();
    if(!m_video_module->start_camera()){
        LOG_ERROR("Failed to start Video Camera.\n");
        return false;
    }
    m_record = AMIRecord::create();
    if (!m_record) {
        LOG_ERROR("Failed to get AMIRecord instance!\n");
        return false;
    }
    if(!m_record->init()){
        LOG_ERROR("Failed to init AMIRecord instance!\n");
        return false;
    }
    read_sole_config("/tmp/config/etc/bpi/.file_muxer_bitmap",
                     (void*)&m_muxer_bitmap, sizeof(m_muxer_bitmap));
    if(0 == p_user_conf->record_control_mode){
        m_file_duration  = p_user_conf->file_duration;
        m_recording_duration = p_user_conf->record_duration_after_motion_starts;
        m_file_number = (m_recording_duration -1 + m_file_duration) / m_file_duration;
        muxer_param.file_duration_int32.value.v_int32 = m_file_duration;
        muxer_param.file_duration_int32.is_set = true;
        muxer_param.recording_duration_u32.value.v_u32 = m_recording_duration;
        muxer_param.recording_duration_u32.is_set = true;
        muxer_param.recording_file_num_u32.value.v_u32 = m_file_number;
        muxer_param.recording_file_num_u32.is_set = true;
    }
    muxer_param.muxer_id_bit_map = m_muxer_bitmap;
    if (!m_record->set_muxer_param(muxer_param)) {
        LOG_ERROR("Failed to set muxer parameters.\n");
        return false;
    }

    AM_FILE_OPERATION_CB_TYPE cb_type;
    cb_type = (AM_FILE_OPERATION_CB_TYPE)(AM_OPERATION_CB_FILE_CREATE);
    if(!m_record->set_file_operation_callback(m_muxer_bitmap, cb_type,file_operation_callback)){
        LOG_ERROR("Failed to set_file_operation_callback -- AM_OPERATION_CB_FILE_CREATE!\n");
        return false;
    }
    cb_type = (AM_FILE_OPERATION_CB_TYPE)(AM_OPERATION_CB_FILE_FINISH);
    if(!m_record->set_file_operation_callback(m_muxer_bitmap, cb_type,file_operation_callback)){
        LOG_ERROR("Failed to set_file_operation_callback -- AM_OPERATION_CB_FILE_FINISH!\n");
        return false;
    }
    if(p_user_conf->thumbnail){
        OryxRecorderWrapper::s_capture.set_rotation(p_user_conf->rotate);
        OryxRecorderWrapper::s_capture.enable(p_user_conf->storage_folder);
    }

    m_video_module->start_smart_avc(p_user_conf->smart_avc);
    m_video_module->start_overlay(p_user_conf->osd_label);
    m_video_module->start_linux_aaa();

    return true;
}

bool OryxRecorderWrapper::start_recorder()
{
    if(!m_record){
        return false;
    }

    if (!m_record->start()) {
        LOG_ERROR("Failed to start recording!\n");
        return false;
    }

    return true;
}

bool OryxRecorderWrapper::stop_recorder()
{
    if (m_record && !m_record->stop()) {
        LOG_ERROR("Stop recording failed!\n");
        return false;
    }
     fprintf(stderr, "%s %d", __func__, __LINE__);
     m_video_module->stop_smart_avc();
     usleep(100000);
     fprintf(stderr, "%s %d", __func__, __LINE__);
     m_video_module->stop_overlay();
     usleep(100000);
     fprintf(stderr, "%s %d", __func__, __LINE__);
     m_video_module->stop_linux_aaa();
     usleep(100000);
     fprintf(stderr, "%s %d", __func__, __LINE__);
     m_video_module->stop_camera();
     fprintf(stderr, "%s %d", __func__, __LINE__);
    return true;
}

void OryxRecorderWrapper::set_data_handler(void* p_user_data)
{
    if(nullptr != p_user_data){
        OryxRecorderWrapper::m_data_handler = p_user_data;
    }
}

void OryxRecorderWrapper::file_operation_callback(AMRecordFileInfo &file_info){
    if (file_info.type == AM_RECORD_FILE_FINISH_INFO) {
        LOG_PRINT("record done -> file name :%s\n", file_info.file_name);
 
        if (file_info.finish_type == AM_RECORD_FILE_FINISH_REACH_RECORD_DURATION || file_info.finish_type == AM_RECORD_FILE_FINISH_REACH_RECORD_FILE_NUM) {
            OryxRecorderWrapper::s_stop_flag = 1;
        }

      /*
        if(0 == file_info.stream_id && OryxRecorderWrapper::m_data_handler){
            BPI_Upload_Task upload_task;
            BPIUploader* uploader =
                (BPIUploader*)(OryxRecorderWrapper::m_data_handler);
            if(!uploader) return;
            strcpy(upload_task.file_name, file_info.file_name);
            strcpy(upload_task.device_id, get_device_id());
            upload_task.dual_stream_id = file_info.stream_id;
            upload_task.create_time = get_time_from_name(file_info.file_name);
            upload_task.file_index = get_num_from_tail(file_info.file_name);
            if(-1 == upload_task.create_time){
                LOG_ERROR("skip illegal named file %s!\n", file_info.file_name);
            }else{
                uploader->addTask(upload_task);
            }
        } */
    }
    else if(file_info.type == AM_RECORD_FILE_CREATE_INFO){
        LOG_PRINT("record started -> file name :%s\n", file_info.file_name);
        if(0 == file_info.stream_id){
            string snap_file(file_info.file_name);
            if(OryxRecorderWrapper::s_capture.rename_file(snap_file)){
                LOG_DEBUG("thumbnail captured %s!\n", snap_file.c_str());
                OryxRecorderWrapper::s_capture.disable();
		/*
                if(OryxRecorderWrapper::m_data_handler){
                    BPIUploader* uploader =
                        (BPIUploader*)(OryxRecorderWrapper::m_data_handler);
                    BPI_Upload_Task upload_task;
                    strcpy(upload_task.file_name, snap_file.c_str());
                    strcpy(upload_task.device_id, get_device_id());
                    upload_task.dual_stream_id = file_info.stream_id;
                    upload_task.create_time = get_time_from_name(snap_file.c_str());
                    upload_task.file_index = get_num_from_tail(snap_file.c_str());
                    uploader->addTask(upload_task);
                }
		*/
            }
        }
    }
}
