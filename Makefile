# Copyright (c) 1997   Dustin Sallings
# $Id: Makefile,v 1.2 1997/03/11 19:10:25 dustin Exp $

CC=cc
CFLAGS=-O2 -g -Wall
LDFLAGS=-g

MAJOR=2
MINOR=2
PATCH=2
VERSION=$(MAJOR).$(MINOR).$(PATCH)

SERV_OBJS=sockets.o main.o utility.o kids.o queue.o protocol.o
SOURCES=sockets.c main.c utility.c kids.c queue.c protocol.c
EXES=pageserv
NAME=pageserv
STUFF=$(SOURCES) pageserv.h Makefile
JUNK=$(SERV_OBJS) $(EXES) *.core

all: pageserv

pageserv: $(SERV_OBJS)
	$(CC) -o $@ $(SERV_OBJS)

tgz: pageserv.tar.gz

pageserv.tar.gz: clean
	rm -f pageserv.tar.gz
	mkdir $(NAME)
	cp $(STUFF) $(NAME)
	tar -cvf $(NAME).tar $(NAME)
	rm -rf $(NAME)
	gzip -9v $(NAME).tar

clean:
	rm -f $(JUNK)
