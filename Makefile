# Copyright (c) 1997   Dustin Sallings
# $Id: Makefile,v 1.12 1997/03/12 16:43:59 dustin Exp $

MAJOR=2
MINOR=0
PATCH=0
VERSION=$(MAJOR).$(MINOR).$(PATCH)

CC=cc
CFLAGS=-g -DVERSION=\"$(VERSION)\"

LIBS=

# For Solaris
# LIBS=-lnsl -lsocket

LDFLAGS=-g $(LIBS)

SERV_OBJS=sockets.o main.o utility.o kids.o queue.o protocol.o userdb.o
PQ_OBJS=pqueue.o utility.o queue.o userdb.o
PU_OBJS=parseusers.o userdb.o utility.o
LU_OBJS=listusers.o userdb.o utility.o
SOURCES=sockets.c main.c utility.c kids.c queue.c protocol.c pqueue.c \
	parseusers.c userdb.c listusers.c
EXES=pageserv pqueue parseusers listusers
NAME=pageserv
ARCHIVE=$(NAME)-$(VERSION)
STUFF=$(SOURCES) pageserv.h Makefile
JUNK=$(EXES) *.o *.core core

all: $(EXES)

pageserv: $(SERV_OBJS)
	$(CC) -o $@ $(SERV_OBJS) $(LDFLAGS)

pqueue: $(PQ_OBJS)
	$(CC) -o $@ $(PQ_OBJS) $(LDFLAGS)

parseusers: $(PU_OBJS)
	$(CC) -o $@ $(PU_OBJS) $(LDFLAGS)

listusers: $(LU_OBJS)
	$(CC) -o $@ $(LU_OBJS) $(LDFLAGS)

tgz: $(ARCHIVE).tar.gz

$(ARCHIVE).tar.gz: $(STUFF)
	rm -f $(ARCHIVE).tar.gz
	mkdir $(ARCHIVE)
	cp $(STUFF) $(ARCHIVE)
	tar -cvf $(ARCHIVE).tar $(NAME)
	rm -rf $(ARCHIVE)
	gzip -9v $(ARCHIVE).tar

clean:
	rm -f $(JUNK)
