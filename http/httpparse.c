/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpparse.c,v 1.7 1997/04/18 21:18:55 dustin Exp $
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
    char *buf2=NULL;
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
                else
                    r.args=NULL;

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
                        r.length=atoi(buf+(strlen(miscnames[i])));
			if(conf.debug>2)
			    printf("Found length:  %d\n", r.length);
                        break;
                }
            }
        }

        if(!finished)
        {
            gettextcr(s, buf);
            if(strlen(buf)==0)
                finished=1;
        }
    }
    r.alist=NULL;
    return(r);
}
