/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpauth.c,v 1.5 1997/07/10 07:12:33 dustin Exp $
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
#include <sys/stat.h>

extern struct config conf;

#define checkpass(a, b) (strcmp(a, (char *)crypt(b, a)) == 0)

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
    int i, needauth=0;

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
	    needauth++;
            fgets(authname, 180, f);
            for(i=0; (authname[i]!='\n'
                      && authname[i]!='\r'
                      && authname[i]!=' ')
                      && authname[i]; i++);
            printf("i is %d\n", i);
            if(i>0)
                authname[i]=0x00;
            fclose(f);
        }
    }

    if(needauth)
        _http_auth_require(s, r, authname);
}

void _http_auth_require(int s, struct http_request r, char *authname)
{
    struct user u;

    if(conf.debug>2)
        printf("authname is ``%s''\n", authname);

    if(r.auth.name == NULL)
        _http_header_needauth(s, authname, r);

    if(!u_exists(r.auth.name))
        _http_header_needauth(s, authname, r);

    if(r.auth.pass == NULL)
        _http_header_needauth(s, authname, r);

    if(strcmp(authname, "admin")==0)
    {
        if(strcmp(r.auth.name, "admin")!=0)
            _http_header_needauth(s, authname, r);
    }
    else
    {
        if(strcmp(r.auth.name, "admin")==0)
            _http_header_needauth(s, authname, r);
    }

    u=getuser(r.auth.name);

    if(checkpass(u.passwd, r.auth.pass))
    {
         return;
    }
    else
    {
        _http_header_needauth(s, authname, r);
    }
}
