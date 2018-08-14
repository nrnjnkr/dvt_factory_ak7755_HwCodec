/*******************************************************************************
 * test_mp4_fixer.cpp
 *
 * History:
 *   2015-12-15 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_mp4_file_parser_if.h"
int main(int argc, char*argv[])
{
  int ret = 0;
  std::string mp4_file_name;
  AMIMp4FileParser* mp4_file_parser = nullptr;
  do {
    if (argc != 2) {
      PRINTF("Please input cmd : test_mp4_parser mp4_file_name");
      break;
    }
    mp4_file_name = argv[1];
    NOTICE("Mp4 file name is %s", mp4_file_name.c_str());
    mp4_file_parser = AMIMp4FileParser::create(mp4_file_name.c_str());
    if(!mp4_file_parser) {
      ERROR("Failed to create mp4 file fixer.");
      ret = -1;
      break;
    }
    if(!mp4_file_parser->parse_file()) {
      ERROR("Failed to parse mp4 file.");
      ret = -1;
      break;
    }
    MovieBox *movie_box = mp4_file_parser->get_movie_box();
    if (!movie_box) {
      ERROR("movie box is invalid.");
      ret = -1;
      break;
    }
    DecodingTimeToSampleBox &audio_stts_box = movie_box->m_audio_track.m_media.\
        m_media_info.m_sample_table.m_stts;
    INFO("audio stts:");
    for (uint32_t i = 0; i < audio_stts_box.m_entry_count; ++ i) {
      INFO("%u : %u:%u", i, audio_stts_box.m_entry_ptr[i].sample_count,
           audio_stts_box.m_entry_ptr[i].sample_delta);
    }
  } while (0);
  mp4_file_parser->destroy();
  return ret;
}

