/*
 * Author : Naresh K
 * MQTT client for publishing and subscribing 
 *
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mosquitto.h"
#include "client_shared.h"
#include "log.h"

#define CONFIG_PATH "/usr/bin/"
#define CONFIG_FILE_NAME  "subscriber_client_config"

/* 
 *Global variables for use in callbacks.
 */

eLOG_LogLevel gnLogLevel = kLOG_Debug;
static int mode = MSGMODE_NONE;
static int mid_sent = 0;

void Usage() {
    gLOG_Log( kLOG_Debug, "Usage: subscriber_client -c <configuration_file>\n");
    gLOG_Log( kLOG_Debug, "configuration_file file with absolute path \n");

}

void my_log_callback(struct mosquitto *mosq, void *user_data, int level, 
                        const char *message) {

	/*
     * TO-DO : Need to decided wheather all messages from client needed to 
     * print or only based on level message can print
     * MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR 
     * MOSQ_LOG_DEBUG
     */
    gLOG_Log( kLOG_Info, "Subscriber: %s\n", message);
}

void my_connect_callback(struct mosquitto *mosq, void *user_data, 
                            int result) {

    int i; 
    struct mosq_config *cfg;
    cfg = (struct mosq_config*) user_data;
    if(!result) {
        gLOG_Log( kLOG_Info, "Subscriber is connected successfull with \
                broker\n");
        /*
         * TO-DO : Do we need say to somebody about the connection successful? 
         * may be to EventManager
         */
        for(i=0; i<cfg->topic_count; i++){
            gLOG_Log( kLOG_Info, "topic : %s\n", cfg->topics[i]);
            mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
        }

    } else  {
        /*
         * TO-DO : If connection is refused do need to take care ?
         */
        gLOG_Log( kLOG_Error, "Connection refused : reason is %d\n", result);
        if(result && !cfg->quiet){
            gLOG_Log( kLOG_Info, "%s\n", mosquitto_connack_string(result));
        }

    }

}

void my_disconnect_callback(struct mosquitto *mosq, void *user_data, int rc) {
/*
 * TO-DO: 
 * If publisher client is disconnected and inform to Event Manager about it.
 */
    if(!rc)
        gLOG_Log( kLOG_Debug, "Subscriber is disconnected successful from \
                    broker as per client request\n");
    else 
        gLOG_Log( kLOG_Warn, "Client disconnected unexpectedly\n");
}

void my_message_callback(struct mosquitto *mosq, void *obj, 
                            const struct mosquitto_message *message) {
/*``
 * TO-DO : This is just response for message delivered to broker 
 * do we need to inform about it to EventManager ?  or Ignore 
 */
    gLOG_Log( kLOG_Info, "Got message \n");
    struct mosq_config *cfg;
    int i;
    bool res;

    //if(process_messages == false) return;

    //assert(obj);
    cfg = (struct mosq_config *)obj;
    if(message->retain && cfg->no_retain) 
        return;

    if(cfg->filter_outs) {
        for(i=0; i<cfg->filter_out_count; i++) {
            mosquitto_topic_matches_sub(cfg->filter_outs[i], 
                    message->topic, &res);
            if(res)
                return;
        }
    }

    if(cfg->verbose) {
        if(message->payloadlen) {
            gLOG_Log( kLOG_Info, "%s ", message->topic);
            gLOG_Log( kLOG_Info, "%s\n", message->payload);
            if(cfg->eol)
                gLOG_Log( kLOG_Debug, "\n");
        } else {
            if(cfg->eol)
                 gLOG_Log( kLOG_Debug, "%s (null)\n", message->topic);
        }
        fflush(stdout);
    } else {

        /* Skpping dumping message to console */

    }

    /* 
     * TO-DO : In anycase do we need to disconnect subscriber here ?
     *
     */

    /*if(cfg->msg_count>0) {
        msg_count++;
        if(cfg->msg_count == msg_count){
            process_messages = false;
            mosquitto_disconnect(mosq); 
        }
    }*/

}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, 
                                int qos_count, const int *granted_qos) {
    int i;
    struct mosq_config *cfg; 
    //assert(obj);
    cfg = (struct mosq_config *)obj;

    if(!cfg->quiet) 
        gLOG_Log( kLOG_Info, "Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(i=1; i<qos_count; i++) { 
        if(!cfg->quiet) 
            gLOG_Log( kLOG_Info, ", %d", granted_qos[i]);
    }
    if(!cfg->quiet) 
        gLOG_Log( kLOG_Info, "\n");
} 

int main(int argc, char **argv) {

	int type = 1;
	int i = 0, rc, rc2,  message_len = 0;
	int port = 0; 
	char config_file[100];
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;
    //Initializing as publisher  
	pub_or_sub = CLIENT_SUB;
    char *topic = NULL, *message=NULL, *username=NULL, *password=NULL;
    int msglen = 0, qos = 0, retain = 0;
    bool quiet;
                                                                                 
    if(argc <= 1)  {
        gLOG_Log( kLOG_Info, "Using default configuration file\n");
        strcpy(config_file, CONFIG_PATH);
        strcat(config_file, CONFIG_FILE_NAME);
        gLOG_Log( kLOG_Info, "Using config file %s\n",config_file);
        if(access(config_file, F_OK ) == -1 ) {
            gLOG_Log( kLOG_Error, "Default configuration is not present in rootfs\n ");
            Usage();
            return 0;
        }
    } else {

        if(!strcmp("-c", argv[1]))
            strcpy(config_file, argv[2]); 
        if(access(config_file, F_OK ) == 0) {
            gLOG_Log( kLOG_Info, "Using config file %s\n", config_file);
        } else {
            gLOG_Log( kLOG_Info, "config file %s is not present, checking for default \
                        configuration file\n", config_file);
            strcpy(config_file, CONFIG_PATH);
            strcat(config_file, CONFIG_FILE_NAME);
            gLOG_Log( kLOG_Info, ": %s\n",config_file);
            if(access(config_file, F_OK ) == -1 ) {
                gLOG_Log( kLOG_Error, "Default configuration is not present in rootfs\n ");
                Usage();
                return 0;
            } else 
                gLOG_Log( kLOG_Debug, "Using config file %s\n", config_file);
        }
    }
	
    //Configure the client 
    init_config(&cfg);
    load_client_config(&cfg, config_file);

    topic = cfg.topic;
    qos = cfg.qos;
    retain = cfg.retain;

    mosquitto_lib_init();
	client_id_generate(&cfg, "mosqsub");
  
    gLOG_Log( kLOG_Info, "id %s\n", cfg.id);
    mosq = mosquitto_new(cfg.id, true, NULL);
    if(!mosq){
        switch(errno){
            case ENOMEM:
                if(!quiet) gLOG_Log( kLOG_Info, "Out of memory.\n");
                break;
            case EINVAL:
                if(!quiet) gLOG_Log( kLOG_Info, "Invalid id.\n");
                break;
        }
        mosquitto_lib_cleanup();
        return 1;
    }
    if(cfg.debug){
        /*
         * For getting logs from the client library.
         */
        mosquitto_log_callback_set(mosq, my_log_callback);
        mosquitto_subscribe_callback_set(mosq, my_subscribe_callback); 
    }
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
    mosquitto_message_callback_set(mosq, my_message_callback);
    mosquitto_user_data_set(mosq, &cfg);

    //Set the options for mosquitto instance
    if(client_opts_set(mosq, &cfg))
        return 1;

    rc = client_connect(mosq, &cfg);

   // mosquitto_loop(mosq, -1, 1);
    if(rc) 
        return rc;

	/*
     * Will be waiting for message from EventManager once we have queue 
     * with message then will publish to client.
	 *
	 * Read event from the EventManager queue and generate message according 
     * to json format 
	 * 
	 */
	while(1) {
		
		/* Wait on message queue */
		//msgrcv();

		/*
         * Once we get the message {cmd, message/payload} from the 
         * EventManager then parse and constrct json format payload
         */
        //prepare_payload(message); 
		rc = mosquitto_loop_forever(mosq, -1, 1);                                   
        mosquitto_destroy(mosq);                                                    
        mosquitto_lib_cleanup();  

        sleep(10);
        break;
    }
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}
