/*
 * Author : Naresh K
 * Common mosquitto client configuration header file
 *
 */
#include <stdio.h>
#include<stdbool.h>
#define MSGMODE_NONE 0
#define MSGMODE_CMD 1
#define MSGMODE_STDIN_LINE 2
#define MSGMODE_STDIN_FILE 3
#define MSGMODE_FILE 4
#define MSGMODE_NULL 5

#define CLIENT_PUB 1
#define CLIENT_SUB 2

int pub_or_sub ;
struct mosq_config {
    char *id;
    char *id_prefix;
    int protocol_version;
    int keepalive;
    char *host;
    int port;
    int qos;
    bool retain;
    int pub_mode; /* pub */
    char *file_input; /* pub */
    char *message; /* pub */
    long msglen; /* pub */
    char *topic; /* pub */
    char *bind_address;
#ifdef WITH_SRV
    bool use_srv;
#endif
    bool debug;
    bool quiet;
    unsigned int max_inflight;
    char *username;
    char *password;
    char *will_topic;
    char *will_payload;
    long will_payloadlen;
    int will_qos;
    bool will_retain;
#ifdef WITH_TLS
    char *cafile;
    char *capath;
    char *certfile;
    char *keyfile;
    char *ciphers;
    bool insecure;
    char *tls_version;
#ifdef WITH_TLS_PSK
    char *psk;
    char *psk_identity;
# endif
#endif
    bool clean_session; /* sub */
    char **topics; /* sub */
    int topic_count; /* sub */
    bool no_retain; /* sub */
    char **filter_outs; /* sub */
    int filter_out_count; /* sub */
    bool verbose; /* sub */
    bool eol; /* sub */
    int msg_count; /* sub */
#ifdef WITH_SOCKS
    char *socks5_host;
    int socks5_port;
    char *socks5_username;
    char *socks5_password;
#endif
};

void client_config_cleanup(struct mosq_config *cfg);
int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg);
int client_id_generate(struct mosq_config *cfg, const char *id_base);
int client_connect(struct mosquitto *mosq, struct mosq_config *cfg);
