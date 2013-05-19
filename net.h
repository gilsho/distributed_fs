
int netInit(unsigned short port,int drop);
int netSend(void *buf, size_t n);
size_t netRecv(void *buf, size_t n, long *src_addr, int timeout_ms);
int netClose();
