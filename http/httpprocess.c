/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpprocess.c,v 1.2 1997/04/14 04:36:28 dustin Exp $
 */

#define IWANTDOCINFO 1

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

void http_senddoc(int s, struct http_request r)
{
    FILE *f;
    char buf[2048];

    strcpy(buf, conf.webroot);
    strcat(buf, r.request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

    f=fopen(buf, "r");
    while(!feof(f))
    {
    fgets(buf, 2048, f);
    puttext(s, buf);
    }
    fclose(f);
}

void http_process_get(int s, struct http_request r)
{
    http_header_ok(s);

    if(r.special==1)
    {
        puttext(s, "<html><head><title>OK</title></head>");
        puttext(s, "<body bgcolor=\"ffffff\">");
        puttext(s, "Your special request was ");
        puttext(s, r.request);
    }
    else
    {
        http_senddoc(s, r);
    }

    http_footer(s);
}

int http_verifydoc(int s, struct http_request *r)
{
    int i;
    char buf[2048];

    for(i=0; i<NDOCS; i++)
    {
    if(strcmp(r->request, docnames[i])==0)
    {
        break;
    }
    }

    if(i==NOTADOC)
    {
        strcpy(buf, conf.webroot);
        strcat(buf, r->request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

        if(access(buf, R_OK)==0)
        {
        r->special=0;
        return(0);
        }

    }
    else
    {
        r->special=1;
        r->docnum=i;
        return(0);
    }
}

void http_process(int s, struct http_request r)
{
    if(http_verifydoc(s, &r)!=0)
    {
        http_header_notfound(s, r);
    }

    if(conf.debug)
    {
        puts("Request was good");
        if(r.special==1)
            puts("It was a special request");
        else
            puts("It was a normal request");
    }

    switch(r.method)
    {
        case HTTP_GET:
            http_process_get(s, r); break;

        default:
            http_error(s, r);
    }
}
