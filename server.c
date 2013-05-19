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
#include "net.h"

#define MAX_ARG_LEN 100
#define ERROR(msg) {fprintf(stderr,msg); fprintf(stderr,"\n"); exit(-1);}


int
main(int argc, char *argv[]) {

	unsigned short port = -1;
	char *mountdir = NULL;
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

	printf("launching file server...\n");
	printf("port: %d, mountdir: %s, drop: %d\n", port, mountdir, drop);

	netInit(port,drop);

	/************************/
	char *msg = "hello world!";
	if (drop == 1) {
		netSend(msg,strlen(msg));
	}
	else {

		char buf[128];
		char str_addr[128];
		struct sockaddr_in sender;

		unsigned int msglen;
		if ((msglen = netRecv(buf,128,&sender,100000)) > 0) 
		{
			buf[msglen] = '\0';
			//inet_ntop(AF_INET, &sender, str_addr, INET_ADDRSTRLEN);
			printf("msg: [%s], len:[%d] received from: [%s] \n",buf,msglen,
																										inet_ntoa(sender.sin_addr));
		}
		else
			printf("no income received.\n");
	}
	/*************************/

	printf("closing file server...\n");

	netClose();
	if (mountdir)
		free(mountdir);

}


