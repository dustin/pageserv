/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: sendpage.c,v 1.4 1997/08/07 08:04:22 dustin Exp $
 */

#include <pageserv.h>
#include <http.h>

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

void _http_sendpageerror(int s, char *message)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Error:  ");
    puttext(s, message);
    puttext(s, "</title></head><body bgcolor=\"fFfFfF\"><h2>Error:</h2>");
    puttext(s, message);
}

void _http_sendpagesuccess(int s)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Page accepted</title></head>");
    puttext(s, "<body bgcolor=\"fFfFfF\"><h2>Success!</h2>");
    puttext(s, "Page accepted and queued.");
}

void _http_sendpage(int s, struct http_request r)
{
    struct queuent q;
    char *tmp;

    if(conf.debug>2)
	puts("Sendpage request");

    memset(&q, 0x00, sizeof(q));

    tmp=_http_getcgiinfo(r, "priority");
    if(tmp==NULL)
    {
	_http_sendpageerror(s, "somebody screwed up");
	return;
    }

    if(conf.debug>2)
	printf("Page is being sent priority ``%s''\n", tmp);

    if(tolower(tmp[0])=='h')
        q.priority=PR_HIGH;
    else
        q.priority=PR_NORMAL;

    tmp=_http_getcgiinfo(r, "id");
    if(tmp==NULL)
    {
	_http_sendpageerror(s, "somebody screwed up");
	return;
    }
    strcpy(q.to, tmp);

    if(conf.debug>2)
	printf("Page is being sent to ``%s''\n", tmp);

    tmp=_http_getcgiinfo(r, "message");
    if(tmp==NULL)
    {
	_http_sendpageerror(s, "somebody screwed up");
	return;
    }
    strcpy(q.message, tmp);

    if(conf.debug>2)
	printf("Page being sent is ``%s''\n", tmp);

    if(u_exists(q.to))
    {
	if(storequeue(s, q, STORE_QUIET)>0)
	{
	    _http_sendpageerror(s, MESG_BADTIME);
	}
	else
	{
	    _http_sendpagesuccess(s);
	}
    }
    else
    {
	_http_sendpageerror(s, "no such user");
    }
}
