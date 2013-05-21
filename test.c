#include "client.h"
#include "utils.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cvector.h"
#include "protocol.h"

#define PORT 41056
#define NUM_SERVERS 3
#define DROP 0

CVector *wlog;


// p *(struct write_block *)CVectorNth(wlog,0)
int
main(int argc, char *argv[])
{
	char buf[] = "hello world!\n";
	printf("test module\n");

	if (InitReplFs(PORT,DROP,NUM_SERVERS) != NormalReturn)
		return -1;
	int fd = OpenFile("abc.txt");

	WriteBlock(fd, buf, 0, 5);
	WriteBlock(fd, buf+5, 5, 2);
	WriteBlock(fd, buf+7, 7, 1);
	WriteBlock(fd, buf+8, 8, 2);
	WriteBlock(fd, buf+10,10,4);
	Commit(fd);

	CloseFile(fd);

}

