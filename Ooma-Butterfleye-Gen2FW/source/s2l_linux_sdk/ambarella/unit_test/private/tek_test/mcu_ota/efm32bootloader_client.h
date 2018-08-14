#ifndef XMODEM_CLIENT_H
#define XMODEM_CLIENT_H

#include <cstdio>
#include <string>
#include "qbytearray.h"

#include <pthread.h>

typedef enum {
    IDLE,
    WAIT_FOR_BL,   // send U, wait for ChipID...
    WAIT_FOR_NCG,  // send u, wait for NCG
    SEND_CHUNK,    // send SOT SEQ SEQ1 [..CHUNK..] CRCH CRCL, ignore input
                 // during this period
    WAIT_FOR_CHUNK_ACK,  // wait for ACK
    WAIT_FOR_EOT_ACK,    // send EOT, wait for ACK
    WAIT_FOR_CRC,        // check CRC of APP
    DONE_SUCEED,
    DONE_FAILED
} EFM32BLStatus;

class EFM32BootloaderClient {
   public:

    explicit EFM32BootloaderClient(const char* uart_path);
    ~EFM32BootloaderClient();

    bool request_send(const char* path);
    void slot_onReadyRead();

   private:
    int m_port;
    EFM32BLStatus m_status;

    std::FILE* m_file;

    int m_seq;
    int m_seq_count;

    bool m_needs_read;
    pthread_t m_thread_read;
    QByteArray m_pkt;

    void send_chunk();
};

#endif  // XMODEM_CLIENT_H
