/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.11 1998/01/10 01:32:57 dustin Exp $
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

static void _pageserv_init(void);
static void _pageserv_main(modpass p);
static int _pageserv_socket(void);

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

static int _pageserv_socket(void)
{
    if(mod_pageserv.listening)
        return(mod_pageserv.s);
    else
	return(-1);
}

static void _pageserv_init(void)
{
    if(conf.pageserv==1)
    {
        mod_pageserv.s=getservsocket(conf.pageport);

        if(mod_pageserv.s>=0)
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

static void child_onalarm()
{
    _ndebug(2, ("Received alarm, exiting.\n"));
    exit(0);
}

/* Child's main loop.  Called immediately after parent's fork() */

static void _pageserv_main(modpass p)
{
    char buf[BUFLEN];

    /* Cheezy welcome banner */
    sprintf(buf, MESG_WELCOME, VERSION);
    puttext(p.socket, buf);

    /* Child will only live a certain number of seconds */
    alarm(conf.childlifetime);
    signal(SIGALRM, child_onalarm);

    puttext(p.socket, PROMPT_CMD);
    gettextcr(p.socket, buf);
    process(p.socket, buf, p);

    _mod_pageserv_exit(p.socket, 0);
}
