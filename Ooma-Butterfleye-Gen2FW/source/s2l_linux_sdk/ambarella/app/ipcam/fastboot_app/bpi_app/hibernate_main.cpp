/*
 * hibernate_main.cpp
 *
 *
 * History:
 *       2016/12/15 - [CZ LIN] created file
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
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mount.h>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <iav_ioctl.h>
#include <sys/mman.h>

#include "device.h"
#include "bpi_typedefs.h"
#include "bpi_app_config.h"
#include "cloud_comm.h"
#include "bpi_utils.h"
#include "ubi/bpi_ubi.h"
#include "adc/adc_util.h"
#include "bpi_mcu_proxy.h"
#include "bpi_uploader.h"
#include "bpi_mcu_event_monitor.h"
#include "bpi_wlan.h"
#include "bpi_oryx_config.h"
#include "bpi_vca.h"
#include "streaming_rtp_over_udp.h"
#include "streaming_audio_playback.h"
#include "config.h"

#define  ADD_MTD   (9)
#define DMA_DESCRIPTOR_OFFSET 0x00100000 /* the value is defined in amboot */

using std::string;
const string BPI_swp =  "/dev/mtd8";

static int  run_normal_mode(BPIMcuProxy& proxy);
static int  run_hibernate_mode(BPIMcuProxy& proxy);
static app_conf_t g_app_config;

int get_audio_param_from_iav(unsigned int *audio_op_address, unsigned int *audio_op_size){
    int fd_iav = open("/dev/iav", O_RDWR, 0);
    if(fd_iav < 0){
        return -1;
    }
    do{
        struct iav_querybuf buf;
        int ret = -1;
        memset(&buf, 0, sizeof(buf));
	 buf.buf = IAV_BUFFER_FB_AUDIO;
	 while ( (ret = ioctl(fd_iav, IAV_IOC_QUERY_BUF, &buf)) < 0 && errno == EINTR);
        close(fd_iav);
	 if(ret < 0){
	     return -1;
        }
	 printf("get_audio_param_from_iav --  Base Address: [0x%08X], Size [%8d KB].\n", buf.offset, (buf.length >> 10));
	 fflush(stdout);
	 *audio_op_address = buf.offset;
	 *audio_op_size = buf.length;
        return 0;
    }while(0);
}

static int wait_dma_audio(u32 *dest_addr, u32 op_address, u32 safe_size)
{
    int ret = -1;
    const u32 audio_safe_region = op_address + safe_size;
    u32 start = get_current_time();
    u32 now = start;
    while(now - start < 500 * 1000){
        printf("dma ch6 has writen to: %x, safe region: %x\n", *dest_addr, audio_safe_region);
        if(*dest_addr < audio_safe_region && (*dest_addr > op_address + DMA_DESCRIPTOR_OFFSET)){
            ret = (*dest_addr - op_address) / DMA_DESCRIPTOR_OFFSET + 1;
            printf("%s: dma audio desc offset: %d\n", __func__, ret);
            break;
        }else{
            sleep(1);
        }
        now = get_current_time();
    }
    return ret;
}

int peek_dma_audio(u32 op_address, u32 safe_size)
{
    int dma_fd = 0;
    int ret = 0;

    //these values are defined in Ambarella_A12_Hardware_Programming_Reference_Manual
    u32 dma_address = 0xe0005000; /* page aligned */
    u32 dma_reg_size = 0x1000; /* page aligned */
    u32 dma_reg_offset = 0x0368; /* DMA channel 6 main destination address */

    u32 *mem_dma = NULL, *mem_dma_dest_addr = NULL;

    do{
        dma_fd = open("/dev/ambad", O_RDWR, 0);
        if(dma_fd < 0){
            perror("/dev/ambad failed to open");
            ret = -1;
            break;
        }

        mem_dma = (u32*)mmap(NULL, dma_reg_size,
                    PROT_READ|PROT_WRITE, MAP_SHARED, dma_fd, dma_address);
        if(mem_dma == MAP_FAILED){
            perror("mmap");
            close(dma_fd);
            ret = -1;
            break;
        }
        mem_dma_dest_addr = mem_dma + dma_reg_offset / sizeof(u32);
        ret = wait_dma_audio(mem_dma_dest_addr, op_address, safe_size);
        if(ret < 0){
            printf("ERROR: wait dma timeout\n");
            break;
        }
    }while(0);

    if(mem_dma != NULL){
        munmap(mem_dma, dma_reg_size);
    }
    if(dma_fd){
        close(dma_fd);
    }
    return ret;
}

static void mount_rw_partition()
{
    if (0 != bpi_ubi_attach(0, ADD_MTD)){
        LOG_ERROR("attach mtd %d to ubi 0 failed\n", ADD_MTD);
    }
    if (0 != mount("/dev/ubi0_0", "/tmp/config", "ubifs", MS_SYNCHRONOUS, NULL)){
        LOG_ERROR("mount /dev/ubi0_0 on /tmp/config failed\n");
    }
    if (0 != mount("/tmp/config/etc/oryx", "/etc/oryx", "", MS_BIND, NULL)){
        LOG_ERROR("mount to /etc/oryx failed\n");
    }
    if (0 != mount("/tmp/config/etc/bpi", "/etc/bpi", "", MS_BIND, NULL)){
        LOG_ERROR("mount to /etc/bpi failed\n");
    }
    if (0 != mount("/tmp/config/calibration", "/mnt/provision", "", MS_BIND, NULL)){
        LOG_ERROR("mount to /mnt/provision failed\n");
    }
}

static void umount_rw_partition()
{
    if (0 != umount("/etc/oryx")){
        LOG_ERROR("umount /etc/oryx failed\n");
    }
    if (0 != umount("/etc/bpi")){
        LOG_ERROR("umount /etc/bpi failed\n");
    }
    if (0 != umount("/mnt/provision")){
        LOG_ERROR("umount /mnt/provision failed\n");
    }
    if (0 != umount("/tmp/config")){
        LOG_ERROR("umount /tmp/config failed\n");
    }
    if (0 != bpi_ubi_detach(0, ADD_MTD)){
        LOG_ERROR("bpi_ubi_detach mtd %d failed\n", ADD_MTD);
    }
}

//todo
static void recording_run(McuEventMonitor &mcu_monitor){
    unsigned int cur_time = 0;
    unsigned int start_time = 0;
    int duration = 0;
    start_time = get_current_time();
    while(1){
        cur_time = get_current_time();
        duration = (cur_time - start_time) / 1000;
        if (duration >= g_app_config.recording_max_duration) {
           LOG_DEBUG("Reached max recording duration.\n");
           break;
        }
        if (0 == g_app_config.record_control_mode && 1 == OryxRecorderWrapper::s_stop_flag) {
            LOG_DEBUG("Reached motion start mode duration.\n");
            OryxRecorderWrapper::s_stop_flag = 0;
            break;
        }else if(1 == g_app_config.record_control_mode){
            unsigned int base_time = mcu_monitor.get_timer_start_point();
            duration = (cur_time - base_time) / 1000;
            if (base_time > 0 && duration >= g_app_config.record_duration_after_motion_stops){
                LOG_DEBUG("motion stop mode, cur_time = %u, base time = %u\n", cur_time, base_time);
                break;
            }
        }
        usleep(100000);
    }
}

static void streaming_run(unsigned int durationInMilliseconds){
    unsigned int cur_time = 0;
    unsigned int start_time = 0;
    start_time = get_current_time();
    while(1){
        if(check_standby_flag()){
            break;
        }
        cur_time = get_current_time();
        if ((cur_time - start_time) > durationInMilliseconds){
            break;
        }
        usleep(100000);
    }
}

static void update_amboot_params(app_conf_t* config)
{
    struct amboot_params params = {0};
    params.enable_audio = config->audio_enable;
    params.enable_fastosd = 0;
    params.enable_ldc = (unsigned char)config->enable_ldc;
    params.rotation_mode = config->rotate;

    if(config->video0_enable){
        params.stream0_enable = 1; //enable
        if(1280 == config->video0_width && 720 == config->video0_height){
            params.stream0_resolution = 1; //720p
        }else if(1920 == config->video0_width && 1080 == config->video0_height){
            params.stream0_resolution = 0; //1080p
        }else{
            LOG_ERROR("unsupported video 0 resolution(%dx%d)", config->video0_width, config->video0_height);
        }
        params.stream0_fmt = 1; //h264
        params.stream0_fps = config->video0_frame_rate;
        params.stream0_bitrate = (unsigned int)config->video0_recording_bitrate;
    }

    if(config->video1_enable){
        params.stream1_enable = 1; //enable
        if(720 == config->video1_width && 480 == config->video1_height){
            params.stream0_resolution = 2; //480p
        }else{
            LOG_ERROR("unsupported video 1 resolution(%dx%d)", config->video1_width, config->video1_height);
        }
        params.stream1_fmt = 1; //h264
        params.stream1_fps = 30;
        params.stream1_bitrate = 1000000;
    }

    snprintf(params.fastosd_string, sizeof(params.fastosd_string), "AMBA");
    params.enable_vca = config->vca_enable;
    params.vca_frame_num = (unsigned char)config->vca_frame_num;
    adc_util_update(&params);
}

static void config_run(BPIMcuProxy& proxy){
    LOG_PRINT("use 'fg' command if bpi_app is running in the background.\n");
    system("vi /etc/bpi/setting.ini");
    //load again to get modified conf
    app_conf_t tmp_app_config = {0};
    if(load_app_conf(&tmp_app_config) < 0){
        LOG_ERROR("load app config failed\n");
        return;
    }
    //config oryx settings
    if(!config_oryx_engine(&tmp_app_config)){
        LOG_ERROR("fail to config oryx configuration files.\n");
        return;
    }
    //update amboot parameters into ADC
    if(config_modified(&tmp_app_config, &g_app_config)){
        update_amboot_params(&tmp_app_config);
    }

    g_app_config = tmp_app_config;
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv){
    const char *short_options = "tdv";
    struct option long_options[] = {
        { "enable_log_time", 0, 0, 't' },
        { "enable_debug_log", 0, 0, 'd' },
        { "enable_vca_buf_dump", 0, 0, 'v' }
    };

    int ch = 0;
    int log_options = 0;
    setenv("DUMP_VCA_BUF","false",1);
    while(1){
        ch = getopt_long(argc, argv, short_options, long_options, NULL);
        if(-1 == ch) break;
        switch (ch)
        {
        case 't':
            log_options |= BPI_LOG_ENABLE_TIMESTAMP;
            break;
        case 'd':
            log_options |= BPI_LOG_ENABLE_DEBUG_LOG;
            break;
        case 'v':
            setenv("DUMP_VCA_BUF","true",1);
            break;
        default:
            printf("\n");
            printf("usage: bpi_app [-d] [-t]\n");
            printf("        -d,--enable_debug_log      enable debug logs\n");
            printf("        -t,--enable_log_time       enable log timestamp\n");
            printf("        -v,--enable_vca_buf_dump       enable vca buf dump, default to /sdcard.\n");
            printf("\n");
            return 1;
        }
    }
    set_log_options(log_options);
    BPIMcuProxy* mcu_proxy = new BPIMcuProxy();
    // int mcu_version;
    if(!mcu_proxy->init()){
        LOG_ERROR("bpi_app connect mcu failed!\n");
        return -1;
    }
    // mcu_version = mcu_proxy->get_mcu_version();
    // if(-1 == mcu_version){
    //     LOG_ERROR("bpi_app get mcu version failed!\n");
    //     return -1;
    // }
    // LOG_PRINT("mcu version is %d.%d\n", MCU_MAIN_VERSION(mcu_version), MCU_SUB_VERSION(mcu_version));
    mcu_proxy->send_cmd((MCU_CMD_TYPE)0xA0);
    if(run_normal_mode(*mcu_proxy)){
        mcu_proxy->send_cmd((MCU_CMD_TYPE)0xA2);
        return -1;
    }

    // need to revisit this loop 
    while(1) {
       run_hibernate_mode(*mcu_proxy);
       //notify mcu to power off
       sync();
       mcu_proxy->send_cmd(MCU_CMD_POWEROFF_CPU_DRAM);
       usleep(100000);
   }
    //This process can not exit,  otherwise it will close tcp-keep-alive fd
  //  while(1) usleep(100000);
    return 0;
}


//Normal mode, hibnate_image will be built.
//    Return value 0 indicates operation is successful.
//
static void suspend_to_disk()
{
    unsigned int op_address = 0x08601000;
    unsigned int op_size = 0x400000;
    umount_rw_partition();
    get_audio_param_from_iav(&op_address,&op_size);

    if (peek_dma_audio(op_address, DMA_DESCRIPTOR_OFFSET * 5 / 2) < 0) {
        printf("peek dma audio error, abort\n");
    }
    string flash_cmd = "flash_eraseall";
    system((flash_cmd + " " + BPI_swp).c_str());
    system("sync && echo 3 > /proc/sys/vm/drop_caches && echo disk > /sys/power/state");
}

static int  run_normal_mode(BPIMcuProxy& proxy)
{
    int ap_status = EWlan_Disconnected;

    if(load_app_conf(&g_app_config) < 0){
        LOG_ERROR("load app config failed\n");
        return -1;
    }
    //config oryx settings
    if (!config_oryx_engine(&g_app_config)){
        LOG_ERROR("fail to config oryx configuration files.\n");
        return -1;
    }

    ap_status = connect_ap(g_app_config.ap_ssid, g_app_config.ap_password, g_app_config.ap_wpa);
    if (EWlan_Connected != ap_status){
        LOG_PRINT("Failed to connect AP\n");
        return -1;
    }
    //wifi_host_sleep();
    update_amboot_params(&g_app_config);
    suspend_to_disk();
    return 0;
}
//
//hibernation mode,  provide elektra_services, for example, streaming, recording,etc.
//
static int run_hibernate_mode(BPIMcuProxy& proxy)
{
    LOG_PRINT(COLOR_RESET "Resume from hibernation, get_current_time = %u\n", get_current_time());
    handle_t cloud_agent = NULL;
    McuEventMonitor mcu_monitor;
    int wifi_state = -1;
    int ka_result = 0;

    mount_rw_partition();
    MCU_TRIGGER_TYPE wakeup_event = MCU_TRIGGER_BASE;
    wakeup_event = proxy.get_trigger_event();
    AM_BPI_MODE_MAP* mode_map = g_app_config.mode_map;

    if(load_app_conf(&g_app_config) < 0){
        LOG_ERROR("load app config failed\n");
        umount_rw_partition();
        return -1;
    }

    //do vca
    int vca_detected_or_not_enabled = 1;
    if (g_app_config.vca_enable && (mode_map[wakeup_event].bpi_mode == AM_BPI_MODE_RECORDING) && (g_app_config.vca_frame_num > 0)) {
        proxy.send_cmd(MCU_CMD_LED_RED_BLINK);

        VCA *vca = VCA::get_instance();
        vca->set_buffer_id(1);//buffer 1(source buffer of prev_c_yuv), prev_c_yuv
        vca->set_frame_size(g_app_config.vca_width, g_app_config.vca_height);
        vca->set_frame_number(g_app_config.vca_frame_num);
        vca->set_time_out(g_app_config.vca_timeout);
        if(vca->run() == 0){
            //Detected, recording continue
            LOG_PRINT("recording vca done with person\n");
            vca_detected_or_not_enabled = 1;
        }else{
            //No human is detected, just suspend again.
            LOG_PRINT("recording vca done with no person\n");
            vca_detected_or_not_enabled = 0;
        }
        if(!vca_detected_or_not_enabled){
            wifi_host_sleep();
            goto SUSPEND;
        }
    }

    proxy.send_cmd(MCU_CMD_LED_GREEN_ON);

    wifi_state = hibernate_wlan_resume();
    proxy.attach(&mcu_monitor);
    if(MCU_TRIGGER_PIR_ON == wakeup_event || MCU_TRIGGER_WIFI_WAKEUP == wakeup_event){
        cloud_agent = start_device_agent((AM_BPI_MODE_RECORDING == mode_map[wakeup_event].bpi_mode) ? DEVICE_MODE_RECORDING : DEVICE_MODE_STREAMING);
        if(cloud_agent){
            device_connect_cloud(cloud_agent);
            mcu_monitor.set_cloud_agent(cloud_agent);
        }
    }
    if(AM_BPI_MODE_RECORDING == mode_map[wakeup_event].bpi_mode){
        OryxRecorderWrapper recorder;
        BPIUploader bpi_uploader;
        char server_url[128] = {0};
        if(!strncmp(g_app_config.storage_folder, "/sdcard", strlen("/sdcard"))){
            mount_sdcard();
        }
        recorder.set_data_handler(&bpi_uploader);
        if(!recorder.init_recorder(&g_app_config)){
            LOG_ERROR("Failed to init recorder engine.\n");
            return -1;
        }
        snprintf(server_url, sizeof(server_url), "http://%s:6024", g_app_config.cloud_server_ip);
        bpi_uploader.set_url(server_url);
        bpi_uploader.scan_folder(g_app_config.storage_folder);
        bpi_uploader.start();
        recorder.start_recorder();
        mcu_monitor.init_timer_start_point();
        recording_run(mcu_monitor);
        recorder.stop_recorder();
        if(!bpi_uploader.wait_jobs_done(5)){
            LOG_DEBUG("uploader not empty, continue it next time\n");
        }
        bpi_uploader.stop();
        if(!strncmp(g_app_config.storage_folder, "/sdcard", strlen("/sdcard"))){
            umount_sdcard();
        }
    }
    else if(AM_BPI_MODE_STREAMING == mode_map[wakeup_event].bpi_mode){
        LOG_PRINT("Streaming mode\n");
        transport_info_t transport_info = {0};
        char *audio_codec_name = NULL;
        int audio_payload_type = -1;
        AMPlaybackRtpUri rtp_uri;
        bpi_streamer_info streamer_info = {0};
        rtp_uri.udp_port = 8288;
        rtp_uri.ip_domain  = AM_PLAYBACK_IPV4;
        char *streaming_dest = get_fast_streaming_client();
        if(streaming_dest){
            if (48000 == g_app_config.audio_sample_rate){
                audio_codec_name = (char*)"aac-48k";
                audio_payload_type = 97;
                rtp_uri.audio_type =  AM_AUDIO_AAC;
                rtp_uri.sample_rate = 48000;
                rtp_uri.channel = 1;
            }else if (16000 == g_app_config.audio_sample_rate){
                audio_codec_name = (char*)"aac-16k";
                audio_payload_type = 97;
                rtp_uri.audio_type =  AM_AUDIO_AAC;
                rtp_uri.sample_rate = 16000;
                rtp_uri.channel = 1;
            }else if (8000 == g_app_config.audio_sample_rate){
                audio_codec_name = (char*)"g711-8k";
                audio_payload_type = 8;//g711-ALaw
                rtp_uri.audio_type =  AM_AUDIO_G711A;
                rtp_uri.sample_rate = 8000;
                rtp_uri.channel = 1;
            }
            snprintf(transport_info.ipaddress, sizeof(transport_info.ipaddress), "%s", streaming_dest);//TODO
            transport_info.is_ipv4 = 1;
            transport_info.video.enable = g_app_config.video0_enable;
            transport_info.video.port = 8554;
            transport_info.video.payload_type = 96;
            transport_info.video.ssrc = 0xfeadbeaf;
            transport_info.audio.enable = g_app_config.audio_enable;
            transport_info.audio.port = 8556;
            transport_info.audio.payload_type = audio_payload_type;
            transport_info.audio.ssrc = 0xfeadbea1;
            streamer_info.stream_id = 0;
            streamer_info.enable_lbr = g_app_config.smart_avc;
            streamer_info.target_bitrate = g_app_config.video0_streaming_bitrate;
            streamer_info.audio_codec_name = audio_codec_name;
            streamer_info.transport_info = &transport_info;
            streamer_info.osd_label = g_app_config.osd_label;
            streaming_initialize(&streamer_info);
            //audio_play_initialize(rtp_uri);
            streaming_run(g_app_config.streaming_duration * 1000);
            audio_play_finalize();
            streaming_finalize();
        }
    }
    else if(AM_BPI_MODE_DEBUG == mode_map[wakeup_event].bpi_mode){
        LOG_PRINT("Debug mode\n");
        config_run(proxy);
        cloud_agent = start_device_agent(DEVICE_MODE_STANDBY_TCP);
        if(cloud_agent){
            device_connect_cloud(cloud_agent);
        }
    }
    else if(AM_BPI_MODE_WLAN_RECONNECT == mode_map[wakeup_event].bpi_mode){
        LOG_PRINT("--------->> wifi reconnect mode\n");
        if(AP_LOSS_BEACON == wifi_state || AP_DISASSOCIATION_OR_AUTHENTICATION == wifi_state ||WIFI_WAKE_NORMAL == wifi_state){
            int ap_status = EWlan_Disconnected;
            ap_status = connect_ap(g_app_config.ap_ssid, g_app_config.ap_password, g_app_config.ap_wpa);
            if (EWlan_Connected != ap_status){
                LOG_PRINT("Failed to reconnect AP, wifi wake reason=%d \n", wifi_state);
            }
        }
    }
    else{

    }

    if (cloud_agent) {
        mcu_monitor.set_cloud_agent(NULL);
        device_destroy(&cloud_agent);
        cloud_agent = NULL;
    }
    ka_result = wlan_keepalive(g_app_config.keepalive_interval,
                        g_app_config.dtim_interval,
                        g_app_config.wakeup_token,
                        g_app_config.cloud_server_ip);
    if (0 == ka_result) {
        LOG_PRINT("Keep alive: OK\n");
    } else if (ECloud_Disconnected == ka_result){
        LOG_PRINT("Server not reachable, wakeup later to reconnect\n");
        proxy.send_cmd(MCU_CMD_NETWORK_NEED_REBUILD);
    }else{
        LOG_ERROR("Keep alive: Fail\n");
    }
SUSPEND:
    umount_rw_partition();
    return 0;
}
