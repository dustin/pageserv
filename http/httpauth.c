/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: httpauth.c,v 1.1 1997/07/08 06:40:04 dustin Exp $
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

void _http_auth_require(int s, struct http_request r, char *authname)
{
    struct user u;

    if(r.auth.name == NULL)
	_http_header_needauth(s, authname, r);

    if(!u_exists(r.auth.name))
	_http_header_needauth(s, authname, r);

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
