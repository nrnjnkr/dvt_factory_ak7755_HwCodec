 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void __print_memory_non_zero(char *p, int width, int pitch, int height)
{
    int i = 0, j = 0;
    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            if (p[i]) {
                printf("%02x ", p[i]);
            } else {
                printf("   ", p[i]);
            }
        }
        printf("\n");
        p += pitch;
    }
}

static char *load_bin_to_mem(char *filename)
{
    if (!filename) {
        printf("error: NULL params\n");
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("error: open(%s) fail\n", filename);
        return NULL;
    }

    fseek(file, 0x0, SEEK_END);
    long filesize = ftell(file);

    char *p = (char *)malloc(filesize);
    if (!p) {
        printf("error: not enough memory, request size %ld\n", filesize);
        return NULL;
    }

    fseek(file, 0x0, SEEK_SET);
    fread(p, 1, filesize, file);

    return p;
}

typedef struct {
    unsigned short offset;
    unsigned char width;
    unsigned char rev;
} s_font_map_index;

static s_font_map_index *read_font_index(char *p_font_index, char c)
{
    int index_offset = c << 2;
    return (s_font_map_index *)(p_font_index + index_offset);
}

int main(int argc, char **argv)
{
    char *p_font_index = load_bin_to_mem((char *)"font_index.bin");
    char *p_font_map = load_bin_to_mem((char *)"font_map.bin");

    int preset_pitch = 16 * 32;
    int preset_height = 32;

    if (!p_font_index || !p_font_map) {
        printf("no memory?\n");
        return (-1);
    }

    char *p_str = (char *)"0123456789:/- ";
    int str_length = strlen(p_str);

    if (argc > 1) {
        p_str = argv[1];
        str_length = strlen(p_str);
    }

    s_font_map_index *font_index;
    for (int i = 0; i < str_length; i ++) {
        font_index = read_font_index(p_font_index, p_str[i]);
        printf("p_str[i] %c, offset %d, width %d\n", p_str[i], font_index->offset, font_index->width);
        __print_memory_non_zero(p_font_map + font_index->offset, font_index->width, preset_pitch, preset_height);
    }

    free(p_font_index);
    free(p_font_map);

    return 0;
}

