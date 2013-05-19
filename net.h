#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int netInit(unsigned short port,int drop);
int netSend(void *buf, size_t n);
size_t netRecv(void *buf, size_t n, struct sockaddr_in *sender, int timeout_ms);
int netClose();
