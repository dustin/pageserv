/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: kids.c,v 1.2 1997/03/11 05:58:13 dustin Exp $
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

void onalarm()
{
    exit(0);
}

void childmain(int s)
{
    char buf[BUFLEN];
    int size;

    sprintf(buf, "Welcome to Dustin's pager server version %s.\n", VERSION);
    send(s, buf, strlen(buf), 0);

    /* lifetime of 120 seconds */
    alarm(CHILD_LIFETIME);
    signal(SIGALRM, onalarm);

    if( (size=recv(s, buf, BUFLEN-1, 0)) >0 )
    {
         buf[size]=0x00;
         kw(buf);
	 process(s, buf);
    }
    else
    {
         /* If this isn't a pipe, I don't know what the hell... */
         exit(0);
    }
}
