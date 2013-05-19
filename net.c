#include <sys/types.h> 
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <net.h>
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


#define ERROR(msg) {fprintf(stderr,msg "\n"); return -1;}
#define MULTICAST_GROUP	 0xe0010101

struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IPv4 Class D multicast address */
    struct in_addr imr_interface; /* IPv4 address of local interface */
};

int sid;
struct ip_mreq mreq;  
struct sockaddr_in sdest;
fd_set fdmask;
int droprate;


int
netInit(unsigned short port, int drop)
{	

		droprate = drop;
		printf("setting drop rate to %d\n",droprate);

		/** Allocate a UDP socket and set the multicast options */
		if ((sid = socket(AF_INET,SOCK_DGRAM, 0)) < 0) 
			ERROR("can't create socket");


		/** multicast group information */
		struct sockaddr_in shost;
		shost.sin_family = AF_INET;
		shost.sin_port = htons(port);
		shost.sin_addr.s_addr = htonl(INADDR_ANY); 

		sdest.sin_family = AF_INET;
		sdest.sin_port = htons(port);
		sdest.sin_addr.s_addr = htonl(MULTICAST_GROUP); 


		/** bind the UDP socket to the mcast address to recv messages 
			  from the group */
		if((bind(sid,(struct sockaddr *) &shost, sizeof(shost))== -1)) ERROR("error in bind");


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
		FD_ZERO(&fdmask);
		FD_SET(sid, &fdmask);

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
size_t netRecv(void *buf, size_t n, long *src_addr, int timeout_ms)
{

	struct sockaddr_in ssrc;
	ssrc.sin_addr.s_addr = 0;
	unsigned int slen = 0;
	ssize_t msglen = 0;
	
	struct timeval to_wait;
	to_wait.tv_sec = timeout_ms/1000;
	to_wait.tv_usec = (timeout_ms % 1000)*1000;

	struct timeval tlast;
	gettimeofday(&tlast,NULL);

	while (true) {
		printf("sec: %ld, usec: %ld\n", to_wait.tv_sec, to_wait.tv_usec);
		//if (select(sid+1,&fdmask,NULL,NULL,&to_wait) > 0) {
			int r = rand() %100;
			printf("r: %d, droprate: %d\n",r,droprate);
			int templen = recvfrom(sid, buf,n, 0,(struct sockaddr *)&ssrc,&slen);
			if (r > droprate) {
				msglen = templen;
				break;
			} else {
				struct timeval now;
				gettimeofday(&now,NULL);
				to_wait.tv_sec = to_wait.tv_sec - (now.tv_sec - tlast.tv_sec);
				to_wait.tv_usec = to_wait.tv_usec - (now.tv_usec - tlast.tv_usec);
				memcpy(&tlast,&now,sizeof(struct timeval));
			}
		//} else {
		//	break;
		//}
	}

	if (src_addr) *src_addr = ntohl(ssrc.sin_addr.s_addr);
	return msglen;
}






