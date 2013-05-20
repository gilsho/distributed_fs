
#include "utils.h"

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