/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.8 1997/03/26 07:26:15 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "pageserv.h"

struct config conf;

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

    readconfig(CONFIGFILE);
    if(conf.debug>0)
        showconfig();

    if(conf.debug==0)
        detach();

    s=initialize();

    FD_ZERO(&tfdset);
    FD_SET(s, &tfdset);

    for(;;)
    {
        fdset=tfdset;
        t.tv_sec=conf.childlifetime;
        t.tv_usec=0;
        fromlen=sizeof(fsin);

        if( select(s+1, &fdset, NULL, NULL, &t) > 0)
        {
             if( (ns=accept(s, (struct sockaddr *)&fsin, &fromlen)) >=0 )
             {
                 pid=fork();

                 if(pid==0)
		 {
		     /* Run child's main loop */
                     childmain(ns);
		 }
                 else
		 {
		     /* parent just closes its copy of the socket */
                     close(ns);

		     if(conf.debug>2)
			 printf("Spawned new child on pid %d\n", pid);
		 }
             }
        }
        reaper();
    }

    cleanconfig();
}
