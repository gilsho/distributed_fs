
#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/time.h>
#include "protocol.h"
#include "cvector.h"

#define ERROR(msg) {fprintf(stderr,msg); fprintf(stderr,"\n"); return(-1);}

#define BUFFER_SIZE   1024
#define ADDR_STR_SIZE 128

#define MILLISEC_IN_SEC 1000
#define MICROSEC_IN_SEC 1000000
#define MICROSEC_IN_MILLISEC 1000

struct timeval time_diff(struct timeval ta, struct timeval tb);
long time_diff_ms(struct timeval ta, struct timeval tb);
struct timeval time_sum(struct timeval ta, struct timeval tb);
int checksum(struct replfs_msg *msg);

void print_write_log(CVector *wlog);

#endif