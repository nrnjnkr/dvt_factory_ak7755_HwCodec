/*
 * Author : Naresh K
 * Common functions used by mosquitto clients 
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mosquitto.h"
#include "client_shared.h"
#include "log.h"

void init_config(struct mosq_config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->port = 1883;    //Port need to change accordling 
    cfg->max_inflight = 20;  //
    cfg->keepalive = 60;   //Keep alive 
    cfg->clean_session = true;
    cfg->eol = true;
    cfg->protocol_version = MQTT_PROTOCOL_V31;
}


void client_config_cleanup(struct mosq_config *cfg)
{
    int i;
    free(cfg->id);
    free(cfg->id_prefix);
    free(cfg->host);
    free(cfg->file_input);
    free(cfg->message);
    free(cfg->topic);
    free(cfg->bind_address);
    free(cfg->username);
    free(cfg->password);
    free(cfg->will_topic);
	free(cfg->will_payload);
#ifdef WITH_TLS
    free(cfg->cafile);
    free(cfg->capath);
    free(cfg->certfile);
    free(cfg->keyfile);
    free(cfg->ciphers);
    free(cfg->tls_version);
#  ifdef WITH_TLS_PSK
    free(cfg->psk);
    free(cfg->psk_identity);
#  endif
#endif
    if(cfg->topics){
        for(i=0; i<cfg->topic_count; i++){
            free(cfg->topics[i]);
        }
        free(cfg->topics);
    }
    if(cfg->filter_outs){
        for(i=0; i<cfg->filter_out_count; i++){
            free(cfg->filter_outs[i]);
        }
        free(cfg->filter_outs);
    }
}


/*
 * Set options for mosquitto instance before going for connect options 
 * like username, password, will message, in flight number and etc.
 *
 */
int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg)
{
    int rc;
    if(cfg->will_topic && mosquitto_will_set(mosq, cfg->will_topic,
                            cfg->will_payloadlen, cfg->will_payload, 
                            cfg->will_qos, cfg->will_retain)) {
        if(!cfg->quiet) 
            gLOG_Log( kLOG_Error,"Problem setting will.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
	if(cfg->username && mosquitto_username_pw_set(mosq, 
                            cfg->username, cfg->password)) {
        if(!cfg->quiet) 
            gLOG_Log( kLOG_Error,"Problem setting username and password.\n");
        mosquitto_lib_cleanup();
        return 1;
	}
/*
 * configuration can be set if TLS library is enabled 
 */
#ifdef WITH_TLS
	if((cfg->cafile || cfg->capath) && mosquitto_tls_set(mosq, 
              cfg->cafile, cfg->capath, cfg->certfile, cfg->keyfile, NULL)){

        if(!cfg->quiet) 
            gLOG_Log( kLOG_Error,"Problem setting TLS options.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
    if(cfg->insecure && mosquitto_tls_insecure_set(mosq, true)){
        if(!cfg->quiet)
            gLOG_Log( kLOG_Error,"Problem setting TLS insecure option.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
#ifdef WITH_TLS_PSK
    if(cfg->psk && mosquitto_tls_psk_set(mosq, cfg->psk, 
                                cfg->psk_identity, NULL)){
        if(!cfg->quiet)
            gLOG_Log( kLOG_Error,"Problem setting TLS-PSK options.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
#endif
    if((cfg->tls_version || cfg->ciphers) && mosquitto_tls_opts_set(mosq, 1, cfg->tls_version, cfg->ciphers)){
        if(!cfg->quiet)
            gLOG_Log( kLOG_Error,"Problem setting TLS options.\n");
        mosquitto_lib_cleanup();
        return 1;
    }
#endif
    mosquitto_max_inflight_messages_set(mosq, cfg->max_inflight);
    mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(cfg->protocol_version));
    return MOSQ_ERR_SUCCESS;
}

/*
 * Generating id for publisher client used in transactions
 */
int client_id_generate(struct mosq_config *cfg, const char *id_base){
    int len;
    char hostname[256];

    if(cfg->id_prefix){
        cfg->id = malloc(strlen(cfg->id_prefix)+10);
        if(!cfg->id){
            if(!cfg->quiet) 
                gLOG_Log( kLOG_Error,"Out of memory.\n");
            mosquitto_lib_cleanup();
            return 1;
        }
        snprintf(cfg->id, strlen(cfg->id_prefix)+10, "%s%d", 
                cfg->id_prefix, getpid());
    }else if(!cfg->id){
        hostname[0] = '\0';
        gethostname(hostname, 256);
        hostname[255] = '\0';
        len = strlen(id_base) + strlen("|-") + 6 + strlen(hostname);
        cfg->id = malloc(len);
        if(!cfg->id){
            if(!cfg->quiet)
                gLOG_Log( kLOG_Error,"Out of memory.\n");
            mosquitto_lib_cleanup();
            return 1;
        }
        snprintf(cfg->id, len, "%s|%d-%s", id_base, getpid(), hostname);
        if(strlen(cfg->id) > MOSQ_MQTT_ID_MAX_LENGTH){
            /* Enforce maximum client id length of 23 characters */
            cfg->id[MOSQ_MQTT_ID_MAX_LENGTH] = '\0';
        }
    }
    return MOSQ_ERR_SUCCESS;
}

int client_connect(struct mosquitto *mosq, struct mosq_config *cfg){
	
	int rc;
/*
 *
 * mosq	        : a valid mosquitto instance.
 * host	        : the hostname or ip address of the broker to connect to.
 * port	        : the network port to connect to.  Usually 1883.
 * keepalive    : the number of seconds after which the broker should 
 * 		  send a PING message to the client if no other messages 
 *		  `have been exchanged in that time.
 * bind_address : the hostname or ip address of the local network interface 
 * 		  to bind to.
 *
 */
	gLOG_Log( kLOG_Info, "host : %s\n", cfg->host);
	strcpy(cfg->host, "localhost");
	gLOG_Log( kLOG_Info, "host : %s\n", cfg->host);
	gLOG_Log( kLOG_Info, "bind address : %s\n", cfg->bind_address);
	gLOG_Log( kLOG_Info, "port : %d\n", cfg->port);
	gLOG_Log( kLOG_Info, "keepalive : %d\n",cfg->keepalive);

	rc = mosquitto_connect_bind(mosq, cfg->host, cfg->port, 
                            cfg->keepalive, cfg->bind_address);
    if(rc>0){
        if(!cfg->quiet){
            gLOG_Log( kLOG_Error,"Unable to connect (%s).\n", mosquitto_strerror(rc));
        }
        mosquitto_lib_cleanup();
        return rc;
    }
    return MOSQ_ERR_SUCCESS;
}
