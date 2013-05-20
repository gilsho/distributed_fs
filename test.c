#include "client.h"
#include "utils.h"
#include <stdio.h>

#define PORT 41056
#define NUM_SERVERS 2
#define DROP 0

int
main(int argc, char *argv[])
{
	printf("test module\n");
	if (InitReplFs(PORT,DROP,NUM_SERVERS) != NormalReturn)
		return -1;
	OpenFile("abc.txt");
	

}