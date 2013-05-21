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
#include "cvector.h"



#define MAX_ARG_LEN 100
#define DEFAULT_PORT 41056
#define MAX_IDLE_TIME 24*60
#define MAX_FILE_LEN 128

int remote_fd;
char filepath[2*MAX_FILE_LEN];
CVector *wlog;
//struct sockaddr_in *owner;

char mountdir[MAX_FILE_LEN];

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
	remote_fd = -1; //REMOVE ME!									
	if (remote_fd == -1)		 {
		//create the file
		strcpy(filepath,mountdir);
		strcat(filepath,payload->filename);
		int local_fd = open(payload->filename,
		 										  O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (local_fd > 0) {
			close(local_fd);
			remote_fd = payload->fd;
			wlog = CVectorCreate(sizeof(struct write_block),0,wbfree);
			printf("sending open success\n");
			send_open_success(payload->fd);
		} else {
			printf("sending open fail\n");
			send_open_fail(payload->fd);
		}
	}

}

void
process_close(struct replfs_msg *msg, struct sockaddr_in client)
{
	printf("processing close msg...\n");
	struct replfs_msg_open *payload = 
										(struct replfs_msg_open*) get_payload(msg);
	if (payload->fd == remote_fd) {
		remote_fd = -1;
		// printf("write log\n");
		// printf("--------------------------\n");
		// print_write_log(wlog);
		if (wlog)
			CVectorDispose(wlog);
		wlog = NULL;
		printf("sending close success\n");
		send_close_success(payload->fd);
	} else {
		printf("sending close fail\n");	
		send_close_fail(payload->fd);
	}

}

void process_write(struct replfs_msg *msg, struct sockaddr_in client) 
{
	printf("processing write msg...\n"); 
	struct write_block *payload = (struct write_block *) get_payload(msg);
	if (payload->fd != remote_fd)
		return;

	void *dataload = ((char *)payload) + sizeof(struct write_block);
	payload->data = malloc(payload->len);
	memcpy(payload->data,dataload,payload->len);
	CVectorAppend(wlog,payload);
}

void clear_write_log(int from_wid)
{
	if (from_wid == -1) {
		CVectorDispose(wlog);
		wlog = CVectorCreate(sizeof(struct write_block),0,wbfree);
		return;
	}
	for(int i=0;i<CVectorCount(wlog);) {
		struct write_block *wb;
		wb = (struct write_block *) CVectorNth(wlog,i);
		if (wb->wid < from_wid) 
			CVectorRemove(wlog,i);
		else
			i++;
	}
}

/* range is inclusive */
void append_range(CVector *missing, int from, int to)
{
	for (int i=from; i<=to; i++) {
		// printf("(appending %d) ",i);
		CVectorAppend(missing,&i);
	}
}

CVector *missing_writes(int from_wid, int to_wid)
{
	CVectorSort(wlog,(CVectorCmpElemFn) wbcmp);
	CVector *missing = CVectorCreate(sizeof(int),0,NULL);
	struct write_block *curwb = (struct write_block *)CVectorFirst(wlog);
	while (curwb && curwb->wid <= to_wid)
	{
		// printf("curwb->wid: %d, from_wid: %d, to_wid: %d ",
			// curwb->wid,from_wid,to_wid);
		if (curwb->wid >= from_wid) {
			if (curwb->wid > from_wid) 
				append_range(missing,from_wid,curwb->wid-1);
			from_wid = curwb->wid+1;
		}
		curwb = (struct write_block *) CVectorNext(wlog,curwb);
		// printf("next: %p\n", curwb);
	}
	append_range(missing,from_wid,to_wid);
	return missing;
}

void process_try_commit(struct replfs_msg *msg, struct sockaddr_in client) 
{
	printf("processing try-commit msg...\n");
	struct replfs_msg_commit *payload = 
							(struct replfs_msg_commit *) get_payload(msg);
	
	if (payload->fd != remote_fd)
		return; 
	
	clear_write_log(payload->from_wid);
	CVector *missing = missing_writes(payload->from_wid, payload->to_wid);
	if (CVectorCount(missing) == 0) {
		send_try_commit_success(payload->fd, payload->from_wid, payload->to_wid);
	} else {
		int n;
		void * dataload = CVectorToArray(missing,&n);
		send_try_commit_fail(payload->fd,payload->from_wid,
																		 payload->to_wid,dataload,n);
		free(dataload);
	}

	

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
		case MsgClose:
			process_close(msg,client);
			break;
		case MsgCloseFail:
			//do nothing;
			break;
		case MsgCloseSuccess:
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
	strcpy(mountdir,".");

	for (int i=1; i<argc-1;i++) 
	{
		if (!strncmp(argv[i], "-port",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid port number");
			port = atoi(argv[++i]);
		}

		else if (!strncmp(argv[i],"-mount",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid mount directory");
			strcpy(mountdir,argv[++i]);
		}

		else if (!strncmp(argv[i], "-drop",MAX_ARG_LEN)) {
			if (*argv[i+1] == '-') ERROR("invalid drop percentage");
			drop = atoi(argv[++i]);
		}

	}

	strcat(mountdir,"/");

	/* empty file */
	remote_fd = -1;
	wlog = NULL;


	printf("launching file server...\n");
	printf("port: %d, mountdir: %s, drop: %d\n", port, mountdir, drop);

	if (netInit(port,drop) )
		ERROR("unable to connect to network.\n");

	run_server();

	printf("closing file server...\n");

	netClose();

}


