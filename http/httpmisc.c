/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmisc.c,v 1.1 1997/04/14 03:51:28 dustin Exp $
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

void http_error(int s,  struct http_request r)
{
    puttext(s, "HTTP/1.0 501 Unimplemented\n\n");
    puttext(s, "<h1>501 Not Implemented</h1>Sorry, but that's not here yet.\n");
    close(s);
    exit(0);
}

void http_header_ok(int s)
{
    puttext(s, "HTTP/1.0 200 OK\nContent-type: text/html\n\n");
}

void http_header_notfound(int s, struct http_request r)
{
    puttext(s, "HTTP/1.0 404 Not Found\nContent-type: text/html\n\n");
    puttext(s, "<html><head><title>404 Not Found</title><body \n");
    puttext(s, "bgcolor=\"#fFfFff\"><h2>404 Not Found</h2>I'm sorry,");
    puttext(s, "but the document you requested:  ");
    puttext(s, r.request);
    puttext(s, " Doesn't exist.  Thank you, now go away\n");
    http_footer(s);
    close(s);
    exit(0);
}

void http_footer(int s)
{
    puttext(s, "<p><hr align=\"left\" width=\"50%\">");
    puttext(s, "<font size=\"-2\"> Copyright &copy;  1997 Dustin Sallings");
    puttext(s, "</font></body></html>");
}
