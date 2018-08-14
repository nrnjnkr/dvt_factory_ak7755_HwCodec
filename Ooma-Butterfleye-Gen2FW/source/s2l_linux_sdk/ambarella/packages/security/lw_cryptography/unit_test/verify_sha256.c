/*******************************************************************************
 * verify_sha256.c
 *
 * History:
 *  2017/03/02 - [Zhi He] create file
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
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cryptography_if.h"

static const unsigned char gs_sha256_input_vector[128] =
{
    0x12, 0x56, 0x34, 0x78, 0xef, 0x9a, 0xcd, 0x0b,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
    0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11,
    0x7a, 0x6c, 0xf2, 0xe5, 0x87, 0x03, 0xd2, 0xca,
    0x42, 0x56, 0x34, 0x68, 0xef, 0x9a, 0xcd, 0xdb,
    0x50, 0x32, 0x54, 0xa6, 0x98, 0xba, 0xdc, 0xde,
    0x1a, 0xbb, 0xcc, 0xed, 0xee, 0xff, 0x00, 0x21,
    0x8a, 0x6c, 0xf2, 0xe5, 0x87, 0x03, 0xd2, 0xc5,
    0x12, 0x56, 0x34, 0x78, 0xef, 0x9a, 0xcd, 0x0b,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
    0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11,
    0x7a, 0x6c, 0xf2, 0xe5, 0x87, 0x03, 0xd2, 0xca,
    0x42, 0x56, 0x34, 0x68, 0xef, 0x9a, 0xcd, 0xdb,
    0x50, 0x32, 0x54, 0xa6, 0x98, 0xba, 0xdc, 0xde,
    0x1a, 0xbb, 0xcc, 0xed, 0xee, 0xff, 0x00, 0x21,
    0x8a, 0x6c, 0xf2, 0xe5, 0x87, 0x03, 0xd2, 0xc5,
};

static const unsigned char gs_sha256_output_vector_64[32] =
{
    0x78, 0x4b, 0xe8, 0x88, 0x8c, 0xfe, 0x9d, 0xa9,
    0x0b, 0x5f, 0x7c, 0xb7, 0xa7, 0x57, 0x7d, 0xf4,
    0x15, 0x09, 0xaf, 0x2d, 0xd0, 0x83, 0x6a, 0xdb,
    0x6a, 0xb2, 0x65, 0x6c, 0xdc, 0x13, 0x8d, 0x58,
};

static const unsigned char gs_sha256_output_vector_128[32] =
{
    0x66, 0xe4, 0xd8, 0x44, 0xa7, 0xe3, 0x01, 0x6e,
    0xd1, 0xe5, 0x13, 0x76, 0xeb, 0x6d, 0x44, 0x7c,
    0x1b, 0x61, 0x24, 0x1b, 0x3f, 0xae, 0xb2, 0xdb,
    0x3b, 0x27, 0xfb, 0xc8, 0x03, 0x35, 0x9d, 0xee,
};

static void __print_mem(const unsigned char * p, unsigned int len)
{
    while (len > 15) {
        DCRYPT_LOG("%02x %02x %02x %02x ", p[0], p[1], p[2], p[3]);
        DCRYPT_LOG("%02x %02x %02x %02x\n", p[4], p[5], p[6], p[7]);
        DCRYPT_LOG("%02x %02x %02x %02x ", p[8], p[9], p[10], p[11]);
        DCRYPT_LOG("%02x %02x %02x %02x\n", p[12], p[13], p[14], p[15]);
        p += 16;
        len -= 16;
    }
    if (len > 0) {
        while (len > 0) {
            DCRYPT_LOG("%02x ", p[0]);
            p ++;
            len --;
        }
        DCRYPT_LOG("\n");
    }
}

int main()
{
    unsigned char output[32] = {0};

    printf("verify sha256 ...\n");

    digest_sha256(gs_sha256_input_vector, 64, output);

    if (memcmp(output, gs_sha256_output_vector_64, 32)) {
        printf("fail, expect\n");
        __print_mem(gs_sha256_output_vector_64, 32);
        printf("but\n");
        __print_mem((const unsigned char *) output, 32);
    } else {
        printf("pass\n");
    }

    digest_sha256(gs_sha256_input_vector, 128, output);

    if (memcmp(output, gs_sha256_output_vector_128, 32)) {
        printf("fail, expect\n");
        __print_mem(gs_sha256_output_vector_128, 32);
        printf("but\n");
        __print_mem((const unsigned char *) output, 32);
    } else {
        printf("pass\n");
    }

    return 0;
}

