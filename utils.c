#include <stdio.h>
#include "utils.h"
#include "protocol.h"

struct timeval time_diff(struct timeval ta, struct timeval tb)
{
  struct timeval diff;
  diff.tv_sec = ta.tv_sec - tb.tv_sec;
  if (ta.tv_usec > tb.tv_usec) {
    diff.tv_usec = ta.tv_usec - tb.tv_usec;
  } else {
    diff.tv_usec = ta.tv_usec - tb.tv_usec + MICROSEC_IN_SEC;
    diff.tv_sec--;
  }
  return diff;
}

long time_diff_ms(struct timeval ta, struct timeval tb)
{
  long diff = (ta.tv_sec - tb.tv_sec) * MILLISEC_IN_SEC +
              (ta.tv_usec - tb.tv_usec) / MICROSEC_IN_MILLISEC;
  return diff;
}

struct timeval time_sum(struct timeval ta, struct timeval tb)
{
  struct timeval sum;
  long sum_usec = ta.tv_usec + tb.tv_usec;
  sum.tv_usec = sum_usec % MICROSEC_IN_SEC;
  sum.tv_sec = ta.tv_sec + tb.tv_sec + (sum_usec / MICROSEC_IN_SEC);
  return sum;
}

void print_write_log(CVector *wlog) 
{
  for (int i=0; i<CVectorCount(wlog); i++) {
    struct write_block *wb = (struct write_block *)CVectorNth(wlog,i);
    printf("wid: [%d], fd: [%d], offset: [%d], len: [%d] data:[%c]\n",
            wb->wid,wb->fd,wb->offset,wb->len,wb->data[0]);
  }
}