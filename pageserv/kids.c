/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.4 1997/04/13 22:01:00 dustin Exp $
 * $State: Exp $
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <pageserv.h>

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

void child_onalarm()
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
    int flag;

    /* Get rid of that bastard Nagle algorithm */
    flag=1;

    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
	sizeof(int)) <0)
    {
	if(conf.debug>0)
	    puts("Looks like we're sticking with ol' Nagle.");
    }

    /* Cheezy welcome banner */
    sprintf(buf, MESG_WELCOME, VERSION);
    puttext(s, buf);

    /* Child will only live a certain number of seconds */
    alarm(conf.childlifetime);
    signal(SIGALRM, child_onalarm);

    puttext(s, PROMPT_CMD);
    gettextcr(s, buf);
    process(s, buf);

    exit(0);
}
