/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmain.c,v 1.6 1997/07/31 07:31:31 dustin Exp $
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

module mod_webserv={
    _http_init,
    _http_main,
    _http_socket,
    "Web module",
    0,
    0
};

void http_onalarm()
{
    if(conf.debug>2)
        puts("Web server received alarm, exiting...");

    exit(0);
}

int _http_socket(void)
{
    if(mod_webserv.listening)
        return(mod_webserv.s);
    else
        return(-1);
}

void _http_init(void)
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

void _http_main(modpass p)
{
    struct http_request r;
    int s;

    s=p.socket;

    alarm(conf.childlifetime);
    signal(SIGALRM, http_onalarm);

    r=http_parserequest(s);

    http_process(s, r);

    _http_free_request(r);

    close(s);
    exit(0);
}
