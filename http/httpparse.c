/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpparse.c,v 1.3 1997/04/14 06:56:15 dustin Exp $
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

struct http_request http_parserequest(int s)
{
    char buf[REQUESTSIZE];
    int i, j, start, finished=0, tmp;
    struct http_request r;

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
                break;
            }
        }
        if(!finished)
        {
            gettextcr(s, buf);
            if(strlen(buf)==0)
                finished=1;
        }
    }
    return(r);
}