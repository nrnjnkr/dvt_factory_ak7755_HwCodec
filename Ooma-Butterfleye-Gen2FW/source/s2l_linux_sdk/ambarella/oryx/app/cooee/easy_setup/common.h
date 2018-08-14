 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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
#ifndef __EASY_SETUP_COMMON_H__
#define __EASY_SETUP_COMMON_H__

#include <stdio.h>

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int

#define int8 char
#define int16 short
#define int32 int

/* direct log message to your system utils, default printf */
extern int debug_enable;
#define LOGE(fmt, arg...) do {if (1 | debug_enable) printf(fmt, ##arg);} while (0);
#define LOGD(fmt, arg...) do {if (debug_enable) printf(fmt, ##arg);} while (0);

typedef struct {
    uint8 type;
    uint8 length;
    uint8 value[0];
} tlv_t;

/**
 * Structure for storing a Service Set Identifier (i.e. Name of Access Point)
 */
typedef struct {
    uint8 len;     /**< SSID length */
    uint8 val[32]; /**< SSID name (AP name)  */
} ssid_t;

/**
 * Structure for storing a MAC address (Wi-Fi Media Access Control address).
 */
typedef struct {
    uint8 octet[6]; /**< Unique 6-byte MAC address */
} mac_t;

/**
 * Structure for storing a IP address.
 */
typedef struct {
    uint8 version;   /**< IPv4 or IPv6 type */

    union {
        uint32 v4;   /**< IPv4 IP address */
        uint32 v6[4];/**< IPv6 IP address */
    } ip;
} ip_address_t;

#endif /* __EASY_SETUP_COMMON_H__ */
