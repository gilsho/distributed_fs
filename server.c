/*
Servers must handle at least a single client, single file, and single pending transaction at a time.
Your server executable must accept and honor these parameters:
-port port#: UDP port # for fs communication. (so we can test it)
-mount filepath : place where committed local copies are to be stored by the server.
-drop percentage : the percentage (0 to 100) of packets that should be randomly dropped, on this server only, when they are 
received from the network. (Clients have a similar argument to drop packets randomly on receipt; in no case should packets be 
randomly dropped on transmit.)

You may assume these parameters are always specified in that exact order to simplify your command line parsing.
Your server must maintain committed versions of files in the location specified by the -mount parameter. On server startup, if 
this directory already exists, the server MUST die with a message "machine already in use". If such a directory does not exist, 
it MUST create it. Your server MUST NOT remove the mount directory on server termination! For example, if the server is started 
with:
replFsServer -port 4137 -mount /folder1/fs244b -drop 3

And a client opens file "jane.txt" using OpenFile("jane.txt"), the server must create the file: /folder1/fs244b/jane.txt.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "net.h"
#include "utils.h"
#include "protocol.h"
#include "clist.h"



#define MAX_ARG_LEN 100
#define DEFAULT_PORT 41056
#define MAX_IDLE_TIME 24*60
#define MAX_FILE_LEN 128

struct remote_file {
	int remote_fd;
	int local_fd;
	CList *cl;
	//struct sockaddr_in *owner;
};

struct remote_file rf;
char *mountdir = NULL;

void 
process_discover(struct sockaddr_in client)
{
	printf("discover msg received.\n");
	send_discover_ack();
}


void 
process_open(struct replfs_msg *msg, struct sockaddr_in client) 
{ 
	printf("processing open msg...\n");
	struct replfs_msg_open_long *payload = 
										(struct replfs_msg_open_long *) get_payload(msg);
	if (rf.remote_fd == -1)		 {
		//if ((rf.local_fd = open(payload->filename,
		// 										  O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) > 0) {
			rf.remote_fd = payload->fd;
			CListInit(rf.cl);
			printf("sending open success\n");
			send_open_success(payload->fd);
			return;
		//}
	}
	printf("sending open fail\n");
	send_open_fail(payload->fd);


}

void process_write(struct replfs_msg *msg, struct sockaddr_in client) 
{
	printf("processing write msg...\n"); 
}

void process_try_commit(struct replfs_msg *msg, struct sockaddr_in client) 
{
	printf("processing try-commit msg...\n"); 
}

void process_commit(struct replfs_msg *msg, struct sockaddr_in client) 
{
	printf("processing commit msg...\n"); 
}


void
process_msg(struct replfs_msg * msg, struct sockaddr_in client)
{
	printf("msg received.\n");
	switch(msg->msg_type) {

		case MsgDiscover:
			process_discover(client);
			break;
		case MsgDiscoverAck:
			//do nothing
			break;
		case MsgOpen:
			process_open(msg,client);
			break;
		case MsgOpenSuccess:
			//do nothing
			break;
		case MsgOpenFail:
			//do nothing
			break;
		case MsgWrite:
			process_write(msg,client);
			break;
		case MsgTryCommit:
			process_try_commit(msg,client);
			break;
		case MsgTryCommitFail:
			//do nothing
			break;
		case MsgTryCommitSuccess:
			//do nothing
			break;
		case MsgCommit:
			process_commit(msg,client);
			break;
		case MsgCommitSuccess:
			//do nothing
			break;
		case MsgCommitFail:
			//do nothing
			break;
		default:
			printf("unknown msg type.\n");
			break;
	}
}

void
run_server()
{
	printf("server running...\n");

	struct sockaddr_in client;
	struct timeval deadline;
	gettimeofday(&deadline,NULL);
	deadline.tv_sec += MAX_IDLE_TIME;

	char buf[BUFFER_SIZE];
	while (true) {
			netRecv(buf, BUFFER_SIZE, &client,deadline);
			process_msg((struct replfs_msg *) buf, client);
			deadline.tv_sec += MAX_IDLE_TIME;
			//extension: send keep alive message
	}
}

int
main(int argc, char *argv[]) {

	unsigned short port = DEFAULT_PORT;
	int drop = 0;

	for (int i=1; i<argc-1;i++) 
	{
		if (!strncmp(argv[i], "-port",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid port number");
			port = atoi(argv[++i]);
		}

		else if (!strncmp(argv[i],"-mount",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid mount directory");
			mountdir = malloc(strlen(argv[i+1])+1);
			strcpy(mountdir,argv[++i]);
		}

		else if (!strncmp(argv[i], "-drop",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid drop percentage");
			drop = atoi(argv[++i]);
		}

	}

	/* empty file */
	rf.remote_fd = -1;
	rf.local_fd = -1;
	rf.cl = NULL;


	printf("launching file server...\n");
	printf("port: %d, mountdir: %s, drop: %d\n", port, mountdir, drop);

	if (netInit(port,drop) )
		ERROR("unable to connect to network.\n");

	run_server();

	/************************
	char *msg = "hello world!";
	if (drop == 1) {
		netSend(msg,strlen(msg));
	}
	else {

		char buf[128];
		char str_addr[128];
		struct sockaddr_in sender;
		memset(&sender,0,sizeof(struct sockaddr_in));
		//sender.sin_family = AF_INET;

		unsigned int msglen;
		struct timeval deadline;
		gettimeofday(&deadline,NULL);
		deadline.tv_sec += 100;
		if ((msglen = netRecv(buf,128,&sender,deadline)) > 0) 
		{
			buf[msglen] = '\0';
			inet_ntop(AF_INET, &(sender.sin_addr), str_addr, INET_ADDRSTRLEN);
			printf("msg: [%s], len:[%d] received from: [%s] \n",buf,msglen,
																													str_addr);
		}
		else
			printf("no income received.\n");
	}
	*************************/

	printf("closing file server...\n");

	netClose();
	if (mountdir)
		free(mountdir);

}


