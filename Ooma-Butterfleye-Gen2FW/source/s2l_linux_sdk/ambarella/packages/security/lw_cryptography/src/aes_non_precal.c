/*******************************************************************************
 * aes_non_precal.c
 *
 * History:
 *  2017/03/21 - [Zhi He] create file
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

#include "cryptography_if.h"

#ifndef DNOT_INCLUDE_C_HEADER
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#else
#include <bldfunc.h>
#endif

#ifndef DCONFIG_AES_USE_PRECALCULATED_TABLE

#define DGF28Time(x) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00))
#define DGF28Multiple(x, y) \
    (((y & 1) * x) ^ \
    ((y>>1 & 1) * DGF28Time(x)) ^ \
    ((y>>2 & 1) * DGF28Time(DGF28Time(x))) ^ \
    ((y>>3 & 1) * DGF28Time(DGF28Time(DGF28Time(x)))) ^ \
    ((y>>4 & 1) * DGF28Time(DGF28Time(DGF28Time(DGF28Time(x)))))) \



static const unsigned char gcsForwardSBox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};


static const unsigned char gcsReverseSBox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
};


static const unsigned char gcsRoundConstant[10] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
};

static int __aes_set_key(unsigned int *p_rk, const unsigned char *key, unsigned int key_length)
{
    unsigned int i;
    unsigned char v0, v1, v2, v3;
    unsigned int v;
    unsigned int *p_32bit_key = (unsigned int *) key;
    unsigned int *p_ori_rk = p_rk;

    if (128 == key_length) {

        p_rk[0] = p_32bit_key[0];
        p_rk[1] = p_32bit_key[1];
        p_rk[2] = p_32bit_key[2];
        p_rk[3] = p_32bit_key[3];

        for (i = 0; i < 10; i ++, p_rk += 4) {
            //rotate
            v0 = (p_rk[3] >> 8) & 0xff;
            v1 = (p_rk[3] >> 16) & 0xff;
            v2 = (p_rk[3] >> 24) & 0xff;
            v3 = (p_rk[3]) & 0xff;

            //replace
            v0 = gcsForwardSBox[v0];
            v1 = gcsForwardSBox[v1];
            v2 = gcsForwardSBox[v2];
            v3 = gcsForwardSBox[v3];

            //xor
            v0 = v0 ^ gcsRoundConstant[i];
            v = ((unsigned int) v0) | (((unsigned int) v1) << 8) | (((unsigned int) v2) << 16) | (((unsigned int) v3) << 24);

            p_rk[4] = p_rk[0] ^ v;

            p_rk[5] = p_rk[1] ^ p_rk[4];
            p_rk[6] = p_rk[2] ^ p_rk[5];
            p_rk[7] = p_rk[3] ^ p_rk[6];
        }

    } else if (192 == key_length) {

        p_rk[0] = p_32bit_key[0];
        p_rk[1] = p_32bit_key[1];
        p_rk[2] = p_32bit_key[2];
        p_rk[3] = p_32bit_key[3];
        p_rk[4] = p_32bit_key[4];
        p_rk[5] = p_32bit_key[5];

        for (i = 0; i < 8; i ++, p_rk += 6) {
            //rotate
            v0 = (p_rk[5] >> 8) & 0xff;
            v1 = (p_rk[5] >> 16) & 0xff;
            v2 = (p_rk[5] >> 24) & 0xff;
            v3 = (p_rk[5]) & 0xff;

            //replace
            v0 = gcsForwardSBox[v0];
            v1 = gcsForwardSBox[v1];
            v2 = gcsForwardSBox[v2];
            v3 = gcsForwardSBox[v3];

            //xor
            v0 = v0 ^ gcsRoundConstant[i];
            v = ((unsigned int) v0) | (((unsigned int) v1) << 8) | (((unsigned int) v2) << 16) | (((unsigned int) v3) << 24);

            p_rk[6] = p_rk[0] ^ v;

            p_rk[7] = p_rk[1] ^ p_rk[6];
            p_rk[8] = p_rk[2] ^ p_rk[7];
            p_rk[9] = p_rk[3] ^ p_rk[8];
            p_rk[10] = p_rk[4] ^ p_rk[9];
            p_rk[11] = p_rk[5] ^ p_rk[10];
        }

    } else if (256 == key_length) {

        p_rk[0] = p_32bit_key[0];
        p_rk[1] = p_32bit_key[1];
        p_rk[2] = p_32bit_key[2];
        p_rk[3] = p_32bit_key[3];
        p_rk[4] = p_32bit_key[4];
        p_rk[5] = p_32bit_key[5];
        p_rk[6] = p_32bit_key[6];
        p_rk[7] = p_32bit_key[7];

        for (i = 0; i < 7; i ++, p_rk += 8) {
            //rotate
            v0 = (p_rk[7] >> 8) & 0xff;
            v1 = (p_rk[7] >> 16) & 0xff;
            v2 = (p_rk[7] >> 24) & 0xff;
            v3 = (p_rk[7]) & 0xff;

            //replace
            v0 = gcsForwardSBox[v0];
            v1 = gcsForwardSBox[v1];
            v2 = gcsForwardSBox[v2];
            v3 = gcsForwardSBox[v3];

            //xor
            v0 = v0 ^ gcsRoundConstant[i];
            v = ((unsigned int) v0) | (((unsigned int) v1) << 8) | (((unsigned int) v2) << 16) | (((unsigned int) v3) << 24);

            p_rk[8] = p_rk[0] ^ v;

            p_rk[9] = p_rk[1] ^ p_rk[8];
            p_rk[10] = p_rk[2] ^ p_rk[9];
            p_rk[11] = p_rk[3] ^ p_rk[10];

            //no rotate here
            v0 = (p_rk[11]) & 0xff;
            v1 = (p_rk[11] >> 8) & 0xff;
            v2 = (p_rk[11] >> 16) & 0xff;
            v3 = (p_rk[11] >> 24) & 0xff;

            //replace
            v0 = gcsForwardSBox[v0];
            v1 = gcsForwardSBox[v1];
            v2 = gcsForwardSBox[v2];
            v3 = gcsForwardSBox[v3];

            //no xor
            v = ((unsigned int) v0) | (((unsigned int) v1) << 8) | (((unsigned int) v2) << 16) | (((unsigned int) v3) << 24);

            p_rk[12] = p_rk[4] ^ v;

            p_rk[13] = p_rk[5] ^ p_rk[12];
            p_rk[14] = p_rk[6] ^ p_rk[13];
            p_rk[15] = p_rk[7] ^ p_rk[14];
        }

    }

    return 0;
}

static void __add_round_key(unsigned int *data, unsigned int *p_rk)
{
    data[0] ^= p_rk[0];
    data[1] ^= p_rk[1];
    data[2] ^= p_rk[2];
    data[3] ^= p_rk[3];
}

static void __substitute_bytes_and_shift_rows(unsigned char *in, unsigned char *out)
{
    out[0] = gcsForwardSBox[in[0]];
    out[1] = gcsForwardSBox[in[5]];
    out[2] = gcsForwardSBox[in[10]];
    out[3] = gcsForwardSBox[in[15]];

    out[4] = gcsForwardSBox[in[4]];
    out[5] = gcsForwardSBox[in[9]];
    out[6] = gcsForwardSBox[in[14]];
    out[7] = gcsForwardSBox[in[3]];

    out[8] = gcsForwardSBox[in[8]];
    out[9] = gcsForwardSBox[in[13]];
    out[10] = gcsForwardSBox[in[2]];
    out[11] = gcsForwardSBox[in[7]];

    out[12] = gcsForwardSBox[in[12]];
    out[13] = gcsForwardSBox[in[1]];
    out[14] = gcsForwardSBox[in[6]];
    out[15] = gcsForwardSBox[in[11]];
}

static void __mix_columns(unsigned char *in, unsigned char *out)
{
    unsigned int i;
    unsigned char v1, v2, v3;

    for (i = 0; i < 4; i ++, in += 4, out += 4) {
        v1 = in[0];
        v3 = in[0] ^ in[1] ^ in[2] ^ in[3];

        v2  = in[0] ^ in[1];
        v2 = DGF28Time(v2);
        out[0] = in[0] ^ v2 ^ v3;

        v2  = in[1] ^ in[2];
        v2 = DGF28Time(v2);
        out[1] = in[1] ^ v2 ^ v3;

        v2  = in[2] ^ in[3];
        v2 = DGF28Time(v2);
        out[2] = in[2] ^ v2 ^ v3;

        v2  = in[3] ^ v1;
        v2 = DGF28Time(v2);
        out[3] = in[3] ^ v2 ^ v3;
    }
}

static void __inverse_substitute_bytes_and_shift_rows(unsigned char *in, unsigned char *out)
{
    out[0] = gcsReverseSBox[in[0]];
    out[1] = gcsReverseSBox[in[13]];
    out[2] = gcsReverseSBox[in[10]];
    out[3] = gcsReverseSBox[in[7]];

    out[4] = gcsReverseSBox[in[4]];
    out[5] = gcsReverseSBox[in[1]];
    out[6] = gcsReverseSBox[in[14]];
    out[7] = gcsReverseSBox[in[11]];

    out[8] = gcsReverseSBox[in[8]];
    out[9] = gcsReverseSBox[in[5]];
    out[10] = gcsReverseSBox[in[2]];
    out[11] = gcsReverseSBox[in[15]];

    out[12] = gcsReverseSBox[in[12]];
    out[13] = gcsReverseSBox[in[9]];
    out[14] = gcsReverseSBox[in[6]];
    out[15] = gcsReverseSBox[in[3]];
}

static void __inverse_mix_columns(unsigned char *in, unsigned char *out)
{
    unsigned int i;
    unsigned char v1, v2, v3, v4;

    for(i = 0; i < 4; i ++, in += 4, out += 4) {
        v1 = in[0];
        v2 = in[1];
        v3 = in[2];
        v4 = in[3];

        out[0] = DGF28Multiple(v1, 0x0e) ^ DGF28Multiple(v2, 0x0b) ^ DGF28Multiple(v3, 0x0d) ^ DGF28Multiple(v4, 0x09);
        out[1] = DGF28Multiple(v1, 0x09) ^ DGF28Multiple(v2, 0x0e) ^ DGF28Multiple(v3, 0x0b) ^ DGF28Multiple(v4, 0x0d);
        out[2] = DGF28Multiple(v1, 0x0d) ^ DGF28Multiple(v2, 0x09) ^ DGF28Multiple(v3, 0x0e) ^ DGF28Multiple(v4, 0x0b);
        out[3] = DGF28Multiple(v1, 0x0b) ^ DGF28Multiple(v2, 0x0d) ^ DGF28Multiple(v3, 0x09) ^ DGF28Multiple(v4, 0x0e);
    }
}

static void __aes_encrypt_block(aes_context_t *ctx, unsigned char *p_input, unsigned char *output)
{
    unsigned int round = 0;
    unsigned char rev_input[16];
    unsigned char *input;

    memcpy(rev_input, p_input, 16);
    input = (unsigned char *) rev_input;

    __add_round_key((unsigned int *) input, (unsigned int *) ctx->p_round_key);

    for (round = 1; round < ctx->round; round ++) {
        __substitute_bytes_and_shift_rows(input, output);
        __mix_columns(output, input);
        __add_round_key((unsigned int *) input, (unsigned int *) ctx->p_round_key + (round << 2));
    }

    __substitute_bytes_and_shift_rows(input, output);
    __add_round_key((unsigned int *) output, (unsigned int *) ctx->p_round_key + (ctx->round << 2));
}

static void __aes_decrypt_block(aes_context_t *ctx, const unsigned char *p_input, unsigned char *output)
{
    unsigned int round = 0;
    unsigned char rev_input[16];
    unsigned char *input;

    memcpy(rev_input, p_input, 16);
    input = (unsigned char *) rev_input;

    __add_round_key((unsigned int *) input, (unsigned int *) ctx->p_round_key + (ctx->round << 2));

    for (round = ctx->round - 1; round > 0; round --) {
        __inverse_substitute_bytes_and_shift_rows(input, output);
        __add_round_key((unsigned int *) output, (unsigned int *) ctx->p_round_key + (round << 2));
        __inverse_mix_columns(output, input);
    }

    __inverse_substitute_bytes_and_shift_rows(input, output);
    __add_round_key((unsigned int *) output, (unsigned int *) ctx->p_round_key);
}

aes_context_t *aes_init()
{
    aes_context_t *thiz = (aes_context_t *) malloc(sizeof(aes_context_t));
    if (thiz) {
        memset(thiz, 0x0, sizeof(aes_context_t));
    } else {
        DCRYPT_LOG("error: no memory\n");
    }
    return thiz;
}

void aes_destroy(aes_context_t *ctx)
{
    if (ctx) {
        free(ctx);
    }
}

int aes_set_encrypt_key(aes_context_t *ctx, const unsigned char *key, unsigned int key_length)
{
    if ((!ctx) || (!key)) {
        DCRYPT_LOG("error: null pointer\n");
        return (-1);
    }

    if (128 == key_length) {
        ctx->round = 10;
    } else if (192 == key_length) {
        ctx->round = 12;
    } else if (256 == key_length) {
        ctx->round = 14;
    } else {
        DCRYPT_LOG("error: enc invalid key_length %d\n", key_length);
        return (-2);
    }

    return __aes_set_key(ctx->p_round_key, key, key_length);
}

int aes_set_decrypt_key(aes_context_t *ctx, const unsigned char *key, unsigned int key_length)
{
    if ((!ctx) || (!key)) {
        DCRYPT_LOG("error: null pointer\n");
        return (-1);
    }

    if (128 == key_length) {
        ctx->round = 10;
    } else if (192 == key_length) {
        ctx->round = 12;
    } else if (256 == key_length) {
        ctx->round = 14;
    } else {
        DCRYPT_LOG("error: enc invalid key_length %d\n", key_length);
        return (-2);
    }

    return __aes_set_key(ctx->p_round_key, key, key_length);
}

int aes_ecb_encrypt(aes_context_t *ctx, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    if (data_length & 0xf) {
        DCRYPT_LOG("error: invalid data_length, should be multiple of 16\n");
        return (-2);
    }

    while (data_length > 0) {
        __aes_encrypt_block(ctx, input, output);
        input  += 16;
        output += 16;
        data_length -= 16;
    }

    return 0;
}

int aes_ecb_decrypt(aes_context_t *ctx, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    if (data_length & 0xf) {
        DCRYPT_LOG("error: invalid data_length, should be multiple of 16\n");
        return (-2);
    }

    while (data_length > 0) {
        __aes_decrypt_block(ctx, input, output);
        input  += 16;
        output += 16;
        data_length -= 16;
    }

    return 0;
}

int aes_ctr_crypt(aes_context_t *ctx, unsigned char *p_nonce, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    unsigned int i;
    unsigned int cur = ctx->cur_position;
    unsigned char block[16] = {0};

    while (data_length--) {
        if (!cur) {
            __aes_encrypt_block(ctx, input, block);
            for (i = 16; i > 0; i--) {
                if (++ p_nonce[i - 1] != 0) {
                    break;
                }
            }
        }
        *output++ = ((*input++) ^ block[cur]);
        cur = (cur + 1) & 0x0F;
    }
    ctx->cur_position = cur;

    return 0;
}

int aes_cbc_encrypt(aes_context_t *ctx, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    unsigned int i;

    if (data_length & 0xf) {
        DCRYPT_LOG("error: invalid data_length, should be multiple of 16\n");
        return (-2);
    }

    while (data_length > 0) {
        for (i = 0; i < 16; i++) {
            output[i] = input[i] ^ iv[i];
        }

        __aes_encrypt_block(ctx, output, output);
        memcpy(iv, output, 16);

        input  += 16;
        output += 16;
        data_length -= 16;
    }

    return 0;
}

int aes_cbc_decrypt(aes_context_t *ctx, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    unsigned int i;
    unsigned char temp[16] = {0};

    if (data_length & 0xf) {
        DCRYPT_LOG("error: invalid data_length, should be multiple of 16\n");
        return (-2);
    }

    while (data_length > 0) {
        memcpy(temp, input, 16);
        __aes_decrypt_block(ctx, input, output);

        for (i = 0; i < 16; i++) {
            output[i] = output[i] ^ iv[i];
        }

        memcpy(iv, temp, 16);

        input  += 16;
        output += 16;
        data_length -= 16;
    }

    return 0;
}


//one shot api

int aes_ecb_encrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    int ret = 0;
    aes_context_t *thiz = NULL;

    if (!key || !input || !output || !data_length) {
        DCRYPT_LOG("error: invalid parameters\n");
        return (-1);
    }

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    ret = aes_set_encrypt_key(thiz, key, key_length);
    if (ret) {
        return (-2);
    }

    aes_ecb_encrypt(thiz, input, output, data_length);

    aes_destroy(thiz);
    thiz = NULL;

    return 0;
}

int aes_ecb_decrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    int ret = 0;
    aes_context_t *thiz = NULL;

    if (!key || !input || !output || !data_length) {
        DCRYPT_LOG("error: invalid parameters\n");
        return (-1);
    }

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    ret = aes_set_decrypt_key(thiz, key, key_length);
    if (ret) {
        return (-2);
    }

    aes_ecb_decrypt(thiz, input, output, data_length);

    aes_destroy(thiz);
    thiz = NULL;

    return 0;
}

int aes_ctr_crypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *p_nonce, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    int ret = 0;
    aes_context_t *thiz = NULL;

    if (!key || !p_nonce || !input || !output || !data_length) {
        DCRYPT_LOG("error: invalid parameters\n");
        return (-1);
    }

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    ret = aes_set_encrypt_key(thiz, key, key_length);
    if (ret) {
        return (-2);
    }

    aes_ctr_crypt(thiz, p_nonce, input, output, data_length);

    aes_destroy(thiz);
    thiz = NULL;

    return 0;
}


int aes_cbc_encrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    int ret = 0;
    aes_context_t *thiz = NULL;

    if (!key || !iv || !input || !output || !data_length) {
        DCRYPT_LOG("error: invalid parameters\n");
        return (-1);
    }

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    ret = aes_set_encrypt_key(thiz, key, key_length);
    if (ret) {
        return ret;
    }

    ret = aes_cbc_encrypt(thiz, iv, input, output, data_length);
    if (ret) {
        return ret;
    }

    aes_destroy(thiz);
    thiz = NULL;

    return 0;
}

int aes_cbc_decrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length)
{
    int ret = 0;
    aes_context_t *thiz = NULL;

    if (!key || !iv || !input || !output || !data_length) {
        DCRYPT_LOG("error: invalid parameters\n");
        return (-1);
    }

    thiz = aes_init();
    if (!thiz) {
        return (-1);
    }

    ret = aes_set_decrypt_key(thiz, key, key_length);
    if (ret) {
        return ret;
    }

    ret = aes_cbc_decrypt(thiz, iv, input, output, data_length);
    if (ret) {
        return ret;
    }

    aes_destroy(thiz);
    thiz = NULL;

    return 0;
}

#endif

