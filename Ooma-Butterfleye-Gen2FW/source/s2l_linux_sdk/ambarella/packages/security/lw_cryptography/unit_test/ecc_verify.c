/*******************************************************************************
 * ecc_verify.c
 *
 * History:
 *  2017/03/30 - [Zhi He] create file
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
    FILE *f = NULL;
    int ret = 0;
    unsigned int i;
    ecdsa_context_t ecdsa;
    unsigned char fake_digest[32] = {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
    };
    unsigned char signature[512];
    unsigned int signature_length = 0;

    ecdsa_init(&ecdsa);

    if ((f = fopen( "ecc_pub.txt", "rb")) == NULL) {
        printf( "cannot open ecc_pub.txt\n");
        goto exit;
    }

    if ((ret = big_number_read_file(&ecdsa.Q.X, 16, f)) != 0 ||
        (ret = big_number_read_file(&ecdsa.Q.Y, 16, f)) != 0 ||
        (ret = big_number_read_file(&ecdsa.Q.Z, 16, f)) != 0) {
        printf("big_number_read_file fail, return %d\n", ret);
        goto exit;
    }

    fclose(f);
    f = NULL;

    ret = ecc_setup_group(CRYPTO_ECC_DP_SECP192R1, &ecdsa.group);
    if (ret) {
        printf("ecc curve not supported, return %d\n", ret);
        goto exit;
    }

    if ((f = fopen("ecc.sign", "rb")) == NULL) {
        printf("cannot open ecc.sign\n");
        goto exit;
    }

    fseek(f, 0, SEEK_END);
    signature_length = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (signature_length > sizeof(signature)) {
        printf("input file too large, %d\n", signature_length);
        goto exit;
    }

    ret = fread(signature, 1, signature_length, f);
    fclose(f);
    f = NULL;

    ret = ecdsa_read_asn1_signature(&ecdsa, fake_digest, sizeof(fake_digest), signature, signature_length);

    if (ret) {
        printf( "ecdsa_read_asn1_signature fail, retruen %d\n", ret);
        goto exit;
    }

    printf("verify OK\n");

exit:

    if (f != NULL) {
        fclose(f);
    }

    ecdsa_free(&ecdsa);

#if defined(_WIN32)
    printf("press any key to exit.\n" );
    fflush(stdout); getchar();
#endif

    return ret;
}

