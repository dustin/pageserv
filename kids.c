/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.4 1997/03/11 06:47:23 dustin Exp $
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "pageserv.h"

void reaper(void)
{
    int pid=1;
    while(pid>0)
    {
        pid=waitpid(0, NULL, WUNTRACED | WNOHANG);
    }
}

void quit(int s)
{
    puttext(s, MESG_QUIT);
    exit(0);
}

void onalarm()
{
    exit(0);
}

void childmain(int s)
{
    char buf[BUFLEN];

    sprintf(buf, "Welcome to Dustin's pager server version %s.\n", VERSION);
    puttext(s, buf);

    /* lifetime of 120 seconds */
    alarm(CHILD_LIFETIME);
    signal(SIGALRM, onalarm);

    puttext(s, PROMPT_CMD);
    gettext(s, buf);
    process(s, buf);

    exit(0);
}
