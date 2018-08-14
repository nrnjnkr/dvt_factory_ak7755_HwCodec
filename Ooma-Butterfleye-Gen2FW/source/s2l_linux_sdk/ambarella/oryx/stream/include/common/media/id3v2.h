/*******************************************************************************
 * id3v2.h
 *
 * History:
 *   2017年9月8日 - [ypchang] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
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
#ifndef ID3V2_H_
#define ID3V2_H_

#include <stdint.h>

struct ID3v2Header
{
    uint8_t id0;
    uint8_t id1;
    uint8_t id2;
    uint8_t ver_maj;
    uint8_t ver_rev;
    uint8_t flags;
    uint8_t size0;
    uint8_t size1;
    uint8_t size2;
    uint8_t size3;
    bool is_id3v2()
    {
      return ((id0 == 'I') && (id1 == 'D') && (id2 == '3'));
    }
    bool is_unsync()
    {
      return (flags & 0x80);
    }
    bool has_ext_header()
    {
      return (flags & 0x40);
    }
    bool is_experimental()
    {
      return (flags & 0x20);
    }
    bool has_footer()
    {
      return (flags & 0x10);
    }

    uint32_t tag_size() /* Including ID3v2Header size */
    {
      uint32_t size = (size0 << 21) | (size1 << 14) | (size2 << 7) | size3;
      return has_footer() ? (size + 20) : (size + 10);
    }
};

struct ID3v2ExtHeader
{
    uint8_t size0;
    uint8_t size1;
    uint8_t size2;
    uint8_t size3;
    uint8_t flags_number;
    uint8_t flags[];
    uint32_t ext_header_size()
    {
      return (uint32_t)((size0 << 21) | (size1 << 14) | (size2 << 7) | size3);
    }
};

#endif /* ID3V2_H_ */
