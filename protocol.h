
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stddef.h>
#include <stdbool.h>

enum msg_type_t {
	MsgDiscover,
	MsgDiscoverAck,
	MsgOpen,
	MsgOpenFail,
	MsgOpenSuccess,
	MsgClose,
	MsgCloseFail,
	MsgCloseSuccess,
	MsgWrite,
	MsgTryCommit,
	MsgTryCommitFail,
	MsgTryCommitSuccess,
	MsgCommit,
	MsgCommitFail,
	MsgCommitSuccess,
	MsgAbort
};

struct replfs_msg {
	enum msg_type_t	msg_type;
	size_t len;
	int cksum;
};

struct replfs_msg_open_long {
	char filename[128];
	int fd;
};

struct replfs_msg_open {
	int fd;
};

struct replfs_msg_commit {
	int fd;
	int from_wid;
	int to_wid;
};

struct replfs_msg_commit_long {
	int fd;
	int from_wid;
	int to_wid;
	int n;
};

struct write_block {
   int fd;
   int wid;
   int offset; 
   int len;
   char *data;
};

int checksum(struct replfs_msg *msg);

void *get_payload(struct replfs_msg *msg);

bool valid_msg(struct replfs_msg *msg);

void send_discover();

void send_discover_ack();

void send_open(char *filename, int fd);

void send_open_fail(int fd);

void send_open_success(int fd);

void send_close(int fd);

void send_close_fail(int fd);

void send_close_success(int fd);

void send_write(struct write_block *wb);

void send_try_commit(int fd, int from_wid, int to_wid);

void send_try_commit_fail(int fd, int from_wid, int to_wid,int wids[], int n);

void send_try_commit_success(int fd, int from_wid, int to_wid);

void send_commit(int fd, int from_wid, int to_wid);

void send_commit_success(int fd, int from_wid, int to_wid);

void send_commit_fail(int fd, int from_wid, int to_wid);

void send_abort(int fd, int from_wid, int to_wid);

int wbcmp(void *wba, void *wbb);

void wbfree(void *wb);

#endif
