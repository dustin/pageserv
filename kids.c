/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.7 1997/03/24 18:47:16 dustin Exp $
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
    }
}

/* Brilliant alarm handling by Dustin Sallings */

void onalarm()
{
    exit(0);
}

/* Child's main loop.  Called immediately after parent's fork() */

void childmain(int s)
{
    char buf[BUFLEN];

    /* Cheezy welcome banner */
    sprintf(buf, MESG_WELCOME, VERSION);
    puttext(s, buf);

    /* Child will only live CHILD_LIFETIME seconds */
    alarm(CHILD_LIFETIME);
    signal(SIGALRM, onalarm);

    puttext(s, PROMPT_CMD);
    gettextcr(s, buf);
    process(s, buf);

    exit(0);
}
