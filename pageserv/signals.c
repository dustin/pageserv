/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: signals.c,v 1.13 1998/03/03 16:46:54 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>

#define _BSD_SIGNALS
#include <signal.h>

#include <pageserv.h>

extern struct config conf;

static RETSIGTYPE serv_sigint(int sig)
{
    _ndebug(0, ("Exit type signal caught, shutting down...\n"));

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
    _ndebug(0, ("Setting signals...\n"));

    signal(SIGINT, serv_sigint);
    signal(SIGQUIT, serv_sigint);
    signal(SIGTERM, serv_sigint);
    signal(SIGHUP, serv_sighup);
    signal(SIGCHLD, serv_sigchld);
    signal(SIGALRM, SIG_IGN);
}

void resethappytraps(void)
{
    _ndebug(0, ("Setting signals to defaults...\n"));

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}
