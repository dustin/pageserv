/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpprocess.c,v 1.11 1997/07/07 08:47:51 dustin Exp $
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

void striptopath(char *pathname)
{
    int i;

    for(i=strlen(pathname)-1; i>0 && pathname[i]!='/'; i--);

    if(i>0)
        pathname[i]=0x00;
}

void http_checkauth(int s, struct http_request r, char *path)
{
    char authname[180];
    char pathname[1024], buf[1024];
    FILE *f;
    int i;

    strcpy(pathname, path);

    authname[0]=0x00;

    while( (authname[0]==0x00) && (strcmp(pathname, conf.webroot)!=0))
    {
        striptopath(pathname);

	strcpy(buf, pathname);
	strcat(buf, "/");
	strcat(buf, ".htaccess");

	if(conf.debug>2)
	    printf("trying ``%s''\n", buf);

	if( (f=fopen(buf, "r")) != NULL)
	{
	    fgets(authname, 180, f);
	    for(i=strlen(authname); (i=='\n' || i=='\r' || i==' ') &&
	        i>0; i--);
	    if(i>0)
		authname[i+1]=0x00;
	    fclose(f);
	}
    }

    if(authname[0]!=0x00)
    {
        _http_header_needauth(s, authname, r);
    }
}

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

void http_process_get(int s, struct http_request r)
{
    if(r.special==1)
    {
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

    if(conf.debug>2)
    {
        puts("Request was good");
        if(r.special==1)
            puts("It was a special request");
        else
            puts("It was a normal request");
    }

    switch(r.method)
    {
	/* Get and post can use the same function */
        case HTTP_GET:
        case HTTP_POST:
            http_process_get(s, r); break;

        default:
            _http_error(s, r);
    }
}
