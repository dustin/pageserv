# Copyright (c) 1997   Dustin Sallings
# $Id: Makefile,v 1.7 1997/03/11 22:24:14 dustin Exp $

MAJOR=2
MINOR=0
PATCH=0
VERSION=$(MAJOR).$(MINOR).$(PATCH)

CC=cc
CFLAGS=-O2 -g -Wall -DVERSION=\"$(VERSION)\"

LIBS=

# For Solaris
# LIBS=-lnsl -lsocket

LDFLAGS=-g $(LIBS)

SERV_OBJS=sockets.o main.o utility.o kids.o queue.o protocol.o
PQ_OBJS=pqueue.o utility.o queue.o
SOURCES=sockets.c main.c utility.c kids.c queue.c protocol.c pqueue.c
EXES=pageserv pqueue
NAME=pageserv
ARCHIVE=$(NAME)-$(VERSION)
STUFF=$(SOURCES) pageserv.h Makefile
JUNK=$(EXES) *.o *.core core

all: pageserv pqueue

pageserv: $(SERV_OBJS)
	$(CC) -o $@ $(SERV_OBJS) $(LDFLAGS)

pqueue: $(PQ_OBJS)
	$(CC) -o $@ $(PQ_OBJS) $(LDFLAGS)

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
