/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: signals.c,v 1.1 1997/04/01 05:42:02 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>

#define _BSD_SIGNALS
#include <signal.h>

#include <pageserv.h>

extern struct config conf;

void serv_sigint()
{
    if(conf.debug>0)
	puts("Exit type signal caught, shutting down...");

    unlink(conf.pidfile);
    cleanconfig();
    exit(0);
}

void serv_sighup()
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
    signal(SIGHUP, serv_sighup);
}
