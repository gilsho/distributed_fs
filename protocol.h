
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

struct replfs_msg_write {
	int fd;
	int offset;
	int wlen;
	int wid;
};

struct replfs_msg_commit {
	int fd;
	int from_wid;
	int to_wid;
};

struct replfs_msg_commit_long {
	int fd;
	int n;
	int data[];
};



int checksum(struct replfs_msg *msg);

void *get_payload(struct replfs_msg *msg);

bool valid_msg(struct replfs_msg *msg);

void send_discover();

void send_discover_ack();

void send_open(char *filename, int fd);

void send_open_fail(int fd);

void send_open_success(int fd);

void send_write(int fd, int offset, int wlen, void *data, int wid);

void send_try_commit(int fd, int from_wid, int to_wid);

void send_try_commit_fail(int fd, int wids[], int n);

void send_try_commit_success(int fd, int from_wid, int to_wid);

void send_commit(int fd, int from_wid, int to_wid);

void send_commit_success(int fd, int from_wid, int to_wid);

void send_commit_fail(int fd, int from_wid, int to_wid);

void send_abort(int fd, int from_wid, int to_wid);



#endif
