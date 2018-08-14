/*
 * gpio.c
 *
 * History:
 *       2015/03/16 - [Chu Chen] created file
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
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <basetypes.h>

#include "bpi_utils.h"

#define MAX_LENGTH_GPIO_PATH 128

#define GPIO_PORT_SHUTDOWN 112
#define GPIO_PORT_BOOTMOOD_0 92//PC1
#define GPIO_PORT_BOOTMOOD_1 91//PB7
#define GPIO_PORT_BOOTMOOD_2 38//PB8

#define GPIO_SET 49
#define GPIO_CLEAR 48

static int gpio_write(int gpio, u8 value)
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

static int gpio_read(int gpio, u8* value)
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
//////////////////////////////////////////////////////////////////////
void shut_down()
{
    sync();
    usleep(500000);
    while (1) {
        gpio_write(GPIO_PORT_SHUTDOWN, (u8)'1');
        usleep(50000);
    }
    return;
}

void reset_wifi_chip()
{
    gpio_write(GPIO_PORT_SHUTDOWN, (u8)'1');
    usleep(100000);
    gpio_write(GPIO_PORT_SHUTDOWN, (u8)'0');
    return;
}

int check_boot_mode()
{
    //TODO
    u8 value = 0;
    int mode = 0;

    if (gpio_read(GPIO_PORT_BOOTMOOD_0, &value) != 0) {
        LOG_ERROR("gpio_read gpio 92 fail\n");
        return mode;
    }

    if (value == GPIO_SET) {
        mode |= 1;
    }

    if (gpio_read(GPIO_PORT_BOOTMOOD_1, &value) != 0) {
        LOG_ERROR("gpio_read gpio 91 fail\n");
        return mode;
    }

    if (value == GPIO_SET) {
        mode |= 1 << 1;
    }

    if (gpio_read(GPIO_PORT_BOOTMOOD_2, &value) != 0) {
        LOG_ERROR("gpio_read gpio 38 fail\n");
        return mode;
    }

    if (value == GPIO_SET) {
        mode |= 1 << 2;
    }

    return mode;
}
