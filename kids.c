/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.8 1997/03/26 07:26:15 dustin Exp $
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "pageserv.h"

extern struct config conf;

/* The reaper is called after every connection or every 120 seconds,
 * whichever is sooner.  It waits on all the children that have died
 * off.
 */

void reaper(void)
{
    int pid=1;
    while(pid>0)
    {
        pid=waitpid(0, NULL, WUNTRACED | WNOHANG);

        /* Debugging information */
	if(conf.debug>2)
	{
	    if(pid>0)
	        printf("Killed off child %d\n", pid);
	}
    }
}

/* Brilliant alarm handling by Dustin Sallings */

void onalarm()
{
    if(conf.debug>2)
    {
	puts("Received alarm, exiting.");
    }
    exit(0);
}

/* Child's main loop.  Called immediately after parent's fork() */

void childmain(int s)
{
    char buf[BUFLEN];

    /* Cheezy welcome banner */
    sprintf(buf, MESG_WELCOME, VERSION);
    puttext(s, buf);

    /* Child will only live a certain number of seconds */
    alarm(conf.childlifetime);
    signal(SIGALRM, onalarm);

    puttext(s, PROMPT_CMD);
    gettextcr(s, buf);
    process(s, buf);

    exit(0);
}
