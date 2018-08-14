/*******************************************************************************
 * pk_ecc.c
 *
 * History:
 *  2017/03/27 - [Zhi He] create file
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
#include <stdlib.h>
#include <stdio.h>
#else
#include <bldfunc.h>
#endif

#include "big_number.h"

#define DECC_ENABLE_SECP192R1
#define DECC_ENABLE_SECP224R1
#define DECC_ENABLE_SECP256R1
#define DECC_ENABLE_SECP384R1
#define DECC_ENABLE_SECP521R1
#define DECC_ENABLE_SECP192K1
#define DECC_ENABLE_SECP224K1
#define DECC_ENABLE_SECP256K1
#define DECC_ENABLE_BP256R1
#define DECC_ENABLE_BP384R1
#define DECC_ENABLE_BP512R1

#define DMAX_ECC_BITS 521
#define DMAX_ECC_BYTES  ((DMAX_ECC_BITS + 7) >> 3)
#define DMAX_ECC_PAYLOAD_LENGTH   (2 * DMAX_ECC_BYTES + 1)

#define DECC_WINDOW_SIZE 6

#ifdef DECC_ENABLE_SECP192R1
static const unsigned char gc_secp192r1_p[] __attribute__((aligned(8))) = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const unsigned char gc_secp192r1_b[] __attribute__((aligned(8))) = {
    0xB1, 0xB9, 0x46, 0xC1, 0xEC, 0xDE, 0xB8, 0xFE,
    0x49, 0x30, 0x24, 0x72, 0xAB, 0xE9, 0xA7, 0x0F,
    0xE7, 0x80, 0x9C, 0xE5, 0x19, 0x05, 0x21, 0x64,
};

static const unsigned char gc_secp192r1_gx[] __attribute__((aligned(8))) = {
    0x12, 0x10, 0xFF, 0x82, 0xFD, 0x0A, 0xFF, 0xF4,
    0x00, 0x88, 0xA1, 0x43, 0xEB, 0x20, 0xBF, 0x7C,
    0xF6, 0x90, 0x30, 0xB0, 0x0E, 0xA8, 0x8D, 0x18,
};

static const unsigned char gc_secp192r1_gy[] __attribute__((aligned(8))) = {
    0x11, 0x48, 0x79, 0x1E, 0xA1, 0x77, 0xF9, 0x73,
    0xD5, 0xCD, 0x24, 0x6B, 0xED, 0x11, 0x10, 0x63,
    0x78, 0xDA, 0xC8, 0xFF, 0x95, 0x2B, 0x19, 0x07,
};

static const unsigned char gc_secp192r1_n[] __attribute__((aligned(8))) = {
    0x31, 0x28, 0xD2, 0xB4, 0xB1, 0xC9, 0x6B, 0x14,
    0x36, 0xF8, 0xDE, 0x99, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif


#ifdef DECC_ENABLE_SECP224R1
static const unsigned char gc_secp224r1_p[] __attribute__((aligned(8))) = {
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224r1_b[] __attribute__((aligned(8))) = {
    0xB4, 0xFF, 0x55, 0x23, 0x43, 0x39, 0x0B, 0x27,
    0xBA, 0xD8, 0xBF, 0xD7, 0xB7, 0xB0, 0x44, 0x50,
    0x56, 0x32, 0x41, 0xF5, 0xAB, 0xB3, 0x04, 0x0C,
    0x85, 0x0A, 0x05, 0xB4, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224r1_gx[] __attribute__((aligned(8))) = {
    0x21, 0x1D, 0x5C, 0x11, 0xD6, 0x80, 0x32, 0x34,
    0x22, 0x11, 0xC2, 0x56, 0xD3, 0xC1, 0x03, 0x4A,
    0xB9, 0x90, 0x13, 0x32, 0x7F, 0xBF, 0xB4, 0x6B,
    0xBD, 0x0C, 0x0E, 0xB7, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224r1_gy[] __attribute__((aligned(8))) = {
    0x34, 0x7E, 0x00, 0x85, 0x99, 0x81, 0xD5, 0x44,
    0x64, 0x47, 0x07, 0x5A, 0xA0, 0x75, 0x43, 0xCD,
    0xE6, 0xDF, 0x22, 0x4C, 0xFB, 0x23, 0xF7, 0xB5,
    0x88, 0x63, 0x37, 0xBD, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224r1_n[] __attribute__((aligned(8))) = {
    0x3D, 0x2A, 0x5C, 0x5C, 0x45, 0x29, 0xDD, 0x13,
    0x3E, 0xF0, 0xB8, 0xE0, 0xA2, 0x16, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
};
#endif


#ifdef DECC_ENABLE_SECP256R1
static const unsigned char gc_secp256r1_p[] __attribute__((aligned(8))) = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const unsigned char gc_secp256r1_b[] __attribute__((aligned(8))) = {
    0x4B, 0x60, 0xD2, 0x27, 0x3E, 0x3C, 0xCE, 0x3B,
    0xF6, 0xB0, 0x53, 0xCC, 0xB0, 0x06, 0x1D, 0x65,
    0xBC, 0x86, 0x98, 0x76, 0x55, 0xBD, 0xEB, 0xB3,
    0xE7, 0x93, 0x3A, 0xAA, 0xD8, 0x35, 0xC6, 0x5A,
};

static const unsigned char gc_secp256r1_gx[] __attribute__((aligned(8))) = {
    0x96, 0xC2, 0x98, 0xD8, 0x45, 0x39, 0xA1, 0xF4,
    0xA0, 0x33, 0xEB, 0x2D, 0x81, 0x7D, 0x03, 0x77,
    0xF2, 0x40, 0xA4, 0x63, 0xE5, 0xE6, 0xBC, 0xF8,
    0x47, 0x42, 0x2C, 0xE1, 0xF2, 0xD1, 0x17, 0x6B,
};

static const unsigned char gc_secp256r1_gy[] __attribute__((aligned(8))) = {
    0xF5, 0x51, 0xBF, 0x37, 0x68, 0x40, 0xB6, 0xCB,
    0xCE, 0x5E, 0x31, 0x6B, 0x57, 0x33, 0xCE, 0x2B,
    0x16, 0x9E, 0x0F, 0x7C, 0x4A, 0xEB, 0xE7, 0x8E,
    0x9B, 0x7F, 0x1A, 0xFE, 0xE2, 0x42, 0xE3, 0x4F,
};

static const unsigned char gc_secp256r1_n[] __attribute__((aligned(8))) = {
    0x51, 0x25, 0x63, 0xFC, 0xC2, 0xCA, 0xB9, 0xF3,
    0x84, 0x9E, 0x17, 0xA7, 0xAD, 0xFA, 0xE6, 0xBC,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif


#ifdef DECC_ENABLE_SECP384R1
static const unsigned char gc_secp384r1_p[] __attribute__((aligned(8))) = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const unsigned char gc_secp384r1_b[] __attribute__((aligned(8))) = {
    0xEF, 0x2A, 0xEC, 0xD3, 0xED, 0xC8, 0x85, 0x2A,
    0x9D, 0xD1, 0x2E, 0x8A, 0x8D, 0x39, 0x56, 0xC6,
    0x5A, 0x87, 0x13, 0x50, 0x8F, 0x08, 0x14, 0x03,
    0x12, 0x41, 0x81, 0xFE, 0x6E, 0x9C, 0x1D, 0x18,
    0x19, 0x2D, 0xF8, 0xE3, 0x6B, 0x05, 0x8E, 0x98,
    0xE4, 0xE7, 0x3E, 0xE2, 0xA7, 0x2F, 0x31, 0xB3,
};

static const unsigned char gc_secp384r1_gx[] __attribute__((aligned(8))) = {
    0xB7, 0x0A, 0x76, 0x72, 0x38, 0x5E, 0x54, 0x3A,
    0x6C, 0x29, 0x55, 0xBF, 0x5D, 0xF2, 0x02, 0x55,
    0x38, 0x2A, 0x54, 0x82, 0xE0, 0x41, 0xF7, 0x59,
    0x98, 0x9B, 0xA7, 0x8B, 0x62, 0x3B, 0x1D, 0x6E,
    0x74, 0xAD, 0x20, 0xF3, 0x1E, 0xC7, 0xB1, 0x8E,
    0x37, 0x05, 0x8B, 0xBE, 0x22, 0xCA, 0x87, 0xAA,
};

static const unsigned char gc_secp384r1_gy[] __attribute__((aligned(8))) = {
    0x5F, 0x0E, 0xEA, 0x90, 0x7C, 0x1D, 0x43, 0x7A,
    0x9D, 0x81, 0x7E, 0x1D, 0xCE, 0xB1, 0x60, 0x0A,
    0xC0, 0xB8, 0xF0, 0xB5, 0x13, 0x31, 0xDA, 0xE9,
    0x7C, 0x14, 0x9A, 0x28, 0xBD, 0x1D, 0xF4, 0xF8,
    0x29, 0xDC, 0x92, 0x92, 0xBF, 0x98, 0x9E, 0x5D,
    0x6F, 0x2C, 0x26, 0x96, 0x4A, 0xDE, 0x17, 0x36,
};

static const unsigned char gc_secp384r1_n[] __attribute__((aligned(8))) = {
    0x73, 0x29, 0xC5, 0xCC, 0x6A, 0x19, 0xEC, 0xEC,
    0x7A, 0xA7, 0xB0, 0x48, 0xB2, 0x0D, 0x1A, 0x58,
    0xDF, 0x2D, 0x37, 0xF4, 0x81, 0x4D, 0x63, 0xC7,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif


#ifdef DECC_ENABLE_SECP521R1
static const unsigned char gc_secp521r1_p[] __attribute__((aligned(8))) = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp521r1_b[] __attribute__((aligned(8))) = {
    0x00, 0x3F, 0x50, 0x6B, 0xD4, 0x1F, 0x45, 0xEF,
    0xF1, 0x34, 0x2C, 0x3D, 0x88, 0xDF, 0x73, 0x35,
    0x07, 0xBF, 0xB1, 0x3B, 0xBD, 0xC0, 0x52, 0x16,
    0x7B, 0x93, 0x7E, 0xEC, 0x51, 0x39, 0x19, 0x56,
    0xE1, 0x09, 0xF1, 0x8E, 0x91, 0x89, 0xB4, 0xB8,
    0xF3, 0x15, 0xB3, 0x99, 0x5B, 0x72, 0xDA, 0xA2,
    0xEE, 0x40, 0x85, 0xB6, 0xA0, 0x21, 0x9A, 0x92,
    0x1F, 0x9A, 0x1C, 0x8E, 0x61, 0xB9, 0x3E, 0x95,
    0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp521r1_gx[] __attribute__((aligned(8))) = {
    0x66, 0xBD, 0xE5, 0xC2, 0x31, 0x7E, 0x7E, 0xF9,
    0x9B, 0x42, 0x6A, 0x85, 0xC1, 0xB3, 0x48, 0x33,
    0xDE, 0xA8, 0xFF, 0xA2, 0x27, 0xC1, 0x1D, 0xFE,
    0x28, 0x59, 0xE7, 0xEF, 0x77, 0x5E, 0x4B, 0xA1,
    0xBA, 0x3D, 0x4D, 0x6B, 0x60, 0xAF, 0x28, 0xF8,
    0x21, 0xB5, 0x3F, 0x05, 0x39, 0x81, 0x64, 0x9C,
    0x42, 0xB4, 0x95, 0x23, 0x66, 0xCB, 0x3E, 0x9E,
    0xCD, 0xE9, 0x04, 0x04, 0xB7, 0x06, 0x8E, 0x85,
    0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp521r1_gy[] __attribute__((aligned(8))) = {
    0x50, 0x66, 0xD1, 0x9F, 0x76, 0x94, 0xBE, 0x88,
    0x40, 0xC2, 0x72, 0xA2, 0x86, 0x70, 0x3C, 0x35,
    0x61, 0x07, 0xAD, 0x3F, 0x01, 0xB9, 0x50, 0xC5,
    0x40, 0x26, 0xF4, 0x5E, 0x99, 0x72, 0xEE, 0x97,
    0x2C, 0x66, 0x3E, 0x27, 0x17, 0xBD, 0xAF, 0x17,
    0x68, 0x44, 0x9B, 0x57, 0x49, 0x44, 0xF5, 0x98,
    0xD9, 0x1B, 0x7D, 0x2C, 0xB4, 0x5F, 0x8A, 0x5C,
    0x04, 0xC0, 0x3B, 0x9A, 0x78, 0x6A, 0x29, 0x39,
    0x18, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp521r1_n[] __attribute__((aligned(8))) = {
    0x09, 0x64, 0x38, 0x91, 0x1E, 0xB7, 0x6F, 0xBB,
    0xAE, 0x47, 0x9C, 0x89, 0xB8, 0xC9, 0xB5, 0x3B,
    0xD0, 0xA5, 0x09, 0xF7, 0x48, 0x01, 0xCC, 0x7F,
    0x6B, 0x96, 0x2F, 0xBF, 0x83, 0x87, 0x86, 0x51,
    0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


#ifdef DECC_ENABLE_SECP192K1
static const unsigned char gc_secp192k1_p[] __attribute__((aligned(8))) = {
    0x37, 0xEE, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const unsigned char gc_secp192k1_a[] __attribute__((aligned(8))) = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp192k1_b[] __attribute__((aligned(8))) = {
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp192k1_gx[] __attribute__((aligned(8))) = {
    0x7D, 0x6C, 0xE0, 0xEA, 0xB1, 0xD1, 0xA5, 0x1D,
    0x34, 0xF4, 0xB7, 0x80, 0x02, 0x7D, 0xB0, 0x26,
    0xAE, 0xE9, 0x57, 0xC0, 0x0E, 0xF1, 0x4F, 0xDB,
};

static const unsigned char gc_secp192k1_gy[] __attribute__((aligned(8))) = {
    0x9D, 0x2F, 0x5E, 0xD9, 0x88, 0xAA, 0x82, 0x40,
    0x34, 0x86, 0xBE, 0x15, 0xD0, 0x63, 0x41, 0x84,
    0xA7, 0x28, 0x56, 0x9C, 0x6D, 0x2F, 0x2F, 0x9B,
};

static const unsigned char gc_secp192k1_n[] __attribute__((aligned(8))) = {
    0x8D, 0xFD, 0xDE, 0x74, 0x6A, 0x46, 0x69, 0x0F,
    0x17, 0xFC, 0xF2, 0x26, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif


#ifdef DECC_ENABLE_SECP224K1
static const unsigned char gc_secp224k1_p[] __attribute__((aligned(8))) = {
    0x6D, 0xE5, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224k1_a[] __attribute__((aligned(8))) = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224k1_b[] __attribute__((aligned(8))) = {
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224k1_gx[] __attribute__((aligned(8))) = {
    0x5C, 0xA4, 0xB7, 0xB6, 0x0E, 0x65, 0x7E, 0x0F,
    0xA9, 0x75, 0x70, 0xE4, 0xE9, 0x67, 0xA4, 0x69,
    0xA1, 0x28, 0xFC, 0x30, 0xDF, 0x99, 0xF0, 0x4D,
    0x33, 0x5B, 0x45, 0xA1, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224k1_gy[] __attribute__((aligned(8))) = {
    0xA5, 0x61, 0x6D, 0x55, 0xDB, 0x4B, 0xCA, 0xE2,
    0x59, 0xBD, 0xB0, 0xC0, 0xF7, 0x19, 0xE3, 0xF7,
    0xD6, 0xFB, 0xCA, 0x82, 0x42, 0x34, 0xBA, 0x7F,
    0xED, 0x9F, 0x08, 0x7E, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp224k1_n[] __attribute__((aligned(8))) = {
    0xF7, 0xB1, 0x9F, 0x76, 0x71, 0xA9, 0xF0, 0xCA,
    0x84, 0x61, 0xEC, 0xD2, 0xE8, 0xDC, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
};
#endif


#ifdef DECC_ENABLE_SECP256K1
static const unsigned char gc_secp256k1_p[] __attribute__((aligned(8))) = {
    0x2F, 0xFC, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const unsigned char gc_secp256k1_a[] __attribute__((aligned(8))) = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp256k1_b[] __attribute__((aligned(8))) = {
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char gc_secp256k1_gx[] __attribute__((aligned(8))) = {
    0x98, 0x17, 0xF8, 0x16, 0x5B, 0x81, 0xF2, 0x59,
    0xD9, 0x28, 0xCE, 0x2D, 0xDB, 0xFC, 0x9B, 0x02,
    0x07, 0x0B, 0x87, 0xCE, 0x95, 0x62, 0xA0, 0x55,
    0xAC, 0xBB, 0xDC, 0xF9, 0x7E, 0x66, 0xBE, 0x79,
};

static const unsigned char gc_secp256k1_gy[] __attribute__((aligned(8))) = {
    0xB8, 0xD4, 0x10, 0xFB, 0x8F, 0xD0, 0x47, 0x9C,
    0x19, 0x54, 0x85, 0xA6, 0x48, 0xB4, 0x17, 0xFD,
    0xA8, 0x08, 0x11, 0x0E, 0xFC, 0xFB, 0xA4, 0x5D,
    0x65, 0xC4, 0xA3, 0x26, 0x77, 0xDA, 0x3A, 0x48,
};

static const unsigned char gc_secp256k1_n[] __attribute__((aligned(8))) = {
    0x41, 0x41, 0x36, 0xD0, 0x8C, 0x5E, 0xD2, 0xBF,
    0x3B, 0xA0, 0x48, 0xAF, 0xE6, 0xDC, 0xAE, 0xBA,
    0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif


#ifdef DECC_ENABLE_BP256R1
static const unsigned char gc_bp256r1_p[] __attribute__((aligned(8))) = {
    0x77, 0x53, 0x6E, 0x1F, 0x1D, 0x48, 0x13, 0x20,
    0x28, 0x20, 0x26, 0xD5, 0x23, 0xF6, 0x3B, 0x6E,
    0x72, 0x8D, 0x83, 0x9D, 0x90, 0x0A, 0x66, 0x3E,
    0xBC, 0xA9, 0xEE, 0xA1, 0xDB, 0x57, 0xFB, 0xA9,
};

static const unsigned char gc_bp256r1_a[] __attribute__((aligned(8))) = {
    0xD9, 0xB5, 0x30, 0xF3, 0x44, 0x4B, 0x4A, 0xE9,
    0x6C, 0x5C, 0xDC, 0x26, 0xC1, 0x55, 0x80, 0xFB,
    0xE7, 0xFF, 0x7A, 0x41, 0x30, 0x75, 0xF6, 0xEE,
    0x57, 0x30, 0x2C, 0xFC, 0x75, 0x09, 0x5A, 0x7D,
};

static const unsigned char gc_bp256r1_b[] __attribute__((aligned(8))) = {
    0xB6, 0x07, 0x8C, 0xFF, 0x18, 0xDC, 0xCC, 0x6B,
    0xCE, 0xE1, 0xF7, 0x5C, 0x29, 0x16, 0x84, 0x95,
    0xBF, 0x7C, 0xD7, 0xBB, 0xD9, 0xB5, 0x30, 0xF3,
    0x44, 0x4B, 0x4A, 0xE9, 0x6C, 0x5C, 0xDC, 0x26,
};

static const unsigned char gc_bp256r1_gx[] __attribute__((aligned(8))) = {
    0x62, 0x32, 0xCE, 0x9A, 0xBD, 0x53, 0x44, 0x3A,
    0xC2, 0x23, 0xBD, 0xE3, 0xE1, 0x27, 0xDE, 0xB9,
    0xAF, 0xB7, 0x81, 0xFC, 0x2F, 0x48, 0x4B, 0x2C,
    0xCB, 0x57, 0x7E, 0xCB, 0xB9, 0xAE, 0xD2, 0x8B,
};

static const unsigned char gc_bp256r1_gy[] __attribute__((aligned(8))) = {
    0x97, 0x69, 0x04, 0x2F, 0xC7, 0x54, 0x1D, 0x5C,
    0x54, 0x8E, 0xED, 0x2D, 0x13, 0x45, 0x77, 0xC2,
    0xC9, 0x1D, 0x61, 0x14, 0x1A, 0x46, 0xF8, 0x97,
    0xFD, 0xC4, 0xDA, 0xC3, 0x35, 0xF8, 0x7E, 0x54,
};

static const unsigned char gc_bp256r1_n[] __attribute__((aligned(8))) = {
    0xA7, 0x56, 0x48, 0x97, 0x82, 0x0E, 0x1E, 0x90,
    0xF7, 0xA6, 0x61, 0xB5, 0xA3, 0x7A, 0x39, 0x8C,
    0x71, 0x8D, 0x83, 0x9D, 0x90, 0x0A, 0x66, 0x3E,
    0xBC, 0xA9, 0xEE, 0xA1, 0xDB, 0x57, 0xFB, 0xA9,
};
#endif


#ifdef DECC_ENABLE_BP384R1
static const unsigned char gc_bp384r1_p[] __attribute__((aligned(8))) = {
    0x53, 0xEC, 0x07, 0x31, 0x13, 0x00, 0x47, 0x87,
    0x71, 0x1A, 0x1D, 0x90, 0x29, 0xA7, 0xD3, 0xAC,
    0x23, 0x11, 0xB7, 0x7F, 0x19, 0xDA, 0xB1, 0x12,
    0xB4, 0x56, 0x54, 0xED, 0x09, 0x71, 0x2F, 0x15,
    0xDF, 0x41, 0xE6, 0x50, 0x7E, 0x6F, 0x5D, 0x0F,
    0x28, 0x6D, 0x38, 0xA3, 0x82, 0x1E, 0xB9, 0x8C,
};

static const unsigned char gc_bp384r1_a[] __attribute__((aligned(8))) = {
    0x26, 0x28, 0xCE, 0x22, 0xDD, 0xC7, 0xA8, 0x04,
    0xEB, 0xD4, 0x3A, 0x50, 0x4A, 0x81, 0xA5, 0x8A,
    0x0F, 0xF9, 0x91, 0xBA, 0xEF, 0x65, 0x91, 0x13,
    0x87, 0x27, 0xB2, 0x4F, 0x8E, 0xA2, 0xBE, 0xC2,
    0xA0, 0xAF, 0x05, 0xCE, 0x0A, 0x08, 0x72, 0x3C,
    0x0C, 0x15, 0x8C, 0x3D, 0xC6, 0x82, 0xC3, 0x7B,
};

static const unsigned char gc_bp384r1_b[] __attribute__((aligned(8))) = {
    0x11, 0x4C, 0x50, 0xFA, 0x96, 0x86, 0xB7, 0x3A,
    0x94, 0xC9, 0xDB, 0x95, 0x02, 0x39, 0xB4, 0x7C,
    0xD5, 0x62, 0xEB, 0x3E, 0xA5, 0x0E, 0x88, 0x2E,
    0xA6, 0xD2, 0xDC, 0x07, 0xE1, 0x7D, 0xB7, 0x2F,
    0x7C, 0x44, 0xF0, 0x16, 0x54, 0xB5, 0x39, 0x8B,
    0x26, 0x28, 0xCE, 0x22, 0xDD, 0xC7, 0xA8, 0x04,
};

static const unsigned char gc_bp384r1_gx[] __attribute__((aligned(8))) = {
    0x1E, 0xAF, 0xD4, 0x47, 0xE2, 0xB2, 0x87, 0xEF,
    0xAA, 0x46, 0xD6, 0x36, 0x34, 0xE0, 0x26, 0xE8,
    0xE8, 0x10, 0xBD, 0x0C, 0xFE, 0xCA, 0x7F, 0xDB,
    0xE3, 0x4F, 0xF1, 0x7E, 0xE7, 0xA3, 0x47, 0x88,
    0x6B, 0x3F, 0xC1, 0xB7, 0x81, 0x3A, 0xA6, 0xA2,
    0xFF, 0x45, 0xCF, 0x68, 0xF0, 0x64, 0x1C, 0x1D,
};

static const unsigned char gc_bp384r1_gy[] __attribute__((aligned(8))) = {
    0x15, 0x53, 0x3C, 0x26, 0x41, 0x03, 0x82, 0x42,
    0x11, 0x81, 0x91, 0x77, 0x21, 0x46, 0x46, 0x0E,
    0x28, 0x29, 0x91, 0xF9, 0x4F, 0x05, 0x9C, 0xE1,
    0x64, 0x58, 0xEC, 0xFE, 0x29, 0x0B, 0xB7, 0x62,
    0x52, 0xD5, 0xCF, 0x95, 0x8E, 0xEB, 0xB1, 0x5C,
    0xA4, 0xC2, 0xF9, 0x20, 0x75, 0x1D, 0xBE, 0x8A,
};

static const unsigned char gc_bp384r1_n[] __attribute__((aligned(8))) = {
    0x65, 0x65, 0x04, 0xE9, 0x02, 0x32, 0x88, 0x3B,
    0x10, 0xC3, 0x7F, 0x6B, 0xAF, 0xB6, 0x3A, 0xCF,
    0xA7, 0x25, 0x04, 0xAC, 0x6C, 0x6E, 0x16, 0x1F,
    0xB3, 0x56, 0x54, 0xED, 0x09, 0x71, 0x2F, 0x15,
    0xDF, 0x41, 0xE6, 0x50, 0x7E, 0x6F, 0x5D, 0x0F,
    0x28, 0x6D, 0x38, 0xA3, 0x82, 0x1E, 0xB9, 0x8C,
};
#endif


#ifdef DECC_ENABLE_BP512R1
static const unsigned char gc_bp512r1_p[] __attribute__((aligned(8))) = {
    0xF3, 0x48, 0x3A, 0x58, 0x56, 0x60, 0xAA, 0x28,
    0x85, 0xC6, 0x82, 0x2D, 0x2F, 0xFF, 0x81, 0x28,
    0xE6, 0x80, 0xA3, 0xE6, 0x2A, 0xA1, 0xCD, 0xAE,
    0x42, 0x68, 0xC6, 0x9B, 0x00, 0x9B, 0x4D, 0x7D,
    0x71, 0x08, 0x33, 0x70, 0xCA, 0x9C, 0x63, 0xD6,
    0x0E, 0xD2, 0xC9, 0xB3, 0xB3, 0x8D, 0x30, 0xCB,
    0x07, 0xFC, 0xC9, 0x33, 0xAE, 0xE6, 0xD4, 0x3F,
    0x8B, 0xC4, 0xE9, 0xDB, 0xB8, 0x9D, 0xDD, 0xAA,
};

static const unsigned char gc_bp512r1_a[] __attribute__((aligned(8))) = {
    0xCA, 0x94, 0xFC, 0x77, 0x4D, 0xAC, 0xC1, 0xE7,
    0xB9, 0xC7, 0xF2, 0x2B, 0xA7, 0x17, 0x11, 0x7F,
    0xB5, 0xC8, 0x9A, 0x8B, 0xC9, 0xF1, 0x2E, 0x0A,
    0xA1, 0x3A, 0x25, 0xA8, 0x5A, 0x5D, 0xED, 0x2D,
    0xBC, 0x63, 0x98, 0xEA, 0xCA, 0x41, 0x34, 0xA8,
    0x10, 0x16, 0xF9, 0x3D, 0x8D, 0xDD, 0xCB, 0x94,
    0xC5, 0x4C, 0x23, 0xAC, 0x45, 0x71, 0x32, 0xE2,
    0x89, 0x3B, 0x60, 0x8B, 0x31, 0xA3, 0x30, 0x78,
};

static const unsigned char gc_bp512r1_b[] __attribute__((aligned(8))) = {
    0x23, 0xF7, 0x16, 0x80, 0x63, 0xBD, 0x09, 0x28,
    0xDD, 0xE5, 0xBA, 0x5E, 0xB7, 0x50, 0x40, 0x98,
    0x67, 0x3E, 0x08, 0xDC, 0xCA, 0x94, 0xFC, 0x77,
    0x4D, 0xAC, 0xC1, 0xE7, 0xB9, 0xC7, 0xF2, 0x2B,
    0xA7, 0x17, 0x11, 0x7F, 0xB5, 0xC8, 0x9A, 0x8B,
    0xC9, 0xF1, 0x2E, 0x0A, 0xA1, 0x3A, 0x25, 0xA8,
    0x5A, 0x5D, 0xED, 0x2D, 0xBC, 0x63, 0x98, 0xEA,
    0xCA, 0x41, 0x34, 0xA8, 0x10, 0x16, 0xF9, 0x3D,
};

static const unsigned char gc_bp512r1_gx[] __attribute__((aligned(8))) = {
    0x22, 0xF8, 0xB9, 0xBC, 0x09, 0x22, 0x35, 0x8B,
    0x68, 0x5E, 0x6A, 0x40, 0x47, 0x50, 0x6D, 0x7C,
    0x5F, 0x7D, 0xB9, 0x93, 0x7B, 0x68, 0xD1, 0x50,
    0x8D, 0xD4, 0xD0, 0xE2, 0x78, 0x1F, 0x3B, 0xFF,
    0x8E, 0x09, 0xD0, 0xF4, 0xEE, 0x62, 0x3B, 0xB4,
    0xC1, 0x16, 0xD9, 0xB5, 0x70, 0x9F, 0xED, 0x85,
    0x93, 0x6A, 0x4C, 0x9C, 0x2E, 0x32, 0x21, 0x5A,
    0x64, 0xD9, 0x2E, 0xD8, 0xBD, 0xE4, 0xAE, 0x81,
};

static const unsigned char gc_bp512r1_gy[] __attribute__((aligned(8))) = {
    0x92, 0x08, 0xD8, 0x3A, 0x0F, 0x1E, 0xCD, 0x78,
    0x06, 0x54, 0xF0, 0xA8, 0x2F, 0x2B, 0xCA, 0xD1,
    0xAE, 0x63, 0x27, 0x8A, 0xD8, 0x4B, 0xCA, 0x5B,
    0x5E, 0x48, 0x5F, 0x4A, 0x49, 0xDE, 0xDC, 0xB2,
    0x11, 0x81, 0x1F, 0x88, 0x5B, 0xC5, 0x00, 0xA0,
    0x1A, 0x7B, 0xA5, 0x24, 0x00, 0xF7, 0x09, 0xF2,
    0xFD, 0x22, 0x78, 0xCF, 0xA9, 0xBF, 0xEA, 0xC0,
    0xEC, 0x32, 0x63, 0x56, 0x5D, 0x38, 0xDE, 0x7D,
};

static const unsigned char gc_bp512r1_n[] __attribute__((aligned(8))) = {
    0x69, 0x00, 0xA9, 0x9C, 0x82, 0x96, 0x87, 0xB5,
    0xDD, 0xDA, 0x5D, 0x08, 0x81, 0xD3, 0xB1, 0x1D,
    0x47, 0x10, 0xAC, 0x7F, 0x19, 0x61, 0x86, 0x41,
    0x19, 0x26, 0xA9, 0x4C, 0x41, 0x5C, 0x3E, 0x55,
    0x70, 0x08, 0x33, 0x70, 0xCA, 0x9C, 0x63, 0xD6,
    0x0E, 0xD2, 0xC9, 0xB3, 0xB3, 0x8D, 0x30, 0xCB,
    0x07, 0xFC, 0xC9, 0x33, 0xAE, 0xE6, 0xD4, 0x3F,
    0x8B, 0xC4, 0xE9, 0xDB, 0xB8, 0x9D, 0xDD, 0xAA,
};
#endif


static void ecc_point_init(ecc_point_t *point)
{
    big_number_init(&point->X);
    big_number_init(&point->Y);
    big_number_init(&point->Z);
}

static void ecc_group_init(ecc_group_t *group)
{
    memset(group, 0, sizeof(ecc_group_t));
}

static void ecc_keypair_init(ecc_keypair_t *key)
{
    ecc_group_init(&key->group);
    big_number_init(&key->d);
    ecc_point_init(&key->Q);
}

static void ecc_point_free(ecc_point_t *point)
{
    big_number_free(&(point->X));
    big_number_free(&(point->Y));
    big_number_free(&(point->Z));
}

static void ecc_group_free(ecc_group_t *group)
{
    unsigned int i;

    if (group->T) {
        for (i = 0; i < group->T_size; i++) {
            ecc_point_free(&group->T[i]);
        }
        free(group->T);
    }

    memset(group, 0x0, sizeof(ecc_group_t));
}

static void ecc_keypair_free(ecc_keypair_t *key)
{
    ecc_group_free(&key->group);
    big_number_free(&key->d);
    ecc_point_free(&key->Q);
}

static int ecc_copy(ecc_point_t *P, const ecc_point_t *Q)
{
    int ret;

    ret = big_number_copy(&P->X, &Q->X);
    ret = big_number_copy(&P->Y, &Q->Y);
    ret = big_number_copy(&P->Z, &Q->Z);

    return ret;
}

static void ecc_big_number_load(big_number_t *X, const TUINT *p, unsigned int len)
{
    X->s = 1;
    X->n = len / sizeof(TUINT);
    X->p = (TUINT *) p;
}

static void ecc_big_number_set1(big_number_t *X)
{
    static TUINT one[] = {1};
    X->s = 1;
    X->n = 1;
    X->p = one;
}

static void ecc_load_group(ecc_group_t *group,
    const TUINT *p,  unsigned int plen,
    const TUINT *a,  unsigned int alen,
    const TUINT *b,  unsigned int blen,
    const TUINT *gx, unsigned int gxlen,
    const TUINT *gy, unsigned int gylen,
    const TUINT *n,  unsigned int nlen)
{
    ecc_big_number_load(&group->P, p, plen);
    if (a != NULL) {
        ecc_big_number_load(&group->A, a, alen);
    }
    ecc_big_number_load(&group->B, b, blen);
    ecc_big_number_load(&group->N, n, nlen);

    ecc_big_number_load(&group->G.X, gx, gxlen);
    ecc_big_number_load(&group->G.Y, gy, gylen);
    ecc_big_number_set1(&group->G.Z);

    group->pbits = big_number_msb(&group->P);
    group->nbits = big_number_msb(&group->N);
}

int ecc_setup_group(int ecc_dp, ecc_group_t *group)
{
    ecc_group_init(group);
    group->id = ecc_dp;
    switch (ecc_dp) {

        case CRYPTO_ECC_DP_SECP192R1:
#ifdef DECC_ENABLE_SECP192R1
            ecc_load_group(group,
                (const TUINT *) gc_secp192r1_p, (unsigned int) sizeof(gc_secp192r1_p),
                NULL, 0,
                (const TUINT *) gc_secp192r1_b, (unsigned int) sizeof(gc_secp192r1_b),
                (const TUINT *) gc_secp192r1_gx, (unsigned int) sizeof(gc_secp192r1_gx),
                (const TUINT *) gc_secp192r1_gy, (unsigned int) sizeof(gc_secp192r1_gy),
                (const TUINT *) gc_secp192r1_n, (unsigned int) sizeof(gc_secp192r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP224R1:
#ifdef DECC_ENABLE_SECP224R1
            ecc_load_group(group,
                (const TUINT *) gc_secp224r1_p, (unsigned int) sizeof(gc_secp224r1_p),
                NULL, 0,
                (const TUINT *) gc_secp224r1_b, (unsigned int) sizeof(gc_secp224r1_b),
                (const TUINT *) gc_secp224r1_gx, (unsigned int) sizeof(gc_secp224r1_gx),
                (const TUINT *) gc_secp224r1_gy, (unsigned int) sizeof(gc_secp224r1_gy),
                (const TUINT *) gc_secp224r1_n, (unsigned int) sizeof(gc_secp224r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP256R1:
#ifdef DECC_ENABLE_SECP256R1
            ecc_load_group(group,
                (const TUINT *) gc_secp256r1_p, (unsigned int) sizeof(gc_secp256r1_p),
                NULL, 0,
                (const TUINT *) gc_secp256r1_b, (unsigned int) sizeof(gc_secp256r1_b),
                (const TUINT *) gc_secp256r1_gx, (unsigned int) sizeof(gc_secp256r1_gx),
                (const TUINT *) gc_secp256r1_gy, (unsigned int) sizeof(gc_secp256r1_gy),
                (const TUINT *) gc_secp256r1_n, (unsigned int) sizeof(gc_secp256r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP384R1:
#ifdef DECC_ENABLE_SECP384R1
            ecc_load_group(group,
                (const TUINT *) gc_secp384r1_p, (unsigned int) sizeof(gc_secp384r1_p),
                NULL, 0,
                (const TUINT *) gc_secp384r1_b, (unsigned int) sizeof(gc_secp384r1_b),
                (const TUINT *) gc_secp384r1_gx, (unsigned int) sizeof(gc_secp384r1_gx),
                (const TUINT *) gc_secp384r1_gy, (unsigned int) sizeof(gc_secp384r1_gy),
                (const TUINT *) gc_secp384r1_n, (unsigned int) sizeof(gc_secp384r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP521R1:
#ifdef DECC_ENABLE_SECP521R1
            ecc_load_group(group,
                (const TUINT *) gc_secp521r1_p, (unsigned int) sizeof(gc_secp521r1_p),
                NULL, 0,
                (const TUINT *) gc_secp521r1_b, (unsigned int) sizeof(gc_secp521r1_b),
                (const TUINT *) gc_secp521r1_gx, (unsigned int) sizeof(gc_secp521r1_gx),
                (const TUINT *) gc_secp521r1_gy, (unsigned int) sizeof(gc_secp521r1_gy),
                (const TUINT *) gc_secp521r1_n, (unsigned int) sizeof(gc_secp521r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP192K1:
#ifdef DECC_ENABLE_SECP192K1
            ecc_load_group(group,
                (const TUINT *) gc_secp192k1_p, (unsigned int) sizeof(gc_secp192k1_p),
                (const TUINT *) gc_secp192k1_a, (unsigned int) sizeof(gc_secp192k1_a),
                (const TUINT *) gc_secp192k1_b, (unsigned int) sizeof(gc_secp192k1_b),
                (const TUINT *) gc_secp192k1_gx, (unsigned int) sizeof(gc_secp192k1_gx),
                (const TUINT *) gc_secp192k1_gy, (unsigned int) sizeof(gc_secp192k1_gy),
                (const TUINT *) gc_secp192k1_n, (unsigned int) sizeof(gc_secp192k1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP224K1:
#ifdef DECC_ENABLE_SECP224K1
            ecc_load_group(group,
                (const TUINT *) gc_secp224k1_p, (unsigned int) sizeof(gc_secp224k1_p),
                (const TUINT *) gc_secp224k1_a, (unsigned int) sizeof(gc_secp224k1_a),
                (const TUINT *) gc_secp224k1_b, (unsigned int) sizeof(gc_secp224k1_b),
                (const TUINT *) gc_secp224k1_gx, (unsigned int) sizeof(gc_secp224k1_gx),
                (const TUINT *) gc_secp224k1_gy, (unsigned int) sizeof(gc_secp224k1_gy),
                (const TUINT *) gc_secp224k1_n, (unsigned int) sizeof(gc_secp224k1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_SECP256K1:
#ifdef DECC_ENABLE_SECP256K1
            ecc_load_group(group,
                (const TUINT *) gc_secp256k1_p, (unsigned int) sizeof(gc_secp256k1_p),
                (const TUINT *) gc_secp256k1_a, (unsigned int) sizeof(gc_secp256k1_a),
                (const TUINT *) gc_secp256k1_b, (unsigned int) sizeof(gc_secp256k1_b),
                (const TUINT *) gc_secp256k1_gx, (unsigned int) sizeof(gc_secp256k1_gx),
                (const TUINT *) gc_secp256k1_gy, (unsigned int) sizeof(gc_secp256k1_gy),
                (const TUINT *) gc_secp256k1_n, (unsigned int) sizeof(gc_secp256k1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_BP256R1:
#ifdef DECC_ENABLE_BP256R1
            ecc_load_group(group,
                (const TUINT *) gc_bp256r1_p, (unsigned int) sizeof(gc_bp256r1_p),
                (const TUINT *) gc_bp256r1_a, (unsigned int) sizeof(gc_bp256r1_a),
                (const TUINT *) gc_bp256r1_b, (unsigned int) sizeof(gc_bp256r1_b),
                (const TUINT *) gc_bp256r1_gx, (unsigned int) sizeof(gc_bp256r1_gx),
                (const TUINT *) gc_bp256r1_gy, (unsigned int) sizeof(gc_bp256r1_gy),
                (const TUINT *) gc_bp256r1_n, (unsigned int) sizeof(gc_bp256r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_BP384R1:
#ifdef DECC_ENABLE_BP384R1
            ecc_load_group(group,
                (const TUINT *) gc_bp384r1_p, (unsigned int) sizeof(gc_bp384r1_p),
                (const TUINT *) gc_bp384r1_a, (unsigned int) sizeof(gc_bp384r1_a),
                (const TUINT *) gc_bp384r1_b, (unsigned int) sizeof(gc_bp384r1_b),
                (const TUINT *) gc_bp384r1_gx, (unsigned int) sizeof(gc_bp384r1_gx),
                (const TUINT *) gc_bp384r1_gy, (unsigned int) sizeof(gc_bp384r1_gy),
                (const TUINT *) gc_bp384r1_n, (unsigned int) sizeof(gc_bp384r1_n));
            return 0;
#endif
            break;

        case CRYPTO_ECC_DP_BP512R1:
#ifdef DECC_ENABLE_BP512R1
            ecc_load_group(group,
                (const TUINT *) gc_bp512r1_p, (unsigned int) sizeof(gc_bp512r1_p),
                (const TUINT *) gc_bp512r1_a, (unsigned int) sizeof(gc_bp512r1_a),
                (const TUINT *) gc_bp512r1_b, (unsigned int) sizeof(gc_bp512r1_b),
                (const TUINT *) gc_bp512r1_gx, (unsigned int) sizeof(gc_bp512r1_gx),
                (const TUINT *) gc_bp512r1_gy, (unsigned int) sizeof(gc_bp512r1_gy),
                (const TUINT *) gc_bp512r1_n, (unsigned int) sizeof(gc_bp512r1_n));
            return 0;
#endif
            break;

        default:
            break;
    }

    DCRYPT_LOG("not supported ecc %d\n", ecc_dp);
    return (-1);
}

static void ecc_set_zero( ecc_point_t *pt )
{
    big_number_lset(&pt->X, 1);
    big_number_lset(&pt->Y, 1);
    big_number_lset(&pt->Z, 0);
}

static int ecc_is_zero(ecc_point_t *pt)
{
    return (big_number_cmp_int(&pt->Z, 0) == 0);
}


int ecc_point_read_string(ecc_point_t *P, int radix,
    const char *x, const char *y)
{
    int ret;

    D_CLEAN_IF_FAILED(big_number_read_string(&P->X, radix, x));
    D_CLEAN_IF_FAILED(big_number_read_string(&P->Y, radix, y));
    D_CLEAN_IF_FAILED(big_number_lset(&P->Z, 1));

cleanup:
    return ret;
}

int ecc_group_read_string(ecc_group_t *group, int radix,
    const char *p, const char *b,
    const char *gx, const char *gy, const char *n)
{
    int ret;

    D_CLEAN_IF_FAILED(big_number_read_string(&group->P, radix, p));
    D_CLEAN_IF_FAILED(big_number_read_string(&group->B, radix, b));
    D_CLEAN_IF_FAILED(ecc_point_read_string(&group->G, radix, gx, gy));
    D_CLEAN_IF_FAILED(big_number_read_string(&group->N, radix, n));

    group->pbits = big_number_msb(&group->P);
    group->nbits = big_number_msb(&group->N);

cleanup:
    if (ret != 0) {
        ecc_group_free(group);
    }

    return ret;
}

static int ecc_modp(big_number_t *N, const ecc_group_t *group)
{
    int ret;

    if (group->modp == NULL) {
        return big_number_mod_big_number(N, N, &group->P);
    }

    if ((N->s < 0 && big_number_cmp_int( N, 0) != 0) || big_number_msb(N) > 2 * group->pbits) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    D_CLEAN_IF_FAILED(group->modp(N));

    while (N->s < 0 && big_number_cmp_int(N, 0) != 0) {
        D_CLEAN_IF_FAILED(big_number_add_big_number(N, N, &group->P));
    }

    while (big_number_cmp_big_number(N, &group->P) >= 0) {
        D_CLEAN_IF_FAILED(big_number_sub_abs(N, N, &group->P));
    }

cleanup:
    return ret;
}

#define MOD_MUL(N) do { D_CLEAN_IF_FAILED( ecc_modp(&N, group)); } \
    while (0)

#define MOD_SUB(N)                                \
    while (N.s < 0 && big_number_cmp_int(&N, 0) != 0)   \
        D_CLEAN_IF_FAILED(big_number_add_big_number(&N, &N, &group->P))

#define MOD_ADD(N)                                \
    while (big_number_cmp_big_number(&N, &group->P) >= 0)        \
        D_CLEAN_IF_FAILED(big_number_sub_abs(&N, &N, &group->P))

static int ecc_normalize_jac(const ecc_group_t *group, ecc_point_t *pt)
{
    int ret;
    big_number_t Zi, ZZi;

    if (big_number_cmp_int(&pt->Z, 0) == 0) {
        return 0;
    }

    big_number_init(&Zi);
    big_number_init(&ZZi);

    D_CLEAN_IF_FAILED(big_number_inv_mod(&Zi, &pt->Z, &group->P));
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&ZZi, &Zi, &Zi));
    MOD_MUL(ZZi);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->X, &pt->X, &ZZi));
    MOD_MUL(pt->X);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->Y, &pt->Y, &ZZi));
    MOD_MUL(pt->Y);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->Y, &pt->Y, &Zi));
    MOD_MUL(pt->Y);

    D_CLEAN_IF_FAILED(big_number_lset(&pt->Z, 1));

cleanup:

    big_number_free(&Zi);
    big_number_free(&ZZi);

    return ret;
}

static int ecc_normalize_jac_many(const ecc_group_t *group,
    ecc_point_t *T[], unsigned int t_len)
{
    int ret;
    unsigned int i;
    big_number_t *c, u, Zi, ZZi;

    if (t_len < 2) {
        return ecc_normalize_jac(group, *T);
    }

    if ((c = malloc(t_len * sizeof(big_number_t))) == NULL) {
        return CRYPTO_ECODE_ERROR_NO_MEMORY;
    }

    big_number_init(&u);
    big_number_init(&Zi);
    big_number_init(&ZZi);
    for (i = 0; i < t_len; i++) {
        big_number_init(&c[i]);
    }

    D_CLEAN_IF_FAILED(big_number_copy(&c[0], &T[0]->Z));
    for (i = 1; i < t_len; i++) {
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&c[i], &c[i-1], &T[i]->Z));
        MOD_MUL(c[i]);
    }

    D_CLEAN_IF_FAILED(big_number_inv_mod(&u, &c[t_len-1], &group->P));

    for (i = t_len - 1; ; i--) {
        if (i == 0) {
            D_CLEAN_IF_FAILED(big_number_copy(&Zi, &u));
        } else {
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&Zi, &u, &c[i-1]));
            MOD_MUL(Zi);
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&u,  &u, &T[i]->Z));
            MOD_MUL(u);
        }

        D_CLEAN_IF_FAILED(big_number_mul_big_number(&ZZi, &Zi, &Zi));
        MOD_MUL(ZZi);
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&T[i]->X, &T[i]->X, &ZZi));
        MOD_MUL(T[i]->X);
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&T[i]->Y, &T[i]->Y, &ZZi));
        MOD_MUL(T[i]->Y);
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&T[i]->Y, &T[i]->Y, &Zi));
        MOD_MUL(T[i]->Y);

        D_CLEAN_IF_FAILED(big_number_shrink(&T[i]->X, group->P.n));
        D_CLEAN_IF_FAILED(big_number_shrink(&T[i]->Y, group->P.n));
        big_number_free(&T[i]->Z);

        if (i == 0) {
            break;
        }
    }

cleanup:

    big_number_free(&u);
    big_number_free(&Zi);
    big_number_free(&ZZi);
    for (i = 0; i < t_len; i++) {
        big_number_free(&c[i]);
    }
    free(c);

    return ret;
}

static int ecc_safe_invert_jac(const ecc_group_t *group,
    ecc_point_t *Q, unsigned char inv)
{
    int ret;
    unsigned char nonzero;
    big_number_t mQY;

    big_number_init(&mQY);

    D_CLEAN_IF_FAILED(big_number_sub_big_number(&mQY, &group->P, &Q->Y));
    nonzero = big_number_cmp_int(&Q->Y, 0) != 0;
    D_CLEAN_IF_FAILED(big_number_safe_cond_assign(&Q->Y, &mQY, inv & nonzero));

cleanup:
    big_number_free(&mQY);

    return ret;
}

static int ecc_double_jac(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t *P)
{
    int ret;
    big_number_t M, S, T, U;

    big_number_init(&M);
    big_number_init(&S);
    big_number_init(&T);
    big_number_init(&U);

    if (!group->A.p) {
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &P->Z, &P->Z));
        MOD_MUL(S);
        D_CLEAN_IF_FAILED(big_number_add_big_number(&T, &P->X, &S));
        MOD_ADD(T);
        D_CLEAN_IF_FAILED(big_number_sub_big_number(&U, &P->X, &S));
        MOD_SUB(U);
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &T, &U));
        MOD_MUL(S);
        D_CLEAN_IF_FAILED(big_number_mul_int(&M, &S, 3));
        MOD_ADD(M);
    } else {
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &P->X, &P->X));
        MOD_MUL(S);
        D_CLEAN_IF_FAILED(big_number_mul_int(&M, &S, 3));
        MOD_ADD(M);

        if (big_number_cmp_int(&group->A, 0) != 0) {
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &P->Z, &P->Z));
            MOD_MUL(S);
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&T, &S, &S));
            MOD_MUL(T);
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &T, &group->A));
            MOD_MUL(S);
            D_CLEAN_IF_FAILED(big_number_add_big_number(&M, &M, &S));
            MOD_ADD(M);
        }
    }

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T, &P->Y, &P->Y));
    MOD_MUL(T);
    D_CLEAN_IF_FAILED(big_number_shift_l(&T, 1));
    MOD_ADD(T);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &P->X, &T));
    MOD_MUL(S);
    D_CLEAN_IF_FAILED(big_number_shift_l(&S, 1));
    MOD_ADD(S);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&U, &T, &T));
    MOD_MUL(U);
    D_CLEAN_IF_FAILED(big_number_shift_l(&U, 1));
    MOD_ADD(U);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T, &M, &M));
    MOD_MUL(T);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&T, &T, &S));
    MOD_SUB(T);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&T, &T, &S));
    MOD_SUB(T);

    D_CLEAN_IF_FAILED(big_number_sub_big_number(&S, &S, &T));
    MOD_SUB(S);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&S, &S, &M));
    MOD_MUL(S);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&S, &S, &U));
    MOD_SUB(S);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&U, &P->Y, &P->Z));
    MOD_MUL(U);
    D_CLEAN_IF_FAILED(big_number_shift_l(&U, 1));
    MOD_ADD(U);

    D_CLEAN_IF_FAILED(big_number_copy(&R->X, &T));
    D_CLEAN_IF_FAILED(big_number_copy(&R->Y, &S));
    D_CLEAN_IF_FAILED(big_number_copy(&R->Z, &U));

cleanup:
    big_number_free(&M);
    big_number_free(&S);
    big_number_free(&T);
    big_number_free(&U);

    return ret;
}

static int ecc_add_mixed(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t *P, const ecc_point_t *Q)
{
    int ret;
    big_number_t T1, T2, T3, T4, X, Y, Z;

    if (big_number_cmp_int(&P->Z, 0) == 0) {
        return ecc_copy(R, Q);
    }

    if (Q->Z.p != NULL && big_number_cmp_int(&Q->Z, 0) == 0) {
        return ecc_copy(R, P);
    }

    if (Q->Z.p != NULL && big_number_cmp_int(&Q->Z, 1) != 0) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    big_number_init(&T1);
    big_number_init(&T2);
    big_number_init(&T3);
    big_number_init(&T4);
    big_number_init(&X);
    big_number_init(&Y);
    big_number_init(&Z);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T1, &P->Z, &P->Z));
    MOD_MUL(T1);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T2, &T1, &P->Z));
    MOD_MUL(T2);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T1, &T1, &Q->X));
    MOD_MUL(T1);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T2, &T2, &Q->Y));
    MOD_MUL(T2);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&T1, &T1, &P->X));
    MOD_SUB(T1);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&T2, &T2, &P->Y));
    MOD_SUB(T2);

    if (big_number_cmp_int(&T1, 0) == 0) {
        if (big_number_cmp_int(&T2, 0) == 0) {
            ret = ecc_double_jac(group, R, P);
            goto cleanup;
        } else {
            ecc_set_zero(R);
            goto cleanup;
        }
    }

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&Z, &P->Z, &T1));
    MOD_MUL(Z);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T3, &T1, &T1));
    MOD_MUL(T3);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T4, &T3, &T1));
    MOD_MUL(T4);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T3, &T3, &P->X));
    MOD_MUL(T3);
    D_CLEAN_IF_FAILED(big_number_mul_int(&T1, &T3, 2));
    MOD_ADD(T1);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&X, &T2, &T2));
    MOD_MUL(X);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&X, &X, &T1));
    MOD_SUB(X);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&X, &X, &T4));
    MOD_SUB(X);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&T3, &T3, &X));
    MOD_SUB(T3);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T3, &T3, &T2));
    MOD_MUL(T3);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&T4, &T4, &P->Y));
    MOD_MUL(T4);
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&Y, &T3, &T4));
    MOD_SUB(Y);

    D_CLEAN_IF_FAILED(big_number_copy(&R->X, &X));
    D_CLEAN_IF_FAILED(big_number_copy(&R->Y, &Y));
    D_CLEAN_IF_FAILED(big_number_copy(&R->Z, &Z));

cleanup:

    big_number_free(&T1);
    big_number_free(&T2);
    big_number_free(&T3);
    big_number_free(&T4);
    big_number_free(&X);
    big_number_free(&Y);
    big_number_free(&Z);

    return ret;
}

int ecc_add(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t *P, const ecc_point_t *Q)
{
    int ret;

    D_CLEAN_IF_FAILED(ecc_add_mixed(group, R, P, Q));
    D_CLEAN_IF_FAILED(ecc_normalize_jac(group, R));

cleanup:
    return ret;
}

int ecc_sub(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t *P, const ecc_point_t *Q)
{
    int ret;
    ecc_point_t mQ;

    ecc_point_init(&mQ);

    D_CLEAN_IF_FAILED(ecc_copy(&mQ, Q));
    if (big_number_cmp_int(&mQ.Y, 0) != 0) {
        D_CLEAN_IF_FAILED(big_number_sub_big_number(&mQ.Y, &group->P, &mQ.Y));
    }

    D_CLEAN_IF_FAILED(ecc_add_mixed(group, R, P, &mQ));
    D_CLEAN_IF_FAILED(ecc_normalize_jac(group, R));

cleanup:
    ecc_point_free(&mQ);

    return ret;
}

static int ecc_randomize_jac(const ecc_group_t *group, ecc_point_t *pt,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret;
    big_number_t l, ll;
    unsigned int p_size = ( group->pbits + 7 ) / 8;
    int count = 0;

    big_number_init(&l);
    big_number_init(&ll);

    do {
        big_number_fill_random(&l, p_size, f_rng, p_rng);

        while (big_number_cmp_big_number(&l, &group->P ) >= 0) {
            D_CLEAN_IF_FAILED(big_number_shift_r(&l, 1));
        }

        if (count ++ > 12) {
            return CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED;
        }
    } while (big_number_cmp_int(&l, 1) <= 0);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->Z, &pt->Z, &l));
    MOD_MUL(pt->Z);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&ll, &l, &l));
    MOD_MUL(ll);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->X, &pt->X, &ll));
    MOD_MUL(pt->X);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&ll, &ll, &l));
    MOD_MUL(ll);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&pt->Y, &pt->Y, &ll));
    MOD_MUL(pt->Y);

cleanup:
    big_number_free(&l);
    big_number_free(&ll);

    return ret;
}

#define COMB_MAX_D      (DMAX_ECC_BITS + 1) / 2

#define COMB_MAX_PRE    (1 << (DECC_WINDOW_SIZE - 1))

static void ecc_comb_fixed(unsigned char x[], unsigned int d,
    unsigned char w, const big_number_t *m)
{
    unsigned int i, j;
    unsigned char c, cc, adjust;

    memset(x, 0, d + 1);

    for (i = 0; i < d; i++) {
        for (j = 0; j < w; j++) {
            x[i] |= big_number_get_bit(m, i + d * j) << j;
        }
    }

    c = 0;
    for (i = 1; i <= d; i++) {
        cc = x[i] & c;
        x[i] = x[i] ^ c;
        c = cc;

        adjust = 1 - (x[i] & 0x01);
        c |= x[i] & (x[i-1] * adjust);
        x[i] = x[i] ^ (x[i-1] * adjust);
        x[i-1] |= adjust << 7;
    }
}

static int ecc_precompute_comb(const ecc_group_t *group,
    ecc_point_t T[], const ecc_point_t *P,
    unsigned char w, unsigned int d)
{
    int ret;
    unsigned char i, k;
    unsigned int j;
    ecc_point_t *cur, *TT[COMB_MAX_PRE - 1];

    D_CLEAN_IF_FAILED(ecc_copy( &T[0], P));

    k = 0;
    for (i = 1; i < (1U << ( w - 1)); i <<= 1) {
        cur = T + i;
        D_CLEAN_IF_FAILED(ecc_copy(cur, T + ( i >> 1)));
        for (j = 0; j < d; j++) {
            D_CLEAN_IF_FAILED(ecc_double_jac(group, cur, cur));
        }

        TT[k++] = cur;
    }

    D_CLEAN_IF_FAILED(ecc_normalize_jac_many(group, TT, k));

    k = 0;
    for (i = 1; i < ( 1U << ( w - 1 ) ); i <<= 1) {
        j = i;
        while (j--) {
            D_CLEAN_IF_FAILED(ecc_add_mixed(group, &T[i + j], &T[j], &T[i]));
            TT[k++] = &T[i + j];
        }
    }

    D_CLEAN_IF_FAILED(ecc_normalize_jac_many(group, TT, k));

cleanup:
    return ret;
}

static int ecc_select_comb(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t T[], unsigned char t_len,
    unsigned char i)
{
    int ret;
    unsigned char ii, j;

    ii =  (i & 0x7Fu) >> 1;

    for (j = 0; j < t_len; j++) {
        D_CLEAN_IF_FAILED(big_number_safe_cond_assign(&R->X, &T[j].X, j == ii));
        D_CLEAN_IF_FAILED(big_number_safe_cond_assign(&R->Y, &T[j].Y, j == ii));
    }

    D_CLEAN_IF_FAILED(ecc_safe_invert_jac(group, R, i >> 7));

cleanup:
    return ret;
}

static int ecc_mul_comb_core(const ecc_group_t *group, ecc_point_t *R,
    const ecc_point_t T[], unsigned char t_len,
    const unsigned char x[], unsigned int d,
    int (*f_rng)(void *, unsigned char *, unsigned int),
    void *p_rng)
{
    int ret;
    ecc_point_t Txi;
    unsigned int i;

    ecc_point_init(&Txi);

    i = d;
    D_CLEAN_IF_FAILED(ecc_select_comb(group, R, T, t_len, x[i]));
    D_CLEAN_IF_FAILED(big_number_lset(&R->Z, 1));
    if (f_rng != 0) {
        D_CLEAN_IF_FAILED(ecc_randomize_jac(group, R, f_rng, p_rng));
    }

    while (i-- != 0) {
        D_CLEAN_IF_FAILED(ecc_double_jac(group, R, R));
        D_CLEAN_IF_FAILED(ecc_select_comb(group, &Txi, T, t_len, x[i]));
        D_CLEAN_IF_FAILED(ecc_add_mixed(group, R, R, &Txi));
    }

cleanup:
    ecc_point_free(&Txi);

    return ret;
}

static int ecc_mul_comb(ecc_group_t *group, ecc_point_t *R,
    const big_number_t *m, const ecc_point_t *P,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret;
    unsigned char w, m_is_odd, p_eq_g, pre_len, i;
    unsigned int d;
    unsigned char k[COMB_MAX_D + 1];
    ecc_point_t *T;
    big_number_t M, mm;

    big_number_init(&M);
    big_number_init(&mm);

    if (big_number_get_bit(&group->N, 0) != 1) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    w = group->nbits >= 384 ? 5 : 4;

    p_eq_g = 0;

    if (w > DECC_WINDOW_SIZE) {
        w = DECC_WINDOW_SIZE;
    }
    if (w >= group->nbits) {
        w = 2;
    }

    pre_len = 1U << (w - 1);
    d = (group->nbits + w - 1) / w;

    T = p_eq_g ? group->T : NULL;

    if (!T) {
        T = malloc(pre_len * sizeof(ecc_point_t));
        if (T == NULL) {
            ret = CRYPTO_ECODE_ERROR_NO_MEMORY;
            goto cleanup;
        }

        for (i = 0; i < pre_len; i++) {
            ecc_point_init(&T[i]);
        }

        D_CLEAN_IF_FAILED(ecc_precompute_comb(group, T, P, w, d));

        if (p_eq_g) {
            group->T = T;
            group->T_size = pre_len;
        }
    }

    m_is_odd = (big_number_get_bit(m, 0) == 1);
    D_CLEAN_IF_FAILED(big_number_copy(&M, m));
    D_CLEAN_IF_FAILED(big_number_sub_big_number(&mm, &group->N, m));
    D_CLEAN_IF_FAILED(big_number_safe_cond_assign(&M, &mm, ! m_is_odd));

    ecc_comb_fixed(k, d, w, &M);
    D_CLEAN_IF_FAILED(ecc_mul_comb_core(group, R, T, pre_len, k, d, f_rng, p_rng));

    D_CLEAN_IF_FAILED(ecc_safe_invert_jac(group, R, ! m_is_odd));
    D_CLEAN_IF_FAILED(ecc_normalize_jac(group, R));

cleanup:

    if (T != NULL && ! p_eq_g) {
        for (i = 0; i < pre_len; i++) {
            ecc_point_free(&T[i]);
        }
        free(T);
    }

    big_number_free(&M);
    big_number_free(&mm);

    if (ret != 0) {
        ecc_point_free(R);
    }

    return ret;
}

int ecc_mul(ecc_group_t *group, ecc_point_t *R,
    const big_number_t *m, const ecc_point_t *P,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret;

    if (big_number_cmp_int(&P->Z, 1) != 0) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    if ((ret = ecc_check_privkey(group, m)) != 0 ||
        (ret = ecc_check_pubkey(group, P)) != 0) {
        return ret;
    }

    return ecc_mul_comb(group, R, m, P, f_rng, p_rng);
}

static int ecc_check_pubkey_sw(const ecc_group_t *group, const ecc_point_t *pt)
{
    int ret;
    big_number_t YY, RHS;

    if (big_number_cmp_int(&pt->X, 0) < 0 ||
        big_number_cmp_int(&pt->Y, 0) < 0 ||
        big_number_cmp_big_number(&pt->X, &group->P) >= 0 ||
        big_number_cmp_big_number(&pt->Y, &group->P) >= 0) {
        return CRYPTO_ECODE_INVALID_KEY;
    }

    big_number_init(&YY);
    big_number_init(&RHS);

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&YY, &pt->Y, &pt->Y));
    MOD_MUL(YY);
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&RHS, &pt->X, &pt->X));
    MOD_MUL(RHS);

    if (group->A.p == NULL) {
        D_CLEAN_IF_FAILED(big_number_sub_int(&RHS, &RHS, 3));
        MOD_SUB(RHS);
    } else {
        D_CLEAN_IF_FAILED(big_number_add_big_number(&RHS, &RHS, &group->A));
        MOD_ADD(RHS);
    }

    D_CLEAN_IF_FAILED(big_number_mul_big_number(&RHS, &RHS, &pt->X));
    MOD_MUL(RHS);
    D_CLEAN_IF_FAILED(big_number_add_big_number(&RHS, &RHS, &group->B));
    MOD_ADD(RHS);

    if (big_number_cmp_big_number(&YY, &RHS) != 0) {
        ret = CRYPTO_ECODE_INVALID_KEY;
    }

cleanup:

    big_number_free(&YY);
    big_number_free(&RHS);

    return ret;
}

int ecc_check_pubkey(const ecc_group_t *group, const ecc_point_t *pt)
{
    if (big_number_cmp_int(&pt->Z, 1) != 0) {
        return CRYPTO_ECODE_INVALID_KEY;
    }

    return ecc_check_pubkey_sw(group, pt);
}

int ecc_check_privkey(const ecc_group_t *group, const big_number_t *d)
{
    if (big_number_cmp_int(d, 1) < 0 || big_number_cmp_big_number(d, &group->N) >= 0) {
        return CRYPTO_ECODE_INVALID_KEY;
    }
    return 0;
}

static int ecc_gen_keypair(ecc_group_t *group, big_number_t *d, ecc_point_t *Q,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret;
    unsigned int n_size = (group->nbits + 7) / 8;

    int count = 0;
    unsigned char rnd[DMAX_ECC_BYTES];

    do {
        f_rng(p_rng, rnd, n_size);
        D_CLEAN_IF_FAILED(big_number_read_binary(d, rnd, n_size));
        D_CLEAN_IF_FAILED(big_number_shift_r(d, 8 * n_size - group->nbits));

        if (++count > 32) {
            return CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED;
        }
    } while (big_number_cmp_int(d, 1) < 0 ||
        big_number_cmp_big_number(d, &group->N) >= 0);

cleanup:
    if (ret != 0) {
        return ret;
    }

    return ecc_mul(group, Q, d, &group->G, f_rng, p_rng);
}

int ecc_gen_key(int ecc_dp, ecc_keypair_t *key,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret = -1;

    ecc_keypair_init(key);
    ret = ecc_setup_group(ecc_dp, &key->group);
    if (!ret) {
        ret = ecc_gen_keypair(&key->group, &key->d, &key->Q, f_rng, p_rng);
        if (ret) {
            ecc_keypair_free(key);
        }
    }

    return ret;
}

int ecc_check_key_pair(const ecc_keypair_t *pub, const ecc_keypair_t *prv)
{
    int ret;
    ecc_point_t Q;
    ecc_group_t group;

    if (pub->group.id != prv->group.id ||
        big_number_cmp_big_number(&pub->Q.X, &prv->Q.X) ||
        big_number_cmp_big_number(&pub->Q.Y, &prv->Q.Y) ||
        big_number_cmp_big_number(&pub->Q.Z, &prv->Q.Z)) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    ecc_point_init(&Q);
    ecc_group_init(&group);

    ecc_setup_group(prv->group.id, &group);

    D_CLEAN_IF_FAILED(ecc_mul(&group, &Q, &prv->d, &prv->group.G, NULL, NULL));

    if (big_number_cmp_big_number(&Q.X, &prv->Q.X) ||
        big_number_cmp_big_number(&Q.Y, &prv->Q.Y) ||
        big_number_cmp_big_number(&Q.Z, &prv->Q.Z)) {
        ret = CRYPTO_ECODE_BAD_INPUT_DATA;
        goto cleanup;
    }

cleanup:
    ecc_point_free(&Q);
    ecc_group_free(&group);

    return ret;
}

static int __derive_big_number(const ecc_group_t *group, big_number_t *x,
    const unsigned char *buf, unsigned int blen)
{
    int ret;
    unsigned int n_size = ( group->nbits + 7 ) / 8;
    unsigned int use_size = blen > n_size ? n_size : blen;

    D_CLEAN_IF_FAILED(big_number_read_binary( x, buf, use_size));
    if (use_size * 8 > group->nbits) {
        D_CLEAN_IF_FAILED(big_number_shift_r(x, use_size * 8 - group->nbits));
    }

    if (big_number_cmp_big_number(x, &group->N) >= 0) {
        D_CLEAN_IF_FAILED(big_number_sub_big_number(x, x, &group->N));
    }

cleanup:
    return ret;
}

void ecdsa_init(ecdsa_context_t *ctx)
{
    ecc_group_init(&ctx->group);
    big_number_init(&ctx->d);
    ecc_point_init(&ctx->Q);
    big_number_init(&ctx->r);
    big_number_init(&ctx->s);
}

void ecdsa_free(ecdsa_context_t *ctx)
{
    ecc_group_free(&ctx->group);
    big_number_free(&ctx->d);
    ecc_point_free(&ctx->Q);
    big_number_free(&ctx->r);
    big_number_free(&ctx->s);
}

int ecdsa_sign(ecc_group_t *group, big_number_t *r, big_number_t *s,
    const big_number_t *d, const unsigned char *buf, unsigned int blen,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng)
{
    int ret, key_tries, sign_tries, blind_tries;
    ecc_point_t R;
    big_number_t k, e, t;

    if (!group->N.p) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    ecc_point_init(&R);
    big_number_init(&k);
    big_number_init(&e);
    big_number_init(&t);

    sign_tries = 0;
    do {
        key_tries = 0;
        do {
            D_CLEAN_IF_FAILED(ecc_gen_keypair(group, &k, &R, f_rng, p_rng));
            D_CLEAN_IF_FAILED(big_number_mod_big_number(r, &R.X, &group->N));

            if (key_tries ++ > 12) {
                ret = CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED;
                goto cleanup;
            }
        } while (big_number_cmp_int(r, 0) == 0);

        D_CLEAN_IF_FAILED(__derive_big_number(group, &e, buf, blen));

        blind_tries = 0;
        do {
            unsigned int n_size = (group->nbits + 7) / 8;
            D_CLEAN_IF_FAILED(big_number_fill_random(&t, n_size, f_rng, p_rng));
            D_CLEAN_IF_FAILED(big_number_shift_r(&t, 8 * n_size - group->nbits));

            if (++blind_tries > 30) {
                return CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED;
            }
        } while (big_number_cmp_int(&t, 1) < 0 ||
               big_number_cmp_big_number(&t, &group->N) >= 0);

        D_CLEAN_IF_FAILED(big_number_mul_big_number(s, r, d));
        D_CLEAN_IF_FAILED(big_number_add_big_number(&e, &e, s));
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&e, &e, &t));
        D_CLEAN_IF_FAILED(big_number_mul_big_number(&k, &k, &t));
        D_CLEAN_IF_FAILED(big_number_inv_mod(s, &k, &group->N));
        D_CLEAN_IF_FAILED(big_number_mul_big_number(s, s, &e));
        D_CLEAN_IF_FAILED(big_number_mod_big_number(s, s, &group->N));

        if (sign_tries++ > 10) {
            ret = CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED;
            goto cleanup;
        }
    }
    while(big_number_cmp_int(s, 0) == 0);

cleanup:
    ecc_point_free(&R);
    big_number_free(&k);
    big_number_free(&e);
    big_number_free(&t);

    return ret;
}

int ecdsa_verify(ecc_group_t *group,
    const unsigned char *buf, unsigned int blen,
    const ecc_point_t *Q, const big_number_t *r, const big_number_t *s)
{
    int ret;
    big_number_t e, s_inv, u1, u2;
    ecc_point_t R, P;

    ecc_point_init(&R);
    ecc_point_init(&P);
    big_number_init(&e);
    big_number_init(&s_inv);
    big_number_init(&u1);
    big_number_init(&u2);

    if (!group->N.p) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    if (big_number_cmp_int(r, 1) < 0 || big_number_cmp_big_number(r, &group->N) >= 0 ||
        big_number_cmp_int(s, 1) < 0 || big_number_cmp_big_number(s, &group->N) >= 0) {
        ret = CRYPTO_ECODE_SIGNATURE_VERIFY_FAIL;
        goto cleanup;
    }

    D_CLEAN_IF_FAILED(ecc_check_pubkey(group, Q));
    D_CLEAN_IF_FAILED(__derive_big_number(group, &e, buf, blen));

    D_CLEAN_IF_FAILED(big_number_inv_mod(&s_inv, s, &group->N));
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&u1, &e, &s_inv));
    D_CLEAN_IF_FAILED(big_number_mod_big_number(&u1, &u1, &group->N));
    D_CLEAN_IF_FAILED(big_number_mul_big_number(&u2, r, &s_inv));
    D_CLEAN_IF_FAILED(big_number_mod_big_number(&u2, &u2, &group->N));

    D_CLEAN_IF_FAILED(ecc_mul(group, &R, &u1, &group->G, NULL, NULL));
    D_CLEAN_IF_FAILED(ecc_mul(group, &P, &u2, Q, NULL, NULL));
    D_CLEAN_IF_FAILED(ecc_add(group, &R, &R, &P));

    if (ecc_is_zero(&R)) {
        ret = CRYPTO_ECODE_SIGNATURE_VERIFY_FAIL;
        goto cleanup;
    }

    D_CLEAN_IF_FAILED(big_number_mod_big_number(&R.X, &R.X, &group->N));
    if (big_number_cmp_big_number(&R.X, r) != 0) {
        ret = CRYPTO_ECODE_SIGNATURE_VERIFY_FAIL;
        goto cleanup;
    }

cleanup:
    ecc_point_free(&R);
    ecc_point_free(&P);
    big_number_free(&e);
    big_number_free(&s_inv);
    big_number_free(&u1);
    big_number_free(&u2);

    return ret;
}

int ecdsa_read_asn1_signature(ecdsa_context_t *ctx,
    const unsigned char *hash, unsigned int hlen,
    const unsigned char *sig, unsigned int slen)
{
    int ret;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    unsigned int len;

    //asn1 1
    if ((ASN1_CONSTRUCTED | ASN1_SEQUENCE) != p[0]) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }
    p ++;

    if (0x82 == p[0]) {
        len = (p[1] << 8) | p[2];
        p += 3;
    } else if (0x81 == p[0]) {
        len = p[1];
        p += 2;
    } else if (128 > p[0]) {
        len = p[0];
        p ++;
    } else {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    if (p + len != end) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    //asn1 r
    if (ASN1_INTEGER != p[0]) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }
    p ++;

    if (0x82 == p[0]) {
        len = (p[1] << 8) | p[2];
        p += 3;
    } else if (0x81 == p[0]) {
        len = p[1];
        p += 2;
    } else if (128 > p[0]) {
        len = p[0];
        p ++;
    } else {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    ret = big_number_read_binary(&ctx->r, (const unsigned char *) p, len);
    if (ret) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }
    p += len;

    //asn1 s
    if (ASN1_INTEGER != p[0]) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }
    p ++;

    if (0x82 == p[0]) {
        len = (p[1] << 8) | p[2];
        p += 3;
    } else if (0x81 == p[0]) {
        len = p[1];
        p += 2;
    } else if (128 > p[0]) {
        len = p[0];
        p ++;
    } else {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    ret = big_number_read_binary(&ctx->s, (const unsigned char *) p, len);
    if (ret) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }
    p += len;

    if ((ret = ecdsa_verify(&ctx->group, hash, hlen, &ctx->Q, &ctx->r, &ctx->s)) != 0) {
        return ret;
    }

    if (p != end) {
        return CRYPTO_ECODE_SIGNATURE_INVALID_LENGTH;
    }

    return 0;
}

int ecdsa_write_asn1_signature(ecdsa_context_t *ctx,
    const unsigned char *hash, unsigned int hlen,
    unsigned char *sig, unsigned int *slen,
    int (*f_rng)(void *, unsigned char *, unsigned int),
    void *p_rng)
{
    int ret = 0;
    unsigned char r_buf[DMAX_ECC_BYTES + 4];
    unsigned char s_buf[DMAX_ECC_BYTES + 4];

    unsigned int r_len = 0;
    unsigned int s_len = 0;
    unsigned int r_need_zero_heading = 0;
    unsigned int s_need_zero_heading = 0;
    unsigned int r_lentag_len = 0;
    unsigned int s_lentag_len = 0;

    unsigned int len = 0;
    unsigned int write_len = 0;

    ret = ecdsa_sign(&ctx->group, &ctx->r, &ctx->s, &ctx->d, hash, hlen, f_rng, p_rng);
    if (ret) {
        DCRYPT_LOG("error: ecdsa_sign fail\n");
        return ret;
    }

    r_len = big_number_size(&ctx->r);
    s_len = big_number_size(&ctx->s);

    if ((r_len > sizeof(r_buf)) || (s_len > sizeof(s_buf))) {
        DCRYPT_LOG("error: r/s too long\n");
        return CRYPTO_ECODE_INTERNAL_ERROR;
    }

    big_number_write_binary(&ctx->r, r_buf, r_len);
    big_number_write_binary(&ctx->s, s_buf, s_len);

    if (ctx->r.s && (r_buf[0] & 0x80)) {
        r_need_zero_heading = 1;
    }

    if (ctx->s.s && (s_buf[0] & 0x80)) {
        s_need_zero_heading = 1;
    }

    if ((r_len + r_need_zero_heading) < 128) {
        r_lentag_len = 1;
    } else if ((r_len + r_need_zero_heading) < 256) {
        r_lentag_len = 2;
    } else if ((r_len + r_need_zero_heading) < 65536) {
        r_lentag_len = 3;
    }

    if ((s_len + s_need_zero_heading) < 128) {
        s_lentag_len = 1;
    } else if ((s_len + s_need_zero_heading) < 256) {
        s_lentag_len = 2;
    } else if ((s_len + s_need_zero_heading) < 65536) {
        s_lentag_len = 3;
    }

    len = r_len + s_len + r_need_zero_heading + s_need_zero_heading + 1 + 1 + r_lentag_len + s_lentag_len;

    write_len = 0;

    //asn1 1
    sig[write_len] = ASN1_CONSTRUCTED | ASN1_SEQUENCE;
    write_len ++;

    if (len < 128) {
        sig[write_len] = len;
        write_len ++;
    } else if (len < 256) {
        sig[write_len] = 0x81;
        sig[write_len + 1] = len;
        write_len += 2;
    } else if (len < 65536) {
        sig[write_len] = 0x82;
        sig[write_len + 1] = (len >> 8) & 0xff;
        sig[write_len + 2] = len & 0xff;
        write_len += 3;
    }

    //asn1 r
    sig[write_len] = ASN1_INTEGER;
    write_len ++;

    len = r_len + r_need_zero_heading;
    if (1 == r_lentag_len) {
        sig[write_len] = len;
        write_len ++;
    } else if (2 == r_lentag_len) {
        sig[write_len] = 0x81;
        sig[write_len + 1] = len;
        write_len += 2;
    } else if (3 == r_lentag_len) {
        sig[write_len] = 0x82;
        sig[write_len + 1] = (len >> 8) & 0xff;
        sig[write_len + 2] = len & 0xff;
        write_len += 3;
    }

    if (r_need_zero_heading) {
        sig[write_len] = 0;
        write_len ++;
    }

    memcpy(sig + write_len, r_buf, r_len);
    write_len += r_len;

    //asn1 s
    sig[write_len] = ASN1_INTEGER;
    write_len ++;

    len = s_len + s_need_zero_heading;
    if (1 == s_lentag_len) {
        sig[write_len] = len;
        write_len ++;
    } else if (2 == s_lentag_len) {
        sig[write_len] = 0x81;
        sig[write_len + 1] = len;
        write_len += 2;
    } else if (3 == s_lentag_len) {
        sig[write_len] = 0x82;
        sig[write_len + 1] = (len >> 8) & 0xff;
        sig[write_len + 2] = len & 0xff;
        write_len += 3;
    }

    if (s_need_zero_heading) {
        sig[write_len] = 0;
        write_len ++;
    }

    memcpy(sig + write_len, s_buf, s_len);
    write_len += s_len;

    *slen = write_len;

    return 0;
}

