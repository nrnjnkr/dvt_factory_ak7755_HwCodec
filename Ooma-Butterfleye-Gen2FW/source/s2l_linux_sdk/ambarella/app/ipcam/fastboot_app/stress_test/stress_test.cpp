/*
 * stress_test.cpp
 *
 * History:
 *       2016/09/13 - [j Yi] created file
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


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <thread>

#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "bpi_utils.h"
#include "device.h"
#include "bpi_wlan.h"
#include "cloud_comm.h"
#include "bpi_utils.h"
#include "am_api_helper.h"
#include "bpi_mcu_proxy.h"
#include "bpi_recorder.h"
#include "bpi_uploader.h"

int pipe_fd[2] = {-1};
fd_set fds;
BPIMcuProxy mcu_proxy;

bool stop_sig = false;
bool ready_to_quit = true;
const char *path = "/tmp/stress-test/";

static void wait_sigstop()
{
    fd_set read_fd;
    while(stop_sig){
        char buf[1] = {0};
        read_fd = fds;
        int val = select(pipe_fd[1], &read_fd, NULL, NULL, NULL);
        LOG_PRINT("select result is: %d\n", val);
        if(val == 1 && FD_ISSET(pipe_fd[0], &read_fd)){
            read(pipe_fd[0], buf, sizeof(buf));
            if(buf[0] == 'q'){
                LOG_PRINT("caught stop signal\n");
                ready_to_quit = false;
                break;
            }
        }
        if(val < 0){
            perror("select");
        }
    }
}

static int make_dir(const char *path)
{
    struct stat file_stat;
    int ret;
    ret = stat(path, &file_stat);
    if(ret < 0){
        if(errno == ENOENT){
            ret = mkdir(path, 0755);
            if(ret < 0){
                LOG_PRINT("create directory failed\n");
                return -1;
            }
        }
        else{
            LOG_PRINT("file path wrong\n");
            return -1;
        }
    }

    return 0;
}
static int wifi_test(void)
{
    int loop_total = 0;
    int loop_count = 0;
    int keepalive_interval = 1;
    app_conf_t app_config = {0};

    LOG_PRINT("Enter keepalive interval: ");
    int n = scanf("%d", &keepalive_interval);
    if(n != 1 || keepalive_interval < 1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }

    LOG_PRINT("Enter test times, -1 represents endless suspend-and-resume: ");
    n = scanf("%d", &loop_total);
    if(n != 1 || loop_total < -1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }

    if(load_app_conf(&app_config) < 0){
        LOG_ERROR("load app config failed\n");
        return -1;
    }

    if(connect_ap(app_config.ap_ssid,app_config.ap_password,app_config.ap_wpa)==EWlan_Connected){
        LOG_DEBUG("Connect AP: OK\n");
    }else{
        LOG_ERROR("Connect AP: FAIL\n");
        ready_to_quit = false;
        return -1;
    }

    if (connect_server(app_config.cloud_server_ip, CONF_CLOUD_DEV_PORT, Net_TCP) < 0){
        LOG_PRINT("connect to server: failed\n");
        ready_to_quit = false;
        return -1;
    }

    int wakeReason = 0;
    memset(&app_config, 0, sizeof(app_config));
    app_config.dtim_interval = 600;
    app_config.keepalive_interval = keepalive_interval;
    strcpy(app_config.wakeup_token, "amba_wakeup");
    mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
    for(loop_count = 0; loop_count != loop_total; loop_count++){
        LOG_PRINT("############################START AGAIN############################\n");
        LOG_PRINT("This is No.%d\n", loop_count);
        if(wlan_keepalive(app_config.keepalive_interval, app_config.dtim_interval, app_config.wakeup_token, app_config.cloud_server_ip) < 0){
            LOG_PRINT("keep alive failed\n");
            ready_to_quit = false;
            return -1;
        }
        usleep(500000);
        system("echo mem > /sys/power/state");

        if(resume_wifi(&wakeReason, 1) == -1){
            LOG_PRINT("resume wifi failed\n");
            ready_to_quit = false;
            return -1;
        }else{
            LOG_PRINT("resume wifi done, now you can use ctrl+C to kill the program in the following seconds.\n");
        }
        mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
        usleep(500000);
        wait_sigstop();
        if(!ready_to_quit){
            LOG_PRINT("stop test loop\n");
            break;
        }
    }
    ready_to_quit = false;
    return 0;
}

static int normal_test(void)
{
    int loop_total = 0;
    int loop_count = 0;


    LOG_PRINT("Enter test times, -1 represents endless suspend-and-resume: ");
    int n = scanf("%d", &loop_total);
    if(n != 1 || loop_total < -1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }
    mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
    for(loop_count = 0; loop_count != loop_total; loop_count++){
        LOG_PRINT("############################START AGAIN############################\n");
        LOG_PRINT("This is No.%d\n", loop_count);
        usleep(500000);
        system("echo mem > /sys/power/state");
        mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
        usleep(500000);
        wait_sigstop();
        if(!ready_to_quit){
            LOG_PRINT("stop test loop\n");
            break;
        }
    }
    ready_to_quit = false;
    return 0;
}

static void run_SR_iav(char *cmd)
{
    LOG_PRINT("cmd_out is %s\n", cmd);
    system(cmd);
    ready_to_quit = false;
}

static int iav_test(void)
{
    int cmd_in = 0;
    char *encode_type = NULL;
    int loop_total = 0;
    char cmd_out[100] = {0};

    LOG_PRINT("choose encode type [1] h264, [2] h265: ");
    int n = scanf("%d", &cmd_in);
    if(n == 1 && cmd_in == 1){
        encode_type = (char*)"h264";
    }
    else if(n == 1 && cmd_in == 2){
        encode_type = (char*)"h265";
    }
    else{
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }
    LOG_PRINT("Enter test times, -1 represents endless suspend-and-resume: ");
    n = scanf("%d", &loop_total);
    if(n != 1 || loop_total < -1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }

    if(make_dir(path) != 0){
        LOG_PRINT("path error\n");
        ready_to_quit = false;
        return -1;
    }

    sprintf(cmd_out, "test_SR_iav.sh %s %s %s %d %s", "init", encode_type, "loop", loop_total, "rm");
    LOG_PRINT("cmd_out is %s\n", cmd_out);
    mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
    std::thread (run_SR_iav, cmd_out).detach();
    while(ready_to_quit){
        mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
        wait_sigstop();
        usleep(500000);
    }
    ready_to_quit = false;
    return 0;
}

static int oryx_recording_test(void)
{
    //init recorder
    BPIRecorder bpi_recorder;
    BPIUploader bpi_uploader;
    app_conf_t g_app_config = {0};

    int loop_total = 0;
    int loop_count = 0;
    int rec_interval = 1;
    char rm_cmd[128];

    LOG_PRINT("Enter recording interval: ");
    int n = scanf("%d", &rec_interval);
    if(n != 1 || rec_interval < 1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }
    LOG_PRINT("Enter test times, -1 represents endless suspend-and-resume: ");
    n = scanf("%d", &loop_total);
    if(n != 1 || loop_total < -1){
        LOG_PRINT("wrong parameter.\n");
        ready_to_quit = false;
        return -1;
    }

    if(make_dir(path) != 0){
        LOG_PRINT("path error\n");
        ready_to_quit = false;
        return -1;
    }
    sprintf(rm_cmd, "rm -R %s*", path);

    if(load_app_conf(&g_app_config) < 0){
        LOG_ERROR("load app config failed\n");
        return -1;
    }
    g_app_config.record_duration_after_motion_starts = rec_interval;
    g_app_config.file_duration = rec_interval;
    g_app_config.video0_enable = true;
    g_app_config.video1_enable = false;
    strcpy(g_app_config.storage_folder, path);

    if(g_app_config.video0_enable){
        BPIStreamConfig stream_config;
        memset(&stream_config, 0, sizeof(stream_config));
        stream_config.video_config.enable = true;
        stream_config.video_config.codec_type = g_app_config.video0_fmt;
        stream_config.video_config.rotation_type = g_app_config.rotate;
        stream_config.video_config.width = g_app_config.video0_width;
        stream_config.video_config.height = g_app_config.video0_height;
        stream_config.video_config.frame_rate = g_app_config.video0_frame_rate;
        stream_config.video_config.bitrate = g_app_config.video0_recording_bitrate;
        stream_config.video_config.enable_two_ref = g_app_config.enable_two_ref;
        stream_config.video_config.enable_lbr = g_app_config.smart_avc;
        stream_config.video_config.enable_thumbnail = g_app_config.thumbnail;
        stream_config.video_config.enable_ldc = g_app_config.enable_ldc;
        strcpy(stream_config.video_config.osd_label, g_app_config.osd_label);
        if(g_app_config.audio_enable){
            stream_config.audio_config.enable = true;
            stream_config.audio_config.codec_type = g_app_config.audio_fmt;
        }
        stream_config.file_muxer_config.file_muxer_type = g_app_config.file_fmt;
        stream_config.file_muxer_config.file_duration = g_app_config.file_duration;
        stream_config.file_muxer_config.recording_duration = g_app_config.record_duration_after_motion_starts;
        stream_config.file_muxer_config.file_num = (g_app_config.record_duration_after_motion_starts - 1 + g_app_config.file_duration) / g_app_config.file_duration;
        strcpy(stream_config.file_muxer_config.storage_location, g_app_config.storage_folder);
        strcpy(stream_config.file_muxer_config.pre_fix, g_app_config.file_name_prefix);
        if(!bpi_recorder.set_config(0, &stream_config)){
            LOG_ERROR("bpi recorder set config failed!");
            return false;
        }
    }

    if(!bpi_recorder.init_engine()){
        LOG_ERROR("bpi recorder init failed!");
        ready_to_quit = false;
        return -1;
    }
    bpi_recorder.set_data_handler(&bpi_uploader);
    mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
    //do test loop
    for(loop_count = 0; loop_count != loop_total; loop_count++){
        LOG_PRINT("############################START AGAIN############################\n");
        LOG_PRINT("This is No.%d\n", loop_count);
        bpi_recorder.enable_file_muxer();
        bpi_recorder.start_engine();
        sleep(rec_interval);
        bpi_recorder.stop_engine();
        usleep(500000);
        system("echo mem > /sys/power/state");
        mcu_proxy.send_cmd(MCU_CMD_LED_GREEN_ON);
        usleep(500000);
        system(rm_cmd);
        wait_sigstop();
        if(!ready_to_quit){
            LOG_PRINT("stop test loop\n");
            break;
        }
    }

    //exit
    ready_to_quit = false;
    return 0;
}

static void sigstop(int signum)
{
    LOG_PRINT("send stop signal\n");
    stop_sig = true;
}

int main(int argc, char *argv[])
{
    int cmd_in = 0;
    std::thread m_thread;
    pipe(pipe_fd);
    FD_ZERO(&fds);
    FD_SET(pipe_fd[0], &fds);

    LOG_PRINT("Choose mode: [1]normal linux suspend-and-resume\n");
    LOG_PRINT("             [2]test suspend-and-resume with keep-alive releted\n");
    LOG_PRINT("             [3]test suspend-and-resume with h264/h265 encoder running\n");
    LOG_PRINT("             [4]test suspend-and-resume with recording mp4\n");
    LOG_PRINT("Choose from [1/2/3/4]: ");
    int n = scanf("%d", &cmd_in);
    if((n != 1) || (cmd_in < 1 || cmd_in > 4)){
        LOG_PRINT("wrong parameter.\n");
        return 0;
    }

    bool init_val = mcu_proxy.init();
    if(!init_val){
        return -1;
    }
    //start
    mcu_proxy.send_cmd(MCU_CMD_SET_STRESS_TEST_MODE);

    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    //4 cases
    switch(cmd_in){
    //1.normal
        case 1:
            LOG_PRINT("normal linux suspend-and-resume\n");
            m_thread = thread(normal_test);
            break;
    //2.wifi
        case 2:
            LOG_PRINT("test suspend-and-resume with keep-alive related\n");
            m_thread = thread(wifi_test);
            break;
    //3.iav
        case 3:
            LOG_PRINT("test suspend-and-resume with h264/h265 encoder running\n");
            m_thread = thread(iav_test);
            break;
    //4.oryx
        case 4:
            LOG_PRINT("test suspend-and-resume with recording mp4\n");
            m_thread = thread(oryx_recording_test);
            break;
        default:
            break;
    }

    while(ready_to_quit){
        int n = 0;
        if(stop_sig){
            n = write(pipe_fd[1], "q", 1);
        }
        if(n == 1){
            LOG_PRINT("write q to pipe\n");
            break;
        }
        usleep(200000);
    }

    if(m_thread.joinable()){
        m_thread.join();
    }

    mcu_proxy.send_cmd(MCU_CMD_LED_RED_ON);
    do{
        LOG_PRINT("change mcu mode to normal\n");
        mcu_proxy.send_cmd(MCU_CMD_SET_NORMAL_MODE);
    }while(0);
    LOG_PRINT("exit\n");

    return 0;
}
