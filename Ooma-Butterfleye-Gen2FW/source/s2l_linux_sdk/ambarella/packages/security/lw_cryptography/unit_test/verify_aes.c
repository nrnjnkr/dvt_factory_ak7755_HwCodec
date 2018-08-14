/*******************************************************************************
 * verify_aes.c
 *
 * History:
 *  2017/02/28 - [Zhi He] create file
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cryptography_if.h"

//AES test vectors from: http://csrc.nist.gov/archive/aes/rijndael/rijndael-vals.zip

//ECB
static const unsigned char gs_aes128_dec_test_ecb_vector[16] =
{
    0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
    0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0
};

static const unsigned char gs_aes192_dec_test_ecb_vector[16] =
{
    0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
    0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4
};

static const unsigned char gs_aes256_dec_test_ecb_vector[16] =
{
    0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
    0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE
};

static const unsigned char gs_aes128_enc_test_ecb_vector[16] =
{
    0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
    0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F
};

static const unsigned char gs_aes192_enc_test_ecb_vector[16] =
{
    0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
    0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14
};

static const unsigned char gs_aes256_enc_test_ecb_vector[16] =
{
    0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
    0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4
};

//CBC

static const unsigned char gs_aes128_dec_test_cbc_vector[16] =
{
    0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73,
    0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86
};

static const unsigned char gs_aes192_dec_test_cbc_vector[16] =
{
    0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75,
    0xB6, 0x17, 0x68, 0xC6, 0xAD, 0xEF, 0x7C, 0x7B
};

static const unsigned char gs_aes256_dec_test_cbc_vector[16] =
{
    0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75,
    0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13
};

static const unsigned char gs_aes128_enc_test_cbc_vector[16] =
{
    0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84,
    0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D
};

static const unsigned char gs_aes192_enc_test_cbc_vector[16] =
{
    0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB,
    0x85, 0xD2, 0xAD, 0xFA, 0xE8, 0x7B, 0xB1, 0x04
};

static const unsigned char gs_aes256_enc_test_cbc_vector[16] =
{
    0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5,
    0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0
};

static void __print_mem(unsigned char * p, unsigned int len)
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

//ECB

static int __verify_aes_ecb_decrypt()
{
    int ret = 0;
    unsigned int i = 0;
    aes_context_t *thiz = NULL;
    unsigned char key[32] = {0};
    unsigned char buf[64] = {0};

    printf("verify aes128 ecb decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_decrypt_key(thiz, key, 128);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_decrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes128_dec_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes192 ecb decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_decrypt_key(thiz, key, 192);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_decrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes192_dec_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes256 ecb decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_decrypt_key(thiz, key, 256);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_decrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes256_dec_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    return 0;
}

static int __verify_aes_ecb_encrypt()
{
    int ret = 0;
    unsigned int i = 0;
    aes_context_t *thiz = NULL;
    unsigned char key[32] = {0};
    unsigned char buf[64] = {0};

    printf("verify aes128 ecb encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_encrypt_key(thiz, key, 128);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_encrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes128_enc_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes192 ecb encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_encrypt_key(thiz, key, 192);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_encrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes192_enc_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes256 ecb encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    ret = aes_set_encrypt_key(thiz, key, 256);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_ecb_encrypt(thiz, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes256_enc_test_ecb_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    return 0;
}

//CBC

static int __verify_aes_cbc_decrypt()
{
    int ret = 0;
    unsigned int i = 0;
    aes_context_t *thiz = NULL;
    unsigned char key[32] = {0};
    unsigned char buf[64] = {0};
    unsigned char iv[16] = {0};

    printf("verify aes128 cbc decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    ret = aes_set_decrypt_key(thiz, key, 128);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_decrypt(thiz, iv, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes128_dec_test_cbc_vector, 16)) {
        printf("  fail\n");
    } else {
        printf("  pass\n");
    }

    printf("verify aes192 cbc decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    ret = aes_set_decrypt_key(thiz, key, 192);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_decrypt(thiz, iv, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes192_dec_test_cbc_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes256 cbc decryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    ret = aes_set_decrypt_key(thiz, key, 256);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_decrypt(thiz, iv, buf, buf, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(buf, gs_aes256_dec_test_cbc_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    return 0;
}

static int __verify_aes_cbc_encrypt()
{
    int ret = 0;
    unsigned int i = 0;
    aes_context_t *thiz = NULL;
    unsigned char key[32] = {0};
    unsigned char buf[64] = {0};
    unsigned char iv[16] = {0};
    unsigned char tmp[16] = {0};
    unsigned char prv[16] = {0};

    printf("verify aes128 cbc encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    memset(prv, 0x0, sizeof(prv));
    ret = aes_set_encrypt_key(thiz, key, 128);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_encrypt(thiz, iv, buf, buf, 16);
        memcpy(tmp, prv, 16);
        memcpy(prv, buf, 16);
        memcpy(buf, tmp, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(prv, gs_aes128_enc_test_cbc_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes192 cbc encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    memset(prv, 0x0, sizeof(prv));
    ret = aes_set_encrypt_key(thiz, key, 192);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_encrypt(thiz, iv, buf, buf, 16);
        memcpy(tmp, prv, 16);
        memcpy(prv, buf, 16);
        memcpy(buf, tmp, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(prv, gs_aes192_enc_test_cbc_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    printf("verify aes256 cbc encryption ...\n");

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    memset(buf, 0x0, sizeof(buf));
    memset(key, 0x0, sizeof(key));
    memset(iv, 0x0, sizeof(iv));
    memset(prv, 0x0, sizeof(prv));
    ret = aes_set_encrypt_key(thiz, key, 256);
    if (ret) {
        return (-2);
    }

    for (i = 0; i < 10000; i ++) {
        aes_cbc_encrypt(thiz, iv, buf, buf, 16);
        memcpy(tmp, prv, 16);
        memcpy(prv, buf, 16);
        memcpy(buf, tmp, 16);
    }

    aes_destroy(thiz);
    thiz = NULL;

    if (memcmp(prv, gs_aes256_enc_test_cbc_vector, 16)) {
        printf("  fail\n");
        __print_mem(buf, 16);
    } else {
        printf("  pass\n");
    }

    return 0;
}


int main()
{
    __verify_aes_ecb_decrypt();
    __verify_aes_ecb_encrypt();
    __verify_aes_cbc_decrypt();
    __verify_aes_cbc_encrypt();
    return 0;
}

