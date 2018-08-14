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
#ifndef __NEEZE_H__
#define __NEEZE_H__

#include <easy_setup.h>

#define NEEZE_KEY_STRING_LEN (16)
#define NEEZE_NONCE_PAD_LEN (13)

typedef struct {
    uint8 key_bytes[NEEZE_KEY_STRING_LEN];  /* key string for decoding */
    uint8 random_bytes[NEEZE_NONCE_PAD_LEN]; /* random bytes */
    uint8 key_bytes_qqcon[NEEZE_KEY_STRING_LEN];  /* key string for decoding for qqcon */
    uint8 random_bytes_qqcon[NEEZE_NONCE_PAD_LEN]; /* random bytes for qqcon */
} neeze_param_t;

typedef struct {
    easy_setup_result_t es_result;
    ip_address_t host_ip_address;      /* setup client's ip address */
    uint16 host_port;            /* setup client's port */
} neeze_result_t;

void neeze_get_param(void* p);
void neeze_set_result(const void* p);

int neeze_set_key(const char* key);
int neeze_set_key_qqcon(const char* key);
int neeze_get_sender_ip(char buff[], int buff_len);
int neeze_get_sender_port(uint16* port);

#endif /* __NEEZE_H__ */
