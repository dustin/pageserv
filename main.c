/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.9 1997/03/29 00:48:51 dustin Exp $
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

void daemon_main(void)
{
    struct sockaddr_in fsin;
    int s, ns, fromlen, pid;
    fd_set fdset, tfdset;
    struct timeval t;

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

}

void rehash_main(void)
{
    int i;

    i=parseusers();
    printf("Parsed %d users.\n", i);

    i=parseterms();
    printf("Parsed %d terminal servers.\n", i);
}

void ldb_main(void)
{
    puts("Users:\n------------");
    printusers();
    puts("\nTerminals:\n------------");
    printterms();
}

void pq_main(void)
{
    printqueue();
}

void main(int argc, char **argv)
{

    readconfig(CONFIGFILE);
    getoptions(argc, argv);
    if(conf.debug>0)
        showconfig();
    switch(conf.mode)
    {
	case MODE_DAEMON:
	    daemon_main(); break;

	case MODE_REHASH:
	    rehash_main(); break;

	case MODE_LDB:
	    ldb_main(); break;

	case MODE_PQ:
	    pq_main(); break;

    }
    cleanconfig();
}
