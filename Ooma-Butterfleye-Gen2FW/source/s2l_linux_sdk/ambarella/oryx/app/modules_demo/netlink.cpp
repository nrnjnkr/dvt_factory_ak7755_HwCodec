/*******************************************************************************
 * netlink.cpp
 *
 * History:
 *   Feb 22, 2017 - [Huqing Wang] created file
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
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <linux/netlink.h>
#include "netlink.h"

int send_msg_to_kernel(nl_config_t &config)
{
  int ret = 0;
  struct sockaddr_nl daddr = {0};
  struct msghdr msg = {0};
  struct nlmsghdr *nlhdr = nullptr;
  struct iovec iov = {0};

  daddr.nl_family = AF_NETLINK;
  daddr.nl_pid = 0;
  daddr.nl_groups = 0;
  daddr.nl_pad = 0;

  nlhdr = (nlmsghdr *)config.nl_send_buf;
  nlhdr->nlmsg_pid = config.msg.pid;
  nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(nl_msg_data));
  nlhdr->nlmsg_flags = 0;
  memcpy(NLMSG_DATA(nlhdr), &config.msg, sizeof(nl_msg_data));

  iov.iov_base = (void *)nlhdr;
  iov.iov_len = nlhdr->nlmsg_len;
  msg.msg_name = (void *)&daddr;
  msg.msg_namelen = sizeof(daddr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if (sendmsg(config.fd, &msg, 0) < 0) {
    ret = -1;
  }

  return ret;
}

int init_netlink(nl_config_t &config)
{
  int ret = 0;
  do {
    struct sockaddr_nl saddr = {0};

    if ((config.fd = socket(AF_NETLINK, SOCK_RAW, config.msg.port)) == -1) {
      perror("NL_PORT_THAW");
      ret = -1;
      break;
    }
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = config.msg.pid;
    saddr.nl_groups = 0;
    saddr.nl_pad = 0;
    if (bind(config.fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
      perror("bind");
      ret = -1;
      break;
    }

    config.msg.type = NL_MSG_TYPE_SESSION;
    config.msg.dir = NL_MSG_DIR_CMD;
    config.msg.cmd = NL_SESS_CMD_CONNECT;
    config.msg.status = 0;
    ret = send_msg_to_kernel(config);
  } while (0);

  return ret;
}

int deinit_netlink(nl_config_t &config)
{
  if(config.fd > 0) {
    config.msg.type = NL_MSG_TYPE_SESSION;
    config.msg.dir = NL_MSG_DIR_CMD;
    config.msg.cmd = NL_SESS_CMD_DISCONNECT;
    config.msg.status = 0;
    send_msg_to_kernel(config);

    close(config.fd);
    config.fd = -1;
  }

  return 0;
}

static inline int recvmsg_reliable(int fd, struct msghdr *msg, int flags)
{
  int ret = 0;
  int retry = 3;

  while (true) {
    //block here
    ret = recvmsg(fd, msg, flags);
    if (ret > 0) {
      break;
    } else if (ret == 0) {
      printf("Netlink connect is shutdown!\n");
      break;
    } else {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("system interrupt occur, try %d times again\n", retry);
        --retry;
        if (retry == 0) {
          break;
        }
        continue;
      } else {
        perror("recvmsg");
        break;
      }
    }
  }
  return ret;
}

static inline int check_recv_msg(nlmsghdr *msg)
{

  if (msg->nlmsg_len <  sizeof(struct nlmsghdr)) {
    printf("Corruptted kernel message!\n");
    return -1;
  }

  u32 msg_len = msg->nlmsg_len - NLMSG_LENGTH(0);
  if (msg_len < sizeof(struct nl_msg_data)) {
    printf("Unknown kernel message!!\n");
    return -1;
  }

  return 0;
}

int receive_msg_from_kernel(nl_config_t &config)
{
  int ret = 0;

  do {
    struct sockaddr_nl sa = {0};
    struct msghdr msg = {0};
    struct iovec iov = {0};
    nlmsghdr *nlhdr = (nlmsghdr *)config.nl_recv_buf;

    iov.iov_base = (void *)nlhdr;
    iov.iov_len = MAX_NL_MSG_LEN;
    msg.msg_name = (void *)&(sa);
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    if (config.fd > 0) {
      if ((ret = recvmsg_reliable(config.fd, &msg, 0)) < 0) {
        break;
      }
    } else {
      printf("Netlink socket is not opened for receive message!\n");
      ret = -1;
      break;
    }

    if ((ret = check_recv_msg(nlhdr)) < 0) {
      break;
    }
    nl_msg_data *data = (nl_msg_data *)NLMSG_DATA(nlhdr);
    //saving time, comment memcpy here
    //memcpy(&config.msg, data, sizeof(nl_msg_data));
    config.msg.type = data->type;
    config.msg.status = data->status;
  } while (0);

  return ret;
}
