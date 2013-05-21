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

#define TIMEOUT_CONNECT 1000
#define TIMEOUT_OPEN    1000
#define RETRY_CONNECT 5
#define RETRY_OPEN    5

CVector *servers;
//CMap *mapserv;


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
  else if (sa->sin_addr.s_addr < sb->sin_addr.s_addr)
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

int 
open_remote(int fd, char *filename, long timeout_ms)
{
  char buf[BUFFER_SIZE];
  struct replfs_msg *msg;
  struct replfs_msg_open *payload;

  CVector *responders = CVectorCreate(sizeof(struct sockaddr_in), 
                                      CVectorCount(servers),NULL);

  /* send open */
  send_open(filename, fd);

  /* collect reponses */
  struct timeval deadline,now;
  gettimeofday(&now,NULL);
  deadline = compute_deadline(now,timeout_ms);

  struct sockaddr_in s;
  while (true)
  {
    printf("%d servers reponded.\n",CVectorCount(responders));
    gettimeofday(&now,NULL);
    if (CVectorCount(responders) >= CVectorCount(servers)) {
      return NormalReturn;
    }
    
    if (time_diff_ms(deadline,now) < 1) {
      return ErrorReturn;
    }
    
    if (netRecv(buf, BUFFER_SIZE, &s, deadline) > 0) {
      msg = (struct replfs_msg *) buf;
      payload = (struct replfs_msg_open *)get_payload(msg);
      printf("payload->fd: %d, fd: %d, msg type: %d, MsgOpenSuccess: %d\n",
              payload->fd, fd, msg->msg_type, MsgOpenSuccess);
      if (msg->msg_type == MsgOpenSuccess && 
          payload->fd == fd) {
          printf("recieved open success\n");
          if (known_server(&s) && new_responder(responders,&s)) {
              printf("new response form known server\n");
              CVectorAppend(responders, &s);
        } else if (msg->msg_type == MsgOpenFail && payload->fd == fd) {
          return ErrorReturn;
        }
      }
    }
  }
  //execution thread shouldn't get here
  return ErrorReturn;
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

  int success = -1;
  for (int i=0; i<RETRY_CONNECT; i++)
    if ((success = locate_servers(numServers,TIMEOUT_CONNECT)) == NormalReturn)
      break;

  if (success != NormalReturn)
    ERROR("unable to locate servers");

  printf("connection established.\n");
  print_servers();

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
  int fd;

  ASSERT( fileName );

  printf( "OpenFile: Opening File '%s'\n", fileName );

  fd = open( fileName, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR );

  if ( fd < 0 )
    ERROR("unable to open the file locally");

  if (open_remote(fd,fileName,TIMEOUT_OPEN) == ErrorReturn)
    ERROR("unable to open file remotely");

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

	/****************/
	/* Commit Phase */
	/****************/

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

  return(NormalReturn);
}

/* ------------------------------------------------------------------ */

void 
CloseReplFs()
{
  netClose();
  CVectorDispose(servers);
}


