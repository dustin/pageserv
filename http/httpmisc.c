/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpmisc.c,v 1.5 1997/07/07 08:47:49 dustin Exp $
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

extern struct config conf;

void _http_error(int s,  struct http_request r)
{
    puttext(s, "HTTP/1.1 501 Unimplemented\n\n");
    puttext(s, "<h1>501 Not Implemented</h1>Sorry, but that's not here yet.\n");
    close(s);
    _http_free_request(r);
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

void _http_header_needauth(int s, char *authname, struct http_request r)
{
    char buf[2048];
    puttext(s, "HTTP/1.0 401 Auth required\n");
    strcpy(buf, "WWW-Authenticate: Basic realm=\"");
    strcat(buf, authname);
    strcat(buf, "\"\n");
    puttext(s, buf);
    puttext(s, "Content-type: text/html\n\n");
    puttext(s, "<html><head><title>401 Auth Required</title><body \n");
    puttext(s, "bgcolor=\"#fFfFfF\"><h2>401 Auth Required</h2>I'm sorry,");
    puttext(s, "but you do not have the proper authentication to get to ");
    puttext(s, r.request);
    puttext(s, ".  Now please go away.\n");
    _http_footer(s);
    close(s);
    _http_free_request(r);
    exit(0);
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
    _http_free_request(r);
    exit(0);
}

void _http_footer(int s)
{
    puttext(s, HTTP_FOOTER);
}

void _http_free_largs(struct http_list *l)
{
    if(conf.debug>4)
        printf("Got http_list entry at %p\n", l);

    if(l == NULL)
        return;

    if(conf.debug>4)
        printf("Freeing http_list entry at %p\n", l);

    if(l->next != NULL)
        _http_free_largs(l->next);

    if(l->name)
        free(l->name);
    if(l->value)
        free(l->value);
}

void _http_free_request(struct http_request r)
{
    if(conf.debug>2)
        puts("Freeing http_request struct members");

    if(r.args)
        free(r.args);

    if(conf.debug>4)
        if(r.largs == NULL)
            puts("Odd, r.largs was NULL");

    _http_free_largs(r.largs);

    if(r.auth.name)
        free(r.auth.name);

    if(r.auth.pass)
        free(r.auth.pass);

    if(r.auth.string)
        free(r.auth.string);
}

void _http_init_request(struct http_request *r)
{
    if(conf.debug>3)
        puts("Initializing http_request struct.");

    /* normal stuff */
    memset(r, 0x00, sizeof(struct http_request));

    /* auth info */
    memset(&r->auth, 0x00, sizeof(struct http_authinfo));
}
