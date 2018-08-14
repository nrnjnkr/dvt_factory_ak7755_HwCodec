#include <stdio.h>
#include "bpi_wlan.h"
#include "bpi_utils.h"
int main(){
    keep_alive_param_t param;
    param.server_addr = "192.168.172.25";
    param.server_port = 6024;
    param.keepalive_interval = 30;//seconds
    param.dtim_interval = 600;//mseconds
    param.timeout = 10;//seconds
    param.verbose = 1;
    param.wake_data_len = snprintf((char*)param.wake_data,sizeof(param.wake_data),"%s","amba_wakeup");
    int result = -1;
    LOG_PRINT("wifi standby TCP starts\n");
    int fd_client = connect_server(param.server_addr, param.server_port, Net_TCP);
    if (fd_client < 0) {
        LOG_ERROR("connect to server fail\n");
        result  = -1;
        return result;
    }
    result = tcp_keep_alive(&param, fd_client);
    return result;
}
