/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmain.c,v 1.2 1997/04/16 06:10:28 dustin Exp $
 */

#define IWANTMETHODNAMES 1

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
    char buf[BUFLEN];
    int s;

    s=p.socket;

    alarm(conf.childlifetime);
    signal(SIGALRM, http_onalarm);

    r=http_parserequest(s);

    http_process(s, r);

    close(s);
    exit(0);

    puttext(s, "HTTP/1.0 200 OK\n\n<html><head><title>Dustin's Pager Server");
    puttext(s, "</title></head><body bgcolor=\"#fFfFfF\"><h2>");
    puttext(s, "Dustin's Pager Server</h2>");
    puttext(s, "Sorry, but the web listener isn't quite done yet...<br>\n");
    puttext(s, "Your request was: ");
    puttext(s, r.request);
    puttext(s, "</body></html>\n");

    close(s);
    exit(0);
}
