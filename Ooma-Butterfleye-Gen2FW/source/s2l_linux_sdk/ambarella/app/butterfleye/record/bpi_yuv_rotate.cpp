/*
 * bpi_yuv_rotate.c
 *
 * History:
 *       2016/09/01 - [ShengJiang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "bpi_yuv_rotate.h"
#include <arm_neon.h>
#include <string.h>

void BPIYUVRotate::BPIYUV420Rotate(unsigned char* dstImg,
                                   unsigned char* srcImgY,
                                   unsigned char* srcImgUV, int width,
                                   int height, int pitch, int rotate)
{
    unsigned char* dstImgTmp = dstImg;
    unsigned char* srcImgTmpY = srcImgY;
    unsigned char* srcImgTmpUV = srcImgUV;
    switch (rotate) {
    case 1:
        NeonRotateYUV420Y90(dstImg, srcImgY, width, height, pitch);
        NeonRotateYUV420UV90(dstImg + width * height, srcImgUV,
                             width, height / 2, pitch);
        break;
    case 2:
        NeonRotateYUV420Y180(dstImg, srcImgY, width, height, pitch);
        NeonRotateYUV420UV180(dstImg + width * height, srcImgUV,
                              width, height / 2, pitch);
        break;
    case 3:
        NeonRotateYUV420Y270(dstImg, srcImgY, width, height, pitch);
        NeonRotateYUV420UV270(dstImg + width * height, srcImgUV,
                              width, height / 2, pitch);
        break;
    default:
        for (int i = 0; i < height; i++) {
            memcpy(dstImgTmp, srcImgTmpY, width);
            dstImgTmp += width;
            srcImgTmpY += pitch;
        }
        for (int i = 0; i < height / 2; i++) {
            memcpy(dstImgTmp, srcImgTmpUV, width);
            dstImgTmp += width;
            srcImgTmpUV += pitch;
        }
        break;
    }
}

void BPIYUVRotate::NeonRotateYUV420Y180(unsigned char* dstImg,
                                        unsigned char* srcImg, int width,
                                        int height, int pitch)
{
    uint8x8x4_t y_mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t y_mat2;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            y_mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            y_mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            y_mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            y_mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            y_mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            y_mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            y_mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            y_mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 reverse every element in a row
            y_mat1.val[0] = vrev64_u8(y_mat1.val[0]);
            y_mat1.val[1] = vrev64_u8(y_mat1.val[1]);
            y_mat1.val[2] = vrev64_u8(y_mat1.val[2]);
            y_mat1.val[3] = vrev64_u8(y_mat1.val[3]);
            y_mat2.val[0] = vrev64_u8(y_mat2.val[0]);
            y_mat2.val[1] = vrev64_u8(y_mat2.val[1]);
            y_mat2.val[2] = vrev64_u8(y_mat2.val[2]);
            y_mat2.val[3] = vrev64_u8(y_mat2.val[3]);
            //step2 store every row in reverse order
            vst1_u8(dstImg + (height - i - 8) * width + (width - j - 8),
                    y_mat2.val[3]);
            vst1_u8(dstImg + (height - i - 7) * width + (width - j - 8),
                    y_mat2.val[2]);
            vst1_u8(dstImg + (height - i - 6) * width + (width - j - 8),
                    y_mat2.val[1]);
            vst1_u8(dstImg + (height - i - 5) * width + (width - j - 8),
                    y_mat2.val[0]);
            vst1_u8(dstImg + (height - i - 4) * width + (width - j - 8),
                    y_mat1.val[3]);
            vst1_u8(dstImg + (height - i - 3) * width + (width - j - 8),
                    y_mat1.val[2]);
            vst1_u8(dstImg + (height - i - 2) * width + (width - j - 8),
                    y_mat1.val[1]);
            vst1_u8(dstImg + (height - i - 1) * width + (width - j - 8),
                    y_mat1.val[0]);
        }
    }
}

void BPIYUVRotate::NeonRotateYUV420UV180(unsigned char* dstImg,
                                         unsigned char* srcImg, int width,
                                         int height, int pitch)
{
    uint8x8x4_t y_mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t y_mat2;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            y_mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            y_mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            y_mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            y_mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            y_mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            y_mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            y_mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            y_mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 reverse every element in a row
            y_mat1.val[0] = vrev64_u8(y_mat1.val[0]);
            y_mat1.val[0] = vrev16_u8(y_mat1.val[0]);

            y_mat1.val[1] = vrev64_u8(y_mat1.val[1]);
            y_mat1.val[1] = vrev16_u8(y_mat1.val[1]);

            y_mat1.val[2] = vrev64_u8(y_mat1.val[2]);
            y_mat1.val[2] = vrev16_u8(y_mat1.val[2]);

            y_mat1.val[3] = vrev64_u8(y_mat1.val[3]);
            y_mat1.val[3] = vrev16_u8(y_mat1.val[3]);

            y_mat2.val[0] = vrev64_u8(y_mat2.val[0]);
            y_mat2.val[0] = vrev16_u8(y_mat2.val[0]);

            y_mat2.val[1] = vrev64_u8(y_mat2.val[1]);
            y_mat2.val[1] = vrev16_u8(y_mat2.val[1]);

            y_mat2.val[2] = vrev64_u8(y_mat2.val[2]);
            y_mat2.val[2] = vrev16_u8(y_mat2.val[2]);

            y_mat2.val[3] = vrev64_u8(y_mat2.val[3]);
            y_mat2.val[3] = vrev16_u8(y_mat2.val[3]);
            //step2 store every row in reverse order
            vst1_u8(dstImg + (height - i - 8) * width + (width - j - 8),
                    y_mat2.val[3]);
            vst1_u8(dstImg + (height - i - 7) * width + (width - j - 8),
                    y_mat2.val[2]);
            vst1_u8(dstImg + (height - i - 6) * width + (width - j - 8),
                    y_mat2.val[1]);
            vst1_u8(dstImg + (height - i - 5) * width + (width - j - 8),
                    y_mat2.val[0]);
            vst1_u8(dstImg + (height - i - 4) * width + (width - j - 8),
                    y_mat1.val[3]);
            vst1_u8(dstImg + (height - i - 3) * width + (width - j - 8),
                    y_mat1.val[2]);
            vst1_u8(dstImg + (height - i - 2) * width + (width - j - 8),
                    y_mat1.val[1]);
            vst1_u8(dstImg + (height - i - 1) * width + (width - j - 8),
                    y_mat1.val[0]);
        }
    }
}

void BPIYUVRotate::NeonRotateYUV420Y270(unsigned char* dstImg,
                                        unsigned char* srcImg, int width,
                                        int height, int pitch)
{
    uint8x8x4_t y_mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t y_mat2;

    uint8x8x2_t temp1;
    uint8x8x2_t temp2;
    uint8x8x2_t temp3;
    uint8x8x2_t temp4;

    uint16x4x2_t temp5;
    uint16x4x2_t temp6;
    uint16x4x2_t temp7;
    uint16x4x2_t temp8;
    uint16x4x2_t temp9;
    uint16x4x2_t temp10;
    uint16x4x2_t temp11;
    uint16x4x2_t temp12;

    uint32x2x2_t temp13;
    uint32x2x2_t temp14;
    uint32x2x2_t temp15;
    uint32x2x2_t temp16;
    uint32x2x2_t temp17;
    uint32x2x2_t temp18;
    uint32x2x2_t temp19;
    uint32x2x2_t temp20;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            y_mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            y_mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            y_mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            y_mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            y_mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            y_mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            y_mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            y_mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 trn nearby registers
            temp1 = vtrn_u8(y_mat1.val[0], y_mat1.val[1]);
            temp2 = vtrn_u8(y_mat1.val[2], y_mat1.val[3]);
            temp3 = vtrn_u8(y_mat2.val[0], y_mat2.val[1]);
            temp4 = vtrn_u8(y_mat2.val[2], y_mat2.val[3]);
            //step2 trn 1,3 2,4 5,7 6,8
            temp5.val[0] = vreinterpret_u16_u8(temp1.val[0]);
            temp5.val[1] = vreinterpret_u16_u8(temp1.val[1]);
            temp6.val[0] = vreinterpret_u16_u8(temp2.val[0]);
            temp6.val[1] = vreinterpret_u16_u8(temp2.val[1]);
            temp7.val[0] = vreinterpret_u16_u8(temp3.val[0]);
            temp7.val[1] = vreinterpret_u16_u8(temp3.val[1]);
            temp8.val[0] = vreinterpret_u16_u8(temp4.val[0]);
            temp8.val[1] = vreinterpret_u16_u8(temp4.val[1]);
            temp9 = vtrn_u16(temp5.val[0], temp6.val[0]);
            temp10 = vtrn_u16(temp5.val[1], temp6.val[1]);
            temp11 = vtrn_u16(temp7.val[0], temp8.val[0]);
            temp12 = vtrn_u16(temp7.val[1], temp8.val[1]);
            //step3 trn 1,5 2,6 3,7 4,8
            temp13.val[0] = vreinterpret_u32_u16(temp9.val[0]);
            temp13.val[1] = vreinterpret_u32_u16(temp9.val[1]);
            temp14.val[0] = vreinterpret_u32_u16(temp10.val[0]);
            temp14.val[1] = vreinterpret_u32_u16(temp10.val[1]);
            temp15.val[0] = vreinterpret_u32_u16(temp11.val[0]);
            temp15.val[1] = vreinterpret_u32_u16(temp11.val[1]);
            temp16.val[0] = vreinterpret_u32_u16(temp12.val[0]);
            temp16.val[1] = vreinterpret_u32_u16(temp12.val[1]);
            temp17 = vtrn_u32(temp13.val[0], temp15.val[0]);
            temp18 = vtrn_u32(temp13.val[1], temp15.val[1]);
            temp19 = vtrn_u32(temp14.val[0], temp16.val[0]);
            temp20 = vtrn_u32(temp14.val[1], temp16.val[1]);
            //step4 store bytes in correct position,the order now is 1,2,3,4,5,6,7,8
            temp1.val[0] = vreinterpret_u8_u32(temp20.val[1]);
            temp1.val[1] = vreinterpret_u8_u32(temp18.val[1]);
            temp2.val[0] = vreinterpret_u8_u32(temp19.val[1]);
            temp2.val[1] = vreinterpret_u8_u32(temp17.val[1]);
            temp3.val[0] = vreinterpret_u8_u32(temp20.val[0]);
            temp3.val[1] = vreinterpret_u8_u32(temp18.val[0]);
            temp4.val[0] = vreinterpret_u8_u32(temp19.val[0]);
            temp4.val[1] = vreinterpret_u8_u32(temp17.val[0]);
            vst1_u8(dstImg + (width - j - 8) * height + i, temp1.val[0]);
            vst1_u8(dstImg + (width - j - 7) * height + i, temp1.val[1]);
            vst1_u8(dstImg + (width - j - 6) * height + i, temp2.val[0]);
            vst1_u8(dstImg + (width - j - 5) * height + i, temp2.val[1]);
            vst1_u8(dstImg + (width - j - 4) * height + i, temp3.val[0]);
            vst1_u8(dstImg + (width - j - 3) * height + i, temp3.val[1]);
            vst1_u8(dstImg + (width - j - 2) * height + i, temp4.val[0]);
            vst1_u8(dstImg + (width - j - 1) * height + i, temp4.val[1]);
        }
    }
}

void BPIYUVRotate::NeonRotateYUV420UV270(unsigned char* dstImg,
                                         unsigned char* srcImg, int width,
                                         int height, int pitch)
{
    uint8x8x4_t y_mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t y_mat2;

    uint8x8x2_t temp1;
    uint8x8x2_t temp2;
    uint8x8x2_t temp3;
    uint8x8x2_t temp4;

    uint16x4x2_t temp5;
    uint16x4x2_t temp6;
    uint16x4x2_t temp7;
    uint16x4x2_t temp8;
    uint16x4x2_t temp9;
    uint16x4x2_t temp10;
    uint16x4x2_t temp11;
    uint16x4x2_t temp12;

    uint32x2x2_t temp13;
    uint32x2x2_t temp14;
    uint32x2x2_t temp15;
    uint32x2x2_t temp16;
    uint32x2x2_t temp17;
    uint32x2x2_t temp18;
    uint32x2x2_t temp19;
    uint32x2x2_t temp20;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            y_mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            y_mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            y_mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            y_mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            y_mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            y_mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            y_mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            y_mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 trn nearby registers
            temp1 = vtrn_u8(y_mat1.val[0], y_mat1.val[1]);
            temp2 = vtrn_u8(y_mat1.val[2], y_mat1.val[3]);
            temp3 = vtrn_u8(y_mat2.val[0], y_mat2.val[1]);
            temp4 = vtrn_u8(y_mat2.val[2], y_mat2.val[3]);
            //step2 trn 1,3 2,4 5,7 6,8
            temp5.val[0] = vreinterpret_u16_u8(temp1.val[0]);
            temp5.val[1] = vreinterpret_u16_u8(temp1.val[1]);
            temp6.val[0] = vreinterpret_u16_u8(temp2.val[0]);
            temp6.val[1] = vreinterpret_u16_u8(temp2.val[1]);
            temp7.val[0] = vreinterpret_u16_u8(temp3.val[0]);
            temp7.val[1] = vreinterpret_u16_u8(temp3.val[1]);
            temp8.val[0] = vreinterpret_u16_u8(temp4.val[0]);
            temp8.val[1] = vreinterpret_u16_u8(temp4.val[1]);
            temp9 = vtrn_u16(temp5.val[0], temp6.val[0]);
            temp10 = vtrn_u16(temp5.val[1], temp6.val[1]);
            temp11 = vtrn_u16(temp7.val[0], temp8.val[0]);
            temp12 = vtrn_u16(temp7.val[1], temp8.val[1]);
            //step3 trn 1,5 2,6 3,7 4,8
            temp13.val[0] = vreinterpret_u32_u16(temp9.val[0]);
            temp13.val[1] = vreinterpret_u32_u16(temp9.val[1]);
            temp14.val[0] = vreinterpret_u32_u16(temp10.val[0]);
            temp14.val[1] = vreinterpret_u32_u16(temp10.val[1]);
            temp15.val[0] = vreinterpret_u32_u16(temp11.val[0]);
            temp15.val[1] = vreinterpret_u32_u16(temp11.val[1]);
            temp16.val[0] = vreinterpret_u32_u16(temp12.val[0]);
            temp16.val[1] = vreinterpret_u32_u16(temp12.val[1]);
            temp17 = vtrn_u32(temp13.val[0], temp15.val[0]);
            temp18 = vtrn_u32(temp13.val[1], temp15.val[1]);
            temp19 = vtrn_u32(temp14.val[0], temp16.val[0]);
            temp20 = vtrn_u32(temp14.val[1], temp16.val[1]);
            //step4 store bytes in correct position,the order now is 1,2,3,4,5,6,7,8
            temp1.val[0] = vreinterpret_u8_u32(temp18.val[1]);
            temp1.val[1] = vreinterpret_u8_u32(temp20.val[1]);
            temp2.val[0] = vreinterpret_u8_u32(temp17.val[1]);
            temp2.val[1] = vreinterpret_u8_u32(temp19.val[1]);
            temp3.val[0] = vreinterpret_u8_u32(temp18.val[0]);
            temp3.val[1] = vreinterpret_u8_u32(temp20.val[0]);
            temp4.val[0] = vreinterpret_u8_u32(temp17.val[0]);
            temp4.val[1] = vreinterpret_u8_u32(temp19.val[0]);
            vst2_u8(dstImg + ((width / 2 - 4 - j / 2) * height * 2 + i * 2),
                    temp1);
            vst2_u8(dstImg + ((width / 2 - 3 - j / 2) * height * 2 + i * 2),
                    temp2);
            vst2_u8(dstImg + ((width / 2 - 2 - j / 2) * height * 2 + i * 2),
                    temp3);
            vst2_u8(dstImg + ((width / 2 - 1 - j / 2) * height * 2 + i * 2),
                    temp4);
        }
    }
}

void BPIYUVRotate::NeonRotateYUV420Y90(unsigned char* dstImg,
                                       unsigned char* srcImg, int width,
                                       int height, int pitch)
{
    uint8x8x4_t mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t mat2;

    uint8x8x2_t temp1;
    uint8x8x2_t temp2;
    uint8x8x2_t temp3;
    uint8x8x2_t temp4;

    uint16x4x2_t temp5;
    uint16x4x2_t temp6;
    uint16x4x2_t temp7;
    uint16x4x2_t temp8;
    uint16x4x2_t temp9;
    uint16x4x2_t temp10;
    uint16x4x2_t temp11;
    uint16x4x2_t temp12;

    uint32x2x2_t temp13;
    uint32x2x2_t temp14;
    uint32x2x2_t temp15;
    uint32x2x2_t temp16;
    uint32x2x2_t temp17;
    uint32x2x2_t temp18;
    uint32x2x2_t temp19;
    uint32x2x2_t temp20;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 trn nearby registers
            temp1 = vtrn_u8(mat1.val[1], mat1.val[0]);
            temp2 = vtrn_u8(mat1.val[3], mat1.val[2]);
            temp3 = vtrn_u8(mat2.val[1], mat2.val[0]);
            temp4 = vtrn_u8(mat2.val[3], mat2.val[2]);
            //step2 trn 1,3 2,4 5,7 6,8
            temp5.val[0] = vreinterpret_u16_u8(temp1.val[0]);
            temp5.val[1] = vreinterpret_u16_u8(temp1.val[1]);
            temp6.val[0] = vreinterpret_u16_u8(temp2.val[0]);
            temp6.val[1] = vreinterpret_u16_u8(temp2.val[1]);
            temp7.val[0] = vreinterpret_u16_u8(temp3.val[0]);
            temp7.val[1] = vreinterpret_u16_u8(temp3.val[1]);
            temp8.val[0] = vreinterpret_u16_u8(temp4.val[0]);
            temp8.val[1] = vreinterpret_u16_u8(temp4.val[1]);
            temp9 = vtrn_u16(temp6.val[0], temp5.val[0]);
            temp10 = vtrn_u16(temp6.val[1], temp5.val[1]);
            temp11 = vtrn_u16(temp8.val[0], temp7.val[0]);
            temp12 = vtrn_u16(temp8.val[1], temp7.val[1]);
            //step3 trn 1,5 2,6 3,7 4,8
            temp13.val[0] = vreinterpret_u32_u16(temp9.val[0]);
            temp13.val[1] = vreinterpret_u32_u16(temp9.val[1]);
            temp14.val[0] = vreinterpret_u32_u16(temp10.val[0]);
            temp14.val[1] = vreinterpret_u32_u16(temp10.val[1]);
            temp15.val[0] = vreinterpret_u32_u16(temp11.val[0]);
            temp15.val[1] = vreinterpret_u32_u16(temp11.val[1]);
            temp16.val[0] = vreinterpret_u32_u16(temp12.val[0]);
            temp16.val[1] = vreinterpret_u32_u16(temp12.val[1]);
            temp17 = vtrn_u32(temp15.val[0], temp13.val[0]);
            temp18 = vtrn_u32(temp15.val[1], temp13.val[1]);
            temp19 = vtrn_u32(temp16.val[0], temp14.val[0]);
            temp20 = vtrn_u32(temp16.val[1], temp14.val[1]);
            //step4 store bytes in correct position,the order now is 1,2,3,4,5,6,7,8
            temp1.val[0] = vreinterpret_u8_u32(temp17.val[0]);
            temp1.val[1] = vreinterpret_u8_u32(temp19.val[0]);
            temp2.val[0] = vreinterpret_u8_u32(temp18.val[0]);
            temp2.val[1] = vreinterpret_u8_u32(temp20.val[0]);
            temp3.val[0] = vreinterpret_u8_u32(temp17.val[1]);
            temp3.val[1] = vreinterpret_u8_u32(temp19.val[1]);
            temp4.val[0] = vreinterpret_u8_u32(temp18.val[1]);
            temp4.val[1] = vreinterpret_u8_u32(temp20.val[1]);
            vst1_u8(dstImg + j * height + height - i - 8, temp1.val[0]);
            vst1_u8(dstImg + (j + 1) * height + height - i - 8, temp1.val[1]);
            vst1_u8(dstImg + (j + 2) * height + height - i - 8, temp2.val[0]);
            vst1_u8(dstImg + (j + 3) * height + height - i - 8, temp2.val[1]);
            vst1_u8(dstImg + (j + 4) * height + height - i - 8, temp3.val[0]);
            vst1_u8(dstImg + (j + 5) * height + height - i - 8, temp3.val[1]);
            vst1_u8(dstImg + (j + 6) * height + height - i - 8, temp4.val[0]);
            vst1_u8(dstImg + (j + 7) * height + height - i - 8, temp4.val[1]);
        }
    }
}

void BPIYUVRotate::NeonRotateYUV420UV90(unsigned char* dstImg,
                                        unsigned char* srcImg, int width,
                                        int height, int pitch)
{
    uint8x8x4_t mat1; //use 2 register array to load a 8x8 patch
    uint8x8x4_t mat2;

    uint8x8x2_t temp1;
    uint8x8x2_t temp2;
    uint8x8x2_t temp3;
    uint8x8x2_t temp4;

    uint16x4x2_t temp5;
    uint16x4x2_t temp6;
    uint16x4x2_t temp7;
    uint16x4x2_t temp8;
    uint16x4x2_t temp9;
    uint16x4x2_t temp10;
    uint16x4x2_t temp11;
    uint16x4x2_t temp12;

    uint32x2x2_t temp13;
    uint32x2x2_t temp14;
    uint32x2x2_t temp15;
    uint32x2x2_t temp16;
    uint32x2x2_t temp17;
    uint32x2x2_t temp18;
    uint32x2x2_t temp19;
    uint32x2x2_t temp20;
    for (int i = 0; i < height; i += 8)
        {
        for (int j = 0; j < width; j += 8)
            {
            //step0 load 8x8 bytes in 8 registers
            mat1.val[0] = vld1_u8(srcImg + i * pitch + j);
            mat1.val[1] = vld1_u8(srcImg + (i + 1) * pitch + j);
            mat1.val[2] = vld1_u8(srcImg + (i + 2) * pitch + j);
            mat1.val[3] = vld1_u8(srcImg + (i + 3) * pitch + j);
            mat2.val[0] = vld1_u8(srcImg + (i + 4) * pitch + j);
            mat2.val[1] = vld1_u8(srcImg + (i + 5) * pitch + j);
            mat2.val[2] = vld1_u8(srcImg + (i + 6) * pitch + j);
            mat2.val[3] = vld1_u8(srcImg + (i + 7) * pitch + j);
            //step1 trn nearby registers
            temp1 = vtrn_u8(mat1.val[1], mat1.val[0]);
            temp2 = vtrn_u8(mat1.val[3], mat1.val[2]);
            temp3 = vtrn_u8(mat2.val[1], mat2.val[0]);
            temp4 = vtrn_u8(mat2.val[3], mat2.val[2]);
            //step2 trn 1,3 2,4 5,7 6,8
            temp5.val[0] = vreinterpret_u16_u8(temp1.val[0]);
            temp5.val[1] = vreinterpret_u16_u8(temp1.val[1]);
            temp6.val[0] = vreinterpret_u16_u8(temp2.val[0]);
            temp6.val[1] = vreinterpret_u16_u8(temp2.val[1]);
            temp7.val[0] = vreinterpret_u16_u8(temp3.val[0]);
            temp7.val[1] = vreinterpret_u16_u8(temp3.val[1]);
            temp8.val[0] = vreinterpret_u16_u8(temp4.val[0]);
            temp8.val[1] = vreinterpret_u16_u8(temp4.val[1]);
            temp9 = vtrn_u16(temp6.val[0], temp5.val[0]);
            temp10 = vtrn_u16(temp6.val[1], temp5.val[1]);
            temp11 = vtrn_u16(temp8.val[0], temp7.val[0]);
            temp12 = vtrn_u16(temp8.val[1], temp7.val[1]);
            //step3 trn 1,5 2,6 3,7 4,8
            temp13.val[0] = vreinterpret_u32_u16(temp9.val[0]);
            temp13.val[1] = vreinterpret_u32_u16(temp9.val[1]);
            temp14.val[0] = vreinterpret_u32_u16(temp10.val[0]);
            temp14.val[1] = vreinterpret_u32_u16(temp10.val[1]);
            temp15.val[0] = vreinterpret_u32_u16(temp11.val[0]);
            temp15.val[1] = vreinterpret_u32_u16(temp11.val[1]);
            temp16.val[0] = vreinterpret_u32_u16(temp12.val[0]);
            temp16.val[1] = vreinterpret_u32_u16(temp12.val[1]);
            temp17 = vtrn_u32(temp15.val[0], temp13.val[0]);
            temp18 = vtrn_u32(temp15.val[1], temp13.val[1]);
            temp19 = vtrn_u32(temp16.val[0], temp14.val[0]);
            temp20 = vtrn_u32(temp16.val[1], temp14.val[1]);
            //step4 store bytes in correct position,the order now is 1,2,3,4,5,6,7,8
            temp1.val[0] = vreinterpret_u8_u32(temp17.val[0]);
            temp1.val[1] = vreinterpret_u8_u32(temp19.val[0]);
            temp2.val[0] = vreinterpret_u8_u32(temp18.val[0]);
            temp2.val[1] = vreinterpret_u8_u32(temp20.val[0]);
            temp3.val[0] = vreinterpret_u8_u32(temp17.val[1]);
            temp3.val[1] = vreinterpret_u8_u32(temp19.val[1]);
            temp4.val[0] = vreinterpret_u8_u32(temp18.val[1]);
            temp4.val[1] = vreinterpret_u8_u32(temp20.val[1]);
            vst2_u8(dstImg + 2 * height * (j / 2 + 1) - 16 - 2 * i, temp1);
            vst2_u8(dstImg + 2 * height * (j / 2 + 2) - 16 - 2 * i, temp2);
            vst2_u8(dstImg + 2 * height * (j / 2 + 3) - 16 - 2 * i, temp3);
            vst2_u8(dstImg + 2 * height * (j / 2 + 4) - 16 - 2 * i, temp4);
        }
    }
}
