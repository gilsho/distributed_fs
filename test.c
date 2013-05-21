#include "client.h"
#include "utils.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 41056
#define NUM_SERVERS 3
#define DROP 0

int
main(int argc, char *argv[])
{
	printf("test module\n");

	if (InitReplFs(PORT,DROP,NUM_SERVERS) != NormalReturn)
		return -1;
	OpenFile("abc.txt");
	


}