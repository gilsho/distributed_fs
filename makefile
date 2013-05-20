
CFLAGS	= -g -Wall -std=c99 -DSUN
# CFLAGS	= -g -Wall -DDEC
CC	= gcc
CCF	= $(CC) $(CFLAGS)

H	= .
C_DIR	= .

INCDIR	= -I$(H)
LIBDIRS = -L$(C_DIR)
LIBS    = -lclientReplFs

CLIENT_OBJECTS = client.o net.o cvector.o cmap.o utils.o protocol.o clist.o

all:	appl server test

appl:	appl.o $(C_DIR)/libclientReplFs.a
	$(CCF) -o appl appl.o $(LIBDIRS) $(LIBS)

appl.o:	appl.c client.h appl.h
	$(CCF) -c $(INCDIR) appl.c

$(C_DIR)/libclientReplFs.a:	$(CLIENT_OBJECTS)
	ar cr libclientReplFs.a $(CLIENT_OBJECTS)
	ranlib libclientReplFs.a

# net.o: net.c net.h utils.c utils.h
#	$(CCF) -c $(INCDIR) net.c utils.o

# client.o:	client.c client.h
#	$(CCF) -c $(INCDIR) client.c utils.o

#server.o: server.c
# $(CCF) -c $(INCDIR) server.c

server: server.o client.o net.o cvector.o cmap.o utils.o protocol.o clist.o
	$(CCF) $(INCDIR) -o replFsServer server.o net.o utils.o protocol.o clist.o

test: test.o $(C_DIR)/libclientReplFs.a
	$(CCF) $(INCDIR) -o tst test.o $(LIBDIRS) $(LIBS)

.o: utils.c
	$(CCF) $(INCDIR) $@.c -o $@.o utils.o

clean:
	rm -f appl replFsServer *.o *.a tst 

