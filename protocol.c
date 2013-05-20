#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "protocol.h"

int checksum(struct replfs_msg *msg)
{
	int cksum = msg->cksum;		
	msg->cksum = 0;	//exclude checksum field
  char *ptr = (char *)msg;
  int sum = 0;
  for (size_t i=0; i < msg->len; i++) {
    sum += ptr[i];
  }
  msg->cksum = cksum;	
  return sum;
}


bool
valid_msg(struct replfs_msg *msg)
{
	return (checksum(msg) == msg->cksum);
}

void *
get_payload(struct replfs_msg *msg)
{
	return (((char *)msg) + sizeof(struct replfs_msg));
}

void 
send_discover()
{
	struct replfs_msg msg;
	msg.msg_type = MsgDiscover;
	msg.len = sizeof(struct replfs_msg);
	msg.cksum = checksum(&msg);
	//printf("sending discover.\n");
	netSend(&msg,msg.len);
}


void send_discover_ack()
{
	struct replfs_msg msg;
	msg.msg_type = MsgDiscoverAck;
	msg.len = sizeof(struct replfs_msg);
	msg.cksum = checksum(&msg);
	//printf("sending discover ack.\n");
	netSend(&msg,msg.len);
}



#include <stdio.h>
void 
send_open(char *filename, int fd)
{
	struct replfs_msg *msg;
	struct replfs_msg_open_long *payload;

	int len = sizeof(struct replfs_msg) + sizeof(struct replfs_msg_open_long); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = MsgOpen;
	msg->len = len;

	payload = (struct replfs_msg_open_long *) get_payload(msg);
	strcpy(payload->filename,filename); //extension: make a safe version.
	payload->fd = fd;

	msg->cksum = checksum(msg);
	//printf("sending open.\n");
	netSend(msg,msg->len);
	free(msg);
}

void
send_open_fail(int fd)
{
	struct replfs_msg *msg;
	struct replfs_msg_open *payload;

	int len = sizeof(struct replfs_msg) + sizeof(struct replfs_msg_open); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = MsgOpenFail;
	msg->len = len;

	payload = (struct replfs_msg_open *) get_payload(msg);
	payload->fd = fd;

	msg->cksum = checksum(msg);
	//printf("sending open ack.\n");
	netSend(msg,msg->len);
	free(msg);
}


void
send_open_success(int fd)
{
	struct replfs_msg *msg;
	struct replfs_msg_open *payload;

	int len = sizeof(struct replfs_msg) + sizeof(struct replfs_msg_open); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = MsgOpenSuccess;
	msg->len = len;

	payload = (struct replfs_msg_open *) get_payload(msg);
	payload->fd = fd;

	msg->cksum = checksum(msg);
	//printf("sending open ack.\n");
	netSend(msg,msg->len);
	free(msg);
}


void
send_write(int fd, int offset, int wlen, void *data, int wid)
{
	struct replfs_msg *msg;
	struct replfs_msg_write *payload;

	int len = sizeof(struct replfs_msg) + sizeof(struct replfs_msg_write); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = MsgWrite;
	msg->len = len;

	payload = (struct replfs_msg_write *) get_payload(msg);
	payload->fd = fd;
	payload->offset = offset;
	payload->wlen = wlen;
	payload->wid = wid;

	msg->cksum = checksum(msg);
	//printf("sending write.\n");
	netSend(msg,msg->len);
	free(msg);
}

void
send_generic_commit(int fd, int from_wid, int to_wid, enum msg_type_t msg_type)
{
	struct replfs_msg *msg;
	struct replfs_msg_commit *msg_commit;

	int len = sizeof(struct replfs_msg) + sizeof(struct replfs_msg_commit); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = msg_type;
	msg->len = len;

	msg_commit = (struct replfs_msg_commit *) get_payload(msg);
	msg_commit->fd = fd;
	msg_commit->from_wid = from_wid;
	msg_commit->to_wid = to_wid;

	msg->cksum = checksum(msg);
	//printf("sending try commit.\n");
	netSend(msg,msg->len);
	free(msg);
}


void 
send_try_commit(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd,from_wid,to_wid,MsgTryCommit);
}



void
send_try_commit_fail(int fd, int wids[], int n)
{
	struct replfs_msg *msg;
	struct replfs_msg_commit_long *payload;

	int len = sizeof(struct replfs_msg) + 
						sizeof(struct replfs_msg_commit_long) + (n-1)*sizeof(int); 

	msg = (struct replfs_msg *) malloc(len);
	msg->msg_type = MsgTryCommitFail;
	msg->len = len;

	payload = (struct replfs_msg_commit_long *) get_payload(msg);
	payload->fd = fd;
	payload->n = n;
	memcpy(&payload->data,wids,n*sizeof(int));

	msg->cksum = checksum(msg);
	//printf("sending try commit fail.\n");
	netSend(msg,msg->len);
	free(msg);
}

void
send_try_commit_success(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd, from_wid, to_wid, MsgTryCommitSuccess);

}

void 
send_commit(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd, from_wid, to_wid, MsgCommit);
}

void 
send_commit_success(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd, from_wid, to_wid,MsgCommitSuccess);
}

void 
send_commit_fail(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd, from_wid, to_wid,MsgCommitFail);
}

void 
send_abort(int fd, int from_wid, int to_wid)
{
	send_generic_commit(fd, from_wid, to_wid, MsgAbort);
}




