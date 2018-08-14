/*******************************************************************************
 * aes_precalc_table.c
 *
 * History:
 *  2017/02/27 - [Zhi He] create file
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

static unsigned char gsForwardSBox[256] = {0};
static unsigned int gsForwardTable0[256] = {0};
static unsigned int gsForwardTable1[256] = {0};
static unsigned int gsForwardTable2[256] = {0};
static unsigned int gsForwardTable3[256] = {0};

static unsigned char gsReverseSBox[256] = {0};
static unsigned int gsReverseTable0[256] = {0};
static unsigned int gsReverseTable1[256] = {0};
static unsigned int gsReverseTable2[256] = {0};
static unsigned int gsReverseTable3[256] = {0};

static unsigned int gsRoundConstant[10] = {0};

static void __write_precalulated_tables(const char *filename)
{
    unsigned int i = 0, j = 0, index = 0;
    FILE *p_file = fopen(filename, "wt+");

    if (p_file) {

        index = 0;
        fprintf(p_file, "static const unsigned char gcsForwardSBox[256] = {\n");
        for (j = 0; j < 16; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 16; i ++, index ++) {
                fprintf(p_file, "0x%02x, ", gsForwardSBox[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned char gcsReverseSBox[256] = {\n");
        for (j = 0; j < 16; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 16; i ++, index ++) {
                fprintf(p_file, "0x%02x, ", gsReverseSBox[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");


        index = 0;
        fprintf(p_file, "static const unsigned int gcsForwardTable0[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsForwardTable0[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsForwardTable1[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsForwardTable1[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsForwardTable2[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsForwardTable2[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsForwardTable3[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsForwardTable3[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsReverseTable0[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsReverseTable0[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsReverseTable1[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsReverseTable1[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsReverseTable2[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsReverseTable2[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsReverseTable3[256] = {\n");
        for (j = 0; j < 64; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 4; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsReverseTable3[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

        index = 0;
        fprintf(p_file, "static const unsigned int gcsRoundConstant[10] = {\n");
        for (j = 0; j < 2; j ++) {
            fprintf(p_file, "    ");
            for (i = 0; i < 5; i ++, index ++) {
                fprintf(p_file, "0x%08x, ", gsRoundConstant[index]);
            }
            fprintf(p_file, "\n");
        }
        fprintf(p_file, "};\n\n\n");

    }

    return;
}

int main()
{

#define DLogicLeftRotate8(x) ((x << 8) & 0xFFFFFFFF) | (x >> 24)
#define DGF28Time(x) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00))
#define DGF28Multiply(x,y) ((x && y) ? pow[(log[x] + log[y]) % 255] : 0)

    unsigned int i, x, y, z;
    unsigned int pow[256] = {0};
    unsigned int log[256] = {0};

    for ( i = 0, x = 1; i < 256; i++ ) {
        pow[i] = x;
        log[x] = i;
        x = (x ^ DGF28Time(x)) & 0xFF;
    }

    for ( i = 0, x = 1; i < 10; i++ ) {
        gsRoundConstant[i] = (unsigned int) x;
        x = DGF28Time(x) & 0xFF;
    }

    gsForwardSBox[0x00] = 0x63;
    gsReverseSBox[0x63] = 0x00;

    for ( i = 1; i < 256; i++ ) {
        x = pow[255 - log[i]];
        y  = x;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y ^ 0x63;

        gsForwardSBox[i] = (unsigned char) x;
        gsReverseSBox[x] = (unsigned char) i;
    }

    for( i = 0; i < 256; i ++ ) {
        x = gsForwardSBox[i];
        y = DGF28Time(x) & 0xFF;
        z =  (y ^ x) & 0xFF;

        gsForwardTable0[i] = y ^ (x <<  8) ^ (x << 16) ^ (z << 24);

        gsForwardTable1[i] = DLogicLeftRotate8(gsForwardTable0[i]);
        gsForwardTable2[i] = DLogicLeftRotate8(gsForwardTable1[i]);
        gsForwardTable3[i] = DLogicLeftRotate8(gsForwardTable2[i]);

        x = gsReverseSBox[i];

        gsReverseTable0[i] = (DGF28Multiply(0x0E, x)) ^ (DGF28Multiply(0x09, x) <<  8) ^ (DGF28Multiply(0x0D, x) << 16) ^ (DGF28Multiply(0x0B, x) << 24);

        gsReverseTable1[i] = DLogicLeftRotate8(gsReverseTable0[i]);
        gsReverseTable2[i] = DLogicLeftRotate8(gsReverseTable1[i]);
        gsReverseTable3[i] = DLogicLeftRotate8(gsReverseTable2[i]);
    }

    __write_precalulated_tables("precalc_aes_tables.txt");

    return 0;
}


