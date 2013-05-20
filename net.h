#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

int netInit(unsigned short portNum, int packetLoss_);
int netSend(void *buf, size_t n);
size_t netRecv(void *buf, size_t n, struct sockaddr_in *sender, 
							 struct timeval deadline);
int netClose();
