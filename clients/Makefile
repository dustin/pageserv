# Copyright (c) 1997   Dustin Sallings
# $Id: Makefile,v 1.2 1997/03/13 00:31:45 dustin Exp $

MAJOR=2
MINOR=0
PATCH=0
VERSION=$(MAJOR).$(MINOR).$(PATCH)

CC=cc
INCLUDES=-I..
CFLAGS=-g $(INCLUDES) -DVERSION=\"$(VERSION)\"

LIBS=

# For Solaris
# LIBS=-lnsl -lsocket

LDFLAGS=-g $(LIBS)

PUSH_OBJS=pushqueue.o client.o
SOURCES=pushqueue.c
EXES=pushqueue
NAME=clients
ARCHIVE=$(NAME)-$(VERSION)
STUFF=$(SOURCES) Makefile pushqueue.pl
JUNK=$(EXES) *.o *.core core

all: $(EXES)

pushqueue: $(PUSH_OBJS)
	$(CC) -o $@ $(PUSH_OBJS) $(LDFLAGS)

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
