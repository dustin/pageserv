/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: signals.c,v 1.9 1998/01/10 01:33:08 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>

#define _BSD_SIGNALS
#include <signal.h>

#include <pageserv.h>

extern struct config conf;

static RETSIGTYPE serv_sigint(int sig)
{
    if(conf.debug>0)
	puts("Exit type signal caught, shutting down...");

    unlink(conf.pidfile);
    cleanconfig();
    exit(0);
}

static RETSIGTYPE serv_sighup(int sig)
{
    cleanconfig();
    rdconfig(CONFIGFILE);

    if(conf.debug>0)
    {
	puts("Server reconfiguring...\n");
	showconfig();
    }
    resetservtraps();
}

static RETSIGTYPE serv_sigchld(int sig)
{
    reaper();
    resetservtraps();
}

void resetservtraps(void)
{
    if(conf.debug>0)
	puts("Setting signals...");

    signal(SIGINT, serv_sigint);
    signal(SIGQUIT, serv_sigint);
    signal(SIGTERM, serv_sigint);
    signal(SIGHUP, serv_sighup);
    signal(SIGCHLD, serv_sigchld);
}

static RETSIGTYPE del_sigint(int sig)
{
    cleanmylocks();
    exit(1);
}

void resetdelivertraps(void)
{
    if(conf.debug>0)
	puts("Setting signals...");

    signal(SIGINT, del_sigint);
    signal(SIGQUIT, del_sigint);
    signal(SIGTERM, del_sigint);
    signal(SIGHUP, del_sigint);
    signal(SIGALRM, del_sigint);
}
