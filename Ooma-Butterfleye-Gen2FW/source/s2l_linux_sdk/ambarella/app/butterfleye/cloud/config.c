#include<errno.h>
#include<client_shared.h>
#include <mosquitto.h>

#include <stdlib.h>                                                             
#include <string.h>                                                             
#include <netdb.h>                                                              
                    
#include <sys/socket.h>                                                         
#include <netinet/in.h>                                                         
#include <arpa/inet.h>    

#include "log.h"

void trim_token(char *token) {
    int len = 0;
    if(token != NULL) {
        len = strlen(token);
        if(token[len-1] == '\n')
            token[len-1] = '\0';
    }
}

void setconfigure(char *token, struct mosq_config *cfg) {
    if(token[0] == "=") //Skip the comment 
        return;

    if(!strcmp(token, "port")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            gLOG_Log( kLOG_Debug, "token : %s\n", token);
            cfg->port = atoi(token);
            gLOG_Log( kLOG_Debug, "port : %d\n", cfg->port);
            /* 
             * TO-DO : Check appropriate port number and update the code
             */
            if(cfg->port<1 || cfg->port>65535)
                gLOG_Log( kLOG_Warn, "Invalid port given: %d\n", cfg->port);
        } else
            gLOG_Log( kLOG_Debug, "Port number is not mentioned in \
                        configuration file\n");
    }else if (!strcmp(token, "bind_address")) {
        token = strtok(NULL, "=");
        if(token != NULL){
            trim_token(token);
            cfg->bind_address = strdup(token);
        }
        else
            gLOG_Log( kLOG_Warn, "Bind address is not specified in \
                    configuration file");
#ifdef WITH_TLS
    } else if (!strcmp(token, "cafile")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->cafile = strdup(token);
        }
        else
            gLOG_Log( kLOG_Warn, "cafile is not specified in \
                    configuration file");
    } else if (!strcmp(token, "capath")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->capath = strdup(token);
        }
        else
            gLOG_Log( kLOG_Warn, "capath is not specified in \
                    configuration file");
    } else if(!strcmp(token, "certfile")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->certfile = strdup(token);
        }
        else 
            gLOG_Log( kLOG_Warn, "certfile is not specified in \
                    configuration file");
    } else if (!strcmp(token, "ciphers")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->ciphers = strdup(token);
        }
        else 
            gLOG_Log( kLOG_Warn, "certfile is not specified in \
                    configuration file");
#endif
    } else if (!strcmp(token, "msg_count")) {
		 if(pub_or_sub == CLIENT_PUB);
		 else  {
			token = strtok(NULL, "=");
			if(token != NULL) {
                trim_token(token);
				cfg->msg_count = atoi(token);
				if(cfg->msg_count < 1) 
					gLOG_Log( kLOG_Warn, "Invalid message count \"%d\".",
                            cfg->msg_count);
			}
			else 
				gLOG_Log( kLOG_Warn, "message count is not specified in \
                        configuration file");
		}
	} else if (!strcmp(token, "debug")) {
		token = strtok(NULL, "=");
        trim_token(token);
		if(token == NULL || !strcmp(token, "false"))
			cfg->debug = false;
		else 
			cfg->debug = true;
	} else if (!strcmp(token, "file_input")) {
		if(pub_or_sub == CLIENT_SUB); 
		else {
			token = strtok(NULL, "=");
			if(token != NULL) {
                trim_token(token);
				cfg->pub_mode = MSGMODE_FILE;
				cfg->file_input = strdup(token);
			} else
				gLOG_Log( kLOG_Warn, "file input is not mentioned\n");
		}
	} else if (!strcmp(token, "host")) {
		token = strtok(NULL, "=");
		if(token != NULL) {
                trim_token(token);
                cfg->host = strdup(token);
        }
		else 
			gLOG_Log( kLOG_Warn, "host address is not specified in \
                    configurations\n");
#ifdef WITH_TLS
	} else if (!strcmp(token, "insecure")) {
		token = strtok(NULL, "=");
        trim_token(token);
		if(token == NULL || !strcmp(token, "false"))
			cfg->insecure = false;
		else if(!strcmp(token, "true"))
			cfg->insecure = true;
#endif
	} else if (!strcmp(token, "id")) {
		token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
			cfg->id = strdup(token);
        } else
            gLOG_Log( kLOG_Warn, "Id is not specified in the configuration\n");
	} else if (!strcmp(token, "id_prefix")) {
        token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->id_prefix = strdup(token);
        }
		else 
            gLOG_Log( kLOG_Warn, "id_prefix not specified in the \
                    configuration");
	} else if (!strcmp(token, "keepalive")) {
		token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->keepalive = atoi(token);
            if(cfg->keepalive > 65535) 
                gLOG_Log( kLOG_Warn, "Invalid keepalive given\n");
        } else
            gLOG_Log( kLOG_Warn, "Keepalive not specified in the \
                    configuration");
#ifdef WITH_TLS 
	} else if (!strcmp(token, "keyfile")) {
        token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->keyfile = strdup(token);
        }
		else
            gLOG_Log( kLOG_Warn, "key is not specified in the \
                    configuration\n");
#endif
	} else if (!strcmp(token, "message")) {
        if(pub_or_sub == CLIENT_SUB); 
        else {
            token = strtok(NULL, "=");
            if(token != NULL){
                trim_token(token);
                cfg->message = strdup(token);
                cfg->msglen = strlen(cfg->message);
                cfg->pub_mode = MSGMODE_CMD;
            } else 
                gLOG_Log( kLOG_Warn, "message is not specified in the \
                        configuration\n");
        }
    } else if (!strcmp(token, "max_inflight")) {
		token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->max_inflight = atoi(token);
        } else
            gLOG_Log( kLOG_Warn, "Max inflight is empty\n");
	} else if (!strcmp(token, "protocol_version")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL) 
            gLOG_Log( kLOG_Warn, "MQTT protocol version not specified in \
                    the configuration\n");
        else if(!strcmp(token, "mqttv31")) 
            cfg->protocol_version = MQTT_PROTOCOL_V31;
        else if(!strcmp(token, "mqttv311")) 
            cfg->protocol_version = MQTT_PROTOCOL_V311;
#ifdef WITH_SOCKS
	} else if (!strcmp(token, "--proxy")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL)
            gLOG_Log( kLOG_Warn, "Proxy is not specified in the \
                    configuration\n");
        else
            gLOG_Log( kLOG_Warn, "TO-DO: Need to handle \n");
			//TO-DO: Need to handle 
#endif
#ifdef WITH_TLS_PSK
	} else if (!strcmp(token, "psk")) {
		token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->psk = strdup(token);
        } else
            gLOG_Log( kLOG_Warn, "psk is not specified in the \
                    configuration\n");
	} else if (!strcmp(token, "psk-identity")) {
        token = strtok(NULL, "=");
		if(token != NULL) {
            trim_token(token);
            cfg->psk_identity = strdup(token);		
        }
		else
            gLOG_Log( kLOG_Warn, "psk identity is not specified in the \
                    configuration\n");
#endif
	} else if (!strcmp(token, "qos")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->qos = atoi(token);
            if((cfg->qos < 0) || (cfg->qos > 2))
                gLOG_Log( kLOG_Warn, "Invalid QoS given\n");
        } else
            gLOG_Log( kLOG_Warn, "qos is not specified in the configuration");
	} else if (!strcmp(token, "quiet")) {
		token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL || !strcmp(token, "false"))
            cfg->quiet = false;
        else 
            cfg->quiet = true;
	} else if (!strcmp(token, "retain")) {
        if(pub_or_sub == CLIENT_SUB); 
        else {
            token = strtok(NULL, "=");
            trim_token(token);
            if(token == NULL || !strcmp(token, "false"))
                cfg->retain = 0;
            else if (!strcmp(token, "true"))
                cfg->retain = 1;
            else
                gLOG_Log( kLOG_Warn, "Invalid valied for retain given in \
                        the configuration");
        }
#ifdef WITH_SRV
	} else if (!strcmp(token, "use_service")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL || !strcmp(token, "false"))
            cfg->use_srv = false;
        else  if(!strcmp(token, "true"))
            cfg->use_srv = true;
        else 
            gLOG_Log( kLOG_Warn, "Invalid value provided for user_service \
                    in the configuration\n");
#endif
	} else if (!strcmp(token, "topic")) {
		token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL)
            gLOG_Log( kLOG_Warn, "Topic is not specified in the \
                    configuration\n");
        else  {
            if(pub_or_sub == CLIENT_PUB) {
                if(mosquitto_pub_topic_check(token) == MOSQ_ERR_INVAL)
                    gLOG_Log( kLOG_Warn, "Invalid publish topic '%s' , \
                                does it contain '+' or '='?\n", token);
                else 
                    cfg->topic = strdup(token);
            } else {
                if(mosquitto_pub_topic_check(token) == MOSQ_ERR_INVAL) 
                    gLOG_Log( kLOG_Warn, "Invalid subscription topic '%s' , \
                           are all '+' or '=' wildcards correctly?\n", token);
				else {
                        gLOG_Log( kLOG_Info, "subscriber topics set\n");
					cfg->topic_count++;
					cfg->topics = realloc(cfg->topics,
                                    cfg->topic_count*sizeof(char *));
					cfg->topics[cfg->topic_count-1] = strdup(token);
				}
			}
		}
	} else if (!strcmp(token, "filter_out")) {
		if(pub_or_sub == CLIENT_PUB);
		else {
			token = strtok(NULL, "=");
            trim_token(token);
			if(token == NULL)
				gLOG_Log( kLOG_Debug, "topics to filter_out are empty\n");
			else  {
				if(mosquitto_pub_topic_check(token) == MOSQ_ERR_INVAL) 
					gLOG_Log( kLOG_Warn, "Invalid filter topic '%s',  \
                            are all '+' and '=' wildcards correct?\n",token);
				else {
					cfg->filter_out_count++;
					cfg->filter_outs = realloc(cfg->filter_outs, 
                                    cfg->filter_out_count*sizeof(char *));
					cfg->filter_outs[cfg->filter_out_count-1] = strdup(token);
				}
			}
		}
#ifdef WITH_TLS
	} else if (!strcmp(token, "tls_version")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token != NULL) 
            cfg->tls_version = strdup(token);
        else  
            gLOG_Log( kLOG_Warn, "tls version is not specified in the \
                    configuration\n");
#endif		
	} else if (!strcmp(token, "username")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token != NULL) 
            cfg->username = strdup(token);
        else
            gLOG_Log( kLOG_Warn, "Username is not specified in the \
                    configuration\n");
	} else if (!strcmp(token, "password")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token != NULL) 
            cfg->password = strdup(token);
		else  
            gLOG_Log( kLOG_Warn, "password is not specified in the \
                    configuration\n");
		
	} else if (!strcmp(token, "will_payload")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->will_payload = strdup(token);
            cfg->will_payloadlen = strlen(token);
        } else  
            gLOG_Log( kLOG_Warn, "Will payload is not specified in the \
                    configuration\n");
    } else if (!strcmp(token, "will_qos")) {
        token = strtok(NULL, "=");
        if(token != NULL) {
            trim_token(token);
            cfg->will_qos = atoi(token);
            if(cfg->will_qos < 0 || cfg->will_qos > 2) 
                gLOG_Log( kLOG_Debug, "Invalid will QoS\n");
        } else 
            gLOG_Log( kLOG_Warn, "will_qos is not mentioned in the \
                    configuration\n");
	} else if (!strcmp(token, "will_retain")) {
        token = strtok(NULL, "=");
        trim_token(token);
        if(token == NULL || !strcmp(token, "false")) 
            cfg->will_retain = false;
        else if (!strcmp(token, "true"))
            cfg->will_retain = true;
        else
            gLOG_Log( kLOG_Warn, "Invalid will retain specified in the \
                    configuration\n");
	} else if (!strcmp(token, "will_topic")) {
        token = strtok(NULL, "=");
        if(token != NULL ) {
            trim_token(token);
            if(mosquitto_pub_topic_check(token) == MOSQ_ERR_INVAL) 
                gLOG_Log( kLOG_Debug, "Error: Invalid will topic '%s', \
                        does it contain '+' or '#'?\n",token);
            else
                cfg->will_topic = strdup(token);
        } else 
            gLOG_Log( kLOG_Warn, "Will topic is not mentioned in the \
                    configuration\n");
    } else if (!strcmp(token, "clean_session")) {
        if(pub_or_sub == CLIENT_PUB);
		else {
		    token = strtok(NULL, "=");
            trim_token(token);
            if(token == NULL|| !strcmp(token, "false")) 
                cfg->clean_session = false; 
            else if (!strcmp(token, "true")) 
                cfg->clean_session = true;
            else
                gLOG_Log( kLOG_Warn, "Invalid value mentioned for clean \
                        session in the configuration file\n");
		}
	} else if (!strcmp(token, "eol")) {
        if(pub_or_sub == CLIENT_PUB);
        else {
            token = strtok(NULL, "=");
            trim_token(token);
            if(token == NULL || !strcmp(token, "false"))
                cfg->eol = false;
            else if (!strcmp(token, "true"))
                cfg->eol = true;
            else 
                gLOG_Log( kLOG_Warn, "Invalid value mentioned for eol in the \
                            configuration file\n");
        }
	} else if (!strcmp(token, "no_retain")) {
        if(pub_or_sub == CLIENT_PUB);
        else {
            token = strtok(NULL, "=");
            trim_token(token);
            if(token == NULL || !strcmp(token, "false"))
                cfg->no_retain = false;
            else if (!strcmp(token, "true"))
                cfg->no_retain = true;
            else  
                gLOG_Log( kLOG_Warn, "Invalid value mentioned for no retain \
                        in the configuration file\n");
        }	
	} else if (!strcmp(token, "verbose")) {
        if(pub_or_sub == CLIENT_PUB);
        else {
            token = strtok(NULL, "=");
            trim_token(token);
            if(token == NULL|| !strcmp(token, "false")) 
                cfg->verbose = false;
            else if(!strcmp(token, "true"))
                cfg->verbose = true;
            else 
                gLOG_Log( kLOG_Warn, "Invalid value mentioned for verbose in \
                            the configuration file\n");
        }
    } else {
     //  gLOG_Log( kLOG_Debug, "Invalid option is mentioned in the file\n");
    }
}


void parse_configuration(char *line, struct mosq_config *cfg) {
	
    char *token;
    //Delimeter is "="
    token = strtok(line, "=");
    setconfigure(token, cfg);
}

void load_client_config(struct mosq_config *cfg, char *filename) {

    FILE *stream;
    char line[1024];
    int i = 0;
    stream = fopen(filename, "r");
	
    if(stream) {
            while(fgets(line, 1024, stream) != NULL) {
                parse_configuration(line, cfg);
            }
            gLOG_Log( kLOG_Debug, "Configurations: \n");
            gLOG_Log( kLOG_Debug, "port : %d\n", cfg->port);
            gLOG_Log( kLOG_Debug, "bind_address: %s\n", cfg->bind_address);
#ifdef WITH_TLS
            gLOG_Log( kLOG_Debug, "cafile : %s\n", cfg->cafile);
            gLOG_Log( kLOG_Debug, "capath : %s\n", cfg->capath);
            gLOG_Log( kLOG_Debug, "cert : %s\n", cfg->certfile);
            gLOG_Log( kLOG_Debug, "ciphers : %s\n", cfg->ciphers);
#endif
            gLOG_Log( kLOG_Debug, "msg count: %d\n", cfg->msg_count);
            gLOG_Log( kLOG_Debug, "debug : %s\n", cfg->debug?"true":"false");
            gLOG_Log( kLOG_Debug, "file_input : %s\n", cfg->file_input);
            gLOG_Log( kLOG_Debug, "host : %s\n", cfg->host);
#ifdef WITH_TLS
            gLOG_Log( kLOG_Debug, "insecure : %s\n", \
                        msg->insecure?"true":"false");
#endif
            gLOG_Log( kLOG_Debug, "id : %s\n", cfg->id);
            gLOG_Log( kLOG_Debug, "id_prefix : %s\n", cfg->id_prefix);
            gLOG_Log( kLOG_Debug, "keepalive : %d\n", cfg->keepalive);
#ifdef WITH_TLS 
            gLOG_Log( kLOG_Debug, "key : %s\n", cfg->keyfile);
#endif
            if(pub_or_sub != CLIENT_SUB)
                gLOG_Log( kLOG_Debug, "message : %s\n", cfg->message);
            gLOG_Log( kLOG_Debug, "max inflight : %d\n", cfg->max_inflight);
            gLOG_Log( kLOG_Debug, "protocol version : %d\n", \
                        cfg->protocol_version);
#ifdef WITH_SOCKS
            gLOG_Log( kLOG_Debug, "proxy : %s\n", cfg->proxy);
#endif
#ifdef WITH_TLS_PSK
            gLOG_Log( kLOG_Debug, "psk : %s\n", cfg->psk);
            gLOG_Log( kLOG_Debug, "psk-identity : %s\n", cfg->psk_identity);
#endif
            gLOG_Log( kLOG_Debug, "QoS : %d\n", cfg->qos);
            gLOG_Log( kLOG_Debug, "quiet : %s\n",cfg->quiet ?"true":"false");
            gLOG_Log( kLOG_Debug, "retain : %d\n", cfg->retain);
#ifdef WITH_SRV
            gLOG_Log( kLOG_Debug, "Service: %s\n",
                        cfg->use_srv?"true":"false");
#endif
            gLOG_Log( kLOG_Debug, "Topics: ");
            if(pub_or_sub == CLIENT_PUB)
                gLOG_Log( kLOG_Debug, "%s\n", cfg->topic);
            else {
                gLOG_Log( kLOG_Debug, "topic count %d\n", cfg->topic_count);
                for(i = 0; i < cfg->topic_count; i++)
                    gLOG_Log( kLOG_Debug, "%s ", cfg->topics[i]);
            }
            gLOG_Log( kLOG_Debug, "filter out : ");
            if(pub_or_sub == CLIENT_SUB) {
                for(i = 0; i< cfg->filter_out_count; i++)
                    gLOG_Log( kLOG_Debug, "%s ", cfg->filter_outs[i]);
            }
            gLOG_Log( kLOG_Debug, "\n");
#ifdef WITH_TLS
            gLOG_Log( kLOG_Debug, "tls version : %s \n", cfg->tls_version);
#endif
            gLOG_Log( kLOG_Debug, "username : %s\n", cfg->username);
            gLOG_Log( kLOG_Debug, "password: %s\n", cfg->password);
            gLOG_Log( kLOG_Debug, "will payload : %s\n", cfg->will_payload);
            gLOG_Log( kLOG_Debug, "will qos : %d\n", cfg->will_qos);
            gLOG_Log( kLOG_Debug, "will retain : %s\n", \
                        cfg->will_retain?"true":"false");
            gLOG_Log( kLOG_Debug, "will topic : %s\n", cfg->will_topic);
            if(pub_or_sub != CLIENT_PUB) {
                gLOG_Log( kLOG_Debug, "clean session: %s\n",
                        cfg->clean_session?"true":"false");
                gLOG_Log( kLOG_Debug, "eol : %s\n", cfg->eol?"true":"false");
                gLOG_Log( kLOG_Debug, "no retain : %s\n", \
                            cfg->no_retain?"true":"false");
                gLOG_Log( kLOG_Debug, "verbose : %d\n", cfg->verbose);
            }
    } else
        gLOG_Log( kLOG_Debug, "Couldn't read configuration file \
                        : %s\n",strerror(errno));
}

/* 
 * Function to resolve cloud URL to ip address for connecting to MQTT broker.
 */
int resolve_ip(char *url, char *ip) {
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
 
    if ( (rv = getaddrinfo( url , "http" , &hints , &servinfo)) != 0) {
        gLOG_Log(kLOG_Error, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
 
    //loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        gLOG_Log( kLOG_Debug, "IP addres is %s\n", inet_ntoa(h->sin_addr));
        if(!h->sin_addr.s_addr) 
            return 0;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }
     
    freeaddrinfo(servinfo); // done with this structurea
    return 0;	
}
