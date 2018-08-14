/*
 * bpi_vca.h
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

#ifndef __BPI_VCA_H_
#define __BPI_VCA_H_

#include "am_video_reader_if.h"
#include "am_video_address_if.h"

typedef bool (*detected_callback_t)(void *);

class VCA {
public:
    virtual ~VCA();
    static VCA *get_instance();

    /* common for sync & async run */
    void set_buffer_id(int buffer_id);
    void set_frame_size(unsigned int width, unsigned int height);
    void set_frame_number(int frames);
    void set_time_out(int time_out);

    /* specific for async run */
    void set_detected_callback(detected_callback_t cb, void *arg);

    /* run vca synchronously */
    virtual int run();

    /* run & pause vca asynchronously */
    virtual int run_async();
    virtual int pause();

protected:
    VCA();

    virtual int read_yuv();
    virtual int pre_vca();
    virtual int do_vca() = 0;
    virtual int post_vca();

    unsigned int m_buffer_width;
    unsigned int m_buffer_height;
    int m_frame_number;
    int m_time_out;
    unsigned char *m_buffer_data;
    unsigned char *m_buffer_y;
    unsigned char *m_buffer_uv;
    AMIVideoReaderPtr m_video_reader;
    AMIVideoAddressPtr m_video_address;
    int iav_fd;
#ifdef PRE_STORED_YUV
    /*For fastboot,  yuv data has been captured already*/
    unsigned char* m_yuv_src_addr;
    unsigned int m_remain_size;
    unsigned int m_src_size;
    unsigned int m_frame_width;
    unsigned int m_frame_height;
    unsigned int m_frame_pitch;
    unsigned int m_max_height;
    unsigned int m_uv_height;
    int query_yuv_info();
#endif

    /* specific for async run */
    friend void cleanup(void *);
    friend void *worker(void *);

    pthread_t m_tid;
    pthread_mutex_t m_mtx;
    pthread_cond_t m_cond;
    bool m_is_thread_created;
    bool m_is_thread_started;
    int m_buffer_id;
    detected_callback_t m_detected_cb;
    void *m_detected_cb_arg;
};

#endif /* __BPI_VCA_H_ */
