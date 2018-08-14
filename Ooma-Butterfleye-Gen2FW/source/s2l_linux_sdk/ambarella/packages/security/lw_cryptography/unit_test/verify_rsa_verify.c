/*******************************************************************************
 * verify_rsa_verify.c
 *
 * History:
 *  2017/03/03 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "cryptography_if.h"

int main()
{
    int ret = 0;
    rsa_context_t rsa;

    unsigned char digest[32] = {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
    };

    unsigned char signature[128] = {
        0xab, 0x5f, 0x36, 0x98, 0x16, 0x45, 0xa0, 0x1c,
        0x0e, 0xe0, 0xae, 0x74, 0x70, 0x2e, 0xd5, 0x1c,
        0x63, 0x4a, 0xc4, 0x0a, 0x5b, 0x56, 0xe5, 0x15,
        0xf8, 0x22, 0x1f, 0x91, 0x8e, 0x97, 0xb3, 0xfe,
        0x34, 0xe9, 0x76, 0x33, 0xc2, 0xdf, 0x7d, 0x01,
        0x62, 0x74, 0x4d, 0xf0, 0x6b, 0x8d, 0x84, 0x14,
        0xf7, 0xb5, 0x5d, 0x6a, 0x5c, 0xac, 0xc2, 0x9d,
        0x8d, 0x34, 0xa2, 0x75, 0x49, 0x0d, 0xdd, 0x40,
        0xad, 0x0c, 0x15, 0x9d, 0x61, 0xa2, 0xa2, 0x39,
        0x34, 0x1e, 0xe7, 0xad, 0xfb, 0xea, 0x6c, 0x97,
        0x58, 0xf9, 0xf9, 0xdc, 0xcb, 0x3b, 0x3b, 0x8b,
        0x83, 0xba, 0xcd, 0xd5, 0x54, 0xc1, 0x43, 0xa3,
        0x6f, 0x26, 0x7c, 0xe6, 0x68, 0xb7, 0x87, 0x4d,
        0x1c, 0xd4, 0xad, 0xdf, 0xe4, 0x32, 0x41, 0xdc,
        0xec, 0x41, 0x48, 0xaf, 0x92, 0x96, 0xde, 0x33,
        0x82, 0xa7, 0xcd, 0xa3, 0x28, 0x68, 0xad, 0x3e,
    };

    char rsa_pub_key_n[] = "B317C5F0841504CBD415DA47CFD0138EC1160270E64FB0374E8E39E0E054232FE04A90EB02B03661BBD99CA4314AC69407FFB0DE520D033F63151842C42BCB8DC48144C509C16C348D533F4E65786F4D81F53A7887640ABDF5A9D46307DE77CA8360C83FE200195FDCE38B2736CE884DC567C6A152C0F64359D1854F0D5ED5C9";
    char rsa_pub_key_e[] = "010001";

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    if (256 < strlen(rsa_pub_key_n)) {
        printf("bad N length %d\n", (unsigned int) strlen(rsa_pub_key_n));
        goto exit;
    }

    ret = big_number_read_string(&rsa.N, 16, rsa_pub_key_n);
    if (0 > ret) {
        printf("read N fail\n");
        goto exit;
    }

    ret = big_number_read_string(&rsa.E, 16, rsa_pub_key_e);
    if (0 > ret) {
        printf("read E fail\n");
        goto exit;
    }

    rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;
    if (128 != rsa.len){
        printf("only support 1024 bit rsa, len %d\n", rsa.len);
        goto exit;
    }

    if ((ret = rsa_sha256_verify(&rsa, digest, signature)) != 0) {
        printf( "rsa_sha256_verify fail, retruen %d\n", ret);
        goto exit;
    }

    printf("verify OK\n");

exit:

    rsa_free(&rsa);

    return ret;
}

