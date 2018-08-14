/*******************************************************************************
 * ecc_genkey.c
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

#if defined (_WIN32) || defined(_WIN64)
#include <time.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "cryptography_if.h"

static int __random_generator(void* context, unsigned char* output, unsigned int output_len)
{
    static unsigned char seed4 = 29, seed3 = 43, seed2 = 17, seed1 = 97;
    //unsigned char digest[32] = {0};
    //unsigned int i = 0 , len0 = 0;

#if defined (_WIN32) || defined(_WIN64)
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    memcpy(output, &wtm, sizeof(wtm));
#else
    struct timeval tt;
    gettimeofday(&tt, NULL);
    memcpy(output, &tt, sizeof(tt));
#endif

#if 0
    if (32 > output_len) {
        len0 = output_len;
    } else {
        len0 = 32;
    }
#endif

    output[0] = output[1] ^ seed1;
    output[1] = output[2] ^ seed2;
    output[2] = output[3] ^ seed3;
    output[3] = output[0] ^ seed4;

    pseudo_random_scamble_sequence(output, output_len);
#if 0
    digest_sha256(output, output_len, digest);
    for (i = 0; i < len0; i ++) {
        output[i] = output[i] ^ digest[i];
    }

    pseudo_random_scamble_sequence(output, output_len);
    digest_sha256(output, output_len, digest);
    for (i = 0; i < len0; i ++) {
        output[i] = output[i] ^ digest[i];
    }
#endif
    seed1 += 163;
    seed2 += 59;
    seed3 += 101;
    seed4 += 23;

    return 0;
}

int main()
{
    int ret;
    ecc_keypair_t key_pair;
    FILE *fpub  = NULL;
    FILE *fpriv = NULL;

    ret = ecc_gen_key(CRYPTO_ECC_DP_SECP192R1, &key_pair, __random_generator, NULL);

    if ((fpub = fopen( "ecc_pub.txt", "wb+")) == NULL) {
        printf("cannot open ecc_pub.txt\n");
        ret = 1;
        goto exit;
    }

    if ((ret = big_number_write_file("X = ", &key_pair.Q.X, 16, fpub)) != 0 ||
        (ret = big_number_write_file("Y = ", &key_pair.Q.Y, 16, fpub)) != 0 ||
        (ret = big_number_write_file("Z = ", &key_pair.Q.Z, 16, fpub)) != 0) {
        printf("big_number_write_file fail, ret %d\n", ret);
        goto exit;
    }

    if ((fpriv = fopen("ecc_priv.txt", "wb+")) == NULL) {
        printf("cannot open ecc_priv.txt\n");
        ret = 1;
        goto exit;
    }

    if ((ret = big_number_write_file("X = ", &key_pair.Q.X, 16, fpriv)) != 0 ||
        (ret = big_number_write_file("Y = ", &key_pair.Q.Y, 16, fpriv)) != 0 ||
        (ret = big_number_write_file("Z = ", &key_pair.Q.Z, 16, fpriv)) != 0 ||
        (ret = big_number_write_file("d = ", &key_pair.d, 16, fpriv)) != 0) {
        printf("big_number_write_file fail, ret %d\n", ret);
        goto exit;
    }

exit:

    if (fpub != NULL)
        fclose(fpub);

    if (fpriv != NULL)
        fclose(fpriv);

    big_number_free(&key_pair.Q);
    big_number_free(&key_pair.d);

#if defined(_WIN32)
    printf("press any key to exit\n" );
    fflush(stdout); getchar();
#endif

    return ret;
}

