/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpparse.c,v 1.1 1997/04/14 03:51:31 dustin Exp $
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
    int i, j, start;
    struct http_request r;

    buf[0]=0x00;
    gettextcr(s, buf);

    while(strlen(buf)>0)
    {
        for(i=0; i<4; i++)
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
                for(j=start; j<strlen(buf); j++)
                {
                    if(buf[j]==' ')
                    {
                        buf[j]=0x00;
                        strcpy(r.request, buf+start);
                        break;
                    }
                }
                break;
            }
        }
        gettextcr(s, buf);
    }
    return(r);
}
