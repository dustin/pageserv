/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpprocess.c,v 1.8 1997/04/18 20:27:34 dustin Exp $
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
#include <sys/stat.h>

extern struct config conf;

void http_senddoc(int s, struct http_request r)
{
    FILE *f;
    char buf[2048], buf2[BUFLEN];
    struct stat st;

    strcpy(buf, conf.webroot);
    strcat(buf, r.request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

    if(r.version>0)
    {
        stat(buf, &st);
	_http_header_ok(s, (int)st.st_size+strlen(HTTP_FOOTER)+1);
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
    struct http_list *tmp;

    if(r.special==1)
    {
	_http_parseargs(s, &r);

        switch(r.docnum)
	{
	    case DOC_USERMOD: break;
	    case DOC_SENDPAGE: _http_sendpage(s, r); break;
	}
    }
    else
    {
        http_senddoc(s, r);
    }

    _http_footer(s);
}

int http_verifydoc(int s, struct http_request *r)
{
    int i, ret=1;
    char buf[2048];

    for(i=0; i<NDOCS; i++)
    {
        if(strcmp(r->request, docnames[i])==0)
        {
	    ret=0;
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
	    ret=0;
        }
    }
    else
    {
        r->special=1;
        r->docnum=i;
	ret=0;
    }

    return(ret);
}

void http_process(int s, struct http_request r)
{
    if(http_verifydoc(s, &r)!=0)
    {
        _http_header_notfound(s, r);
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
            _http_error(s, r);
    }
}
