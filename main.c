/*
 * Copyright (c{) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.3 1997/03/11 21:05:46 dustin Exp $
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
       printf("Running on PID %d\n", pid);
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
        fromlen=sizeof(fsin);

	if( select(s+1, &fdset, NULL, NULL, &t) > 0)
	{
	     if( (ns=accept(s, (struct sockaddr *)&fsin, &fromlen)) >=0 )
             {
	         pid=fork();

	         if(pid==0)
	             childmain(ns);
	         else
		     close(ns);
             }
	     else
	     {
		 perror("accept");
	     }
	}
	reaper();
    }
}
