/*
 * bpi_uploader.cpp
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

#include <unistd.h>
#include <stdio.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <regex>

#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_uploader.h"
#include "bpi_utils.h"
using std::string;
using std::regex;
using std::smatch;

BPIUploader::BPIUploader(){
    pthread_mutex_init(&m_mutex,nullptr);
    pthread_cond_init(&m_cond,nullptr);
    m_http_url = nullptr;
    m_pid = 0;
    m_pid2 = 0;
}

BPIUploader::~BPIUploader(){
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
    if(m_http_url) free(m_http_url);
}

bool BPIUploader::set_url(const char* url){
    if(m_http_url) free(m_http_url);
    m_http_url = strdup(url);
    return (m_http_url != nullptr);
}

bool BPIUploader::addTask(BPI_Upload_Task task){
    pthread_mutex_lock (&m_mutex);
    task.policy = BPI_UPLOAD_POLICY_FIFO;
    if(m_new_files_queue.empty()) pthread_cond_signal(&m_cond);
    m_new_files_queue.push(task);
    LOG_PRINT("add new file(%s) to uploader\n", task.file_name);
    pthread_mutex_unlock (&m_mutex);
    return true;
}

bool BPIUploader::scan_folder(char* scan_folder){
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    const regex folder_pattern("\\d+");//oryx record folder name pattern
    const regex file_pattern("(.*)_(\\d+)_stream0_(\\d+)_(\\d+).(mp4|ts|jpg)");// filename pattern
    if((dp = opendir(scan_folder)) == nullptr) {
        return false;
    }
    while((entry = readdir(dp)) != nullptr) {
        string full_path = string(scan_folder) + string(entry->d_name);
        lstat(full_path.c_str(), &statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            if(regex_match(string(entry->d_name), folder_pattern)){
                DIR *sub_dp;
                char sub_folder[512];
                sprintf(sub_folder, "%s%s", scan_folder, entry->d_name);
                if((sub_dp = opendir(sub_folder)) == nullptr) {
                    continue;
                }
                while((entry = readdir(sub_dp)) != nullptr) {
                    full_path = string(sub_folder) + string(entry->d_name);
                    lstat(full_path.c_str(), &statbuf);
                    string str(entry->d_name);
                    if(S_ISREG(statbuf.st_mode)) {
                        if(regex_match(str, file_pattern)){
                            BPI_Upload_Task upload_task;
                            string file_name = string(sub_folder) + '/' + string(entry->d_name);
                            memset(&upload_task, 0, sizeof(BPI_Upload_Task));
                            strcpy(upload_task.file_name, file_name.c_str());
                            upload_task.create_time =get_time_from_name(entry->d_name);
                            upload_task.policy = BPI_UPLOAD_POLICY_LATEST_FIRST;
                            upload_task.dual_stream_id = get_id_from_name(entry->d_name);
                            strcpy(upload_task.device_id, get_device_id());
                            LOG_PRINT("add old file(%s) to uploader\n", upload_task.file_name);
                            m_old_files_queue.push(upload_task);
                        }
                    }
                }
                closedir(sub_dp);
            }
        }
    }
    closedir(dp);
    return true;
}

static size_t curl_read_cb(void *buffer, size_t size, size_t nmemb, void *instream){
    FILE *fd = (FILE*)instream;
    return fread(buffer, size, nmemb, fd);
}

bool BPIUploader::upload(BPI_Upload_Task* upload_task){
    FILE* p_file = fopen(upload_task->file_name, "rb");
    if (nullptr == p_file) {
        return false;
    }
    bool transfer_ok = false;
    CURL *curl = nullptr;
    char event_type[10];
    char event_id[64];
    char event_num[64];
    struct curl_httppost *formpost = nullptr;
    struct curl_httppost *lastptr = nullptr;
    struct curl_slist *headerlist = nullptr;
    static const char buf[] = "Expect:";
    if(strstr(upload_task->file_name,".mp4") || strstr(upload_task->file_name,".ts")){
        strcpy(event_type, "0");
    }else if(strstr(upload_task->file_name,".jpg")){
        strcpy(event_type, "1");
    }else{
        strcpy(event_type, "unknown");
    }
    snprintf(event_id, sizeof(event_id),"%u",(unsigned int)upload_task->create_time);
    snprintf(event_num,sizeof(event_num),"%u",(unsigned int)upload_task->file_index);

    do{
        LOG_PRINT("start to upload --> %s\n", upload_task->file_name);
        curl = curl_easy_init();
        if(curl) {
            /* Fill in the file upload field. This makes libcurl load data from
             the given file name when curl_easy_perform() is called. */
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, upload_task->file_name, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "device_id", CURLFORM_PTRCONTENTS, upload_task->device_id, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "event_type", CURLFORM_PTRCONTENTS, event_type, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "event_id", CURLFORM_PTRCONTENTS, event_id, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "event_num", CURLFORM_PTRCONTENTS, event_num, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "event_detail", CURLFORM_PTRCONTENTS, upload_task->file_name, CURLFORM_END);

            /* what URL that receives this POST */
            curl_easy_setopt(curl, CURLOPT_URL, m_http_url);
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
            curl_easy_setopt(curl, CURLOPT_READDATA, p_file);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, &curl_read_cb);

            /*initalize custom header list (stating that Expect: 100-continue is not
             wanted */
            headerlist = curl_slist_append(headerlist, buf);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

            CURLcode retcode = curl_easy_perform(curl);
            if(CURLE_OK == retcode){
                smatch match;
                char uploaded_filename[256];
                char cmd[1024];
                const regex ftype_pattern(".(mp4|ts|jpg)");//record file filename pattern
                string tmp_str(upload_task->file_name);
                regex_search(tmp_str, match, ftype_pattern);
                if(match.ready()){
                    snprintf(uploaded_filename, sizeof(uploaded_filename), "%s_uploaded%s\n", match.prefix().str().c_str(), match[0].str().c_str());
                    snprintf(cmd, sizeof(cmd), "mv %s %s\n", upload_task->file_name, uploaded_filename);
                    system(cmd);
                }
                transfer_ok = true;
                LOG_PRINT("upload success --> %s\n", upload_task->file_name);
            }else{
                LOG_ERROR("upload failed, try again later, curl ret code:%d\n", retcode);
                sleep(5);
            }
        }
    }while(0);

    if(curl) curl_easy_cleanup(curl);
    if(formpost) curl_formfree(formpost);//then cleanup the formpost chain
    if(headerlist) curl_slist_free_all(headerlist);//free slist
    fclose(p_file);
    return transfer_ok;
}

void* BPIUploader::on_new_files(void *arg){
    BPIUploader* pUploader = (BPIUploader*)arg;
    while (true) {
        pthread_mutex_lock (&pUploader->m_mutex);
        if(pUploader->m_new_files_queue.empty()){
            pthread_cond_wait(&pUploader->m_cond, &pUploader->m_mutex);
        }
        BPI_Upload_Task task = pUploader->m_new_files_queue.top();
        pUploader->m_new_files_queue.pop();
        pthread_mutex_unlock (&pUploader->m_mutex);
        if(-1 == task.file_index) {
            LOG_PRINT("uploader exited!!\n");
            break;
        }

        if(0 != access(task.file_name, F_OK)) {
            LOG_ERROR("file(%s) not exist, skip it\n", task.file_name);
            continue;
        }

        if(false == pUploader->upload(&task)){
            pthread_mutex_lock (&pUploader->m_mutex);
            if(pUploader->m_new_files_queue.empty()) pthread_cond_signal(&pUploader->m_cond);
            pUploader->m_new_files_queue.push(task);
            pthread_mutex_unlock (&pUploader->m_mutex);
        }
   }
   pthread_exit(0);
}

void* BPIUploader::on_old_files(void *arg){
    BPIUploader* pUploader = (BPIUploader*)arg;
    while (!pUploader->m_old_files_queue.empty() && pUploader->m_pid) {
        BPI_Upload_Task task = pUploader->m_old_files_queue.top();
        pUploader->m_old_files_queue.pop();
        if(0 != access(task.file_name, F_OK)) {
            LOG_ERROR("file(%s) not exist, skip it\n", task.file_name);
            continue;
        }
        if(false == pUploader->upload(&task)){
            pUploader->m_old_files_queue.push(task);
        }
   }
   pthread_exit(0);
}

bool BPIUploader::start(){
    if(0 != pthread_create(&m_pid, nullptr, on_new_files, this)){
        return false;
    }
    if(!m_old_files_queue.empty()){
        if(0 != pthread_create(&m_pid2, nullptr, on_old_files, this)){
            return false;
        }
    }
    return true;
}

bool BPIUploader::stop(){
    if(m_pid) {
        BPI_Upload_Task stop_task;
        memset(&stop_task, 0, sizeof(BPI_Upload_Task));
        stop_task.highest_priority = true;
        stop_task.file_index = -1;
        addTask(stop_task);
        pthread_join(m_pid, nullptr);
        m_pid = 0;
    }
    if(m_pid2) pthread_join(m_pid2, nullptr);
    while(!m_old_files_queue.empty()){
        m_old_files_queue.pop();
    }
    while(!m_new_files_queue.empty()){
        m_new_files_queue.pop();
    }
    return true;
}

bool BPIUploader::wait_jobs_done(int timeout_sec){
    bool all_task_done = false;
    while(1){
        pthread_mutex_lock (&m_mutex);
        all_task_done = m_new_files_queue.empty();
        pthread_mutex_unlock (&m_mutex);
        if(all_task_done){
            break;
        }else if(timeout_sec > 0){
            sleep(1);
            timeout_sec--;
            continue;
        }else{
            break;
        }
    }

    return all_task_done;
}
