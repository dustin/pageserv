/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.7 1997/04/10 06:23:46 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pageserv.h>

struct config conf;

void writepid(int pid)
{
    FILE *f;

    if(access(conf.pidfile, F_OK)==0)
    {
	puts("PID file found, what's up with that?  I'll overwrite it.");
    }

    if(NULL ==(f=fopen(conf.pidfile, "w")) )
    {
	perror(conf.pidfile);
	return;
    }

    fprintf(f, "%d\n", pid);

    fclose(f);
}

void detach(void)
{
   int pid;

   pid=fork();

   if(pid>0)
   {
       printf("Running on PID %d\n", pid);
       writepid(pid);
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

    resetservtraps(); /* set signal traps */

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

    eraseuserdb();
    erasetermdb();

    i=parseusers();
    printf("Parsed %d users.\n", i);

    i=parseterms();
    printf("Parsed %d terminal servers.\n", i);
}

void killserver(void)
{
    int pid;
    FILE *f;

    if(access(conf.pidfile, F_OK)!=0)
    {
	puts("Error, no PID file found, is it running?");
	exit(1);
    }

    f=fopen(conf.pidfile, "r");
    if(f==NULL)
    {
	perror(conf.pidfile);
	exit(1);
    }

    if( fscanf(f, "%d", &pid) == EOF)
    {
	puts("Error:  No PID found in pidfile");
	exit(1);
    }

    kill(pid, SIGTERM);
    sleep(1);

    if(access(conf.pidfile, F_OK)==0)
    {
	puts("Error, pid file still exists, may not have shut down properly");
	exit(1);
    }
    else
    {
	puts("Successfully shut down.");
	exit(1);
    }

    fclose(f);
    exit(0);
}

void ldb_main(void)
{
    puts("Users:\n------------");
    printusers();
    puts("\nTerminals:\n------------");
    printterms();
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
            printqueue(); break;

	case MODE_VERS:
	    showversion(); break;

        case MODE_RUNQ:
	    runqueue(); break;

        case MODE_KILL:
	    killserver(); break;

    }
    cleanconfig();
}
