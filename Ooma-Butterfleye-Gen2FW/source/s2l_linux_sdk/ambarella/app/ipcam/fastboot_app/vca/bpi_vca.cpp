/*
 * bpi_vca.cpp
 *
 * History:
 *       2016/09/13 - [niu zhifeng] created file
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
 */
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <iav_ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "bpi_vca.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_utils.h"

#ifdef PRE_STORED_YUV
#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#define S2LM_READ_IAV_VCA
#ifdef S2LM_READ_IAV_VCA
const unsigned int pts_line_gap = 1;
#else
const unsigned int pts_line_gap = 0;
#endif

int VCA::query_yuv_info()
{
#if 0
    AM_RESULT ret = AM_RESULT_OK;
    do {
        AMQueryFrameDesc frame_desc;
        AMBufferInfo binfo;
        AM_SOURCE_BUFFER_ID id = AM_SOURCE_BUFFER_ID(m_buffer_id);
        if (AM_RESULT_OK != m_video_reader->query_yuv_frame(frame_desc, id, false)) {
            ret = AM_RESULT_ERR_INVALID;
            LOG_ERROR("Query yuv frame failed.\n");
            break;
        }
        if(frame_desc.yuv.width != m_buffer_width || frame_desc.yuv.height != m_buffer_height){
            LOG_ERROR("width/height error, actual buffer [%dx%d], was set [%dx%d]\n", frame_desc.yuv.width,frame_desc.yuv.height, m_buffer_width, m_buffer_height);
              ret = AM_RESULT_ERR_INVALID;
            break;
        }
        binfo.buf_id = id;
        if (AM_RESULT_OK != m_video_reader->query_buffer_info(binfo)) {
            ret = AM_RESULT_ERR_INVALID;
            LOG_ERROR("Query buffer info failed.\n");
            break;
        }

        const AMAddress addr =  m_video_address->addr_get(AM_DATA_FRAME_TYPE_YUV, 0, id);
        if (!addr.data) {
            LOG_ERROR("Failed get buffer:%d address!\n", m_buffer_id);
            ret = AM_RESULT_ERR_MEM;
            break;
        }
        m_yuv_src_addr = addr.data + addr.offset;
        m_remain_size = addr.max_size - addr.offset;

        m_frame_width = frame_desc.yuv.width;
        m_frame_height = frame_desc.yuv.height;
        m_frame_pitch = frame_desc.yuv.pitch;
        m_max_height = ROUND_UP(binfo.max_size.height, 16);
        LOG_PRINT("width=%d, height= %d, pitch=%d, max_height=%d, rounded max_height=%d\n", m_frame_width, m_frame_height, m_frame_pitch, binfo.max_size.height, m_max_height);
        if (frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV420) {
            m_uv_height = frame_desc.yuv.height / 2;
            m_src_size = m_frame_pitch * m_max_height * 3 / 2;
            LOG_PRINT("The yuv format is 420.\n");
        }else {
            ret = AM_RESULT_ERR_INVALID;
            LOG_ERROR("Unsupported chroma format in YUV dump.\n");
            break;
        }
    }while(0);
#endif

#ifdef S2LM_READ_IAV_VCA

    int ret = 0;
    do
    {
        iav_fd = open("/dev/iav", O_RDWR, 0);
        if (iav_fd < 0) {
            perror("fail to open iav\n");
            break;
        }

        struct iav_querybuf querybuf;
        querybuf.buf = IAV_BUFFER_VCA;
        if (ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
            perror("IAV_IOC_QUERY_BUF");
            break;
        }
        m_remain_size = querybuf.length;
        m_yuv_src_addr = (unsigned char *)mmap(NULL, m_remain_size,
                                               PROT_READ, MAP_SHARED,
                                               iav_fd, querybuf.offset);
        if (m_yuv_src_addr == MAP_FAILED) {
            perror("mmap failed\n");
            break;
        }
    }while(0);
    m_frame_width = m_buffer_width;  // 720
    m_frame_height = m_buffer_height;  // 480
    m_frame_pitch = m_buffer_width + 16;  // 736
    m_uv_height = m_frame_height / 2;
    m_max_height = m_frame_height + m_uv_height + pts_line_gap;
    m_src_size = m_frame_pitch * m_max_height;

#endif
    return ret;
}
#endif

/* FIXME: read fps from configuration file or somewhere else */
#define VCA_FPS 15
VCA::VCA():
    m_buffer_width(0),
    m_buffer_height(0),
    m_frame_number(-1),
    m_time_out(-1),
    m_buffer_data(nullptr),
    m_buffer_y(nullptr),
    m_buffer_uv(nullptr),
    m_video_reader(nullptr),
    m_video_address(nullptr),
    iav_fd(-1),
#ifdef PRE_STORED_YUV
    m_yuv_src_addr(nullptr),
    m_remain_size(0),
    m_src_size(0),
    m_frame_width(0),
    m_frame_height(0),
    m_frame_pitch(0),
    m_max_height(0),
    m_uv_height(0),
#endif
    m_mtx(PTHREAD_MUTEX_INITIALIZER),
    m_cond(PTHREAD_COND_INITIALIZER),
    m_is_thread_created(false),
    m_is_thread_started(false),
    m_buffer_id(0),
    m_detected_cb(NULL),
    m_detected_cb_arg(NULL)
{
    // m_video_reader = AMIVideoReader::get_instance();
    // m_video_address = AMIVideoAddress::get_instance();

    // assert(m_video_reader && m_video_address);
}

VCA::~VCA()
{
    if (m_is_thread_created)
        pthread_cancel(m_tid);

    if (m_buffer_data) {
        delete [] m_buffer_data;
        m_buffer_data = nullptr;
    }

    if(iav_fd > 0) close(iav_fd);
    munmap(m_yuv_src_addr, m_remain_size);

    m_buffer_y = nullptr;
    m_buffer_uv = nullptr;
    m_video_reader = nullptr;
    m_video_address = nullptr;
}

void VCA::set_buffer_id(int id)
{
    m_buffer_id = id;
}

void VCA::set_frame_size(unsigned int width, unsigned int height)
{
    if (m_buffer_data && m_buffer_width == width && m_buffer_height == height)
        return;

    if (m_buffer_data) {
        delete [] m_buffer_data;
        m_buffer_data = nullptr;
    }

    m_buffer_width = width;
    m_buffer_height = height;
    m_buffer_data = new unsigned char[m_buffer_width * m_buffer_height * 3 / 2];
    m_buffer_y = m_buffer_data;
    m_buffer_uv = m_buffer_data + m_buffer_width * m_buffer_height;
}

void VCA::set_frame_number(int frames)
{
    m_frame_number = frames;
}

void VCA::set_time_out(int time_out)
{
    m_time_out = time_out;
}

void VCA::set_detected_callback(detected_callback_t cb, void *arg)
{
    m_detected_cb = cb;
    m_detected_cb_arg = arg;
}

int VCA::read_yuv()
{
    int ret = 0;
#ifdef PRE_STORED_YUV
    do {
        if (m_remain_size > m_src_size){
            // copy Y
            unsigned char *y_out = m_buffer_y;
            unsigned char *y_in = m_yuv_src_addr + pts_line_gap * m_frame_pitch;
            for (unsigned int line = 0; line < m_frame_height; ++line) {
                memcpy(y_out, y_in, m_frame_width);
                y_in += m_frame_pitch;
                y_out += m_frame_width;
            }
            // copy UV
            unsigned char *uv_out = m_buffer_uv;
            unsigned char *uv_in = m_yuv_src_addr + (m_max_height - m_uv_height)
                                   * m_frame_pitch;
            for (unsigned int line = 0; line < m_uv_height; line++) {
                memcpy(uv_out, uv_in, m_frame_width);
                uv_in += m_frame_pitch;
                uv_out += m_frame_width;
            }
            m_yuv_src_addr += m_src_size;
            m_remain_size -= m_src_size;
        }
    } while (0);

#else
    AMQueryFrameDesc frame_desc;
    AMAddress y_addr;
    AMAddress uv_addr;

    do {
        if (AM_RESULT_OK != m_video_reader->query_yuv_frame(frame_desc,
                    (enum AM_SOURCE_BUFFER_ID)m_buffer_id, false)) {
            LOG_ERROR("failed to query yuv frame\n");
            ret = -1;
            break;
        }

        if (AM_RESULT_OK != m_video_address->yuv_y_addr_get(frame_desc, y_addr)) {
            LOG_ERROR("failed to get y address\n");
            ret = -1;
            break;
        }

        if (AM_RESULT_OK != m_video_address->yuv_uv_addr_get(frame_desc, uv_addr)) {
            LOG_ERROR("failed to get uv address\n");
            ret = -1;
            break;
        }

        if (frame_desc.yuv.pitch != (unsigned)m_buffer_width) {
            unsigned char *y_in = y_addr.data;
            unsigned char *y_out = m_buffer_y;
            unsigned char *uv_in = uv_addr.data;
            unsigned char *uv_out = m_buffer_uv;
            for (unsigned int i = 0; i < m_buffer_height; i++) {
                memcpy(y_out, y_in, m_buffer_width);
                y_in += frame_desc.yuv.pitch;
                y_out += m_buffer_width;
            }
            for (unsigned int i = 0; i < m_buffer_height / 2; i++) {
                memcpy(uv_out, uv_in, m_buffer_width);
                uv_in += frame_desc.yuv.pitch;
                uv_out += m_buffer_width;
            }
        } else {
            memcpy(m_buffer_y, y_addr.data, m_buffer_width * m_buffer_height);
            memcpy(m_buffer_uv, uv_addr.data, m_buffer_width * m_buffer_height / 2);
        }
    } while (0);
#endif

    return ret;
}

int VCA::run()
{
    int frame_cnt = 0;
    unsigned int start;
    //unsigned int prev;
    unsigned int curr;
    int ret = -1;

    if (m_time_out <= 0) {
        LOG_ERROR("invalid parameter: time_out = %d\n", m_time_out);
        return ret;
    }

    if (m_frame_number <= 0) {
        LOG_ERROR("invalid parameter: frame_number = %d\n", m_frame_number);
        return ret;
    }

    LOG_PRINT("vca detecting...\n");
    do {
        start = get_current_time();
        curr = start;
        char * p;
        FILE * fp = nullptr;

#ifdef PRE_STORED_YUV
        query_yuv_info();
#endif
        if (pre_vca() < 0) {
            LOG_ERROR("pre_vca() failed\n");
            return ret;
        }

        if((p=getenv("DUMP_VCA_BUF"))){
            if(!strcmp("true", p)){
                char vca_buf_dump_file[128];
                snprintf(vca_buf_dump_file, sizeof(vca_buf_dump_file), "/tmp/vca_nv12_%dx%d_%d.yuv", m_buffer_width, m_buffer_height, get_current_time());
                fp = fopen(vca_buf_dump_file,"wb");
                if (!fp){
                    LOG_ERROR("open file %s failed\n", vca_buf_dump_file);
                }
            }
        }

        while (frame_cnt < m_frame_number) {
            //prev = get_current_time();

            if (read_yuv() != 0) {
                LOG_ERROR("read_frame failed\n");
                break;
            }

            if(fp){
                fwrite(m_buffer_y, 1, m_buffer_width * m_buffer_height, fp);
                fwrite(m_buffer_uv, 1, m_buffer_width * m_buffer_height/2, fp);
            }

            if (do_vca() == 0) {
                ret = 0;
                break;
            }

            curr = get_current_time();

            if (curr - start > (unsigned)m_time_out) {
                LOG_PRINT("VCA timeout\n");
                break;
            }
            //disable VCA FPS control
           // if (curr - prev < 1000 / VCA_FPS) {
           //     usleep(1000 * (curr - prev));
          //  }
            frame_cnt++;
        }
        if(fp) fclose(fp);
    } while (0);

    if (post_vca() < 0) {
        LOG_ERROR("post_vca() failed\n");
        /* fall through */
    }

    LOG_PRINT("VCA done with object %sdetected, spent %d ms, "
                "processed %d frames\n", ret == 0 ? "" : "not ",
                curr - start, frame_cnt+1);
    return ret;
}

void cleanup(void *arg);
void *worker(void *arg);

int VCA::run_async()
{
    int ret = 0;

    LOG_PRINT("vca detecting...\n");

    do {
        if (!m_is_thread_created) {
            pthread_create(&m_tid, NULL, worker, this);
            m_is_thread_created = true;
        }

        if (m_is_thread_started) {
            LOG_ERROR("vca thread is already running\n");
            ret = -1;
            break;
        }

        pthread_mutex_lock(&m_mtx);
        if (pre_vca() < 0) {
            LOG_ERROR("pre_vca() failed\n");
            ret = -1;
            break;
        }
        m_is_thread_started = true;
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mtx);
    } while (0);

    return ret;
}

int VCA::pause()
{
    int ret = 0;

    do {
        if (!m_is_thread_started) {
            LOG_ERROR("vca thread is already paused\n");
            ret = -1;
            break;
        }

        pthread_mutex_lock(&m_mtx);
        m_is_thread_started = false;
        if (post_vca() < 0) {
                LOG_ERROR("post_vca() failed\n");
        }
        pthread_mutex_unlock(&m_mtx);
    } while (0);

    return ret;
}

int VCA::pre_vca()
{
    return 0;
}

int VCA::post_vca()
{
    return 0;
}

void cleanup(void *arg)
{
    assert(arg);

    VCA *vca = (VCA *)arg;

    if (vca->post_vca() < 0) {
        LOG_ERROR("post_vca() failed\n");
    }

    pthread_mutex_unlock(&vca->m_mtx);

    return;
}

void *worker(void *arg)
{
    assert(arg);

    VCA *vca = (VCA *)arg;
#ifdef PRE_STORED_YUV
    vca->query_yuv_info();
#endif
    pthread_cleanup_push(cleanup, arg);

    bool unlimited = vca->m_frame_number == -1 ? true : false;
    bool pause_on_detected = false;
    unsigned int prev;
    unsigned int curr;
    int i = 0;

    while (unlimited || i++ < vca->m_frame_number) {
        pthread_mutex_lock(&vca->m_mtx);

        while (!vca->m_is_thread_started)
            pthread_cond_wait(&vca->m_cond, &vca->m_mtx);

        prev = get_current_time();

        if (vca->read_yuv() != 0) {
            LOG_ERROR("read_yuv() failed\n");
            pthread_mutex_unlock(&vca->m_mtx);
            continue;
        }
        if (vca->do_vca() == 0) {
            pause_on_detected = vca->m_detected_cb(vca->m_detected_cb_arg);
            if (pause_on_detected)
                vca->m_is_thread_started = false;
        }

        pthread_mutex_unlock(&vca->m_mtx);

        curr = get_current_time();
        if (curr - prev < 1000 / VCA_FPS) {
            usleep(1000 * (curr - prev));
        }

    };

    pthread_cleanup_pop(0);

    return NULL;
}
