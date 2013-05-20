#include <sys/types.h> 
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netinet/in.h>
#include "net.h"
#include <stdio.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>
#include "utils.h"


#define MULTICAST_GROUP	 0xe0010101

struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IPv4 Class D multicast address */
    struct in_addr imr_interface; /* IPv4 address of local interface */
};

int sid;
struct ip_mreq mreq;  
struct sockaddr_in sdest;
int packetLoss;


int
netInit(unsigned short portNum, int packetLoss_)
{	

		packetLoss = packetLoss_;
		printf("setting packet loss rate to %d\n",packetLoss);

		/** Allocate a UDP socket and set the multicast options */
		if ((sid = socket(AF_INET,SOCK_DGRAM, 0)) < 0) 
			ERROR("can't create socket");


		/** multicast group information */
		struct sockaddr_in shost;
		shost.sin_family = AF_INET;
		shost.sin_port = htons(portNum);
		shost.sin_addr.s_addr = htonl(INADDR_ANY); 

		sdest.sin_family = AF_INET;
		sdest.sin_port = htons(portNum);
		sdest.sin_addr.s_addr = htonl(MULTICAST_GROUP); 


		/** bind the UDP socket to the mcast address to recv messages 
			  from the group */
		if((bind(sid,(struct sockaddr *) &shost, sizeof(shost))== -1)) 
						ERROR("error in bind");


		/** allow multicast datagrams to be transmitted to any site anywhere 
		   value to ttl set according to the value put in "TimeToLive" variable */
		int ttl = 1;
		if( setsockopt(sid,IPPROTO_IP,IP_MULTICAST_TTL, &ttl,
		         sizeof(int)) == -1 ) ERROR("unable to set ttl value");

		int loop = 0;
		if( setsockopt(sid,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,
		           		 sizeof(int)) == -1 ) ERROR("unable to disable loopback");

		
		/* multicast group info structure */
		mreq.imr_multiaddr.s_addr = htonl(MULTICAST_GROUP);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		if ( setsockopt(sid,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *) &mreq,
			  sizeof(mreq)) == -1 ) ERROR("unable to join group");


		/* set file descriptor mask for select statement */

		return 0;
 }


int
netClose()
{
  
  if( setsockopt(sid,IPPROTO_IP,IP_DROP_MEMBERSHIP,
                (char *) &mreq,sizeof(mreq)) == -1 ) 
  								ERROR("unable to leave group");

  close(sid);
	return 0;
}


int netSend(void *buf, size_t n)
{
	
	return sendto(sid, buf, n, 0, (struct sockaddr *) &sdest, 
							sizeof(struct sockaddr));


}

#define MAX
size_t netRecv(void *buf, size_t n, struct sockaddr_in *sender, 
							 struct timeval deadline)
{

	socklen_t slen = sizeof(struct sockaddr);
	ssize_t msglen = 0;

	struct timeval tstart;
	gettimeofday(&tstart,NULL);
	struct timeval to_wait = time_diff(deadline,tstart);

	fd_set fdmask;
	FD_ZERO(&fdmask);
	FD_SET(sid, &fdmask);

	while (true) {
		if (select(sid+1,&fdmask,NULL,NULL,&to_wait) > 0) {
			int r = rand() %100;

			struct sockaddr_in s;
			int templen = recvfrom(sid, buf,n, 0,(struct sockaddr *)&s,&slen);
			char str_addr[INET_ADDRSTRLEN];
			*str_addr = '\0';
			inet_ntop(AF_INET, &(s.sin_addr), str_addr, INET_ADDRSTRLEN);
			printf("received [%d] bytes from [%s]\n",templen, str_addr);
			if (r > packetLoss && valid_msg(buf)) {
				msglen = templen;
				if (sender) memcpy(sender, &s, sizeof(struct sockaddr_in));
				break;
			}
			printf("dropping on floor...\n");
		} else {
			break;
		}
	}

	return msglen;
}






