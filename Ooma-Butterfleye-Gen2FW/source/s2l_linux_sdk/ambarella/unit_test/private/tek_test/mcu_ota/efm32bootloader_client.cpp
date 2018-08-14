#include "efm32bootloader_client.h"

#ifndef QT_VERSION
#include "qbytearray.h"
#endif
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#define qDebug(fmt, args...) \
    printf("%s:%d " fmt "\r\n", __FILE__, __LINE__, ##args)

#define XMODEM_SOH 1
#define XMODEM_EOT 4
#define XMODEM_ACK 6
#define XMODEM_NAK 21
#define XMODEM_CAN 24
#define XMODEM_NCG 67

#define XMODEM_CHUNK_SIZE 128

#define UART_PORT_BAUDRATE B115200
#define UART_PORT_FLOWCTRL 0

#define NOTIFY_STATUS(status)                                       \
    ({                                                              \
        switch (status) {                                           \
            case IDLE:               \
                qDebug("[status] IDLE");                            \
                break;                                              \
            case WAIT_FOR_BL:        \
                qDebug("[status] WAIT_FOR_BL");                     \
                break;                                              \
            case WAIT_FOR_NCG:       \
                qDebug("[status] WAIT_FOR_NCG");                    \
                break;                                              \
            case SEND_CHUNK:         \
            case WAIT_FOR_CHUNK_ACK: \
                break;                                              \
            case WAIT_FOR_EOT_ACK:   \
                qDebug("[status] WAIT_FOR_EOT_ACK");                \
                break;                                              \
            case WAIT_FOR_CRC:       \
                qDebug("[status] WAIT_FOR_CRC");                    \
                break;                                              \
            case DONE_SUCEED:        \
                qDebug("[status] DONE_SUCEED");                     \
                break;                                              \
            case DONE_FAILED:        \
                qDebug("[status] DONE_FAILED");                     \
                break;                                              \
            default:                                                \
                qDebug("[status] unknown");                         \
                break;                                              \
        }                                                           \
    })

static int uart_open(const char* port) {
    int fd;
    struct termios options;
    fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        goto ERR;
    }

    tcflush(fd, TCIOFLUSH);
    if (tcgetattr(fd, &options) < 0) {
        goto ERR;
    }

    // use raw mode
    cfmakeraw(&options);
    options.c_cflag |= CLOCAL;
    // enable/disable RTS/CTS
    if (UART_PORT_FLOWCTRL) {
        options.c_cflag |= CRTSCTS;
    } else {
        options.c_cflag &= ~CRTSCTS;
    }

    cfsetospeed(&options, UART_PORT_BAUDRATE);
    cfsetispeed(&options, UART_PORT_BAUDRATE);

    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        goto ERR;
    }
    tcflush(fd, TCIOFLUSH);

    return fd;
ERR:
    close(fd);
    return -1;
}

static ssize_t uart_write(int fd, const char* data, size_t len) {
    return ::write(fd, data, len);
}

EFM32BootloaderClient::EFM32BootloaderClient(const char* uart_path)
    : m_port(-1),
      m_status(IDLE),
      m_seq(1),
      m_seq_count(0),
      m_needs_read(false),
      m_pkt(256) {
    m_port = uart_open(uart_path);
}

EFM32BootloaderClient::~EFM32BootloaderClient() {
    qDebug("~EFM32BootloaderClient");

    if (m_port != -1) ::close(m_port);

    if (m_file != NULL) std::fclose(m_file);
}

typedef struct {
    EFM32BootloaderClient* client;
    int fd;
    bool* needs_read;
} read_args_t;

static void *uart_read_func(void *p) {
    read_args_t *arg = (read_args_t *) p;

    struct pollfd pfd;
    pfd.fd = arg->fd;
    pfd.events = POLLIN | POLLPRI;

    while (*arg->needs_read) {
        if (poll(&pfd, 1, 200) > 0) {
            arg->client->slot_onReadyRead();
        }
    }
    return NULL;
}

bool EFM32BootloaderClient::request_send(const char* path) {
    if (m_status != IDLE) {
        qDebug("invalid state %d", m_status);
        return false;
    }

    if (m_port < 0) {
        qDebug("SerialPort open failed");
        return false;
    }

    {
        struct stat fs;
        if (stat(path, &fs) != 0) {
            qDebug("fstat failed!");
            return false;
        }
        qDebug("fs.st_size = %ld", fs.st_size);

        m_seq_count =
            (fs.st_size + (XMODEM_CHUNK_SIZE - 1)) / XMODEM_CHUNK_SIZE;
        qDebug("total chunk count is %d", m_seq_count);
    }

    m_file = std::fopen(path, "rb");
    if (m_file == NULL) {
        qDebug("File open failed");
        return false;
    }

    m_needs_read = true;
    read_args_t arg;
    arg.client = this;
    arg.fd = m_port;
    arg.needs_read = &m_needs_read;
    pthread_create(&m_thread_read, NULL, uart_read_func, &arg);

    m_status = WAIT_FOR_BL;
    NOTIFY_STATUS(m_status);

    const char *ch = "@@bl@@U";
    uart_write(m_port, ch, strlen(ch));

    pthread_join(m_thread_read, NULL);
    return m_status == DONE_SUCEED;
}

static int calc_crc16(const char* ptr, int count, int crc = 0) {
    char i;
    while (--count >= 0) {
        crc = crc ^ (int)*ptr++ << 8;
        i = 8;
        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return (crc);
}

void EFM32BootloaderClient::send_chunk() {
    m_status = SEND_CHUNK;
    NOTIFY_STATUS(m_status);

    char head[3];
    char buf[XMODEM_CHUNK_SIZE];
    size_t n;

    if ((n = std::fread(buf, 1, sizeof(buf), m_file)) > 0) {
        head[0] = XMODEM_SOH;
        head[1] = m_seq & 0xFF;
        head[2] = uint8_t(0xFF) - uint8_t(head[1]);
        uart_write(m_port, head, 3);

        if (n < sizeof(buf)) {
            memset(buf + n, 0xFF, sizeof(buf) - n);
        }
        uart_write(m_port, buf, sizeof(buf));

        int crc16 = calc_crc16(buf, sizeof(buf));
        head[0] = (crc16 >> 8) & 0xFF;
        head[1] = crc16 & 0xFF;

        m_status = WAIT_FOR_CHUNK_ACK;
        NOTIFY_STATUS(m_status);

        uart_write(m_port, head, 2);
    }
}

void EFM32BootloaderClient::slot_onReadyRead() {
    char buf[64];
    ssize_t n = ::read(m_port, buf, sizeof(buf));
    if (n < 1) {
        return;
    }
    m_pkt.append(buf, n);

    bool pkt_handled = false;

    if (m_status == WAIT_FOR_BL) {
        if (m_pkt.contains("ChipID:") || m_pkt.contains("\r\n?\r\n")) {
            m_status = WAIT_FOR_NCG;
            NOTIFY_STATUS(m_status);

            const char *ch = "@@bl@@u";
            uart_write(m_port, ch, strlen(ch));
            pkt_handled = true;
        }
    } else if (m_status == WAIT_FOR_NCG) {
        char ncg = XMODEM_NCG;
        if (m_pkt.endsWith(ncg)) {
            send_chunk();  // first_chunk = yes
            pkt_handled = true;
        }
    } else if (m_status == WAIT_FOR_CHUNK_ACK) {
        char ack = XMODEM_ACK;
        char nak = XMODEM_NAK;
        if (m_pkt.contains(ack)) {
            if (m_seq == m_seq_count) {
                m_status = WAIT_FOR_EOT_ACK;
                NOTIFY_STATUS(m_status);

                char ch = XMODEM_EOT;
                uart_write(m_port, &ch, 1);
            } else {
                ++m_seq;
                send_chunk();
            }
            pkt_handled = true;
        } else if (m_pkt.contains(nak)) {
            qDebug("re-send seq %d", m_seq);
            send_chunk();
            pkt_handled = true;
        }
    } else if (m_status == WAIT_FOR_EOT_ACK) {
        char ack = XMODEM_ACK;
        char nak = XMODEM_NAK;

        if (m_pkt.contains(ack)) {
            m_status = WAIT_FOR_CRC;
            NOTIFY_STATUS(m_status);

            const char *ch = "@@bl@@c";
            uart_write(m_port, ch, strlen(ch));
            pkt_handled = true;
        } else if (m_pkt.contains(nak)) {
            qDebug("got NAK after EOT ?");
            m_status = DONE_FAILED;
            NOTIFY_STATUS(m_status);
            // emit signal_result(false);
            m_needs_read = false;

            pkt_handled = true;
        }
    } else if (m_status == WAIT_FOR_CRC) {
        //        QRegExp expr("[0-9A-Z]{8}");
        //        QString crc_str = QString::fromLatin1(m_pkt.constData(),
        //        m_pkt.size());

        //        int n = crc_str.indexOf(expr);
        int n = m_pkt.indexOfFourHexValues();
        if (n != -1) {
            qDebug("found valid crc string: %.*s", 8, m_pkt.constData() + n);

            m_status = DONE_SUCEED;
            NOTIFY_STATUS(m_status);
            // emit signal_result(ok);
            m_needs_read = false;

            pkt_handled = true;
        }
    }

    if (!pkt_handled) {
        qDebug("un-handled resp: %s", m_pkt.toHex().data());
    } else {
        m_pkt.clear();
    }
}
