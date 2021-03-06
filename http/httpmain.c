/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmain.c,v 1.9 1998/01/10 01:32:36 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <http.h>
#include <module.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

static int _http_socket(void);
static void _http_init(void);
static void _http_main(modpass p);

module mod_webserv={
    _http_init,
    _http_main,
    _http_socket,
    "Web module",
    0,
    0
};

static void http_onalarm()
{
    _ndebug(2, ("Web server received alarm, exiting...\n"));

    exit(0);
}

static int _http_socket(void)
{
    if(mod_webserv.listening)
        return(mod_webserv.s);
    else
        return(-1);
}

static void _http_init(void)
{
    if(conf.webserver)
    {
        mod_webserv.s=getservsocket(conf.webport);
        if(mod_webserv.s>=0)
        {
            mod_webserv.listening=1;
        }
        else
        {
            mod_webserv.listening=0;
        }
    }
    else
    {
        mod_webserv.listening=0;
    }
}

static void _http_main(modpass p)
{
    struct http_request r;
    int s;

    s=p.socket;

    alarm(conf.childlifetime);
    signal(SIGALRM, http_onalarm);

    r=http_parserequest(s);

    http_process(s, r, p);

    _http_free_request(r);

    close(s);
    exit(0);
}
