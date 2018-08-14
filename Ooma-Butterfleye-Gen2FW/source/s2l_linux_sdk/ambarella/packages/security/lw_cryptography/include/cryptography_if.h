/*******************************************************************************
 * cryptography_if.h
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
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

#ifndef __CRYPTOGRAPHY_IF_H__
#define __CRYPTOGRAPHY_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define DNOT_INCLUDE_C_HEADER
#ifdef DNOT_INCLUDE_C_HEADER
#define NULL ((void *) 0x0)
#endif

#define DCONFIG_AES_USE_PRECALCULATED_TABLE

#define DCRYPT_LOG printf

#define DAMBOOT_PUT_STRING putstr
#define DAMBOOT_PUT_BYTE putbyte
#define DAMBOOT_PUT_DEC putdec

enum {
    CRYPTO_ECODE_OK = 0x00,
    CRYPTO_ECODE_INVALID_ARGUMENT = -0x01,
    CRYPTO_ECODE_INVALID_INPUT = -0x02,
    CRYPTO_ECODE_INVALID_OUTPUT = -0x03,
    CRYPTO_ECODE_INVALID_KEY = -0x04,
    CRYPTO_ECODE_SIGNATURE_VERIFY_FAIL = -0x05,
    CRYPTO_ECODE_GENERATE_KEY_FAIL = -0x06,
    CRYPTO_ECODE_SIGNATURE_INVALID_PADDING = -0x07,
    CRYPTO_ECODE_SIGNATURE_INVALID_LENGTH = -0x08,

    CRYPTO_ECODE_INTERNAL_ERROR = -0x10,
    CRYPTO_ECODE_ERROR_NO_MEMORY = -0x11,

    CRYPTO_ECODE_FILE_IO_ERROR = -0x20,
    CRYPTO_ECODE_BAD_INPUT_DATA = -0x21,
    CRYPTO_ECODE_INVALID_CHARACTER = -0x22,
    CRYPTO_ECODE_BUFFER_TOO_SMALL = -0x23,
    CRYPTO_ECODE_NEGATIVE_VALUE = -0x24,
    CRYPTO_ECODE_DIVISION_BY_ZERO = -0x25,
    CRYPTO_ECODE_NOT_ACCEPTABLE = -0x26,
    CRYPTO_ECODE_PK_PUBLIC_FAILED = -0x27,
    CRYPTO_ECODE_PK_PRIVATE_FAILED = -0x28,
    CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED = -0x29,

    CRYPTO_ECODE_ASN1_OUT_OF_DATA = -0x30,
    CRYPTO_ECODE_ASN1_INVALID_LENGTH = -0x31,
    CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL = -0x32,
    CRYPTO_ECODE_ASN1_UNEXPECTED_TAG = -0x33,

    CRYPTO_ECODE_OID_NOT_FOUND = -0x40,
};

enum {
    CRYPTO_SHA_TYPE_NONE = 0x0,
    CRYPTO_SHA_TYPE_SHA1 = 0x1,
    CRYPTO_SHA_TYPE_SHA224 = 0x2,
    CRYPTO_SHA_TYPE_SHA256 = 0x3,
    CRYPTO_SHA_TYPE_SHA384 = 0x4,
    CRYPTO_SHA_TYPE_SHA512 = 0x5,
};

#define DCRYPTO_SHA1_DIGEST_LENGTH 20
#define DCRYPTO_SHA256_DIGEST_LENGTH 32
#define DCRYPTO_SHA384_DIGEST_LENGTH 48
#define DCRYPTO_SHA512_DIGEST_LENGTH 64

#define D_CLEAN_IF_FAILED(f) do { if ((ret = f) != CRYPTO_ECODE_OK) goto cleanup; } while(0)

#define D_BIG_NUMBER_MAX_LIMBS                             10000
#define D_BIG_NUMBER_WINDOW_SIZE                           6
#define D_BIG_NUMBER_MAX_SIZE                              1024
#define D_BIG_NUMBER_MAX_BITS                              (8 * D_BIG_NUMBER_MAX_SIZE)

#define RSA_PUBLIC      0
#define RSA_PRIVATE     1
#define RSA_PKCS_V15    0
#define RSA_PKCS_V21    1


#define D_DISABLE_ASM

#ifndef D_DISABLE_ASM

#if (defined(_MSC_VER) && defined(_M_AMD64))
    typedef  long long TSINT;
    typedef unsigned long long TUINT;
    #define DHAVE_INT_64
#else
    #define DHAVE_DOUBLE_LONG_INT
#if (defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__) || defined(__ia64__) || (defined(__arch64__))))
    typedef  long long TSINT;
    typedef unsigned long long TUINT;
    typedef unsigned int TUDBL __attribute__((mode(TI)));
    #define DHAVE_INT_64
#else
    typedef int TSINT;
    typedef unsigned int TUINT;
    typedef unsigned long long TUDBL;
#endif
#endif

#else

    typedef int TSINT;
    typedef unsigned int TUINT;

#if __WORDSIZE == 64
    typedef unsigned long TUDBL;
    #define DHAVE_DOUBLE_LONG_INT
#endif

#endif

typedef struct {
    int s;
    unsigned int n;
    TUINT *p;
} big_number_t;

typedef struct {
    int ver;
    unsigned int len;

    big_number_t N;
    big_number_t E;

    big_number_t D;
    big_number_t P;
    big_number_t Q;
    big_number_t DP;
    big_number_t DQ;
    big_number_t QP;

    big_number_t RN;
    big_number_t RP;
    big_number_t RQ;

    big_number_t Vi;
    big_number_t Vf;

    int padding;
    int hash_id;
} rsa_context_t;

enum {
    CRYPTO_ECC_DP_INVALID = 0,
    CRYPTO_ECC_DP_SECP192R1,
    CRYPTO_ECC_DP_SECP224R1,
    CRYPTO_ECC_DP_SECP256R1,
    CRYPTO_ECC_DP_SECP384R1,
    CRYPTO_ECC_DP_SECP521R1,
    CRYPTO_ECC_DP_BP256R1,
    CRYPTO_ECC_DP_BP384R1,
    CRYPTO_ECC_DP_BP512R1,
    CRYPTO_ECC_DP_SECP192K1,
    CRYPTO_ECC_DP_SECP224K1,
    CRYPTO_ECC_DP_SECP256K1,
};

typedef struct {
    big_number_t X;
    big_number_t Y;
    big_number_t Z;
} ecc_point_t;

typedef struct {
    int id;
    big_number_t P;
    big_number_t A;
    big_number_t B;
    ecc_point_t G;
    big_number_t N;
    unsigned int pbits;
    unsigned int nbits;
    int (*modp)(big_number_t *); //for future optimization
    ecc_point_t *T;
    unsigned int T_size;
} ecc_group_t;

typedef struct {
    ecc_group_t group;
    big_number_t d;
    ecc_point_t Q;
} ecc_keypair_t;

typedef struct
{
    ecc_group_t group;
    big_number_t d;
    ecc_point_t Q;
    big_number_t r;
    big_number_t s;
} ecdsa_context_t;

void pseudo_random_scamble_sequence(unsigned char* p, unsigned int len);

void digest_sha256(const unsigned char* message, unsigned int len, unsigned char* digest);
#ifndef DNOT_INCLUDE_C_HEADER
int digest_sha256_file(const char *file, unsigned char* digest);
void* digest_sha256_init();
void digest_sha256_update(void* ctx, const unsigned char* message, unsigned int len);
void digest_sha256_final(void* ctx, unsigned char* digest);
#endif

void digest_md5(const unsigned char* message, unsigned int len, unsigned char* digest);
void digest_md5_hmac(const unsigned char* key, unsigned int keylen, const unsigned char* input, unsigned int ilen, unsigned char* output);
#ifndef DNOT_INCLUDE_C_HEADER
int digest_md5_file(const char* file, unsigned char* digest);
void* digest_md5_init();
void digest_md5_update(void* ctx, const unsigned char* message, unsigned int len);
void digest_md5_final(void* ctx, unsigned char* digest);
#endif

void rsa_init(rsa_context_t* ctx, int padding, int hash_id);
void rsa_free(rsa_context_t* ctx);
int rsa_check_pubkey(const rsa_context_t* ctx);
int rsa_check_privkey(const rsa_context_t* ctx);
int rsa_check_key_pair(const rsa_context_t* pub, const rsa_context_t* prv);
int rsa_gen_key(rsa_context_t* ctx, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng, unsigned int nbits, int exponent);
int rsa_sha256_sign(rsa_context_t* ctx, const unsigned char* hash, unsigned char* sig);
int rsa_sha256_verify(rsa_context_t* ctx, const unsigned char* hash, const unsigned char* sig);

void ecdsa_init(ecdsa_context_t *ctx);
void ecdsa_free(ecdsa_context_t *ctx);
int ecc_setup_group(int ecc_dp, ecc_group_t *group);
int ecc_check_pubkey(const ecc_group_t *group, const ecc_point_t *pt);
int ecc_check_privkey(const ecc_group_t *group, const big_number_t *d);
int ecc_check_key_pair(const ecc_keypair_t *pub, const ecc_keypair_t *prv);
int ecc_gen_key(int ecc_dp, ecc_keypair_t *key,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng);
int ecdsa_sign(ecc_group_t *group, big_number_t *r, big_number_t *s,
    const big_number_t *d, const unsigned char *buf, unsigned int blen,
    int (*f_rng)(void *, unsigned char *, unsigned int), void *p_rng);
int ecdsa_verify(ecc_group_t *group,
    const unsigned char *buf, unsigned int blen,
    const ecc_point_t *Q, const big_number_t *r, const big_number_t *s);
int ecdsa_read_asn1_signature(ecdsa_context_t *ctx,
    const unsigned char *hash, unsigned int hlen,
    const unsigned char *sig, unsigned int slen);
int ecdsa_write_asn1_signature(ecdsa_context_t *ctx,
    const unsigned char *hash, unsigned int hlen,
    unsigned char *sig, unsigned int *slen,
    int (*f_rng)(void *, unsigned char *, unsigned int),
    void *p_rng);

extern int big_number_read_string(big_number_t* X, int radix, const char* s);
extern int big_number_write_string(const big_number_t* X, int radix, char* s, unsigned int* slen);

extern int big_number_write_binary(const big_number_t* X, unsigned char* buf, unsigned int buflen);
extern int big_number_read_binary(big_number_t* X, const unsigned char* buf, unsigned int buflen);

#ifndef DNOT_INCLUDE_C_HEADER
extern int big_number_write_file(const char* p, const big_number_t* X, int radix, void* f);
extern int big_number_read_file(big_number_t* X, int radix, void* f);
#endif

extern unsigned int big_number_size(const big_number_t* X);
extern unsigned int big_number_msb(const big_number_t *X);


typedef struct {
    unsigned int round;
    unsigned int p_round_key[72];
    unsigned int cur_position;
} aes_context_t;

aes_context_t *aes_init();
void aes_destroy(aes_context_t *ctx);

int aes_set_encrypt_key(aes_context_t *ctx, const unsigned char *key, unsigned int key_length);
int aes_set_decrypt_key(aes_context_t *ctx, const unsigned char *key, unsigned int key_length);

//ecb mode
int aes_ecb_encrypt(aes_context_t *ctx, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_ecb_decrypt(aes_context_t *ctx, unsigned char *input, unsigned char *output, unsigned int data_length);

//ctr mode
int aes_ctr_crypt(aes_context_t *ctx, unsigned char *p_nonce, unsigned char *input, unsigned char *output, unsigned int data_length);

//cbc mode
int aes_cbc_encrypt(aes_context_t *ctx, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_cbc_decrypt(aes_context_t *ctx, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length);


//one shot api
int aes_ecb_encrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_ecb_decrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_ctr_crypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *p_nonce, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_cbc_encrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length);
int aes_cbc_decrypt_one_shot(const unsigned char *key, unsigned int key_length, unsigned char *iv, unsigned char *input, unsigned char *output, unsigned int data_length);


//internal define
//ASN1
#define ASN1_BOOLEAN                 0x01
#define ASN1_INTEGER                 0x02
#define ASN1_BIT_STRING              0x03
#define ASN1_OCTET_STRING            0x04
#define ASN1_NULL                    0x05
#define ASN1_OID                     0x06
#define ASN1_UTF8_STRING             0x0C
#define ASN1_SEQUENCE                0x10
#define ASN1_SET                     0x11
#define ASN1_PRINTABLE_STRING        0x13
#define ASN1_T61_STRING              0x14
#define ASN1_IA5_STRING              0x16
#define ASN1_UTC_TIME                0x17
#define ASN1_GENERALIZED_TIME        0x18
#define ASN1_UNIVERSAL_STRING        0x1C
#define ASN1_BMP_STRING              0x1E
#define ASN1_PRIMITIVE               0x00
#define ASN1_CONSTRUCTED             0x20
#define ASN1_CONTEXT_SPECIFIC        0x80

#ifdef __cplusplus
}
#endif

#endif

