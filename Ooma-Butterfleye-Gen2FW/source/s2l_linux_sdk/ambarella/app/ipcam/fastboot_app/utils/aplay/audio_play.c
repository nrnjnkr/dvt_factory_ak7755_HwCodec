/*
 * audio_play.c
 *
 * History:
 *       2015/05/29 - [Jian Liu] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "audio_play.h"

#ifdef AUDIO_INPUT_ALSA_SUPPORTED
#include <alsa/asoundlib.h>
typedef struct _audio_handle_t {
    snd_pcm_t *snd_pcm;
    snd_output_t *mpLog;
    snd_pcm_stream_t stream;
    snd_pcm_format_t format;
    unsigned int sample_rate;
    unsigned int channel_number;
    snd_pcm_uframes_t mChunkSize;
    int mBitsPerFrame;
}audio_handler_t;
static audio_handler_t *open_audio_hal(unsigned int sample_rate,int channel_number,int frame_samples);
static int  audio_start(audio_handler_t *handler);
static void close_audio_hal(audio_handler_t *handler);
static int  read_audio_stream(audio_handler_t *handler, unsigned char *data,  int size, int *frame_number);
//static int get_data_size(audio_handler_t *handler,int frame_number);
#else
typedef struct _audio_handle_t {
    void *data;
}audio_handler_t;
static audio_handler_t *open_audio_hal(unsigned int sample_rate,int channel_number,int frame_samples){
    audio_handler_t *handle = NULL;
    return handle;
}
static int  audio_start(audio_handler_t *handler){
    return 0;
}
static void close_audio_hal(audio_handler_t *handler){
    return;
}
static int  read_audio_stream(audio_handler_t *handler, unsigned char *data,  int size, int *frame_number){
    return -1;
}
#endif


#ifdef AUDIO_INPUT_ALSA_SUPPORTED
static audio_handler_t audio_handler;
static
audio_handler_t *open_audio_hal(unsigned int sample_rate,int channel_number,int frame_samples)
{
    snd_pcm_hw_params_t *params = NULL;
    snd_pcm_sw_params_t *swparams = NULL;
    snd_pcm_uframes_t buffer_size;
    unsigned int buffer_time = 0;
    unsigned int  start_threshold, stop_threshold;
    int err;

    audio_handler_t *handler = &audio_handler;
    memset(&audio_handler,0,sizeof(audio_handler));
    audio_handler.stream = SND_PCM_STREAM_CAPTURE;
    audio_handler.format = SND_PCM_FORMAT_S16_LE;
    audio_handler.sample_rate = sample_rate;
    audio_handler.channel_number = channel_number;

    err = snd_output_stdio_attach(&handler->mpLog, stderr, 0);
    if(err < 0){
        perror("snd_output_stdio_attach");
        goto exit;
    }

    err = snd_pcm_open(&handler->snd_pcm, "default", handler->stream, 0);
    if (err < 0) {
        err = snd_pcm_open(&handler->snd_pcm, "MICALL", handler->stream, 0);
    }
    if (err < 0) {
        printf("Capture audio open error: %s\n", snd_strerror(err));
        goto exit;
    }

    snd_pcm_hw_params_alloca(&params);
    err = snd_pcm_hw_params_any(handler->snd_pcm, params);
    if (err < 0) {
        perror("Broken configuration for this PCM: no configurations available\n");
        goto exit;
    }

    err = snd_pcm_hw_params_set_access(handler->snd_pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        perror("Access type not available\n");
        goto exit;
    }

    err = snd_pcm_hw_params_set_format(handler->snd_pcm, params, handler->format);
    if (err < 0) {
        perror("Sample format non available\n");
        goto exit;
    }

    err = snd_pcm_hw_params_set_channels(handler->snd_pcm, params, handler->channel_number);
    if (err < 0) {
        perror("Channels count non available\n");
        goto exit;
    }

    err = snd_pcm_hw_params_set_rate_near(handler->snd_pcm, params, &handler->sample_rate, 0);
    //AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    //AM_ASSERT(err >= 0);
    if (buffer_time > 500000)
        buffer_time = 500000;

    handler->mChunkSize = frame_samples;
    err = snd_pcm_hw_params_set_period_size(handler->snd_pcm, params, handler->mChunkSize, 0);
    //AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_set_buffer_time_near(handler->snd_pcm, params, &buffer_time, 0);
    //AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params(handler->snd_pcm, params);
    if (err < 0) {
        snd_pcm_hw_params_dump(params, handler->mpLog);
        goto exit;
    }

    snd_pcm_hw_params_get_period_size(params, &handler->mChunkSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (handler->mChunkSize == buffer_size) {
        goto exit;
    }

    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(handler->snd_pcm, swparams);
    err = snd_pcm_sw_params_set_avail_min(handler->snd_pcm, swparams, handler->mChunkSize);

     start_threshold = 1;
    err = snd_pcm_sw_params_set_start_threshold(handler->snd_pcm, swparams, start_threshold);
    //AM_ASSERT(err >= 0);

    stop_threshold = buffer_size;
    err = snd_pcm_sw_params_set_stop_threshold(handler->snd_pcm, swparams, stop_threshold);
    //AM_ASSERT(err >= 0);

    if (snd_pcm_sw_params(handler->snd_pcm, swparams) < 0) {
        //AM_ERROR("unable to install sw params:\n");
        snd_pcm_sw_params_dump(swparams, handler->mpLog);
        goto exit;
    }
    handler->mBitsPerFrame = snd_pcm_format_physical_width(handler->format) * handler->channel_number;
    return handler;
exit:
    if(handler->mpLog){
       snd_output_close(handler->mpLog);
       handler->mpLog = NULL;
    }
    if (handler->snd_pcm != NULL){
        snd_pcm_close(handler->snd_pcm);
        handler->snd_pcm = NULL;
    }
    return NULL;
}

static
void close_audio_hal(audio_handler_t *handler)
{
     if(handler->mpLog != NULL){
        snd_output_close(handler->mpLog);
        handler->mpLog = NULL;
     }
     if (handler->snd_pcm != NULL){
        snd_pcm_close(handler->snd_pcm);
        handler->snd_pcm = NULL;
     }
}

static
int  audio_start(audio_handler_t *handler)
{
    snd_pcm_status_t *status;
    int err;

    if ((err = snd_pcm_start(handler->snd_pcm)) < 0) {
        //AM_ERROR("PCM start error: %s\n", snd_strerror(err));
        return -1;
    }

    snd_pcm_status_alloca(&status);

    if ((err = snd_pcm_status(handler->snd_pcm, status))<0) {
        //AM_ERROR("Get PCM status error: %s\n", snd_strerror(err));
        return -1;
    }
    //snd_pcm_status_get_trigger_tstamp(status, &mStartTimeStamp);
    //AM_PRINTF("start time %d:%d\n", (AM_INT)mStartTimeStamp.tv_sec, (AM_INT)mStartTimeStamp.tv_usec);

    return 0;
}

static int PcmRead(audio_handler_t *handler,unsigned char *pData, unsigned int rcount);
static
int  read_audio_stream(audio_handler_t *handler, unsigned char *pData,  int dataSize, int *pNumFrames)
{
    //snd_pcm_status_t *status;
    //snd_timestamp_t now, diff, trigger;
    int frm_cnt;

    if(dataSize < (int)(handler->mChunkSize * handler->mBitsPerFrame / 8)){
        // make sure buffer can hold one chunck of data
        return -1;
    }
    frm_cnt = PcmRead(handler,pData, (unsigned int)handler->mChunkSize);

    if (frm_cnt <= 0)
        return -1;

    //AM_ASSERT(frm_cnt == handler->mChunkSize);
    *pNumFrames = frm_cnt;
    //printf("read_audio_stream, chunkSize = %d, mBitsPerFrame = %d, frm_cnt = %d\n",(int)handler->mChunkSize,handler->mBitsPerFrame,frm_cnt);

     return 0;
}

/*
static
int get_data_size(audio_handler_t *handler,int  frame_number){
    return frame_number * handler->mBitsPerFrame / 8;
}
*/

// I/O error handler /
static int Xrun(audio_handler_t *handler)
{
    snd_pcm_status_t *status;
    int err;

    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(handler->snd_pcm, status))<0) {
        //AM_ERROR("status error: %s\n", snd_strerror(err));
        return -1;
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        if ((err = snd_pcm_prepare(handler->snd_pcm))<0) {
            //AM_ERROR("xrun: prepare error: %s", snd_strerror(err));
            return -1;
        }
        return 0;       // ok, data should be accepted again
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        //AM_INFO("capture stream format change? attempting recover...\n");
        if ((err = snd_pcm_prepare(handler->snd_pcm))<0) {
            //AM_ERROR("xrun(DRAINING): prepare error: %s\n", snd_strerror(err));
            return -1;
        }
        return 0;
    }
    return 0;
}

static int Suspend(audio_handler_t *handler)
{
    int err;
    while ((err = snd_pcm_resume(handler->snd_pcm)) == -EAGAIN)
    //usleep(1000000);  /* wait until suspend flag is released */
    if (err < 0) {
        if ((err = snd_pcm_prepare(handler->snd_pcm)) < 0) {
            return -1;
        }
    }
    return 0;
}

static int PcmRead(audio_handler_t *handler,unsigned char *pData, unsigned int rcount)
{
    int err;
    int  r;
    int  result = 0;
    unsigned int  count = rcount;

    if (count != handler->mChunkSize) {
        count = handler->mChunkSize;
    }

    while (count > 0) {
        r = snd_pcm_readi(handler->snd_pcm, pData, count);

        if (r == -EAGAIN || (r >= 0 && (unsigned int )r < count)) {
            //if (!mbNoWait)
            //snd_pcm_wait(handler->snd_pcm, 100);
        } else if (r == -EPIPE) {                   // an overrun occurred
            if ((err = Xrun(handler)) != 0)
                return -1;
        } else if (r == -ESTRPIPE) {            // a suspend event occurred
            if ((err = Suspend(handler)) != 0)
                return -1;
        } else if (r < 0) {
            if(r == -EIO){
                //AM_INFO("-EIO error!\n");
            }else if(r == -EINVAL){
                //AM_INFO("-EINVAL error!\n");
            } else if(r == -EINTR){
                //AM_INFO("-EINTR error!\n");
            }else{
                //AM_ERROR("Read error: %s(%d)\n", snd_strerror(r), r);
            }
            return -1;
        }
        if (r > 0) {
            result += r;
            count -= r;
            pData += r * handler->mBitsPerFrame / 8;      // convert frame num to bytes
        }
    }
    return result;
}
#endif


static volatile int audio_capture_ready_flag = 0;
static void *workaround_routine(void *arg){
    volatile int *exit_flag = (volatile int*)arg;
    void *hal;
    hal =  (void*)open_audio_hal(48000,1,1024);
    if(!hal){
        printf("aplay workaround_routine -- failed to open_audio caputre\n");
        fflush(stdout);
        audio_capture_ready_flag = 1;
        return (void*)NULL;
    }

    audio_handler_t *handler = (audio_handler_t *)hal;
    if(audio_start(handler) < 0){
        printf("aplay workaround_routine -- failed to open_audio caputre\n");
        fflush(stdout);
        close_audio_hal(handler);
        audio_capture_ready_flag = 1;
        return (void*)NULL;
    }

    do{
        unsigned char data[4096];
        int num_frames;
        read_audio_stream(handler, data, (int)4096, &num_frames);
    }while(0);

    audio_capture_ready_flag = 1;
    while(!(*exit_flag)){
        usleep(1000 * 1000);
    }
    close_audio_hal(handler);
    return (void*)NULL;
}

int aplay_audio(char* filepath){
    pthread_t thread_id;
    volatile int exit_flag = 0;
    audio_capture_ready_flag = 0;
    int ret = pthread_create(&thread_id, NULL, workaround_routine, (void*)&exit_flag);
    if (ret != 0) {
        printf("aplay_audio pthread_create fail\n");
        fflush(stdout);
        return -1;
    }
    while(!audio_capture_ready_flag) usleep(10000);

    char cmd[2048];
    snprintf(cmd,sizeof(cmd),"/usr/bin/aplay  %s",filepath);
    system(cmd);
    exit_flag = 1;
    pthread_join(thread_id,NULL);
    return 0;
}

void load_audio_playback_driver(void)
{
#if defined(CONFIG_ELEKTRA_CODEC_WM8974)
    system("/sbin/modprobe i2c-dev");
    system("/sbin/modprobe snd-soc-core pmdown_time=300");
    system("/sbin/modprobe snd-soc-ambarella");
    system("/sbin/modprobe snd-soc-ambarella-i2s capture_enabled=0");
#elif defined(CONFIG_BTFL_CODEC_AK7755)
    system("/sbin/modprobe snd-soc-core pmdown_time=500");
    system("/sbin/modprobe snd-soc-ambarella");
    system("/sbin/modprobe snd-soc-ambarella-i2s capture_enabled=0");
    system("/sbin/modprobe snd-soc-ak7755 fast_boot=1");
#endif
    system("/sbin/modprobe snd-soc-ambdummy");
    system("/sbin/modprobe snd-soc-amba-board");
}

void unload_audio_playback_driver(void)
{
    system("/sbin/modprobe -r snd-soc-amba-board");
    system("/sbin/modprobe -r snd-soc-ambdummy");
#if defined(CONFIG_ELEKTRA_CODEC_WM8974)
    system("/sbin/modprobe -r snd-soc-ambarella-i2s");
    system("/sbin/modprobe -r snd-soc-ambarella");
    system("/sbin/modprobe -r snd-soc-core");
    system("/sbin/modprobe -r i2c-dev");
#elif defined(CONFIG_BTFL_CODEC_AK7755)
    system("/sbin/modprobe -r snd-soc-ak7755");
    system("/sbin/modprobe -r snd-soc-ambarella-i2s");
    system("/sbin/modprobe -r snd-soc-ambarella");
    system("/sbin/modprobe -r snd-soc-core");
#endif
}

void play_audio_by_aplay(const char *filepath)
{
    char spawn_aplay_input[128] = {0};
    snprintf(spawn_aplay_input,sizeof(spawn_aplay_input),"/usr/bin/aplay %s", filepath);
    spawn_aplay_input[sizeof(spawn_aplay_input)-1] = '\0';
    system(spawn_aplay_input);
}
