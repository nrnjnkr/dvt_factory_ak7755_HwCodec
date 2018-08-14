/**
 * app/ipcam/fastboot_app/utils/adc_util.c
 *
 * Author: Caizhang Lin <czlin@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "config.h"
#include "adc.h"
#include "adc_util.h"
#include "upgrade_partition.h"
#include "upgrade_part_adc.h"

#define UPDATED_ADC_IMAGE_PATH	"/tmp/updated_adc.bin"

static int find_mtd_device_path(const char *dev_name, char *dev_info_buf,
                                int dev_info_bufsize)
{
    FILE *stream = 0;
    int device_index = -1;

    if (!dev_name || !dev_info_buf || 0 == dev_info_bufsize) {
        printf("Find mtd device path, NULL input\n");
        return -1;
    }
    memset(dev_info_buf, 0, dev_info_bufsize);

    //get mtd device index and path
    sprintf(dev_info_buf, "cat /proc/mtd | grep %s | cut -d':' -f1 | cut -d'd' -f2", dev_name);
    stream = popen(dev_info_buf , "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd  %s  failed.\n", dev_name);
        return -1;
    }
    fscanf(stream,"%d", &device_index);
    pclose(stream);

    if (device_index < 0) {
        printf("Not found %s partition on /proc/mtd\n", dev_name);
        return -1;
    }
    memset(dev_info_buf, 0, dev_info_bufsize);
    sprintf(dev_info_buf, "/dev/mtd%d", device_index);

    return 0;
}

static int load_adc_to_mem (unsigned int *adc_img_len,
                            unsigned int *adc_img_aligned_len, void **pp_adc_aligned_mem)
{
    char dev_info_buf[128];
    int ret = 0, count = 0;
    int ptb_fd = 0;
    unsigned int ptb_offset;
    unsigned char *ptb_buf = NULL;
    struct mtd_info_user ptb_meminfo;
    loff_t ptb_bad_offset;
    flpart_table_t *table;
    unsigned long long blockstart = 1;
    int fd = 0, bs, badblock = 0;
    struct mtd_info_user meminfo;
    unsigned long ofs;
    void *p_adc_mem_cur = NULL;

    if (!adc_img_len || !adc_img_aligned_len || !pp_adc_aligned_mem) {
        printf("Load ADC to mem, NULL input\n");
        ret = -1;
        goto closeall;
    }

    ret = find_mtd_device_path("ptb", dev_info_buf, sizeof(dev_info_buf));
    if (ret < 0) {
        printf("Find ptb partition failed\n");
        ret = -1;
        goto closeall;
    }

    /* Open the PTB device */
    if ((ptb_fd = open(dev_info_buf, O_RDONLY)) == -1) {
        perror("open PTB");
        ret = -1;
        goto closeall;
    }

    /* Fill in MTD device capability structure */
    if ((ret = ioctl(ptb_fd, MEMGETINFO, &ptb_meminfo)) != 0) {
        perror("PTB MEMGETINFO");
        ret = -1;
        goto closeall;
    }

    for (ptb_offset = 0; ptb_offset < ptb_meminfo.size; ptb_offset += ptb_meminfo.erasesize) {
        ptb_bad_offset = ptb_offset;
        if ((ret = ioctl(ptb_fd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
            perror("ioctl(MEMGETBADBLOCK)");
            goto closeall;
        }

        if (ret == 0) {
            break;
        }
    }
    if (ptb_offset >= ptb_meminfo.size) {
        printf("Can not find good block in PTB.\n");
        ret = -1;
        goto closeall;
    }

    ptb_buf = (unsigned char*)malloc(ptb_meminfo.erasesize);
    memset(ptb_buf, 0, ptb_meminfo.erasesize);

    /* Read partition table.
    * Note: we need to read and save the entire block data, because the
    * entire block will be erased when write partition table back to flash.
    * BTW, flpart_meta_t is located in the same block as flpart_table_t
    */
    count = ptb_meminfo.erasesize;
    if (pread(ptb_fd, ptb_buf, count, ptb_offset) != count) {
        perror("pread PTB");
        ret = -1;
        goto closeall;
    }

    table = PTB_TABLE(ptb_buf);
    *adc_img_len = table->part[PART_ADC].img_len;

    ret = find_mtd_device_path("adc", dev_info_buf, sizeof(dev_info_buf));
    if (ret < 0) {
        printf("Find ADC partition failed\n");
        ret = -1;
        goto closeall;
    }

    /* Open ADC device */
    if ((fd = open(dev_info_buf, O_RDONLY)) == -1) {
        perror("open mtd");
        ret = -1;
        goto closeall;
    }

    /* Fill in MTD device capability structure */
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
        perror("MEMGETINFO");
        ret = -1;
        goto closeall;
    }

    bs = meminfo.writesize;
    *adc_img_aligned_len = ((*adc_img_len)+(bs-1))&(~(bs-1));


    printf("ADC img addr=%u, img len=%u, img aligned_len=%u\n",
        table->part[PART_ADC].mem_addr, *adc_img_len, *adc_img_aligned_len);


    //will be freed when process return
    *pp_adc_aligned_mem = (void*)malloc(*adc_img_aligned_len);
    if (!(*pp_adc_aligned_mem)) {
        printf("Can not malloc memory for load ADC partiton!\n");
        ret = -1;
        goto closeall;
    }
    p_adc_mem_cur = *pp_adc_aligned_mem;

    /* Load the flash contents */
    for (ofs = 0; ofs < *adc_img_aligned_len ; ofs+=bs) {
        // new eraseblock , check for bad block
        if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
            blockstart = ofs & (~meminfo.erasesize + 1);
            if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
                perror("ioctl(MEMGETBADBLOCK)");
                ret = -1;
                goto closeall;
            }
        }

        if (badblock) {
            //memset (p_adc_mem_cur, 0xff, bs);
            continue;
        } else {
            /* Read page data and exit on failure */
            if (pread(fd, p_adc_mem_cur, bs, ofs) != bs) {
                perror("pread");
                ret = -1;
                goto closeall;
            }
        }
        p_adc_mem_cur= (char*)p_adc_mem_cur + bs;
    }

    /* Exit happy */
    ret = 0;

closeall:
    if (ptb_buf) {
        free(ptb_buf);
        ptb_buf  = NULL;
    }

    if (ptb_fd) {
        close(ptb_fd);
        ptb_fd = 0;
    }
    if (fd) {
        close(fd);
        fd = 0;
    }

    return ret;
}

static int update_adc_partition()
{
    char cmd[256];
    FILE   *stream = 0;
    int device_index = -1;

    memset(cmd, 0, sizeof(cmd));

    stream = popen( "cat /proc/mtd | grep adc | cut -d':' -f1 | cut -d'd' -f2", "r" );
    if (NULL == stream) {
        printf("Open /proc/mtd failed\n");
        return -1;
    }
    fscanf(stream, "%d", &device_index);
    pclose(stream);
    if (device_index < 0) {
        printf("Not found ADC partition on /proc/mtd\n");
        return -1;
    }
    //erase nand flash before re-write
    sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);

    system(cmd);
    if (upgrade_partition(device_index, UPDATED_ADC_IMAGE_PATH) != 0) {
        printf("upgrade_partition fail\n");
        return -1;
    }

    return 0;
}

/************************************************
* external functions of adc_util
*************************************************/
static unsigned int adc_img_len_s = 0;   // Lengh of image in the partition
static unsigned int adc_img_aligned_len_s = 0;  //aligned to page size of adc parttiton
static void* p_adc_aligned_mem_s = NULL;   //aligned to page size of adc parttiton
struct adcfw_header * hdr_s = NULL;

// load adc to mem, return mem addr
static int  adc_util_init()
{
    int rval = 0;
    do {
        rval = load_adc_to_mem (&adc_img_len_s, &adc_img_aligned_len_s, &p_adc_aligned_mem_s);
        if (rval < 0) {
            printf("Load ADC to memory failed\n");
            break;
        }

        hdr_s = (struct adcfw_header *)p_adc_aligned_mem_s;
        if (hdr_s->magic != ADCFW_IMG_MAGIC || hdr_s->fw_size != adc_img_len_s) {
            printf("Invalid ADC partition, magic=%u(should be %u), fw_size=%u(should be %u)\n",
                hdr_s->magic, ADCFW_IMG_MAGIC, hdr_s->fw_size, adc_img_len_s);
            rval = -1;
            break;
        }
    } while (0);
    return rval;
}

static void  adc_util_finish()
{
    if (p_adc_aligned_mem_s) {
        free(p_adc_aligned_mem_s);
        p_adc_aligned_mem_s  = NULL;
    }
    hdr_s= NULL;
}

int adc_util_update(struct amboot_params* params)
{
    int rval = -1;
    rval = adc_util_init();
    if (rval != 0){
        printf("adc_util_init failed!\n");
        return -1;
    }
    FILE *f_updated_adc = NULL;

    if (NULL == hdr_s) {
        printf("adc_util not initialized!\n");
        return -1;
    }

    do {
        memcpy(&(hdr_s->params_in_amboot), params, sizeof(hdr_s->params_in_amboot));
        f_updated_adc = fopen(UPDATED_ADC_IMAGE_PATH, "wb");
        if (f_updated_adc == NULL) {
            printf("Open /tmp/updated_adc.bin failed\n");
            rval = -EINVAL;
            break;
        }

        unsigned int length = fwrite(p_adc_aligned_mem_s, 1, adc_img_aligned_len_s, f_updated_adc);
        if (length != adc_img_aligned_len_s) {
            printf("/tmp/updated_adc.bin size [%d] is wrong, should be %u\n", length, adc_img_aligned_len_s);
            rval = -1;
            break;
        }

        fclose(f_updated_adc);
        f_updated_adc = NULL;

        rval = update_adc_partition();
        if (rval != 0) {
            printf("update_adc_partition fail\n");
            break;
        }
        rval = 0;
    } while (0);

    if (f_updated_adc) {
        fclose(f_updated_adc);
        f_updated_adc = NULL;
    }
    adc_util_finish();
    return rval;
}

int adc_util_dump()
{
    int rval = -1;
    rval = adc_util_init();
    if (rval != 0){
        printf("adc_util_init failed!\n");
        return -1;
    }

    if (NULL == hdr_s) {
        printf("adc_util not initialized!\n");
        return -1;
    }

    do {
        printf("adc_util_dump\n");
        printf("params_in_amboot.enable_audio = %d\n", hdr_s->params_in_amboot.enable_audio);
        printf("params_in_amboot.enable_ldc = %d\n", hdr_s->params_in_amboot.enable_ldc);
        printf("params_in_amboot.rotation_mode = %d\n", hdr_s->params_in_amboot.rotation_mode);
        printf("params_in_amboot.stream0_enable = %d\n", hdr_s->params_in_amboot.stream0_enable);

        if (1 == hdr_s->params_in_amboot.stream0_resolution) {
            printf("params_in_amboot.stream0_resolution = %d/720p\n", hdr_s->params_in_amboot.stream0_resolution);
        }
        else if(0 == hdr_s->params_in_amboot.stream0_resolution) {
            printf("params_in_amboot.stream0_resolution = %d/1080p\n", hdr_s->params_in_amboot.stream0_resolution);
        }
        else {
            printf("invalid params_in_amboot.stream0_resolution\n");
        }

        if (1 == hdr_s->params_in_amboot.stream0_fmt) {
            printf("params_in_amboot.stream0_fmt = %d/h264\n", hdr_s->params_in_amboot.stream0_fmt);
        } else {
            printf("invalid params_in_amboot.stream0_fmt\n");
        }

        printf("params_in_amboot.stream0_fps = %d\n", hdr_s->params_in_amboot.stream0_fps);
        printf("params_in_amboot.stream0_bitrate = %d\n", hdr_s->params_in_amboot.stream0_bitrate);
        printf("params_in_amboot.stream1_enable = %d\n", hdr_s->params_in_amboot.stream1_enable);

        if (2 == hdr_s->params_in_amboot.stream1_resolution) {
            printf("params_in_amboot.stream1_resolution = %d/480p\n", hdr_s->params_in_amboot.stream1_resolution);
        } else {
            printf("invalid params_in_amboot.stream1_resolution\n");
        }

        if (1 == hdr_s->params_in_amboot.stream1_fmt) {
            printf("params_in_amboot.stream1_fmt = %d/h264\n", hdr_s->params_in_amboot.stream1_fmt);
        } else {
            printf("invalid params_in_amboot.stream1_fmt\n");
        }

        printf("params_in_amboot.stream1_fps = %d\n", hdr_s->params_in_amboot.stream1_fps);
        printf("params_in_amboot.stream1_bitrate = %d\n", hdr_s->params_in_amboot.stream1_bitrate);
        printf("params_in_amboot.fastosd_string = %s\n", hdr_s->params_in_amboot.fastosd_string);
        printf("params_in_amboot.enable_vca = %d\n", hdr_s->params_in_amboot.enable_vca);
        printf("params_in_amboot.vca_frame_num = %d\n", hdr_s->params_in_amboot.vca_frame_num);
    } while (0);
    adc_util_finish();
    return rval;
}
