/*
 * bpi_mcu_proxy.cpp
 *
 * History:
 *       2016/09/01 - [Zhipeng Dong] created file
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
#include <fcntl.h>
#include <assert.h>
#include <termios.h>

#include "bpi_typedefs.h"
#include "bpi_mcu_proxy.h"
#include "bpi_mcu_event_monitor.h"
#include "bpi_utils.h"
#include "config.h"

#if defined(CONFIG_BSP_BOARD_S2LM_ELEKTRA) || (CONFIG_BSP_BOARD_BTFL)
#include "string.h"

#define MAX_LENGTH_GPIO_PATH 128

#define GPIO_PORT_SHUTDOWN 112
#define GPIO_PORT_BOOTMOOD_0 92//PC1
#define GPIO_PORT_BOOTMOOD_1 91//PB7
#define GPIO_PORT_BOOTMOOD_2 38//PB8

#define GPIO_SET 49
#define GPIO_CLEAR 48
#endif


#define UART_PORT_NAME "/dev/ttyS0"
#define UART_PORT_BAUDRATE B9600
#define UART_PORT_FLOWCTRL 0
using std::unique_lock;
using std::cv_status;

BPIMcuProxy::BPIMcuProxy(){
    m_uart_fd = -1;
    m_pipe_fd[0] = -1;
    m_pipe_fd[1] = -1;
    m_thread = nullptr;
    m_observer = nullptr;
}

BPIMcuProxy::~BPIMcuProxy(){
    if(m_thread){
        char cmd = 'Z';
        write(m_pipe_fd[1], &cmd, 1);
        LOG_PRINT("~BPIMcuProxy\n");
        m_thread->join();
        m_thread = nullptr;
    }

    if(m_uart_fd){
        close(m_uart_fd);
        m_uart_fd = -1;
    }

    if(m_pipe_fd[0]){
        close(m_pipe_fd[0]);
    }

    if(m_pipe_fd[1]){
        close(m_pipe_fd[1]);
    }
}

bool BPIMcuProxy::init(){
#if defined(CONFIG_AMBARELLA_SERIAL_PORT0_NA)
    if(!mcu_connect()){
        LOG_ERROR("mcu connect failed\n");
        return false;
    }

    if (pipe(m_pipe_fd) < 0) {
        LOG_ERROR("create pipe failed\n");
        return false;
    }

    m_thread = new thread(on_mcu_events, this);
    if(nullptr == m_thread){
        return false;
    }
#endif
    return true;
}

#if defined(CONFIG_BSP_BOARD_S2LM_ELEKTRA) || (CONFIG_BSP_BOARD_BTFL)
static int gpio_write(int gpio, unsigned char value)
{
    int fd_gpio = -1;
    int length = 0;
    char path[MAX_LENGTH_GPIO_PATH] = {0};

    //export gpio
    char gpio_num[32] = {0};
    fd_gpio = open("/sys/class/gpio/export", O_WRONLY, 0);
    if (fd_gpio < 0) {
        perror("open /sys/class/gpio/export");
        return -1;
    }
    length = snprintf(gpio_num, 32, "%d", gpio);
    write(fd_gpio, gpio_num, length);
    close(fd_gpio);

    //direction
    char gpio_direction[] = "out";//sw mode
    snprintf(path, MAX_LENGTH_GPIO_PATH, "/sys/class/gpio/gpio%d/direction", gpio);
    fd_gpio = open(path, O_WRONLY, 0);
    if (fd_gpio < 0) {
        perror("open gpio direction");
        return -1;
    }
    write(fd_gpio, gpio_direction, strlen(gpio_direction));
    close(fd_gpio);

    //value
    snprintf(path, MAX_LENGTH_GPIO_PATH, "/sys/class/gpio/gpio%d/value", gpio);
    fd_gpio = open(path, O_WRONLY, 0);
    if (fd_gpio < 0) {
        perror("open gpio value");
        return -1;
    }
    write(fd_gpio, &value, 1);
    close(fd_gpio);

    return 0;
}

static int gpio_read(int gpio, unsigned char* value)
{
    int fd_gpio = -1;
    int length = 0;
    char path[MAX_LENGTH_GPIO_PATH] = {0};

    *value = 0;

    //export gpio
    char gpio_num[32] = {0};
    fd_gpio = open("/sys/class/gpio/export", O_WRONLY, 0);
    if (fd_gpio < 0) {
        perror("open /sys/class/gpio/export");
        return -1;
    }
    length = snprintf(gpio_num, 32, "%d", gpio);
    write(fd_gpio, gpio_num, length);
    close(fd_gpio);

    //direction
    char gpio_direction[] = "in";//sw mode
    snprintf(path, MAX_LENGTH_GPIO_PATH, "/sys/class/gpio/gpio%d/direction", gpio);
    fd_gpio = open(path, O_WRONLY, 0);
    if (fd_gpio < 0) {
        perror("open gpio direction");
        return -1;
    }
    write(fd_gpio, gpio_direction, strlen(gpio_direction));
    close(fd_gpio);

    //value
    snprintf(path, MAX_LENGTH_GPIO_PATH, "/sys/class/gpio/gpio%d/value", gpio);
    fd_gpio = open(path, O_RDONLY, 0);
    if (fd_gpio < 0) {
        perror("open gpio value");
        return -1;
    }
    read(fd_gpio, value, 1);
    close(fd_gpio);

    return 0;
}
#endif

MCU_TRIGGER_TYPE BPIMcuProxy::get_trigger_event(){
#if defined(CONFIG_BSP_BOARD_S2LM_ELEKTRA) || (CONFIG_BSP_BOARD_BTFL)
    MCU_TRIGGER_TYPE ret = MCU_TRIGGER_BASE;
    unsigned char value = 0;
    int mode = 0;

    if (gpio_read(GPIO_PORT_BOOTMOOD_0, &value) != 0) {
        LOG_ERROR("gpio_read gpio 92 fail\n");
        return ret;
    }

    if (value == GPIO_SET) {
        mode |= 1;
    }

    if (gpio_read(GPIO_PORT_BOOTMOOD_1, &value) != 0) {
        LOG_ERROR("gpio_read gpio 91 fail\n");
        return ret;
    }

    if (value == GPIO_SET) {
        mode |= 1 << 1;
    }

    if (gpio_read(GPIO_PORT_BOOTMOOD_2, &value) != 0) {
        LOG_ERROR("gpio_read gpio 38 fail\n");
        return ret;
    }

    if (value == GPIO_SET) {
        mode |= 1 << 2;
    }

    switch(mode){
        case 0:
            ret = MCU_TRIGGER_PWR;
            break;
        case 1:
            ret = MCU_TRIGGER_WIFI_RECONNECT;
            break;
        case 4:
            ret = MCU_TRIGGER_PIR_ON;
            break;
        case 5:
            ret = MCU_TRIGGER_WIFI_WAKEUP;
            break;
        default:
            break;
    }

    return ret;
#else
    unique_lock<mutex> lk(m_mutex_cv);
    char request_code = MCU_CMD_GET_TRIGGER_EVENT;
    int len = write(m_uart_fd, &request_code, 1);
    if(len != 1){
        LOG_ERROR("request trigger event failed\n");
        return MCU_TRIGGER_BASE;
    }else{
        int count = 0;
        do{
            if(++count > 5){
                LOG_ERROR("get trigger event time out!\n");
                return MCU_TRIGGER_BASE;
            }
            if(cv_status::timeout == m_cv.wait_for(lk, std::chrono::milliseconds(100))
             || ((MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_PIR_ON
                && (MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_WIFI_WAKEUP
                && (MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_WIFI_RECONNECT
                && (MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_PWR
                && (MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_PWR_2S
                && (MCU_TRIGGER_TYPE)m_cur_event != MCU_TRIGGER_POWER_EMPTY)){
                continue;
            }else{
                break;
            }
        }while(1);

        return (MCU_TRIGGER_TYPE)m_cur_event;
    }
#endif
}

MCU_TRIGGER_TYPE BPIMcuProxy::get_competition_trigger_event()
{
    unique_lock<mutex> lk(m_mutex_cv);
    char request_code = MCU_CMD_GET_RACE_TRIGGER_EVENT;
    int len = write(m_uart_fd, &request_code, 1);
    if(len != 1){
        LOG_ERROR("request trigger event failed\n");
        return MCU_TRIGGER_BASE;
    }else{
        int count = 0;
        do{
            if(++count > 5){
                LOG_ERROR("request competition trigger event time out!\n");
                return MCU_TRIGGER_BASE;
            }
            if(cv_status::timeout == m_cv.wait_for(lk, std::chrono::milliseconds(100)) || (m_cur_event != MCU_EVENT_RACE_TRIGGER_EVENT)){
                continue;
            }else{
                break;
            }
        }while(1);

        return (MCU_TRIGGER_TYPE)m_cur_data1;
    }
}

int BPIMcuProxy::get_time_cost(){
    unique_lock<mutex> lk(m_mutex_cv);
    char request_code = MCU_CMD_GET_TIME_COST;
    int len = write(m_uart_fd, &request_code, 1);
    if(len != 1){
        LOG_ERROR("request time cost from mcu failed\n");
        return -1;
    }else{
        int count = 0;
        do{
            if(++count > 5){
                LOG_ERROR("request time cost from mcu time out!\n");
                return -1;
            }
            if(cv_status::timeout == m_cv.wait_for(lk, std::chrono::milliseconds(100)) || (m_cur_event != MCU_EVENT_TIME_COST)){
                continue;
            }else{
                break;
            }
        }while(1);
        return m_cur_data1;
    }
}

int BPIMcuProxy::get_mcu_version(){
    unique_lock<mutex> lk(m_mutex_cv);
    char request_code = MCU_CMD_GET_VERSION;
    int len = write(m_uart_fd, &request_code, 1);
    if(len != 1){
        LOG_ERROR("request mcu fw version from mcu failed\n");
        return -1;
    }else{
        int count = 0;
        do{
            if(++count > 5){
                LOG_ERROR("request mcu fw version from mcu time out!\n");
                return -1;
            }
            if(cv_status::timeout == m_cv.wait_for(lk, std::chrono::milliseconds(100)) || (m_cur_event != MCU_EVENT_VERSION)){
                continue;
            }else{
                break;
            }
        }while(1);
        return m_cur_data1;
    }
}

bool BPIMcuProxy::wait_on_event(int event_type, int* data, int timeout_ms){
    unique_lock<mutex> lk(m_mutex_cv);
    auto now = std::chrono::system_clock::now();
    if(m_cv.wait_until(lk, now + std::chrono::milliseconds(timeout_ms), [this, event_type](){return m_cur_event == event_type;})){
        m_cur_event = MCU_EVENT_BASE;
        *data = m_cur_data1;
        return true;
    }else{
        return false;
    }
}

void BPIMcuProxy::send_cmd(MCU_CMD_TYPE cmd_type){
#if defined(CONFIG_BSP_BOARD_S2LM_ELEKTRA) || (CONFIG_BSP_BOARD_BTFL)
    if(MCU_CMD_POWEROFF_CPU_DRAM == cmd_type){
        sync();
        usleep(500000);
        while (1) {
            gpio_write(GPIO_PORT_SHUTDOWN, (unsigned char)'1');
            usleep(50000);
        }
    }else{
        LOG_ERROR("unimplemented cmd(%d)\n", cmd_type);
    }
#else
    char cmd = cmd_type;
    int len = write(m_uart_fd, &cmd, 1);
    if(len != 1){
        LOG_ERROR("send cmd(%d) to mcu failed\n", cmd);
    }
#endif
    return;
}

void BPIMcuProxy::attach(IMcuEventObserver* observer){
    m_observer = observer;
}
bool BPIMcuProxy::mcu_connect(){
    int ret = false;
    do {
        struct termios options;
        m_uart_fd = open(UART_PORT_NAME, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (m_uart_fd < 0) {
            LOG_ERROR("Open serial port fail!\n");
            break;
        }

        tcflush(m_uart_fd, TCIOFLUSH);
        if (tcgetattr(m_uart_fd, &options) < 0) {
            LOG_ERROR("Get port options fail\n");
            break;
        }

        //use raw mode
        cfmakeraw(&options);
        options.c_cflag |= CLOCAL;
        //enable/disable RTS/CTS
        if (UART_PORT_FLOWCTRL) {
            options.c_cflag |= CRTSCTS;
        } else {
            options.c_cflag &= ~CRTSCTS;
        }

        cfsetospeed(&options, UART_PORT_BAUDRATE);
        cfsetispeed(&options, UART_PORT_BAUDRATE);

        if (tcsetattr(m_uart_fd, TCSANOW, &options) < 0) {
            LOG_ERROR("Set port options fail\n");
            break;
        }
        tcflush(m_uart_fd, TCIOFLUSH);
        ret = true;
    } while (0);

    if((false == ret) && m_uart_fd){
        close(m_uart_fd);
        m_uart_fd = -1;
    }
    return ret;
}

void BPIMcuProxy::on_mcu_events(void *arg){
    BPIMcuProxy* m_proxy = (BPIMcuProxy*)arg;
    fd_set fds;
    fd_set readfds;
    FD_ZERO(&fds);
    FD_SET(m_proxy->m_uart_fd, &fds);
    FD_SET(m_proxy->m_pipe_fd[0], &fds);
    char uart_buffer;
    int maxfd = ((m_proxy->m_uart_fd > m_proxy->m_pipe_fd[0]) ? m_proxy->m_uart_fd : m_proxy->m_pipe_fd[0]) + 1;
    while (1) {
        readfds = fds;
        int ret = select(maxfd, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(m_proxy->m_pipe_fd[0], &readfds)){
            if(1 == read(m_proxy->m_pipe_fd[0], &uart_buffer, 1)){
                if (uart_buffer == 'Z') {
                    break;
                }
            }
        }
        if (ret && FD_ISSET(m_proxy->m_uart_fd, &readfds)) {
            if(1 != read(m_proxy->m_uart_fd, &uart_buffer, 1)){
                LOG_ERROR("uart_read fail\n");
            }else{
                bool notify = true;
                unique_lock<mutex> lk(m_proxy->m_mutex_cv);
                m_proxy->m_cur_data1 = 0;
                switch(uart_buffer){
                    case MCU_EVENT_BATTERY_EMPTY:
                    case MCU_EVENT_BATTERY_LOW:
                    case MCU_EVENT_BATTERY_NORMAL:
                    {
                        char battery_quantity;
                        if(1 == read(m_proxy->m_uart_fd, &battery_quantity, 1)){
                            m_proxy->m_cur_data1 = battery_quantity;
                        }
                        else{
                            LOG_ERROR("read battery quantity failed\n");
                            notify = false;
                        }
                    }
                    case MCU_EVENT_PIR_ON:
                    case MCU_EVENT_PIR_OFF:
                    case MCU_EVENT_BATTERY_CHARGE_OFF:
                    case MCU_EVENT_BATTERY_CHARGE_ON:
                    case MCU_EVENT_BATTERY_CHARGE_DONE:
                    case MCU_EVENT_DC_ON:
                    case MCU_EVENT_DC_OFF:
                        notify = false;
                    case MCU_EVENT_PWR:
                        if (m_proxy->m_observer != nullptr){
                            m_proxy->m_observer->update((MCU_EVENT_TYPE)uart_buffer, m_proxy->m_cur_data1);
                        }
                        break;
                    case MCU_EVENT_TIME_COST:
                    {
                        if(4 != read(m_proxy->m_uart_fd, &m_proxy->m_cur_data1, 4)){
                            LOG_ERROR("read time cost from mcu failed\n");
                            notify = false;
                        }
                        break;
                    }
                    case MCU_EVENT_RACE_TRIGGER_EVENT:
                    {
                        char tmp_buf = 0;
                        if(1 != read(m_proxy->m_uart_fd, &tmp_buf, 1)){
                            LOG_ERROR("read race trigger event from mcu failed\n");
                            notify = false;
                        }else{
                            m_proxy->m_cur_data1 = (MCU_TRIGGER_TYPE)tmp_buf;
                        }
                        break;
                    }
                    case MCU_TRIGGER_PIR_ON:
                    case MCU_TRIGGER_WIFI_WAKEUP:
                    case MCU_TRIGGER_WIFI_RECONNECT:
                    case MCU_TRIGGER_PWR:
                    case MCU_TRIGGER_PWR_2S:
                    case MCU_TRIGGER_POWER_EMPTY:
                        break;
                    case MCU_EVENT_VERSION:
                    {
                        char tmp_buf[2];
                        if(2 != read(m_proxy->m_uart_fd, tmp_buf, 2)){
                            LOG_ERROR("get mcu version from mcu failed\n");
                            notify = false;
                        }
                        m_proxy->m_cur_data1 = (tmp_buf[0]<<8)|(tmp_buf[1]);
                        break;
                    }
                    default:
                        LOG_ERROR("unknown mcu event(%d)\n", uart_buffer);
                        notify = false;
                        break;
                }
                if(notify){
                    m_proxy->m_cur_event = (MCU_EVENT_TYPE)uart_buffer;
                    m_proxy->m_cv.notify_all();
                }
            }
        }else{
            LOG_ERROR("uart to mcu failed!\n");
        }
    }
    LOG_PRINT("mcu proxy thread exited\n");
}
