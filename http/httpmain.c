/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmain.c,v 1.1 1997/04/14 03:51:25 dustin Exp $
 */

#define IWANTMETHODNAMES 1

#include <config.h>
#include <pageserv.h>
#include <http.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

void http_onalarm()
{
    if(conf.debug>2)
        puts("Web server received alarm, exiting...");

    exit(0);
}

void httpmain(int s)
{
    struct http_request r;
    char buf[BUFLEN];

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
