/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpprocess.c,v 1.18 1998/01/01 09:40:50 dustin Exp $
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

#if 0
void http_senddoc(int s, struct http_request r)
{
    FILE *f;
    char buf[2048];
    struct stat st;

    strcpy(buf, conf.webroot);
    strcat(buf, r.request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

    http_checkauth(s, r, buf);

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
#endif

void _http_process_get(int s, struct http_request r, modpass p)
{
    if(r.special==1)
    {
        switch(r.docnum)
        {
            case DOC_MODUSER: _http_moduser(s, r);   break;
            case DOC_SENDPAGE: _http_sendpage(s, r, p); break;
            case DOC_ADMIN: _http_admin(s, r); break;
        }

        _http_footer(s);
    }
    else
    {
	/* run it through lex */
	_http_parse(s, r);
    }
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

void http_process(int s, struct http_request r, modpass p)
{
    if(http_verifydoc(s, &r)!=0)
    {
        _http_header_notfound(s, r);
    }

    _ndebug(2, ("Request was good, it was a %s request.\n",
                r.special==1?"special":"normal"));

    switch(r.method)
    {
        /* Get and post can use the same function */
        case HTTP_GET:
        case HTTP_POST:
            _http_process_get(s, r, p); break;

        default:
            _http_error(s, r);
    }
}
