/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.7 1997/12/29 07:44:49 dustin Exp $
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
#include <module.h>

extern struct config conf;

module mod_pageserv={
    _pageserv_init,
    _pageserv_main,
    _pageserv_socket,
    "Pageserv module",
    0,
    0
};

/* The reaper is called after every connection or every 120 seconds,
 * whichever is sooner.  It waits on all the children that have died
 * off.
 */

int _pageserv_socket(void)
{
    if(mod_pageserv.listening)
        return(mod_pageserv.s);
    else
	return(-1);
}

void _pageserv_init(void)
{
    mod_pageserv.s=getservsocket(conf.pageport);

    if(mod_pageserv.s>=0)
    {
	mod_pageserv.listening=1;
    }
}

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

void _pageserv_main(modpass p)
{
    char buf[BUFLEN];
    int flag, s;

    s=p.socket;

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
    process(s, buf, p);

    exit(0);
}
