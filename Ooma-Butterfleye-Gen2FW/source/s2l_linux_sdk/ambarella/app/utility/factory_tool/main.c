#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <errno.h>
#include<string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "factory_client.h"

#define SERV_PORT 5000

am_mp_result_t mptool_result[AM_MP_MAX_ITEM_NUM];
am_mp_result_t test(int fd, char *cmd_name, char *argv[], int argc);

void Usage() {

    fprintf(stdout, "Usage: \n");
    fprintf(stdout,"butterfleye_factory eth\n");
    fprintf(stdout,"butterfleye_factory button\n");
    fprintf(stdout,"butterfleye_factory led RED/GREEN/BLUE On/Off\n");
    fprintf(stdout,"butterfleye_factory irled on/off\n");
    fprintf(stdout,"butterfleye_factory light_sensor\n");
    fprintf(stdout,"butterfleye_factory mac_addr <new_mac_address"
            "(e,g 001122334455)>\n");
    fprintf(stdout,"butterfleye_factory wifi setup/scan/connect ap_name "
            "password/setcountry code/"
            "getcountry/signal_strength/wifi_throughput\n");
    fprintf(stdout,"butterfleye_factory mic\n");
    fprintf(stdout,"butterfleye_factory speaker\n");
    fprintf(stdout,"butterfleye_factory sensor\n");
    fprintf(stdout,"butterfleye_factory lens_focus rtsp_start/"
            "rtsp_stop/get_data\n");
    fprintf(stdout,"butterfleye_factory lens_shad\n");
    fprintf(stdout,"butterfleye_factory bad_pixel\n");
    fprintf(stdout,"butterfleye_factory get_firmware_info\n");
    fprintf(stdout,"butterfleye_factory sdcard\n");
    fprintf(stdout,"butterfleye_factory ircut enable_ircut/"
            "rtsp_start/rtsp_stop\n");
    fprintf(stdout,"butterfleye_factory mcu connect/disconnect\n");
    fprintf(stdout,"butterfleye_factory pir\n");
    fprintf(stdout,"butterfleye_factory gsensor\n");
    fprintf(stdout,"butterfleye_factory voice_detect\n");
    fprintf(stdout,"butterfleye_factory battery\n");
    fprintf(stdout,"butterfleye_factory usb\n");
    fprintf(stdout,"butterfleye_factory bluetooth start_adv/stop_adv\n");
    fprintf(stdout,"butterfleye_factory keydownload <servierip> <key_file_name> <md5sum_checksum>\n");
    fprintf(stdout,"butterfleye_factory standby\n");
    fprintf(stdout,"butterfleye_factory all\n");
    return;
}

int main(int argc, char *argv[]) {

   int32_t sockfd = -1;                                                          
   int32_t connfd = -1;                                                          
   int32_t on = 1;                                                               
   int32_t ret = MP_OK;
   struct sockaddr_in servr_addr, client_addr;                                   
   socklen_t length = sizeof(client_addr); 

   am_mp_msg_t mptool_msg;

    if(argc <= 1 ) {
       Usage();
       return;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);                                   
    if (sockfd < 0) {                                                           
      perror("socket");                                                         
      ret = -1;   
      return;
    }                                                                           
                                                                                
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {    
      perror("setsockopt");                                                     
      ret = -1;                                                                 
      return;                                                                  
    }                                                                           

    memset(&servr_addr, 0, sizeof(servr_addr));
    servr_addr.sin_family = AF_INET;
    servr_addr.sin_port = htons(SERV_PORT);
    servr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   
    if(connect(sockfd,(struct sockaddr*)&servr_addr, length)) {
        fprintf(stderr, "Unable to connect server for test and make sure \
                test pc connected usb ethernet interface\n");
        return;
    }

    memset(&mptool_msg, 0, sizeof(am_mp_msg_t));

    int i = 0;
    //Let's get the initial values of all the test results and ignore
    for (i = 0; i < AM_MP_MAX_ITEM_NUM; i++) {
        read(sockfd, &mptool_msg, sizeof(am_mp_msg_t));
        mptool_result[i].ret = mptool_msg.result.ret;
        /*fprintf(stdout, "Test %s result %d\n", testname(i),
         *          mptool_result[i].ret);
         */
    }
    
    am_mp_result_t result;
    result  = test(sockfd, argv[1], argv, argc);
    if (result.ret == MP_OK )
        fprintf(stderr, "CMD : Success\n");
    else
        fprintf(stderr, "CMD : Failed\n");
    
    close(sockfd);
    return 0;
}
