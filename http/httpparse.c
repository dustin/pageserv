/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpparse.c,v 1.13 1997/07/31 07:31:32 dustin Exp $
 */

#define IWANTMETHODNAMES 1

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

int _http_authdecode_match(char c)
{
    int i;
    static char *map=
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    for(i=0; i<65; i++)
    {
        if(map[i]==c)
            break;
    }

    return(i);
}

void _http_authdecode(struct http_request *r)
{
    char *string;
    char *dest;
    int i;

    string=r->auth.string;

    if(conf.debug>2)
        printf("Decoding:  ``%s''\n", string);

    /* This is overkill, but I don't care */
    dest=(char *)malloc(strlen(string));
    memset(dest, 0x00, strlen(string));

    i=0;
    for(; *string; string+=4)
    {
        string[0]=_http_authdecode_match(string[0]);
        string[1]=_http_authdecode_match(string[1]);
        string[2]=_http_authdecode_match(string[2]);
        string[3]=_http_authdecode_match(string[3]);

        dest[i]=string[0]<<2;
        if(string[1]!=64)
        {
            dest[i++]|= (string[1] & 0x30) >>4;

            dest[i]=(string[1]&0x0f) << 4;
            if(string[2]!=64)
            {
                dest[i++]|= (string[2]&0x3c) >> 2;

                dest[i]= (string[2]&0x03) << 6;

                if(string[3]!=64)
                    dest[i++]|= string[3];
            }
        }
    }

    for(i=0; dest[i]!=':'; i++);
    dest[i]=0x00;
r->auth.name=strdup(dest);
    r->auth.pass=strdup(dest+i+1);

    if(conf.debug>2)
        printf("User:  ``%s'', Pass:  ``%s''\n", r->auth.name,
            r->auth.pass);

    free(dest);
}

struct http_request http_parserequest(int s)
{
    char buf[REQUESTSIZE];
    char *buf2=NULL;
    int i, j, start, finished=0, tmp;
    struct http_request r;

    _http_init_request(&r);
    r.method=-1;
    buf[0]=0x00;
    gettextcr(s, buf);

    while(!finished)
    {
        for(i=0; methodnames[i]!=NULL; i++)
        {
            if(strncmp(buf, methodnames[i], strlen(methodnames[i]))==0)
            {
                if(conf.debug>2)
                {
                    printf("Appears to be the %s method\n",
                        methodnames[i]);
                }

                r.method=i;
                start=strlen(methodnames[i])+1;
                tmp=strlen(buf);
                for(j=start; j<tmp; j++)
                {
                    if(buf[j]==' ')
                    {
                        buf[j]=0x00;
                        break;
                    } else if(buf[j]=='?')
                    {
                        buf2=buf+j+1;
                        buf[j]=0x00;
                    }
                }
                if(j==tmp)
                {
                    r.version=0;
                    finished=1;
                }
                else
                {
                    r.version=1;
                }
                strcpy(r.request, buf+start);

                if(buf2!=NULL)
                    r.args=strdup(buf2);

                break;
            }
        }

        for(i=0; miscnames[i]!=NULL; i++)
        {
            if(strncmp(buf, miscnames[i], strlen(miscnames[i]))==0)
            {
                switch(i)
                {
                    case HTTP_CONTENTLENGTH:
                    case HTTP_CONTENTLENGTH2:
                        r.length=atoi(buf+(strlen(miscnames[i])));
                        if(conf.debug>2)
                            printf("Found length:  %d\n", r.length);
                        break;
                    case HTTP_AUTHBASIC:
                        r.auth.string=strdup(buf+strlen(miscnames[i]));
                        if(conf.debug>2)
                            printf("Found authbasic:  %s\n", r.auth.string);
                        _http_authdecode(&r);
                        break;
                }
            }
        }

        if(!finished)
        {
            gettextcr(s, buf);
            if(strlen(buf)==0)
            {
                /* eat the last character */
                recv(s, &tmp, 1, 0);
                finished=1;
            }
        }
    }

    /* now grab the args if any */
    _http_parseargs(s, &r);

    return(r);
}
