/*
 * bpi_uploader.h
 *
 * History:
 *       2016/07/15 - [Zhipeng Dong] created file
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

#ifndef __BPI_UPLOADER_H_
#define __BPI_UPLOADER_H_
#include <queue>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <memory.h>
using std::priority_queue;
using std::vector;

enum BPI_UPLOADER_POLICY {
    BPI_UPLOAD_POLICY_FIFO,
    BPI_UPLOAD_POLICY_LATEST_FIRST,
    BPI_UPLOAD_POLICY_BY_TIME,
};

struct BPI_Upload_Task
{
    char file_name[512];
    char device_id[128];
    time_t create_time;
    int file_index;
    int dual_stream_id;
    BPI_UPLOADER_POLICY policy;
    bool highest_priority;
    BPI_Upload_Task() :
        create_time(0),
        file_index(-1),
        dual_stream_id(-1),
        policy(BPI_UPLOAD_POLICY_FIFO),
        highest_priority(false)
    {
        memset(file_name, 0, sizeof(file_name));
        memset(device_id, 0, sizeof(device_id));
    }

    BPI_Upload_Task(const BPI_Upload_Task& task) :
        create_time(task.create_time),
        file_index(task.file_index),
        dual_stream_id(task.dual_stream_id),
        policy(task.policy),
        highest_priority(task.highest_priority)
    {
        strcpy(file_name, task.file_name);
        strcpy(device_id, task.device_id);
    }

};

struct cmp{
    bool operator()(const BPI_Upload_Task a, const BPI_Upload_Task b)
    {
        if(a.highest_priority || b.highest_priority){
            return !a.highest_priority;
        }
        else if(BPI_UPLOAD_POLICY_FIFO == a.policy){
            return false;
        }
        else if(BPI_UPLOAD_POLICY_BY_TIME == a.policy){
            return a.create_time > b.create_time;
        }
        else if(BPI_UPLOAD_POLICY_LATEST_FIRST == a.policy){
            return a.create_time < b.create_time;
        }
        return false;
    }
};

class BPIUploader{
public:
    BPIUploader();
    ~BPIUploader();
    bool set_url(const char* url);
    bool addTask(BPI_Upload_Task task);
    bool start();
    bool stop();
    bool scan_folder(char* scan_folder);
    bool wait_jobs_done(int timeout_sec);

private:
    static void* on_old_files(void *arg);
    static void* on_new_files(void *arg);
    bool upload(BPI_Upload_Task* task);
    pthread_t m_pid;
    pthread_t m_pid2;
    pthread_cond_t m_cond;
    pthread_mutex_t  m_mutex;
    priority_queue<BPI_Upload_Task, vector<BPI_Upload_Task>, cmp> m_old_files_queue;
    priority_queue<BPI_Upload_Task, vector<BPI_Upload_Task>, cmp> m_new_files_queue;
    char* m_http_url;
};
#endif //__BPI_UPLOADER_H_
