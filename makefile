
CFLAGS	= -g -Wall -std=c99 -DSUN
# CFLAGS	= -g -Wall -DDEC
CC	= gcc
CCF	= $(CC) $(CFLAGS)

H	= .
C_DIR	= .

INCDIR	= -I$(H)
LIBDIRS = -L$(C_DIR)
LIBS    = -lclientReplFs

CLIENT_OBJECTS = client.o net.o

all:	appl

appl:	appl.o $(C_DIR)/libclientReplFs.a
	$(CCF) -o appl appl.o $(LIBDIRS) $(LIBS)

appl.o:	appl.c client.h appl.h
	$(CCF) -c $(INCDIR) appl.c

$(C_DIR)/libclientReplFs.a:	$(CLIENT_OBJECTS)
	ar cr libclientReplFs.a $(CLIENT_OBJECTS)
	ranlib libclientReplFs.a

net.o: net.c net.h
	$(CCF) -c $(INCDIR) net.c

client.o:	client.c client.h
	$(CCF) -c $(INCDIR) client.c

server.o: server.c
	$(CCF) -c $(INCDIR) server.c

server: server.o client.o net.o
	$(CCF) $(INCDIR) -o replFsServer server.o net.o

clean:
	rm -f appl *.o *.a

