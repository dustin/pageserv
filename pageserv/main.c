/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.12 1997/04/15 21:51:04 dustin Exp $
 * $State: Exp $
 */

#include <config.h>
#include <pageserv.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>

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
    int s, ws, ns, fromlen, pid, upper;
    fd_set fdset, tfdset;
    struct timeval t;

    upper=0;

    if(conf.debug==0)
        detach();

    resetservtraps(); /* set signal traps */

    FD_ZERO(&tfdset);

    s=getservsocket(PORT);
    if(s>upper)
	upper=s;

    /* Tell select to listen to it */
    FD_SET(s, &tfdset);

    if(conf.webserver)
    {
        ws=getservsocket(conf.webport);

        if(ws>upper)
	    upper=ws;

	/* Tell select to listen to it, too */
        FD_SET(ws, &tfdset);

    }

    upper++;  /* one more, just because */

    for(;;)
    {
        fdset=tfdset;
        t.tv_sec=conf.childlifetime;
        t.tv_usec=0;
        fromlen=sizeof(fsin);

        if( select(upper, &fdset, NULL, NULL, &t) > 0)
        {
	    if(FD_ISSET(s, &fdset))
	    {
		if(conf.debug>2)
		    puts("Got a connection on the normal port");

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

	    if(FD_ISSET(ws, &fdset) && conf.webserver)
	    {
		if(conf.debug>2)
		    puts("Got a connection on the web port");

                if( (ns=accept(ws, (struct sockaddr *)&fsin, &fromlen)) >=0 )
                {
                    pid=fork();

                    if(pid==0)
		    {
		        /* Run web child's main loop */
                        httpmain(ns);
		    }
                    else
		    {
		        /* parent just closes its copy of the socket */
                        close(ns);

		        if(conf.debug>2)
			    printf("Spawned new child on pid %d\n", pid);
		    }
                }
	    } /* end of finding the selections */
        } /* end of select loop */
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

void changepasswd(void)
{
    char buf[BUFLEN];
    struct user u;

    fputs("User's password to change:  ", stdout);
    fgets(buf, BUFLEN, stdin);
    kw(buf);

    if(!u_exists(buf))
    {
	puts("No such user.");
	exit(1);
    }

    u=getuser(buf);

    fputs("User's new password:  ", stdout);
    fgets(buf, BUFLEN, stdin);
    kw(buf);

    u=setpasswd(u, buf);
    storeuser(u);

    puts("Password set.");
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

        case MODE_PWCH:
	    changepasswd(); break;
    }
    cleanconfig();
}
