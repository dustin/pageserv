/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmisc.c,v 1.4 1997/04/18 20:27:31 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <http.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

void _http_error(int s,  struct http_request r)
{
    puttext(s, "HTTP/1.1 501 Unimplemented\n\n");
    puttext(s, "<h1>501 Not Implemented</h1>Sorry, but that's not here yet.\n");
    close(s);
    exit(0);
}

void _http_header_ok(int s, int size)
{
    char buf[2048], buf2[40];

    strcpy(buf, "HTTP/1.1 200 OK\nContent-type: text/html\n");
    strcat(buf, "Server: Dustin's Pager Server ");
    strcat(buf, VERSION);
    strcat(buf, "\n");
    if(size>0)
    {
	sprintf(buf2, "Content-Length: %d\n", size);
	strcat(buf, buf2);
    }
    strcat(buf, "\n");
    puttext(s, buf);
}

void _http_header_notfound(int s, struct http_request r)
{
    puttext(s, "HTTP/1.0 404 Not Found\nContent-type: text/html\n\n");
    puttext(s, "<html><head><title>404 Not Found</title><body \n");
    puttext(s, "bgcolor=\"#fFfFff\"><h2>404 Not Found</h2>I'm sorry,");
    puttext(s, "but the document you requested:  ");
    puttext(s, r.request);
    puttext(s, " Doesn't exist.  Thank you, now go away\n");
    _http_footer(s);
    close(s);
    exit(0);
}

void _http_footer(int s)
{
    puttext(s, HTTP_FOOTER);
}
