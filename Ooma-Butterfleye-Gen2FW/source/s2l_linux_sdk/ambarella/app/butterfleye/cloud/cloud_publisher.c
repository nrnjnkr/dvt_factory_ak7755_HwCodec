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
#define CONFIG_FILE_NAME  "publisher_client_config"

/* 
 *Global variables for use in callbacks.
 */

eLOG_LogLevel gnLogLevel = kLOG_Debug;
static int mode = MSGMODE_NONE;
static int mid_sent = 0;

void Usage() {
    gLOG_Log( kLOG_Info, "Usage: publisher_client -c <configuration_file>\n");
    gLOG_Log( kLOG_Info, "configuration_file file with absolute path \n");

}

void my_log_callback(struct mosquitto *mosq, void *user_data, int level, 
                        const char *message) {

	/*
     * TO-DO : Need to decided wheather all messages from client needed to 
     * print or only based on level message can print
     * MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR 
     * MOSQ_LOG_DEBUG
     */
    gLOG_Log( kLOG_Info, "Publisher: %s\n", message);
}

void my_connect_callback(struct mosquitto *mosq, void *user_data, 
                                int result) {
    char *buf;
    int buf_len_actual = 0;
    int retain = 0;
    int rc2;

    if(!result) {
        gLOG_Log( kLOG_Info, "Publisher is connected successfull \
                with broker\n");
        /*
         * TO-DO : Do we need say to somebody about the connection successful? 
         * may be to EventManager
         */
    } else  {
        /*
         * TO-DO : If connection is refused do need to take care ?
         */
        gLOG_Log( kLOG_Info, "Connection refused : reason is %d\n", result);
    }
}

void my_disconnect_callback(struct mosquitto *mosq, void *user_data, int rc) {
/*
 * TO-DO: 
 * If publisher client is disconnected and inform to Event Manager about it.
 */
    if(!rc)
        gLOG_Log( kLOG_Info, "Publisher is disconnected successful from \
                    broker as per client request\n");
    else 
        gLOG_Log( kLOG_Info, "Client disconnected unexpectedly\n");
}

void my_publish_callback(struct mosquitto *mosq, void *user_data, int mid) {
/*
 * TO-DO : This is just response for message delivered to broker 
 * do we need to inform about it to EventManager ?  or Ignore 
 */
    gLOG_Log( kLOG_Info, "Successful published message to broker \n");
}

int main(int argc, char **argv) {

	int type = 1;
	int i = 0, rc, rc2,  message_len = 0;
	int port = 0; 
	char config_file[100];
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;
    //Initializing as publisher  
	pub_or_sub = CLIENT_PUB;
    char *topic = NULL, *message=NULL, *username=NULL, *password=NULL;
    int msglen = 0, qos = 0, retain = 0;
    bool quiet;
                                                                                 
    if(argc <= 1)  {
        gLOG_Log( kLOG_Debug, "Using default configuration file\n");
        strcpy(config_file, CONFIG_PATH);
        strcat(config_file, CONFIG_FILE_NAME);
        gLOG_Log( kLOG_Debug, "Using config file %s\n",config_file);
        if(access(config_file, F_OK ) == -1 ) {
            gLOG_Log( kLOG_Error, "Default configuration is not present in rootfs\n ");
            Usage();
            return 0;
        }
    } else {

        if(!strcmp("-c", argv[1]))
            strcpy(config_file, argv[2]); 
        if(access(config_file, F_OK ) == 0) {
            gLOG_Log( kLOG_Debug, "Using config file %s\n", config_file);
        } else {
            gLOG_Log( kLOG_Debug, "config file %s is not present, checking for default \
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
    message = cfg.message;
    msglen = cfg.msglen;
    qos = cfg.qos;
    retain = cfg.retain;
    mode = cfg.pub_mode;
    username = cfg.username;
    password = cfg.password;
    quiet = cfg.quiet;

    mosquitto_lib_init();
	client_id_generate(&cfg, "mosqpub");
  
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
    }
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
    mosquitto_publish_callback_set(mosq, my_publish_callback);

    //Set the options for mosquitto instance
    if(client_opts_set(mosq, &cfg))
        return 1;

    rc = client_connect(mosq, &cfg);

    mosquitto_loop(mosq, -1, 1);
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
		
        message_len = strlen(message);
        gLOG_Log( kLOG_Info, "Publishing the message\n***************************\n");
        gLOG_Log( kLOG_Info, "topic :%s\n", topic);
        gLOG_Log( kLOG_Info, "message :%s\n", message);
        gLOG_Log( kLOG_Info, "qos : %d\n", qos);
        gLOG_Log( kLOG_Info, "retain : %d\n ***************************\n", retain);
        rc2 = mosquitto_publish(mosq, &mid_sent, topic, msglen, 
                message, qos, retain);
        mosquitto_loop(mosq, -1, 1);
        if(rc2) {
            if(!quiet) 
                gLOG_Log( kLOG_Error, "Publish returned %d, disconnecting.\n", rc2);
            mosquitto_disconnect(mosq);
        }
        sleep(10);
        break;
    }
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}
