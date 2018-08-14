/*
 * test_tcp_connect_nonblock.c
 *
 * History:
 *       2015/06/01 - [Jian Liu] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
int connect_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec){
    int flags,n,error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0){
        if (errno != EINPROGRESS){
            return(-1);
        }
    }
    /* Do whatever we want while the connect is taking place. */
    if (n == 0){
        goto done;/* connect completed immediately */
    }

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;
    while ((n = select(sockfd+1, &rset, &wset, NULL,nsec ? &tval : NULL)) < 0 && errno == EINTR);
    if (n == 0) {
        errno = ETIMEDOUT;
        return(-1);
    }
    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0){
            return(-1);
        }
    } else{
        return (-1);/*select error*/
    }
done:
    fcntl(sockfd, F_SETFL, flags);/* restore file status flags */
    if (error) {
        errno = error;
        return(-1);
    }
    return(0);
}

int test_tcp_connect_nonblock(char *ip,char *port){
    if((ip==NULL) || (port==NULL))
    {
        printf("################ Please check your ip ,port ################ \n");
        return -1;
    }
    struct sockaddr_in server_addr;
    char *server_ip = ip;
    unsigned short server_port = (unsigned short)atoi(port);
    printf("test non-block connect [%s:%u]....\n",server_ip,server_port);

    signal(SIGPIPE,SIG_IGN);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        printf("failed to create socket, errno = %d\n",errno);
        goto fail;
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if(inet_pton(AF_INET,server_ip,&server_addr.sin_addr) <= 0){
        goto fail;
    }
    if(connect_nonb(fd,&server_addr,sizeof(server_addr),10) < 0){
        printf("Failed to connect server,errno = %d\n",errno);
        goto fail;
    }

    printf("connect server successfully\n");
    close(fd);
    return 0;
fail:
    if(fd >= 0){
        close(fd);
    }
    return -1;
}


