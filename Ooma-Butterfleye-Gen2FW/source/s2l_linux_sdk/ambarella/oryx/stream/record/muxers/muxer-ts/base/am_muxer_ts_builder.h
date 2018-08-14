/*
 *am_muxer_ts_builder.h
 *
 * History:
 *    2011/4/17 - [Kaiming Xie] created file
 *    2014/12/16 - [Chengcai Jing] modified file
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
 */
#ifndef __AM_MUXER_TS_BUILDER_H__
#define __AM_MUXER_TS_BUILDER_H__

#include "am_muxer_codec_info.h"
#include "mpeg_ts_defs.h"
#include "am_audio_define.h"
#include "am_video_types.h"

/******
  Defines
 ******/

#define PAT_HDR_SIZE            (8)
#define PAT_ELEMENT_SIZE        (4)
#define PMT_HDR_SIZE            (12)
#define PMT_ELEMENT_SIZE        (5)
#define PMT_STREAM_DSC_SIZE     (3)

#define PES_HEADER_LEN          (9)
#define ADAPT_HEADER_LEN         (2)

#define CTSMUXPSI_STREAM_TOT     (2) //audio & video

#define PCR_FIELD_SIZE		(6)
#define PTS_FIELD_SIZE		(5)
#define DTS_FIELD_SIZE		(5)


/**********
  Typedefs
 **********/

// Stream Descriptor
struct AM_TS_MUXER_PSI_VER{
   uint32_t pat  :5; /*!< version number for pat table. */
   uint32_t pmt  :5; /*!< version number for pmt table. */
   AM_TS_MUXER_PSI_VER():
   pat(0),
   pmt(0)
   {}
};


/**
 *
 * Stream configuration
 */
struct AM_TS_MUXER_PES_PAYLOAD_INFO{
    int64_t  pts          = 0;
    int64_t  dts          = 0;
    int64_t  pcr_base     = 0;
    uint8_t *p_payload    = nullptr;
    int32_t  payload_size = 0;
    uint16_t pcr_ext      = 0;
    uint8_t  first_frame  = 0;
    uint8_t  first_slice  = 0;
    uint8_t  with_pcr     = 0;
};

/**
 *
 * Stream configuration
 */
struct AM_TS_MUXER_PSI_STREAM_INFO{
   uint8_t            *descriptor     = nullptr;
   MPEG_SI_STREAM_TYPE type           = MPEG_SI_STREAM_TYPE_RESERVED_0;
   uint32_t            descriptor_len = 0;
   uint16_t            pid            = 0; /*!< Packet ID to be used in the TS. */
   uint8_t             descriptor_tag = 0;
};

/**
 *
 * Program configuration.
 */
struct AM_TS_MUXER_PSI_PRG_INFO{
   uint16_t pid_pmt = 0; /*!< Packet ID to be used to store the Program Map Table [PMT],13 bit. */
   uint16_t pid_pcr = 0; /*!< Packet ID to be used to store the Program Clock Referene [PCR]. */
   uint16_t prg_num = 0; /*!< Program Number to be used in Program Map Table [PMT]. */
};

/**
 *
 * Program Map Table [PMT] configuration.
 */
struct AM_TS_MUXER_PSI_PMT_INFO{
    AM_TS_MUXER_PSI_PRG_INFO     *prg_info   = nullptr;
    uint8_t                      *descriptor = nullptr;
    /*!< List of stream configurations. */
    AM_TS_MUXER_PSI_STREAM_INFO  *stream[CTSMUXPSI_STREAM_TOT] = {nullptr};
    uint32_t                      descriptor_len = 0;
   /*!< Total number of stream within the program. */
   uint16_t                       total_stream   = 0;
   uint8_t                        descriptor_tag = 0;
};

/**
 *
 * Program Association table [PAT] configuration
 */
struct AM_TS_MUXER_PSI_PAT_INFO{
    AM_TS_MUXER_PSI_PRG_INFO *prg_info  = nullptr; /*!< List of program configurations. */
    uint16_t                  total_prg = 0; /*!< Total number of valid programs. */
};


/**
 *
 * Header for a Program Association table [PAT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
struct PAT_HDR{
#ifdef BIGENDIAN
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 5*/
   uint8_t reserved1                : 2;
   uint8_t version_number           : 5;
   uint8_t current_next_indicator   : 1;
   /*Btype 4*/
   uint8_t transport_stream_id_l    : 8;
   /*Btype 3*/
   uint8_t transport_stream_id_h    : 8;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 1*/
   uint8_t section_syntax_indicator : 1;
   uint8_t b0                       : 1;
   uint8_t reserved0                : 2;
   uint8_t section_length8to11      : 4;
   /*Btype 0*/
   uint8_t table_id                 : 8;
   PAT_HDR() :
     last_section_number(0),
     section_number(0),
     reserved1(0),
     version_number(0),
     current_next_indicator(0),
     transport_stream_id_l(0),
     transport_stream_id_h(0),
     section_length0to7(0),
     section_syntax_indicator(0),
     b0(0),
     reserved0(0),
     section_length8to11(0),
     table_id(0)
   {}
#else
   /*Btype 0*/
   uint8_t table_id                 : 8;
   /*Btype 1*/
   uint8_t section_length8to11      : 4;
   uint8_t reserved0                : 2;
   uint8_t b0                       : 1;
   uint8_t section_syntax_indicator : 1;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 3*/
   uint8_t transport_stream_id_h    : 8;
   /*Btype 4*/
   uint8_t transport_stream_id_l    : 8;
   /*Btype 5*/
   uint8_t current_next_indicator   : 1;
   uint8_t version_number           : 5;
   uint8_t reserved1                : 2;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   PAT_HDR():
     table_id(0),
     section_length8to11(0),
     reserved0(0),
     b0(0),
     section_syntax_indicator(0),
     section_length0to7(0),
     transport_stream_id_h(0),
     transport_stream_id_l(0),
     current_next_indicator(0),
     version_number(0),
     reserved1(0),
     section_number(0),
     last_section_number(0)
   {}
#endif
};

/**
 *
 * Program Association table [PAT] Element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
struct PAT_ELEMENT{
#ifdef BIGENDIAN
   /* Btype 3 */
   uint8_t program_map_pid_l : 8;
   /* Btype 2 */
   uint8_t reserved2         : 3;
   uint8_t program_map_pid_h : 5;
   /* Btype 1 */
   uint8_t program_number_l  : 8;
   /* Btype 0 */
   uint8_t program_number_h  : 8;
   PAT_ELEMENT():
     program_map_pid_l(0),
     reserved2(0),
     program_map_pid_h(0),
     program_number_l(0),
     program_number_h(0)
   {}
#else
   /* Btype 0 */
   uint8_t program_number_h  : 8;
   /* Btype 1 */
   uint8_t program_number_l  : 8;
   /* Btype 2 */
   uint8_t program_map_pid_h : 5;
   uint8_t reserved2         : 3;
   /* Btype 3 */
   uint8_t program_map_pid_l : 8;
   PAT_ELEMENT():
     program_number_h(0),
     program_number_l(0),
     program_map_pid_h(0),
     reserved2(0),
     program_map_pid_l(0)
   {}
#endif
};
/**
 *
 * Header for a Program Map Table [PMT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
struct PMT_HDR{
#ifdef BIGENDIAN
   /*Btype 11*/
   uint8_t program_info_length0to7  : 8;
   /*Btype 10*/
   uint8_t reserved3                : 4;
   uint8_t program_info_length8to11 : 4;
   /*Btype 9*/
   uint8_t pcr_pid0to7              : 8;
   /*Btype 8*/
   uint8_t reserved2                : 3;
   uint8_t pcr_pid8to12             : 5;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 5*/
   uint8_t reserved1                : 2;
   uint8_t version_number           : 5;
   uint8_t current_next_indicator   : 1;
   /*Btype 4*/
   uint8_t program_number_l         : 8;
   /*Btype 3*/
   uint8_t program_number_h         : 8;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 1*/
   uint8_t section_syntax_indicator : 1;
   uint8_t b0                       : 1;
   uint8_t reserved0                : 2;
   uint8_t section_length8to11      : 4;
   /*Btype 0*/
   uint8_t table_id                 : 8;
   PMT_HDR():
     program_info_length0to7(0),
     reserved3(0),
     program_info_length8to11(0),
     pcr_pid0to7(0),
     reserved2(0),
     pcr_pid8to12(0),
     last_section_number(0),
     section_number(0),
     reserved1(0),
     version_number(0),
     current_next_indicator(0),
     program_number_l(0),
     program_number_h(0),
     section_length0to7(0),
     section_syntax_indicator(0),
     b0(0),
     reserved0(0),
     section_length8to11(0),
     table_id(0)
   {}
#else
   /*Btype 0*/
   uint8_t table_id                 : 8;
   /*Btype 1*/
   uint8_t section_length8to11      : 4;
   uint8_t reserved0                : 2;
   uint8_t b0                       : 1;
   uint8_t section_syntax_indicator : 1;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 3*/
   uint8_t program_number_h         : 8;
   /*Btype 4*/
   uint8_t program_number_l         : 8;
   /*Btype 5*/
   uint8_t current_next_indicator   : 1;
   uint8_t version_number           : 5;
   uint8_t reserved1                : 2;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 8*/
   uint8_t pcr_pid8to12             : 5;
   uint8_t reserved2                : 3;
   /*Btype 9*/
   uint8_t pcr_pid0to7              : 8;
   /*Btype 10*/
   uint8_t program_info_length8to11 : 4;
   uint8_t reserved3                : 4;
   /*Btype 11*/
   uint8_t program_info_length0to7  : 8;
   PMT_HDR():
     table_id(0),
     section_length8to11(0),
     reserved0(0),
     b0(0),
     section_syntax_indicator(0),
     section_length0to7(0),
     program_number_h(0),
     program_number_l(0),
     current_next_indicator(0),
     version_number(0),
     reserved1(0),
     section_number(0),
     last_section_number(0),
     pcr_pid8to12(0),
     reserved2(0),
     pcr_pid0to7(0),
     program_info_length8to11(0),
     reserved3(0),
     program_info_length0to7(0)
   {}
#endif
};

/**
 *
 * Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
struct PMT_ELEMENT{
#ifdef BIGENDIAN
   /*Btype 4*/
   uint8_t es_info_length_l    : 8;
   /*Btype 3*/
   uint8_t reserved1           : 4;
   uint8_t es_info_length_h    : 4;
   /*Btype 2*/
   uint8_t elementary_pid0to7  : 8;
   /*Btype 1*/
   uint8_t reserved0           : 3;
   uint8_t elementary_pid8to12 : 5;
   /*Btype 0*/
   uint8_t stream_type         : 8;
   PMT_ELEMENT():
     es_info_length_l(0),
     reserved1(0),
     es_info_length_h(0),
     elementary_pid0to7(0),
     reserved0(0),
     elementary_pid8to12(0),
     stream_type(0)
   {}
#else
   /*Btype 0*/
   uint8_t stream_type         : 8;
   /*Btype 1*/
   uint8_t elementary_pid8to12 : 5;
   uint8_t reserved0           : 3;
   /*Btype 2*/
   uint8_t elementary_pid0to7  : 8;
   /*Btype 3*/
   uint8_t es_info_length_h    : 4;
   uint8_t reserved1           : 4;
   /*Btype 4*/
   uint8_t ES_info_length_l    : 8;
   PMT_ELEMENT():
     stream_type(0),
     elementary_pid8to12(0),
     reserved0(0),
     elementary_pid0to7(0),
     es_info_length_h(0),
     reserved1(0),
     ES_info_length_l(0)
   {}
#endif
};
/**
 *
 * Stream Descriptor for Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
struct PMT_STREAM_DSC{
   uint32_t descriptor_tag:8;
   uint32_t descriptor_length:8;
   uint32_t component_tag:8;
   PMT_STREAM_DSC():
     descriptor_tag(0),
     descriptor_length(0),
     component_tag(0)
   {}
};

class AMTsBuilder
{
  public:
    AMTsBuilder();
    ~AMTsBuilder(){};
    void destroy();

    /**
     * This method creates TS packet with Program Association table [PAT].
     *
     * @param pat_info informaion required for PAT table.
     * @param pat_buf  pointer to the destination of newly created PAT.
     *
     */
    AM_STATE create_pat(AM_TS_MUXER_PSI_PAT_INFO *pat_info, uint8_t * pat_buf);

    /**
     * This method creates TS packet with Program Map Table [PMT].
     *
     * @param pmt_info informaion required for PAT table.
     * @param pmt_buf  pointer to the destination of newly created PMT.
     *
     */
    AM_STATE create_pmt(AM_TS_MUXER_PSI_PMT_INFO *pmt_info, uint8_t * pmt_buf);

    /**
     * This method creates TS packet with Program Clock Reference packet [PCR].
     *
     * @param pcr_pid pid to be used for PCR packet.
     * @param pcr_buf  pointer to the destination of newly created PCR packet.
     *
     */
    AM_STATE create_pcr(uint16_t pcr_pid, uint8_t * pcr_buf);

    /**
     * This method creates TS nullptr packet.
     *
     * @param buf  pointer to the destination of newly created nullptr packet.
     *
     */
    AM_STATE create_null(uint8_t * buf);

    int  create_transport_packet(AM_TS_MUXER_PSI_STREAM_INFO *psi_str_info,
                               AM_TS_MUXER_PES_PAYLOAD_INFO* pes_payload_info,
                               uint8_t *pes_buf);

    AM_STATE update_psi_cc(uint8_t * ts_buf); //continuity_counter increment

    int cal_crc32(uint8_t *buf, int size);
    void crc32_byte(int *preg, int x);
    void set_audio_info(AM_AUDIO_INFO* audio_info);
    void set_video_info(AM_VIDEO_INFO* video_info);

  private:
    AM_TS_MUXER_PSI_VER m_ver;  /*!< version info for psi tables. */
    AM_AUDIO_INFO       m_audio_info;
    AM_VIDEO_INFO       m_video_info;
};

#endif
