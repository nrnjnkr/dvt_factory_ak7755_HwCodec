#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

#include <libev/ev.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

#define CONF_CLOUD_IPC_PORT                 9999
#define CONF_CLOUD_DEV_PORT                 8888

#define KEY_WHAT                            "what"
#define KEY_IP                              "ip"
#define KEY_PORT                            "port"

#define LOGI(fmt, ...)      fprintf(stdout, "[I] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)      fprintf(stdout, "[D] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)      fprintf(stdout, "[E] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)      fprintf(stdout, "[W] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGV(fmt, ...)      fprintf(stdout, "[V] %s(%d): " fmt "\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)

typedef struct ev_loop loop_t, *loop_ptr_t;

/* msg */
// |--8--|--8--|-----16-----|
//
// |-reserved-|-catgory-|-detail-|

#define MSG_RESERVED_OFFSET             24
#define MSG_CAT_OFFSET                  16
#define MSG_VALUE_OFFSET                0

#define MSG_RESERVED_MASK               (0xFF<<MSG_RESERVED_OFFSET)
#define MSG_CAT_MASK                    (0xFF<<MSG_CAT_OFFSET)
#define MSG_VALUE_MASK                  (0xFFFF<<MSG_VALUE_OFFSET)

#define MSG_CAT(what)                   ( ((what) & MSG_CAT_MASK) >> MSG_CAT_OFFSET)
#define MSG_VALUE(what)                 ( ((what) & MSG_VALUE_MASK) >> MSG_VALUE_OFFSET)
#define MSG_WHAT(cat, value)            ((cat)<<MSG_CAT_OFFSET | (value)<<MSG_VALUE_OFFSET)
#define MSG_DELIMITER                   '\n'

enum {
    MSG_CAT_RESERVED                = 0x00,
    MSG_CAT_CLOUD                   = 0x01,
    MSG_CAT_DEVICE                  = 0x02,
    MSG_CAT_APP                     = 0x03,
    MSG_CAT_WEB                     = 0x04,

    MSG_CAT_DBG                     = 0xFE,
    MSG_CAT_SELF                    = 0xFF,
};

enum {
    MSG_DEVICE_HOME                 = MSG_WHAT(MSG_CAT_DEVICE, 0),
    MSG_DEVICE_LOGIN,
    MSG_DEVICE_LOGOUT,
    MSG_DEVICE_BATTERY,
    MSG_DEVICE_CHARGE,
    MSG_DEVICE_DC,
    MSG_DEVICE_PIR,
    MSG_DEVICE_ALARM,
    MSG_DEVICE_SHUTDOWN,
    MSG_DEVICE_STANDBY_TCP,
    MSG_DEVICE_STANDBY_UDP,
    MSG_DEVICE_WAKEUP_TCP,
    MSG_DEVICE_WAKEUP_UDP,
    MSG_DEVICE_STATUS_UPDATE,
    MSG_DEVICE_AGENT_CONNECTED,
    MSG_DEVICE_SYNC,
    MSG_DEVICE_KEEP_ALIVE_TCP,
    MSG_DEVICE_KEEP_ALIVE_UDP,
    MSG_DEVICE_END,

    MSG_APP_HOME                    = MSG_WHAT(MSG_CAT_APP, 0),
    MSG_APP_LOGIN,
    MSG_APP_LOGOUT,
    MSG_APP_GET_DEVICE_LIST,
    MSG_APP_GET_DEVICE_DETAIL,
    MSG_APP_GET_EVENT_LIST,
    MSG_APP_GET_EVENT_DETAIL,
    MSG_APP_END,

    MSG_CLOUD_HOME                  = MSG_WHAT(MSG_CAT_CLOUD, 0),
    MSG_CLOUD_END,

    MSG_DBG_HOME                    = MSG_WHAT(MSG_CAT_DBG, 0),
    MSG_DBG_CANNOT_CONNECT_CLOUD,
    MSG_DBG_DISCONNECT_DEVICE,
    MSG_DBG_END,
};


typedef struct {
    char        *param_ipc_ip;
    int         param_ipc_port;
    int         param_dev_port;
    int         ipc_sock;

    loop_ptr_t  p_loop;
    ev_io       stdin_watcher;
    ev_io       raw_sock_watcher;
} ctx_t, *ctx_ptr_t;

static int parse_param(ctx_t *p_ctx, int argc, char **argv);
static void print_param(ctx_t *p_ctx);

static void stdin_cb (EV_P_ ev_io *watcher, int revents);
static void raw_socket_cb(EV_P_ ev_io *watcher, int32_t revents);

int main(int argc, char **argv) {
    ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.param_ipc_ip = "127.0.0.1";
    ctx.param_ipc_port = CONF_CLOUD_IPC_PORT;
    ctx.param_dev_port = CONF_CLOUD_DEV_PORT;

    int ret = parse_param(&ctx, argc, argv);
    if (ret != 0) {
        goto LABEL_EXIT;
    }
    print_param(&ctx);

    int ipc_sock = -1;
    int raw_sock = -1;
    do {
        // init ipc_sock
        struct sockaddr_in srv_addr;
        bzero(&srv_addr, sizeof(srv_addr));

        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = htons(ctx.param_ipc_port);
        if(inet_pton(AF_INET, ctx.param_ipc_ip, &srv_addr.sin_addr) <= 0) {
            printf("invalid ip: %s", ctx.param_ipc_ip);
            break;
        }

        ipc_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (connect(ipc_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
            printf("connect cloud %s:%d failure", ctx.param_ipc_ip, ctx.param_ipc_port);
            break;
        }
        ctx.ipc_sock = ipc_sock;

        raw_sock = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP));
        if (raw_sock < 0) {
            printf("Permission denied\n\n!!!! Please run in administrator privileges\n");
            break;
        }

        int32_t flag = fcntl(raw_sock, F_GETFL);
        fcntl(raw_sock, F_SETFL, flag | O_NONBLOCK);

        ctx.p_loop = ev_loop_new(0);
        ev_init(&ctx.raw_sock_watcher, raw_socket_cb);
        ev_io_set(&ctx.raw_sock_watcher, raw_sock, EV_READ);
        ev_io_start(ctx.p_loop, &ctx.raw_sock_watcher);

        ev_init(&ctx.stdin_watcher, stdin_cb);
        ev_io_set(&ctx.stdin_watcher, STDIN_FILENO, EV_READ);
        ev_io_start(ctx.p_loop, &ctx.stdin_watcher);

        ev_run(ctx.p_loop, 0);
    } while (0);

LABEL_EXIT:
    if (ctx.p_loop) {
        ev_io_stop(ctx.p_loop, &ctx.stdin_watcher);
        ev_io_stop(ctx.p_loop, &ctx.raw_sock_watcher);

    }

    if (ipc_sock > 0) close(ipc_sock);
    if (raw_sock > 0) close(raw_sock);
    return 0;
}

static void usage() {
    const char* _usage[] = {
        "-i --ip      set cloud ip.       Default: 127.0.0.1\n"
        "-p --port    set cloud ipc port. Default: 9999\n",
        NULL
    };

    char **pp_str = (char**)&_usage[0];
    while (*pp_str != NULL) {
        printf("%s\n", *pp_str);
        pp_str++;
    }
}

static const char *short_options = "i:p:d:h";

#define NO_ARG              0
#define HAS_ARG             1

static struct option long_options[] = {
    {"ip",          HAS_ARG, 0,  'i'},
    {"port",        HAS_ARG, 0,  'p'},
    {"devport",     HAS_ARG, 0,  'd'},
    {0, 0, 0, 0}
};

static int parse_param(ctx_t *p_ctx, int argc, char **argv) {
    int ret = 0;
    int ch;
    int option_index = 0;

    while (!ret && (ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {
        case 'i': {
            p_ctx->param_ipc_ip = optarg;
        } break;
        case 'p': {
            p_ctx->param_ipc_port = atoi(optarg);
            if (p_ctx->param_ipc_port <= 0) {
                printf("invalid ipc port");
                ret = -1;
            }
        } break;
        case 'd': {
            p_ctx->param_dev_port = atoi(optarg);
            if (p_ctx->param_dev_port <= 0) {
                printf("invalid device port");
                ret = -1;
            }
        } break;
        default:
            ret = -1;
            break;
        }
    }

    if (ret) usage();

    return ret;
}

static void print_param(ctx_t *p_ctx) {
    printf("[wdog]\n");
    printf("ipc_server: %s:%d\n", p_ctx->param_ipc_ip, p_ctx->param_ipc_port);
    printf("\n");
    printf("Input \'q\' to exit\n\n");
}

static void stdin_cb (EV_P_ ev_io *watcher, int revents) {
    ctx_ptr_t p_ctx = (ctx_ptr_t)(((char*)watcher) - offsetof(ctx_t, stdin_watcher));

    char buf[2048];
    int32_t len = read(watcher->fd, buf, 2048);
    if (buf[0] == 'q') {
        printf("exit loop\n");
        ev_break (p_ctx->p_loop, EVBREAK_ALL);
    }
}

typedef struct {
    unsigned char   h_verlen;
    unsigned char   tos;
    unsigned short  total_len;
    unsigned short  ident;
    unsigned short  frag_and_flags;
    unsigned char   ttl;
    unsigned char   proto;
    unsigned short  checksum;
    unsigned int    sourceIP;
    unsigned int    destIP;
} ip_hdr_t, *ip_hdr_ptr_t;

typedef struct {
    unsigned short  th_sport;
    unsigned short  th_dport;
    unsigned int    th_seq;
    unsigned int    th_ack;
    unsigned char   th_lenres;
    unsigned char   th_flag;
    unsigned short  th_win;
    unsigned short  th_sum;
    unsigned short  th_urp;
} tcp_hdr_t, *tcp_hdr_ptr_t;

static void raw_socket_cb(EV_P_ ev_io *watcher, int32_t revents) {
    if (EV_ERROR & revents) {
        LOGE("revents:%x", revents);
        return ;
    }

    ctx_ptr_t p_ctx = (ctx_ptr_t)(((char*)watcher) - offsetof(ctx_t, raw_sock_watcher));

    char buf[2048];
    ssize_t len = read(watcher->fd, buf, 2048);

    if (len > 0) {
        ip_hdr_ptr_t p_ip_hdr = (ip_hdr_ptr_t)(buf);
        size_t ip_len = (p_ip_hdr->h_verlen&0x0f)*4;
        if (p_ip_hdr->proto == IPPROTO_TCP) {
            tcp_hdr_ptr_t p_tcp_hdr = (tcp_hdr_ptr_t)(buf +ip_len);
            //analyseTCP(ip,tcp);
            size_t tcp_len = ((p_tcp_hdr->th_lenres >> 4)& 0x0f) * 4;
            if (len <= ip_len + tcp_len + 1) {
                if(( (p_tcp_hdr->th_flag & 0x3F) == 0x10/*ACK*/) ||(((p_tcp_hdr->th_flag & 0x3F) == 0x18) && ((unsigned char)buf[len-1]=='\x2a')))
                {
                    struct in_addr addr;
                    addr.s_addr = p_ip_hdr->sourceIP;
                    char *ip = inet_ntoa(addr);
                    if (!ip) { ip = "0.0.0.0"; }
                    int port = ntohs(p_tcp_hdr->th_dport);

                    // int i = 0;
                    // printf("len: %d\n", len);
                    // for (i = 0; i < len; i++) {
                    //     printf("%02X ", (unsigned char)buf[i]);
                    //     if (i != 0 && ((i+1)%8 == 0)) printf("\n");
                    // }
                    // printf("\n\n");

                    if (port == p_ctx->param_dev_port) {
                        sprintf(buf, "{\"what\":%d, \"ip\":\"%s\", \"port\":%d}\n",
                                MSG_DEVICE_KEEP_ALIVE_TCP,
                                ip,
                                ntohs(p_tcp_hdr->th_sport));

                        int len = strlen(buf);
                        // write(STDIN_FILENO, buf, len);
                        write(p_ctx->ipc_sock, buf, len);
                    }
                }
                else
                {
                    // no match
                }
            } else {
                // FIN/RST should be handled here ?
            }
        }
    } else if (len == 0) {
        LOGW("EOF");
    } else {
        if (errno != EINTR && errno != EAGAIN) {
            LOGW("%d", errno);
            ev_break(p_ctx->p_loop, EVBREAK_ALL);
        }
    }
}
