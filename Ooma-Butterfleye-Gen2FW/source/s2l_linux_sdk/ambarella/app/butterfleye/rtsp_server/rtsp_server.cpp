#include <stdio.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <signal.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "MyH264VideoStreamFramer.hh"
#include "MyH264VideoRTPSink.hh"
#include "H264VideoFileServerMediaSubsession.hh"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <basetypes.h>
#include "iav_ioctl.h"
#include "bsreader.h"
#include "config.h"

#define	MAX_ENCODE_STREAM_NUM	(1)

UsageEnvironment* env;

char const* descriptionString = "Session streamed by \"Amba RTSP Server\"";

static portNumBits clientPort;
static Boolean is_ssm = False;

/* To make the second and subsequent client for each stream reuse the same
 * input stream as the first client (rather than playing the file from the
 * start for each client), change the following "False" to "True":
 */
Boolean reuseFirstSource = True;

int bsreader()
{
	int fd_iav, i, total_size;
	bsreader_init_data_t  init_data;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	memset(&init_data, 0, sizeof(init_data));
	init_data.fd_iav = fd_iav;
	init_data.max_stream_num = MAX_ENCODE_STREAM_NUM;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		switch (i) {
		case 0:
			total_size = 1024*1024*4;  // 4MB
			break;
		case 1:
			total_size = 1024*1024*2;  // 2MB
			break;
		default:
			total_size = 1024*1024*1;  // 1MB
			break;
		}
		init_data.ring_buf_size[i] = total_size;
	}

	/* Enable cavlc encoding when configured.
	 * This option will use more memory in bsreader on A5s arch.
	 */
	init_data.cavlc_possible = 1;

	if (bsreader_init(&init_data) < 0) {
		printf("bsreader init failed \n");
		return -1;
	}

	if (bsreader_open() < 0) {
		printf("bsreader open failed \n");
		return -1;
	}
	return 0;
}

void create_live_stream(RTSPServer * rtspServer, int stream,
	UsageEnvironment * env_stream)
{
	char streamName[32], inputFileName[32];
	sprintf(streamName, "stream%d", (stream+1));
	sprintf(inputFileName, "live_stream%d", (stream+1));

	ServerMediaSession * sms =
		ServerMediaSession::createNew(*env, streamName, streamName,
			descriptionString, is_ssm /*SSM*/);
	ServerMediaSubsession * subsession =
		H264VideoFileServerMediaSubsession::createNew(*env_stream,
			inputFileName, reuseFirstSource);

	sms->addSubsession(subsession);
	subsession->setServerAddressAndPortForSDP(0, clientPort);
	rtspServer->addServerMediaSession(sms);

	char *url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;
}

void setup_streams(RTSPServer * rtspServer)
{
	TaskScheduler * scheduler[MAX_ENCODE_STREAM_NUM];
	UsageEnvironment * env_stream[MAX_ENCODE_STREAM_NUM];
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		scheduler[i] = BasicTaskScheduler::createNew();
		env_stream[i] = BasicUsageEnvironment::createNew(*scheduler[i]);
		create_live_stream(rtspServer, i, env_stream[i]);
	}
}

extern "C" {

void* rtsp_server(void * dummy)
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
	// To implement client access control to the RTSP server, do the following:
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); // replace these with real strings
	// Repeat the above with each <username>, <password> that you wish to allow
	// access to the server.
#endif

	// Create the RTSP server:
	RTSPServer* rtspServer = RTSPServer::createNew(*env, 554, authDB);
	if (rtspServer == NULL) {
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
	    return NULL;
    }
	bsreader();

	setup_streams(rtspServer);

	 // does not return
	env->taskScheduler().doEventLoop();

	return NULL;
}

}
