/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: signals.c,v 1.3 1997/04/01 06:57:08 dustin Stab $
 */

#include <stdio.h>
#include <unistd.h>

#define _BSD_SIGNALS
#include <signal.h>

#include <pageserv.h>

extern struct config conf;

RETSIGTYPE serv_sigint(int sig)
{
    if(conf.debug>0)
	puts("Exit type signal caught, shutting down...");

    unlink(conf.pidfile);
    cleanconfig();
    exit(0);
}

RETSIGTYPE serv_sighup(int sig)
{
    cleanconfig();
    readconfig(CONFIGFILE);

    if(conf.debug>0)
    {
	puts("Server reconfiguring...\n");
	showconfig();
    }
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
}