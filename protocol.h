
enum msg_type_t {
	msg_discover,
	msg_discover_ack,
	msg_open,
	msg_open_ack,
	msg_write_block,
	msg_try_commit,
	msg_try_commit_fail,
	msg_try_commit_success,
	msg_commit,
	msg_commit_success,
	msg_commit_fail
};



struct repfs_msg {
	msg_type_t	msg_type;
	int len;
	int checksum;
};

struct repfs_msg_open {
	char filename[128];
	int fid;
};

struct replfs_msg_open_ack {
	int fid;
};

struct replfs_msg_write {
	int fid;
	int offset;
	int len;
	int wid;
};

struct repfs_commit {
	int fid;
};

