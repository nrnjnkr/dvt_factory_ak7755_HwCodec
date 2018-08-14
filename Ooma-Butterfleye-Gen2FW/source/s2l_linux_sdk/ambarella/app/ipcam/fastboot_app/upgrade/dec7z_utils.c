/*
 * dec7z_utils.c
 *
 * History:
 *       2015/09/21 - [Jian Liu] created file
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "7z.h"
#include "7zFile.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zVersion.h"
#include "Utf2Str.h"
#include "dec7z_utils.h"

typedef struct _dec7z_context_t{
    CFileInStream m_archive_stream;
    ISzAlloc m_alloc_imp;
    ISzAlloc m_alloc_temp_imp;
    CLookToRead m_look_stream;
    CSzArEx m_db;
    int m_f_open;
    int m_ex_init;
}dec7z_context_t;

#define BUFFER_SIZE 256

void * dec7z_create(char *filename){
    if(!filename){
        return (void*)NULL;
    }
    dec7z_context_t *context = (dec7z_context_t*)malloc(sizeof(dec7z_context_t));
    if(!context){
        return (void*)NULL;
    }
    memset(context,0,sizeof(dec7z_context_t));
    do {
        if (InFile_Open(&context->m_archive_stream.file, filename)) {
            printf("Can't open this compressed files: %s, 7z file is invalid!\n",filename);
            fflush(stdout);
            break;
        }

        context->m_f_open = 1;
        context->m_alloc_imp.Alloc = SzAlloc;
        context->m_alloc_imp.Free = SzFree;
        context->m_alloc_temp_imp.Alloc = SzAllocTemp;
        context->m_alloc_temp_imp.Free = SzFreeTemp;

        FileInStream_CreateVTable(&context->m_archive_stream);
        LookToRead_CreateVTable(&context->m_look_stream, 0);

        context->m_look_stream.realStream = &context->m_archive_stream.s;
        LookToRead_Init(&context->m_look_stream);

        CrcGenerateTable();

        SzArEx_Init(&context->m_db);
        context->m_ex_init = 1;
        SRes res = SzArEx_Open(&context->m_db, &context->m_look_stream.s, &context->m_alloc_imp, &context->m_alloc_temp_imp);
        if (res != 0) {
            printf("SzArEx_Open return error.\n");
            fflush(stdout);
            break;
        }
        return (void*)context;
    } while (0);
    dec7z_destroy((void*)context);
    return (void*)NULL;
}

void dec7z_destroy(void *handle){
    dec7z_context_t *context = (dec7z_context_t*)handle;
    if(context){
        if (context->m_ex_init) {
            SzArEx_Free(&context->m_db, &context->m_alloc_imp);
            context->m_ex_init = 0;
        }

        if (context->m_f_open) {
            File_Close(&context->m_archive_stream.file);
            context->m_f_open = 0;
        }
        free(context);
    }
}

int dec7z_is_file_exist(void *handle,char *filename){
    dec7z_context_t *context = (dec7z_context_t*)handle;
    if(!handle){
        return 0;
    }
    if(!filename){
        return 0;
    }

    SRes res = SZ_OK;
    UInt16 *temp = NULL;
    size_t tempSize = 0;
    int i;

    for (i = 0; i < context->m_db.NumFiles; i ++) {
        UInt32 isDir = SzArEx_IsDir(&context->m_db, i);
        size_t len = SzArEx_GetFileNameUtf16(&context->m_db, i, NULL);
        if (len > tempSize) {
            SzFree(NULL, temp);
             tempSize = len;
             temp = (UInt16 *) SzAlloc(NULL, tempSize * sizeof(temp[0]));
             if (temp == 0) {
                 res = SZ_ERROR_MEM;
                 break;
             }
        }
        //file name
        SzArEx_GetFileNameUtf16(&context->m_db, i, temp);

        if (!isDir) {
            CBuf buf;
            Buf_Init(&buf);
            res = Utf16_To_Char(&buf, temp);
            if (res == SZ_OK) {
                //printf("%s\n", (const char *)buf.data);
                if(!strcmp((char*)buf.data, filename)){
                    Buf_Free(&buf, &g_Alloc);
                    return 1;
                }
            }
            Buf_Free(&buf, &g_Alloc);
        }
    }
    return 0;
}

static int create_dir(char *dir){
    int res = mkdir(dir, 0777) == 0 ? 0 : errno;
    if (!res || res == EEXIST) {
        return 0;
    }
    return -1;
}

int dec7z_dec(void *handle, char *dst_dir){
    dec7z_context_t *context = (dec7z_context_t*)handle;
    if(!handle){
        return -1;
    }
    if(!dst_dir){
        return -1;
    }

    SRes res = SZ_OK;
    UInt16 *temp = NULL;
    size_t tempSize = 0;

    /* it can have any value before first call (if outBuffer = 0) */
    UInt32 blockIndex = 0xFFFFFFFF;
    /* it must be 0 before first call for each new archive. */
    Byte *outBuffer = 0;
    /* it can have any value before first call (if outBuffer = 0) */
    size_t outBufferSize = 0;

    do {
        if (access(dst_dir, F_OK)) {
            if (!create_dir(dst_dir)) {
                printf("Failed to create dir %s\n", dst_dir);
            }
        }

        int i;
        for (i = 0; i < context->m_db.NumFiles; i ++) {
            size_t offset = 0;
            size_t outSizeProcessed = 0;
            UInt32 isDir = SzArEx_IsDir(&context->m_db, i);
            size_t len = SzArEx_GetFileNameUtf16(&context->m_db, i, NULL);
            if (len > tempSize) {
                SzFree(NULL, temp);
                tempSize = len;
                temp = (UInt16 *) SzAlloc(NULL, tempSize * sizeof(temp[0]));
                if (temp == 0) {
                    res = SZ_ERROR_MEM;
                    break;
                }
            }
            SzArEx_GetFileNameUtf16(&context->m_db, i, temp);

            if (isDir)
                continue;
            if (!isDir) {
                res = SzArEx_Extract(&context->m_db,
                             &context->m_look_stream.s,
                             i,
                             &blockIndex,
                             &outBuffer,
                             &outBufferSize,
                             &offset,
                             &outSizeProcessed,
                             &context->m_alloc_imp,
                             &context->m_alloc_temp_imp);
                if (res != SZ_OK){
                  break;
                }
            }

            //Extract
            CSzFile outFile;
            size_t processedSize;
            size_t j;
            UInt16 *name = (UInt16 *) temp;
            for (j = 0; name[j] != 0; j ++){
                if (name[j] == '/') {
                    name[j] = 0;
                    char fullname[BUFFER_SIZE] = "";
                    strcpy(fullname, dst_dir);
                    apend_utf16_to_Cstring(fullname, name);
                    if (0 != (res = create_dir(fullname))) {
                        printf("can not mkdir %s, return %d.\n", fullname, res);
                        res = SZ_ERROR_FAIL;
                        break;
                    }
                    name[j] = CHAR_PATH_SEPARATOR;
                }
            }

              if (res != SZ_OK) break;

              if (isDir) {
                   char fullpath[BUFFER_SIZE] = "";
                   strcpy(fullpath, dst_dir);
                   apend_utf16_to_Cstring(fullpath, name);
                   if (0 != (res = create_dir(fullpath))) {
                       printf("can not mkdir %s, return %d.\n", fullpath, res);
                       res = SZ_ERROR_FAIL;
                       break;
                   }
                   continue;
               } else {
                   char fullname[BUFFER_SIZE] = "";
                   strcpy(fullname, dst_dir);
                   apend_utf16_to_Cstring(fullname, name);
                   if(OutFile_Open(&outFile, fullname)) {
                       printf("can not open output file %s\n", fullname);
                       res = SZ_ERROR_FAIL;
                       break;
                  }
              }
              processedSize = outSizeProcessed;
              if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed) {
                  printf("can not write output file\n");
                  res = SZ_ERROR_FAIL;
                  break;
              }
              if (File_Close(&outFile)) {
                  printf("can not close output file\n");
                  res = SZ_ERROR_FAIL;
                  break;
              }
          }

          IAlloc_Free(&context->m_alloc_imp, outBuffer);
          SzFree(NULL, temp);

          if (res == SZ_OK) {
              printf("\nExtract 7z file successfully!\n");
              return 0;
          } else if (res == SZ_ERROR_UNSUPPORTED) {
              printf("decoder doesn't support this archive");
          } else if (res == SZ_ERROR_MEM) {
              printf("can not allocate memory");
          } else if (res == SZ_ERROR_CRC) {
              printf("CRC error");
          } else {
              printf("\nERROR #%d\n", res);
          }
      } while (0);
      return -1;
}

