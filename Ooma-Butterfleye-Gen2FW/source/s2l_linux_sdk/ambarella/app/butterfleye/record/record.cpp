#include<stdio.h>
#include "bpi_app_config.h"
#include "bpi_oryx_config.h" 
#include "bpi_oryx_export.h"
#include <time.h>
#include "bpi_utils.h"

#include "adc/adc_util.h" 

#include <time.h>

OryxRecorderWrapper recorder;
static app_conf_t g_app_config;

/*static unsigned long long get_current_time(void) {                               
    struct timespec now;                                                        
    clock_gettime(CLOCK_MONOTONIC,&now);                                        
    //unsigned long long mseconds = now.tv_sec * 1000+ now.tv_nsec/1000000;       
    unsigned long long mseconds = now.tv_sec + now.tv_nsec/1000000;       
    return mseconds;                                                            
}*/


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
            LOG_ERROR( "unsupported video 0 resolution(%dx%d)", config->video0_width, config->video0_height);
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
            LOG_ERROR( "unsupported video 1 resolution(%dx%d)", config->video1_width, config->video1_height);
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

static void recording_run(){                        
    unsigned int cur_time = 0;                                                  
    unsigned int start_time = 0;                                                
    int duration = 0;                                     
    start_time = get_current_time();                                            
    while(1){                                                                   
        cur_time = get_current_time();                                          
        duration = (cur_time - start_time);                           
	    LOG_DEBUG("duration %d cur_time %d recording max_duration %d\n",
		duration, cur_time, g_app_config.recording_max_duration/10-27);   
        if (duration >= (g_app_config.recording_max_duration/10-27)) {                  
            LOG_DEBUG( "Reached max recording duration.\n");                      
            OryxRecorderWrapper::s_stop_flag = 0;                               
            usleep(1000000);                                                         
           break;                                                               
        }                                                                       
        usleep(1000000);                                                         
    }                                                                           
} 

extern "C" {




int recordstream(int timestamp) {

    if(load_app_conf(&g_app_config) < 0){                                       
        LOG_ERROR( "load app config failed\n");                                  
        return -1;                                                              
    } 

    //config oryx settings                                                      
    if (!config_oryx_engine(&g_app_config)){                                    
        LOG_ERROR( "fail to config oryx configuration files.\n");                
        return -1;                                                              
    }  

    update_amboot_params(&g_app_config);                                  
	
    if(!recorder.init_recorder(&g_app_config)){                             
        LOG_ERROR( "Failed to init recorder engine.\n");                     
            return -1;                                                          
    } 

    recorder.start_recorder();                                              
    recording_run();
    recorder.stop_recorder();  
    LOG_DEBUG("Record is Done\n");
    return 0;

}

}
