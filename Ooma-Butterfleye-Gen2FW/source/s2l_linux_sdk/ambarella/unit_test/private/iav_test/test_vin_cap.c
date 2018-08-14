/*******************************************************************************
 * test_vin_cap.c
 *
 * History:
 *    2017/02/07 - [Zhaoyang Chen] create this file
 *
 * Copyright (c) 2017 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <signal.h>
#include <basetypes.h>

#include "iav_ioctl.h"
#include "iav_netlink.h"

// vin
#include "../vin_test/vin_init.c"

struct nl_vsync_config {
	s32 fd_nl;
	s32 nl_connected;
	struct nl_msg_data msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
};

static int recover_vin_cap(void);

int fd_iav;
static struct nl_vsync_config vsync_config;

enum VIN_CAP_OP {
	VIN_CAP_OP_SUSPEND = 0,
	VIN_CAP_OP_RESUME = 1,
	VIN_CAP_OP_AUTO = 2,
};

#define	NO_ARG		0
#define	HAS_ARG		1
static struct option long_options[] = {
	{"suspend",			NO_ARG,		0,		's' },
	{"resume",			HAS_ARG,	0,		'r' },
	{"auto",			NO_ARG,		0,		'a' },
	{0, 0, 0, 0}
};

static const char *short_options = "asr:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tSuspend DSP VIN capture"},
	{"0|1", "\tResume DSP VIN capture, 0: Don't reset input (Sensor or YUV), "
		"1: Reset input (Sensor or YUV)"},
	{"", "\t\tAutomatically resume VIN capture for vsync loss case"},
};

static void usage(void)
{
	int i;

	printf("test_vin_cap usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\nExamples:\n"
		"  Suspend VIN capture:\n"
		"    test_vin_cap -s\n"
		"  Resume VIN capture without input reset:\n"
		"    test_vin_cap -r 0\n"
		"  Resume VIN capture, including input reset:\n"
		"    test_vin_cap -r 1\n"
		"  Automatically resume VIN capture for vsync loss case:\n"
		"    test_vin_cap -a\n");
	printf("\n");

}

static int init_netlink()
{
	u32 pid;
	struct sockaddr_nl saddr;

	vsync_config.fd_nl = socket(AF_NETLINK, SOCK_RAW, NL_PORT_VSYNC);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(vsync_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	vsync_config.nl_connected = 0;

	return 0;
}

static int send_vsync_msg_to_kernel(struct nl_msg_data vsync_msg)
{
	struct sockaddr_nl daddr;
	struct msghdr msg;
	struct nlmsghdr *nlhdr = NULL;
	struct iovec iov;

	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;
	daddr.nl_pad = 0;

	nlhdr = (struct nlmsghdr *)vsync_config.nl_send_buf;
	nlhdr->nlmsg_pid = getpid();
	nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(vsync_msg));
	nlhdr->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlhdr), &vsync_msg, sizeof(vsync_msg));

	memset(&msg, 0, sizeof(struct msghdr));
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_name = (void *)&daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(vsync_config.fd_nl, &msg, 0);

	return 0;
}

static int recv_vsync_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (vsync_config.fd_nl > 0) {
		ret = recvmsg(vsync_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened to receive message!\n");
		ret = -1;
	}

	return ret;
}

static int check_recv_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
	if (nlhdr->nlmsg_len <  sizeof(struct nlmsghdr)) {
		printf("Corruptted kernel message!\n");
		return -1;
	}
	msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
	if (msg_len < sizeof(struct nl_msg_data)) {
		printf("Unknown kernel message!!\n");
		return -1;
	}

	return 0;
}

static int process_vsync_req(int vsync_req)
{
	int ret = 0;

	if (vsync_req == NL_REQ_VSYNC_RESTORE) {
		ret = recover_vin_cap();
		vsync_config.msg.pid = getpid();
		vsync_config.msg.port = NL_PORT_VSYNC;
		vsync_config.msg.type = NL_MSG_TYPE_REQUEST;
		vsync_config.msg.dir = NL_MSG_DIR_STATUS;
		vsync_config.msg.cmd = NL_REQ_VSYNC_RESTORE;
		if (ret < 0) {
			vsync_config.msg.status = NL_CMD_STATUS_FAIL;
			send_vsync_msg_to_kernel(vsync_config.msg);
		} else {
			vsync_config.msg.status = NL_CMD_STATUS_SUCCESS;
			send_vsync_msg_to_kernel(vsync_config.msg);
		}
	} else {
		printf("Unrecognized kernel message!\n");
		ret = -1;
	}

	return ret;
}

static int process_vsync_session_status(struct nl_msg_data *kernel_msg)
{
	int ret = 0;

	if (kernel_msg->type != NL_MSG_TYPE_SESSION ||
		kernel_msg->dir != NL_MSG_DIR_STATUS) {
		return -1;
	}

	switch (kernel_msg->cmd) {
	case NL_SESS_CMD_CONNECT:
		if (kernel_msg->status == NL_CMD_STATUS_SUCCESS) {
			vsync_config.nl_connected = 1;
			printf("Connection established with kernel.\n");
		} else {
			vsync_config.nl_connected = 0;
			printf("Failed to establish connection with kernel!\n");
		}
		break;
	case NL_SESS_CMD_DISCONNECT:
		vsync_config.nl_connected = 0;
		if (kernel_msg->status == NL_CMD_STATUS_SUCCESS) {
			printf("Connection removed with kernel.\n");
		} else {
			printf("Failed to remove connection with kernel!\n");
		}
		break;
	default:
		printf("Unrecognized session cmd from kernel!\n");
		ret = -1;
		break;
	}

	return ret;
}

static int process_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	struct nl_msg_data *kernel_msg;
	int ret = 0;

	if (check_recv_vsync_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
	kernel_msg = (struct nl_msg_data *)NLMSG_DATA(nlhdr);

	if(kernel_msg->type == NL_MSG_TYPE_REQUEST &&
		kernel_msg->dir == NL_MSG_DIR_CMD) {
		if (process_vsync_req(kernel_msg->cmd) < 0) {
			ret = -1;
		}
	} else if (kernel_msg->type == NL_MSG_TYPE_SESSION &&
				kernel_msg->dir == NL_MSG_DIR_STATUS) {
		if (process_vsync_session_status(kernel_msg) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int nl_send_vsync_session_cmd(int cmd)
{
	int ret = 0;

	vsync_config.msg.pid = getpid();
	vsync_config.msg.port = NL_PORT_VSYNC;
	vsync_config.msg.type = NL_MSG_TYPE_SESSION;
	vsync_config.msg.dir = NL_MSG_DIR_CMD;
	vsync_config.msg.cmd = cmd;
	vsync_config.msg.status = 0;
	send_vsync_msg_to_kernel(vsync_config.msg);

	ret = recv_vsync_msg_from_kernel();

	if (ret > 0) {
		ret = process_vsync_msg();
		if (ret < 0) {
			printf("Failed to process session status!\n");
		}
	} else {
		printf("Error for getting session status!\n");
	}

	return ret;
}

static void * netlink_loop(void * data)
{
	int ret;
	int count = 100;

	while (count && !vsync_config.nl_connected) {
		if (nl_send_vsync_session_cmd(NL_SESS_CMD_CONNECT) < 0) {
			printf("Failed to establish connection with kernel!\n");
		}
		sleep(1);
		count--;
	}

	if (!vsync_config.nl_connected) {
		printf("Please enable kernel vsync loss guard mechanism!!!\n");
		return NULL;
	}

	while (vsync_config.nl_connected) {
		ret = recv_vsync_msg_from_kernel();
		if (ret > 0) {
			ret = process_vsync_msg();
			if (ret < 0) {
				printf("Failed to process the msg from kernel!\n");
			}
		}
		else {
			printf("Error for getting msg from kernel!\n");
		}
	}

	return NULL;
}

static int vin_cap_resume(int enable)
{
	struct iav_vcap_cfg cfg;
	int ret = 0;

	cfg.cid = IAV_VCAP_CFG_STATE;
	cfg.arg.state = enable ? IAV_VCAP_STATE_ACTIVE : IAV_VCAP_STATE_IDLE;
	ret = ioctl(fd_iav, IAV_IOC_SET_VCAP_CFG, &cfg);

	return ret;
}

static int reset_input(void)
{
	struct vindev_mode video_info;
	struct vindev_fps vsrc_fps;
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_MODE, &video_info) < 0) {
		return -1;
	} else {
		printf("Start to restore vin_mode 0x%x and hdr_mode %d.\n",
			video_info.video_mode, video_info.hdr_mode);
	}

	vsrc_fps.vsrc_id = 0;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
		return -1;
	} else {
		printf("Start to restore vin frame rate %d.\n", vsrc_fps.fps);
	}

	if(ioctl(fd_iav, IAV_IOC_VIN_SET_MODE, &video_info) < 0) {
		return -1;
	} else {
		printf("Succeed to restore vin_mode 0x%x and hdr_mode %d.\n",
			video_info.video_mode, video_info.hdr_mode);
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_SET_FPS");
		return -1;
	} else {
		printf("Succeed to restore vin frame rate %d.\n", vsrc_fps.fps);
	}

	return 0;
}

static int recover_vin_cap(void)
{
	int ret = 0;

	ret = vin_cap_resume(0);
	if (ret) {
		return ret;
	}
	ret = reset_input();
	if (ret) {
		return ret;
	}
	ret = vin_cap_resume(1);
	if (ret) {
		return ret;
	}

	return ret;
}

static void vin_cap_auto(void)
{
	init_netlink();
	netlink_loop(NULL);
}

static void sigstop()
{
	nl_send_vsync_session_cmd(NL_SESS_CMD_DISCONNECT);
	exit(1);
}

int vin_vcap_param(int argc, char **argv)
{
	int ch, reset_input_flag;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 's':
			vin_cap_resume(0);
			break;
		case 'r':
			reset_input_flag = atoi(optarg);
			if (reset_input_flag != 0 && reset_input_flag != 1) {
				printf("Invalid resume param %d, should be 0|1!\n",
					reset_input_flag);
				return -1;
			} else if (reset_input_flag) {
				reset_input();
			}
			vin_cap_resume(1);
			break;
		case 'a':
			vin_cap_auto();
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C, Ctrl+'\', and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (vin_vcap_param(argc, argv) < 0) {
		return -1;
	}

	return 0;
}

