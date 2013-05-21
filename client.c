/****************/
/* Your Name	*/
/* Date		*/
/* CS 244B	*/
/* Spring 2013	*/
/****************/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "client.h"
#include <netinet/in.h>

#include "utils.h"
#include "net.h"
#include "protocol.h"
#include "cvector.h"
#include "utils.h"
#include "clist.h"

#define TIMEOUT_CONNECT     1000
#define TIMEOUT_OPEN        1000
#define TIMEOUT_CLOSE       1000
#define TIMEOUT_TRY_COMMIT  1000
#define TIMEOUT_COMMIT      1000

#define RETRY_CONNECT     10
#define RETRY_TRY_COMMIT  50
#define RETRY_COMMIT      10
#define RETRY_OPEN        10
#define RETRY_CLOSE       100

CVector *servers;
CVector *wlog;

int widcount = 1;

int next_wid(){
  return widcount++;
}

void
reset_log()
{
  if (wlog)
    CVectorDispose(wlog);
  wlog = NULL;
}

void 
print_servers()
{
  printf("server list:\n");
  printf("----------------------------------\n");
  for (int i=0; i<CVectorCount(servers); i++) {
    char str[ADDR_STR_SIZE];
    inet_ntop(AF_INET, 
              &(((struct sockaddr_in *)CVectorNth(servers,i))->sin_addr), 
              str, INET_ADDRSTRLEN);
    printf("[%2d] %s\n",i,str);
  }
  printf("----------------------------------\n");
}

int 
sockcmp(const void *a, const void *b)
{
  struct sockaddr_in *sa = (struct sockaddr_in *)a;
  struct sockaddr_in *sb = (struct sockaddr_in *)b;

  if (sa->sin_addr.s_addr > sb->sin_addr.s_addr)
    return 1;
  if (sa->sin_addr.s_addr < sb->sin_addr.s_addr)
    return -1;
  
  return 0;
}

int
intcmp(const void *a, const void *b)
{
  int ia = *(int *)a;
  int ib = *(int *)b;
  if (ia > ib)
    return 1;
  if (ia < ib)
    return -1;
  return 0;
}

struct timeval 
compute_deadline(struct timeval now, long timeout_ms)
{
  struct timeval timeout;
  timeout.tv_sec = timeout_ms / MILLISEC_IN_SEC;
  timeout.tv_usec = (timeout_ms % MILLISEC_IN_SEC) * MICROSEC_IN_MILLISEC;
  return time_sum(now,timeout);
}

bool
known_server(struct sockaddr_in *s)
{
  return (CVectorSearch(servers,s,sockcmp,0,false) != -1);
}

int 
locate_servers(int numServers, long timeout_ms)
{
  char buf[BUFFER_SIZE];
  struct replfs_msg *msg;

  /* send discover message */
  send_discover();

  /* gather responses */
  struct timeval deadline,now;
  gettimeofday(&now,NULL);
  deadline = compute_deadline(now,timeout_ms);

  struct sockaddr_in s;
  while (true)
  {
    printf("currently found %d servers.\n",CVectorCount(servers));
    gettimeofday(&now,NULL);
    if (CVectorCount(servers) >= numServers) {
      CVectorSort(servers, sockcmp);
      return NormalReturn;
    }
    
    if (time_diff_ms(deadline,now) < 1) {
      return ErrorReturn;
    }
    
    if (netRecv(buf, BUFFER_SIZE, &s, deadline) > 0) {
      msg = (struct replfs_msg *) buf;
      if (msg->msg_type == MsgDiscoverAck) 
        if (!known_server(&s))
          CVectorAppend(servers,&s);
    }

  } 

  //execution thread shouldn't get here
  return ErrorReturn;
}

bool
new_responder(CVector *responders, struct sockaddr_in *s)
{
  return (CVectorSearch(responders,s,sockcmp,0,false) == -1);
}

enum MsgHandlerResponse {
  NeutralResponse,
  FatalResponse,
  SuccessReponse
};

typedef enum MsgHandlerResponse (* MsgHandlerFn)
               (struct replfs_msg *msg, void *aux);

enum MsgHandlerResponse open_handler(struct replfs_msg *msg, void *aux)
{
  struct replfs_msg_open *payload = (struct replfs_msg_open *)get_payload(msg);
  int fd = *(int *)aux;

  if (payload->fd != fd)
    return NeutralResponse;

  if (msg->msg_type == MsgOpenFail)
    return FatalResponse;

  if (msg->msg_type == MsgOpenSuccess && payload->fd == fd)
    return SuccessReponse;
  
  return NeutralResponse;

}

enum MsgHandlerResponse close_handler(struct replfs_msg *msg, void *aux)
{
  struct replfs_msg_open *payload = (struct replfs_msg_open *)get_payload(msg);
  int fd = *(int *)aux;

  if (payload->fd != fd)
    return NeutralResponse;

  if (msg->msg_type == MsgCloseFail)
    return FatalResponse;
  
  if (msg->msg_type == MsgCloseSuccess)
    return SuccessReponse;
  
  return NeutralResponse;

}

enum MsgHandlerResponse try_commit_handler(struct replfs_msg *msg, void *aux)
{
  struct replfs_msg_commit_long *payload = 
                (struct replfs_msg_commit_long *)get_payload(msg);
  int *dataload = (int *) (((char *)payload) + 
                            sizeof(struct replfs_msg_commit_long));
  CVector *missing = (CVector *)aux;
  
  //what about checking fd? and range?
  //if (payload->fd != )
  //  return NeutralResponse;

  if (msg->msg_type == MsgTryCommitSuccess)
    return SuccessReponse;

  if (msg->msg_type == MsgTryCommitFail) {
    for (int i=0; i< payload->n; i++)
      if (CVectorSearch(missing,&dataload[i],intcmp,0,false) == -1)
        CVectorAppend(missing,&dataload[i]);
    return FatalResponse;
  }
  
  return NeutralResponse;

}

enum MsgHandlerResponse commit_handler(struct replfs_msg *msg, void *aux)
{
  // struct replfs_msg_commit *payload = 
  //               (struct replfs_msg_commit *)get_payload(msg);
  //what about checking fd? and range?
  //if (payload->fd != )
  //  return NeutralResponse;

  if (msg->msg_type == MsgCommitSuccess)
    return SuccessReponse;

  if (msg->msg_type == MsgCommitFail)
    return FatalResponse;
  
  return NeutralResponse;

}

int 
collect_responses(CVector *responders, MsgHandlerFn fn, 
                  void *aux,long timeout_ms)
{
  char buf[BUFFER_SIZE];
  struct replfs_msg *msg;
  struct timeval deadline,now;
  gettimeofday(&now,NULL);
  deadline = compute_deadline(now,timeout_ms);

  struct sockaddr_in s;
  while (true) {
    printf("%d servers reponded.\n",CVectorCount(responders));
    gettimeofday(&now,NULL);
    if (CVectorCount(responders) >= CVectorCount(servers))
      return NormalReturn; 

    if (time_diff_ms(deadline,now) < 1)
      return ErrorReturn;
    
    if (netRecv(buf, BUFFER_SIZE, &s, deadline) > 0) {
      msg = (struct replfs_msg *) buf;
      enum MsgHandlerResponse mhr = fn(msg,aux);
      if (mhr == SuccessReponse) {
          printf("recieved successful response\n");
          if (known_server(&s) && new_responder(responders,&s)) {
              printf("new response from known server\n");
              CVectorAppend(responders, &s);
        } else if (mhr == FatalResponse) {
          printf("received failure repsonse\n");
          return ErrorReturn;
        }
      }
    }
  }
  //execution thread shouldn't get here
  return ErrorReturn;
}

void retransmit(CVector *missing)
{
  printf("retransmitting %d writes.\n",CVectorCount(missing));
  CVectorRemoveDuplicate(missing, intcmp);
  for (int i=0; i<CVectorCount(missing); i++) {
    struct write_block wb;
    wb.wid = *(int *)CVectorNth(missing,i);
    printf("retrying wid: %d\n", wb.wid);
    int index;
    if((index = CVectorSearch(wlog,&wb,(CVectorCmpElemFn) wbcmp,0,true)) > 0) {
      send_write((struct write_block *) CVectorNth(wlog,index));
    }
  }
}

/* ------------------------------------------------------------------ */
/*
InitReplFs() gives your filesystem a chance to perform any startup tasks and informs your system which UDP port # it is to use. 
It is also given the percentage (0 to 100) of packets that should be randomly dropped, on the client only, when they are received 
from the network. (Servers have a similar argument to drop packets randomly on receipt; in no case should packets be randomly dropped 
on transmit.) Finally, numServers specifies the number of servers that must be present in order to successfully commit changes to a 
file (you can assume servers will not come and go while your client is running). Your system does not have to function correctly if 
InitReplFs() is not called prior to using the other calls. 

Return value: 0 (NormalReturn) 
Return value: -1 on failure (ErrorReturn) MAY be returned if a server is unavailable. 
*/
int
InitReplFs( unsigned short portNum, int packetLoss, int numServers ) {
#ifdef DEBUG
  printf( "InitReplFs: Port number %d, packet loss %d percent, %d servers\n", 
	  portNum, packetLoss, numServers );
#endif

  /****************************************************/
  /* Initialize network access, local state, etc.     */
  /****************************************************/
  if (netInit(portNum,packetLoss))
    ERROR("connection failed");

  servers = CVectorCreate(sizeof(struct sockaddr_in), numServers,NULL);
  int success = ErrorReturn;
  for (int i=0; i<RETRY_CONNECT; i++)
    if ((success = locate_servers(numServers,TIMEOUT_CONNECT)) == NormalReturn)
      break;

  if (success != NormalReturn)
    ERROR("unable to locate servers");

  printf("connection established.\n");
  print_servers();

  reset_log();

  return( NormalReturn );  
}

/* ------------------------------------------------------------------ */
/*
OpenFile() takes the name of a file and returns a file descriptor or error code. The server must not truncate existing files when 
reopened. Your code only needs to handle flat names; it does not need to handle names containing a slash. Your implementation MUST 
handle names up to at least 127 bytes in length, including the null terminator. Your server must open this exact file relative to 
your mount directory; this will be used to test your code. 

Return value: a file descriptor >0 on success. 
Return value: -1 on failure (ErrorReturn) MAY be returned if a server is unavailable. It MAY also be returned if the implementation 
can only handle one open file at a time and the application is attempting to open a second. 
*/

int
OpenFile( char * fileName ) {

  ASSERT( fileName );

  printf( "OpenFile: Opening File '%s'\n", fileName );

  int fd = open( fileName, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR );

  if ( fd < 0 )
    ERROR("unable to open the file locally");

  CVector *responders = CVectorCreate(sizeof(struct sockaddr_in), 
                                      CVectorCount(servers),NULL);
  int success = ErrorReturn;
  for (int i=0; i<RETRY_OPEN; i++) {
    send_open(fileName, fd);
    if ((success = collect_responses(responders,open_handler,
                        (void *)&fd, TIMEOUT_OPEN)) == NormalReturn)
      break;
  }

  CVectorDispose(responders);
  if (success != NormalReturn)
    ERROR("unable to open file remotely");

  wlog = CVectorCreate(sizeof(struct write_block),0,wbfree);

  printf("file opened successfully\n");

  return fd;
}

/* ------------------------------------------------------------------ */
/*
WriteBlock() stages a contiguous chunk of data to be written upon Commit(). fd is the file descriptor of the file to write to, 
as returned by a previous call to OpenFile. buffer is a pointer to the data to be written, and blockSize is the number of bytes in 
buffer. byteOffset is the offset within the file to write to. Your code must handle overlapping writes. 

Return value: The number of bytes staged (blockSize). 
Return value: -1 (ErrorReturn) MAY be returned if the client violates the maximum block size, file size, or number of staged writes 
before Commit(). Assumptions you may make about these limits are listed here. 
This MUST be returned if the file descriptor is invalid. This value MUST NOT be returned if a server is unavailable. 

*/

int
WriteBlock( int fd, char * buffer, int byteOffset, int blockSize ) {
  //char strError[64];
  int bytesWritten;

  ASSERT( fd >= 0 );
  ASSERT( byteOffset >= 0 );
  ASSERT( buffer );
  ASSERT( blockSize >= 0 && blockSize < MaxBlockLength );

#ifdef DEBUG
  printf( "WriteBlock: Writing FD=%d, Offset=%d, Length=%d\n",
	fd, byteOffset, blockSize );
#endif

  if ( lseek( fd, byteOffset, SEEK_SET ) < 0 ) {
    perror( "WriteBlock Seek" );
    return(ErrorReturn);
  }

  if ( ( bytesWritten = write( fd, buffer, blockSize ) ) < 0 ) {
    perror( "WriteBlock write" );
    return(ErrorReturn);
  }

  int wid = next_wid(fd);  
  struct write_block wb;
  wb.fd = fd;
  wb.wid = wid;
  wb.data = malloc(blockSize);
  memcpy(wb.data,buffer,blockSize);
  wb.offset = byteOffset;
  wb.len = blockSize;
  CVectorAppend(wlog,&wb);

  send_write(&wb);


  return( bytesWritten );

}

/* ------------------------------------------------------------------ */
/*
Commit() takes a file descriptor and commits all writes made via WriteBlock() since the last Commit() or Abort(). 

Return value: 0 (NormalReturn) on success (i.e., changes made were committed). 
Return value: -1 (ErrorReturn) MUST be returned if the client had outstanding changes and a server is unavailable 
(so the changes could not be committed). It MAY be returned if the client had no outstanding changes and a server is 
unavailable. 


*/
int
Commit( int fd ) {
  ASSERT( fd >= 0 );

#ifdef DEBUG
  printf( "Commit: FD=%d\n", fd );
#endif

	/****************************************************/
	/* Prepare to Commit Phase			    */
	/* - Check that all writes made it to the server(s) */
	/****************************************************/

  int first_wid = ((struct write_block *) CVectorNth(wlog,0))->wid;
  int last_wid = ((struct write_block *)
                      CVectorNth(wlog,CVectorCount(wlog)-1))->wid;

  CVector *responders = CVectorCreate(sizeof(struct sockaddr_in), 
                                      CVectorCount(servers),NULL);
  CVector *missing = CVectorCreate(sizeof(int),0,NULL);

  int success = ErrorReturn;
  for (int i=0; i<RETRY_TRY_COMMIT; i++) {
    send_try_commit(fd, first_wid, last_wid);
    if ((success = collect_responses(responders,try_commit_handler,
                        missing, TIMEOUT_TRY_COMMIT)) == NormalReturn)
      break;
    retransmit(missing);
  }
  CVectorDispose(missing);
  CVectorDispose(responders);

  if (success != NormalReturn)
    ERROR("first phase of commit failed");


	/****************/
	/* Commit Phase */
	/****************/

  responders = CVectorCreate(sizeof(struct sockaddr_in), 
                                      CVectorCount(servers),NULL);
  success = ErrorReturn;
  for (int i=0; i<RETRY_COMMIT; i++) {
    send_commit(fd, first_wid, last_wid);
    if ((success = collect_responses(responders,commit_handler,
                        &fd, TIMEOUT_COMMIT)) == NormalReturn)
      break;
  }
  CVectorDispose(responders);

  if (success != NormalReturn)
    ERROR("second phase of commit failed");

  printf("commit successful\n");

  reset_log();
  return( NormalReturn );

}

/* ------------------------------------------------------------------ */
/*
Abort() takes a file descriptor and discards all changes since the last commit. 

Return value: 0 (NormalReturn). 
Return value: -1 (ErrorReturn) may be returned if the file descriptor is invalid. 

*/

int
Abort( int fd )
{
  ASSERT( fd >= 0 );

#ifdef DEBUG
  printf( "Abort: FD=%d\n", fd );
#endif


  /*************************/
  /* Abort the transaction */
  /*************************/

  if (!wlog)
    return NormalReturn;
  
  int first_wid = ((struct write_block *) CVectorNth(wlog,0))->wid;
  int last_wid = ((struct write_block *)
                      CVectorNth(wlog,CVectorCount(wlog)-1))->wid;

  send_abort(fd,first_wid,last_wid);
  reset_log();

  return(NormalReturn);
}

/* ------------------------------------------------------------------ */
/*
Close() relinquishes all control that the client had with the file. 
Close() also attempts to commit all changes that have not been saved. 

Return values: same as in Commit().
*/
int
CloseFile( int fd ) {

  ASSERT( fd >= 0 );

#ifdef DEBUG
  printf( "Close: FD=%d\n", fd );
#endif

	/*****************************/
	/* Check for Commit or Abort */
	/*****************************/

  if ( close( fd ) < 0 ) {
    perror("Close");
    return(ErrorReturn);
  }

  if (wlog && CVectorCount(wlog) > 0)
    Commit(fd);

  reset_log();

  CVector *responders = CVectorCreate(sizeof(struct sockaddr_in), 
                                      CVectorCount(servers),NULL);

  int success = ErrorReturn;
  for (int i=0; i<RETRY_CLOSE; i++) {
    send_close(fd);
    if ((success = collect_responses(responders,close_handler,
                        (void *)&fd, TIMEOUT_CLOSE)) == NormalReturn)
      break;
  }

  /* attempt to commit */

  if (success != NormalReturn)
    ERROR("unable to close file remotely");

  printf("file closed.\n");

  return(NormalReturn);
}

/* ------------------------------------------------------------------ */

void 
CloseReplFs()
{
  netClose();
  CVectorDispose(servers);
}


