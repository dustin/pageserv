/*
 * Copyright (c{) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.1 1997/03/10 07:34:38 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "pageserv.h"

void detach(void)
{
   int pid;

   pid=fork();

   if(pid>0)
   {
       exit(0);
   }
}

void main(void)
{
    struct sockaddr_in fsin;
    int s, ns, fromlen, pid;
    fd_set fdset, tfdset;
    struct timeval t;

    detach();

    s=initialize();

    FD_ZERO(&tfdset);
    FD_SET(s, &tfdset);

    for(;;)
    {
	fdset=tfdset;
        t.tv_sec=CHILD_LIFETIME;
        t.tv_usec=0;

	if( select(s+1, &fdset, NULL, NULL, &t) > 0)
	{
	     ns=accept(s, (struct sockaddr *)&fsin, &fromlen);

	     pid=fork();
	     if(pid==0)
	     {
	         process(ns);
	     }
	     else
	     {
		 close(ns);
	     }
	}
	reaper();
    }
}