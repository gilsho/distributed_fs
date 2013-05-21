/****************/
/* Your Name	*/
/* Date		*/
/* CS 244B	*/
/* Spring 2013	*/
/****************/


#ifndef __CLIENT_H__
#define __CLIENT_H__

/* ------------------------------------------------------------------ */

#ifdef ASSERT_DEBUG
#define ASSERT(ASSERTION) \
 { assert(ASSERTION); }
#else
#define ASSERT(ASSERTION) \
{ }
#endif

/* ------------------------------------------------------------------ */

	/********************/
	/* Client Functions */
	/********************/
#ifdef __cplusplus
extern "C" {
#endif

extern int InitReplFs(unsigned short portNum, int packetLoss, int numServers);
extern int OpenFile(char * strFileName);
extern int WriteBlock(int fd, char * strData, int byteOffset, int blockSize);
extern int Commit(int fd);
extern int Abort(int fd);
extern int CloseFile(int fd);

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------ */


#endif






