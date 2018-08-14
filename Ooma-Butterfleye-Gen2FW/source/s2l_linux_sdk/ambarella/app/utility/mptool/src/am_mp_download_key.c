#include <stdio.h>                                                              
#include <unistd.h>                                                             
#include <stdlib.h>                                                             
#include <fcntl.h>                                                              
#include <string.h>                                                             
#include <sys/time.h>                                                           
#include <sys/stat.h>                                                           
#include <sys/types.h>                                                          
#include <sys/vfs.h>                                                            
#include <sys/mount.h>                                                          
#include <errno.h>                                                              
#include "am_mp_server.h"                                                       
#include <sys/vfs.h>                                                            
#include <time.h>
#include "am_mp_download_key.h"

#define LEN 100

typedef struct download {
    char filename[20];
    char serverip[20];
    char md5sum[10];
}tDownloadKeys;

tDownloadKeys download_key;

int download(char *checksum) {
    
    char cmd[100]; 

    snprintf(cmd, 100, "cd /tmp/config/etc/butterfleye && /usr/bin/tftp -gr %s %s", 
            download_key.filename, download_key.serverip);

    system(cmd);

    memset(cmd, 0, 100);
    snprintf(cmd, 100, "/tmp/config/etc/butterfleye/%s", download_key.filename);

    if(access(cmd, F_OK)) {
        strcpy(checksum, "Failed to download keys");
        return MP_ERROR;
    }
    snprintf(cmd, 100,"cd /tmp/config/etc/butterfleye && /usr/bin/md5sum %s", 
            download_key.filename);

    FILE *fp;
    char buf[LEN];
    char *ptr;
    fp = popen(cmd, "r"); 
    if( fp != NULL ) {
        fgets(buf, LEN, fp);
        ptr = strtok(buf, " ");
        fclose(fp);
        if(!strcmp(download_key.md5sum, ptr)) {
            strcpy(checksum, "Success");
            return MP_OK;
        }  else {
            strcpy(checksum, "Checksum failed");
            return MP_ERROR;
        }
    } 
    strcpy(checksum, "Generate checksum failed");
    return MP_ERROR;

}

am_mp_err_t mptool_key_download_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
    
  to_msg->result.ret = MP_OK;
  fprintf(stderr, "keydownload handler\n");
  switch (from_msg->stage) {
    case 0:
      memcpy(&download_key, from_msg->msg, sizeof(tDownloadKeys));
      to_msg->result.ret = download(to_msg->msg);
      break;

    case 1:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
      }
      break;

    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}

