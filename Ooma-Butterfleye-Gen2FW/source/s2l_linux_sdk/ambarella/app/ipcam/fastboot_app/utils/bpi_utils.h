/*
 * bpi_utils.h
 *
 * History:
 *       2015/08/03 - [Jian Liu] created file
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
#ifndef FASTBOOT_UTILS_H__
#define FASTBOOT_UTILS_H__

#include <time.h>
#define MAX_IP_LEN                          64
#define MAX_PORT_LEN                        12

typedef enum {
  BPI_LOG_PRINT  = 0,
  BPI_LOG_ERROR  = 1,
  BPI_LOG_WARN   = 2,
  BPI_LOG_DEBUG  = 3,
} BPI_LOG_TYPE;

typedef enum {
  BPI_LOG_ENABLE_DEBUG_LOG  = 1 << 0,
  BPI_LOG_ENABLE_TIMESTAMP  = 1 << 1,
} BPI_LOG_OPTIONS;

#ifdef __cplusplus
extern "C" {
#endif
//platform related
int mount_sdcard(void);
void umount_sdcard(void);
void enable_usb_ethernet(void);


//GPIO related
void shut_down();
int check_boot_mode();
void reset_wifi_chip();

//utils
unsigned int  get_current_time(void);
int get_macaddress(char *itf_name,unsigned char macAddress[6]);
char *get_device_id(void);
time_t get_time_from_name(const char* filename);
int get_id_from_name(const char* filename);
int get_num_from_tail(const char* filename);
void start_time();

int param_device_parse_server(const char *p_str, char* ip, int* p_port) ;
void bpi_log(BPI_LOG_TYPE log_type, const char *format, ...);
void set_log_options(int log_options);

#ifdef __cplusplus
};
#endif

#define COLOR_RESET "\x1B[0m"
#define LOG_ERROR(format, args...)  bpi_log(BPI_LOG_ERROR, format, ##args)
#define LOG_PRINT(format, args...)  bpi_log(BPI_LOG_PRINT, format, ##args)
#define LOG_DEBUG(format, args...)  bpi_log(BPI_LOG_DEBUG, format, ##args)
#define LOG_WARNING(format, args...)  bpi_log(BPI_LOG_WARN, format, ##args)

#endif//PLATFORM_UTILS_H__

