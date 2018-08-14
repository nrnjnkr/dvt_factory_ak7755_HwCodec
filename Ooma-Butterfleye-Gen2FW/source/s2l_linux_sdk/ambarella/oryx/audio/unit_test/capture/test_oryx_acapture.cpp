/*******************************************************************************
 * test_oryx_acapture.cpp
 *
 * History:
 *   Jan 5, 2017 - [ypchang] created file
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_file.h"
#include "wav.h"

#include <getopt.h>
#include <signal.h>
#include <atomic>

#include "am_audio_utility.h"
#include "am_audio_capture_if.h"


class AudioCaptureTest
{
  public:
    static AudioCaptureTest* create(std::string &interface, std::string &name)
    {
      AudioCaptureTest *result = new AudioCaptureTest();
      if (AM_UNLIKELY(result && !result->init(interface, name))) {
        delete result;
        result = nullptr;
      }

      return result;
    }
    void destroy()
    {
      delete this;
    }

  public:
    bool start(std::string &out_file)
    {
      bool ret = false;

      do {
        m_file.set_file_name(out_file.c_str());
        if (AM_LIKELY(m_file.exists())) {
          if (AM_UNLIKELY(!m_file.open(AMFile::AM_FILE_CLEARWRITE))) {
            ERROR("Failed to open file %s for writing!", out_file.c_str());
            break;
          }
        } else if (AM_UNLIKELY(!m_file.open(AMFile::AM_FILE_CREATE))) {
          ERROR("Failed to open file %s for writing!", out_file.c_str());
          break;
        }
        ret = m_audio_capture->start();
        m_run = ret;
      }while(0);

      return ret;
    }

    bool stop()
    {
      bool ret = true;

      if (AM_LIKELY(m_run)) {
        uint32_t total_size = sizeof(WavRiffHdr) +
                              sizeof(WavFmt) +
                              sizeof(WavDataHdr) + m_data_size - 8;
        m_run = false;
        ret = m_audio_capture->stop();
        do {
          if (AM_UNLIKELY(!m_file.seek(sizeof(uint32_t),
                                       AMFile::AM_FILE_SEEK_SET))) {
            break;
          }
          if (AM_UNLIKELY(m_file.write(&total_size, sizeof(total_size)) < 0)) {
            ERROR("Failed to write file size!");
            break;
          }
          if (AM_UNLIKELY(!m_file.seek(sizeof(WavRiffHdr) +
                                       sizeof(WavFmt) +
                                       sizeof(uint32_t),
                                       AMFile::AM_FILE_SEEK_SET))) {
            break;
          }
          if (AM_UNLIKELY(m_file.write(&m_data_size,
                                       sizeof(m_data_size)) < 0)) {
            ERROR("Failed to write DATA chunk size!");
            break;
          }
        }while(0);
        m_file.close(true);
      }

      return ret;
    }

    void run()
    {
      bool prev = m_run.load();
      while(m_run) {
        usleep(1000000);
      }
      m_run = prev; /* Make sure stop() can be called */
    }

    bool set_audio_parameters(uint32_t channel,
                              uint32_t samplerate,
                              uint32_t chunksize,
                              AM_AUDIO_SAMPLE_FORMAT sampleformat)
    {
      return (m_audio_capture->set_channel(channel) &&
              m_audio_capture->set_sample_rate(samplerate) &&
              m_audio_capture->set_chunk_bytes(chunksize) &&
              m_audio_capture->set_sample_format(sampleformat));
    }

  private:
    static void sigstop(int sig_num)
    {
      m_run = false;
    }

    static void static_audio_capture(AudioCapture *data)
    {
      return ((AudioCaptureTest*)data->owner)->audio_capture(&data->packet);
    }
    void audio_capture(AudioPacket *packet)
    {
      if (AM_LIKELY(m_file.is_open())) {
        if (AM_UNLIKELY(m_first_write)) {
          uint8_t wav_header[sizeof(WavRiffHdr) + sizeof(WavFmt) +
                             sizeof(WavDataHdr)] = {0};
          WavRiffHdr *riff_hdr = (WavRiffHdr*)wav_header;
          WavFmt     *fmt      = (WavFmt*)&wav_header[sizeof(WavRiffHdr)];
          WavDataHdr *data_hdr = (WavDataHdr*)&wav_header[sizeof(WavRiffHdr) +
                                                          sizeof(WavFmt)];

          riff_hdr->chunk_id   = WAV_RIFF;
          riff_hdr->chunk_size = 0; /* FileSize - 8 */
          riff_hdr->type       = WAV_WAVE;
          fmt->fmt_header.chunk_id      = WAV_FMT;
          fmt->fmt_header.chunk_size    = sizeof(WavFmtBody);
          fmt->fmt_body.channels        = m_audio_capture->get_channel();
          fmt->fmt_body.sample_rate     = m_audio_capture->get_sample_rate();
          fmt->fmt_body.bits_per_sample = m_audio_capture->get_sample_size()*8;
          switch(m_audio_capture->get_sample_format()) {
            case AM_SAMPLE_U8:
            case AM_SAMPLE_S16LE:
            case AM_SAMPLE_S16BE:
            case AM_SAMPLE_S24LE:
            case AM_SAMPLE_S24BE:
            case AM_SAMPLE_S24_32LE:
            case AM_SAMPLE_S24_32BE:
            case AM_SAMPLE_S32LE:
            case AM_SAMPLE_S32BE:
              fmt->fmt_body.audio_fmt       = 1;
              break;
            case AM_SAMPLE_F32LE:
            case AM_SAMPLE_F32BE:
              fmt->fmt_body.audio_fmt       = 3;
              break;
            case AM_SAMPLE_ALAW:
              fmt->fmt_body.audio_fmt       = 6;
              break;
            case AM_SAMPLE_ULAW:
              fmt->fmt_body.audio_fmt       = 7;
              break;
            default:
              fmt->fmt_body.bits_per_sample = 0;
              fmt->fmt_body.audio_fmt       = 0;
              break;
          }
          fmt->fmt_body.block_align =
                            m_audio_capture->get_sample_physical_size() *
                            fmt->fmt_body.channels;
          fmt->fmt_body.byte_rate   = fmt->fmt_body.sample_rate *
                                      fmt->fmt_body.block_align;
          data_hdr->chunk_id = WAV_DATA;
          data_hdr->chunk_size = 0;
          if (AM_UNLIKELY(m_file.write_reliable((const void*)wav_header,
                                                (sizeof(WavRiffHdr) +
                                                 sizeof(WavFmt) +
                                                 sizeof(WavDataHdr)))) < 0) {
            ERROR("Failed to write WAV file header!");
            m_run = false;
          }
          m_first_write = false;

        }
        if (AM_LIKELY(m_run && packet && packet->data &&
                      (packet->length > 0))) {
          ssize_t ret = m_file.write_reliable((const void*)packet->data,
                                              packet->length);
          if (ret < 0) {
            m_run = false;
            ERROR("Failed to write!");
          } else {
            m_data_size += ret;
          }
        }
      }
    }

  private:
    AudioCaptureTest(){}
    virtual ~AudioCaptureTest()
    {
      stop();
      AM_DESTROY(m_audio_capture);
    }
    bool init(std::string &interface, std::string &name)
    {
      bool ret = false;

      do {
        signal(SIGINT,  AudioCaptureTest::sigstop);
        signal(SIGQUIT, AudioCaptureTest::sigstop);
        signal(SIGTERM, AudioCaptureTest::sigstop);
        m_audio_capture = create_audio_capture(interface,
                                               name,
                                               this,
                                               static_audio_capture);
        if (AM_UNLIKELY(!m_audio_capture)) {
          ERROR("Failed to create audio capture with interface [%s]",
                interface.c_str());
          break;
        }

        ret = true;
      }while(0);

      return ret;
    }

  private:
    AMIAudioCapture         *m_audio_capture = nullptr;
    uint32_t                 m_data_size     = 0;
    std::atomic<bool>        m_first_write   = {true};
    AMFile                   m_file;
    static std::atomic<bool> m_run;
};
std::atomic<bool> AudioCaptureTest::m_run = {false};

struct hint {
    const char *args;
    const char *help;
};

void usage(option *options, hint *hints)
{
  option *opt = options;
  hint   *hts = hints;
  while(opt && opt->name) {
    if (isalpha(opt->val)) {
      printf("-%c, ", opt->val);
    } else {
      printf("    ");
    }
    printf("--%s ", opt->name);
    printf("%s",    hts->args);
    printf("%s\n",  hts->help);
    ++ opt;
    ++ hts;
  }
}

int main(int argc, char *argv[])
{
  bool        need_break = (argc == 1);
  uint32_t       channel = 2;
  uint32_t   sample_rate = 48000;
  uint32_t   chunk_bytes = 1024;
  AM_AUDIO_SAMPLE_FORMAT sample_fmt = AM_SAMPLE_S16LE;
  AudioCaptureTest *test = nullptr;

  char short_options[] = ":hi:c:r:t:k:f:";
  struct option long_options[] =
  {
    {"help",       no_argument,       nullptr, 'h'},
    {"interface",  required_argument, nullptr, 'i'},
    {"channel",    required_argument, nullptr, 'c'},
    {"samplerate", required_argument, nullptr, 'r'},
    {"sample fmt", required_argument, nullptr, 't'},
    {"chunksize",  required_argument, nullptr, 'k'},
    {"output",     required_argument, nullptr, 'f'},
    {0, 0, 0, 0}
  };
  struct hint hints[] =
  {
    {"",                       "\n          Print this help message"          },
    {"pulse | raw | alsa",     "\n          pulseaudio, raw or alsa interface"},
    {"1 | 2",                  "\n          1 means mono, 2 means stereo"     },
    {"8000 | 16000 | 48000",   "\n          16K or 48K sample rate"           },
    {"u8, s16[lb]e, s24[lb]e", "\n          audio sample format"              },
    {"1024 | 2048 ...",        "\n          data chunk block size"            },
    {"output file name",       "\n          output file name"                 },
  };

  do {
    int val = -1;
    std::string name(basename(argv[0]));
    std::string interface;
    std::string output;

    if (AM_UNLIKELY(need_break)) {
      usage(long_options, hints);
    }
    while(!need_break && ((val = getopt_long(argc,
                                             argv,
                                             short_options,
                                             long_options,
                                             nullptr)) != -1)) {
      switch(val) {
        case 'i': {
          interface = std::string(optarg);
          if ((interface != std::string("pulse")) &&
              (interface != std::string("raw")) &&
              (interface != std::string("alsa"))) {
            ERROR("Invalid interface: %s", optarg);
            usage(long_options, hints);
            need_break = true;
          }
        }break;
        case 'c': {
          uint32_t ch = strtoul(optarg, nullptr, 10);
          switch(ch) {
            case 1:
            case 2: channel = ch; break;
            default: {
              ERROR("Invalid argument: -%c %s", val, optarg);
              need_break = true;
            }break;
          }
        }break;
        case 'r': {
          uint32_t rate = strtoul(optarg, nullptr, 10);
          switch(rate) {
            case 8000:
            case 16000:
            case 48000:
            case 96000: sample_rate = rate; break;
            default: {
              ERROR("Invalid argument: -%c %s", val, optarg);
              need_break = true;
            }break;
          }
        }break;
        case 't': {
          if (is_str_equal(optarg, "s16le")) {
            sample_fmt = AM_SAMPLE_S16LE;
          } else if (is_str_equal(optarg, "s16be")) {
            sample_fmt = AM_SAMPLE_S16BE;
          } else if (is_str_equal(optarg, "s24le")) {
            sample_fmt = AM_SAMPLE_S24LE;
          } else if (is_str_equal(optarg, "s24be")) {
            sample_fmt = AM_SAMPLE_S24BE;
          } else if (is_str_equal(optarg, "s24_32le")) {
            sample_fmt = AM_SAMPLE_S24_32LE;
          } else if (is_str_equal(optarg, "s24_32be")) {
            sample_fmt = AM_SAMPLE_S24_32BE;
          } else if (is_str_equal(optarg, "s32le")) {
            sample_fmt = AM_SAMPLE_S32LE;
          } else if (is_str_equal(optarg, "s32be")) {
            sample_fmt = AM_SAMPLE_S32BE;
          } else if (is_str_equal(optarg, "f32le")) {
            sample_fmt = AM_SAMPLE_F32LE;
          } else if (is_str_equal(optarg, "f32be")) {
            sample_fmt = AM_SAMPLE_F32BE;
          } else if (is_str_equal(optarg, "u8")) {
            sample_fmt = AM_SAMPLE_U8;
          }
        }break;
        case 'k': {
          chunk_bytes = strtoul(optarg, nullptr, 10);
          if (chunk_bytes > 51200) {
            ERROR("Invalid arguments: -%c %s, value is too large", val, optarg);
            need_break = true;
          }
        }break;
        case 'f': {
          output = std::string(optarg);
        }break;
        case ':': {
          ERROR("Options -%c requires an argument!", optopt);
          need_break = true;
        }break;
        case 'h':
        default: {
          usage(long_options, hints);
          need_break = true;
        }break;
      }
    }

    if (AM_UNLIKELY(!need_break && interface.empty())) {
      ERROR("Test interface is not specified!");
      usage(long_options, hints);
      need_break = true;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }

    if (AM_LIKELY(output.empty())) {
      output = interface + std::string("_") +
               std::to_string(sample_rate / 1000) + std::string("k") +
               std::string("_") +
               (channel == 1 ? std::string("mono") : std::string("stereo")) +
               std::string("_") +
               std::string(smp_fmt_to_str(sample_fmt)) +
               std::string(".wav");
    }

    PRINTF("Test Audio Capture:"
           "\n         Interface: %s"
           "\n    Channel Number: %u"
           "\n       Sample Rate: %u"
           "\n     Sample Format: %s"
           "\n       Chunk Bytes: %u"
           "\n       Output File: %s",
           interface.c_str(),
           channel,
           sample_rate,
           smp_fmt_to_str(sample_fmt),
           chunk_bytes,
           output.c_str());

    test = AudioCaptureTest::create(interface, name);
    if (AM_UNLIKELY(!test)) {
      ERROR("Failed to create AudioCaptureTest object!");
      break;
    }
    if (AM_UNLIKELY(!test->set_audio_parameters(channel,
                                                sample_rate,
                                                chunk_bytes,
                                                sample_fmt))) {
      ERROR("Failed to set audio parameters!");
      break;
    }
    need_break = test->start(output);
    test->run(); /* Block Here if start OK */
    test->stop();
  }while(0);

  if (AM_LIKELY(test)) {
    test->destroy();
  }

  return need_break ? -1 : 0;
}
