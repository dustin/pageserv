/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: moduser.c,v 1.7 1997/08/11 03:54:50 dustin Exp $
 */

#include <pageserv.h>
#include <http.h>

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

void _http_modusererror(int s, char *message)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Error:  ");
    puttext(s, message);
    puttext(s, "</title></head><body bgcolor=\"fFfFfF\"><h2>Error:</h2>");
    puttext(s, message);
}

void _http_moduserheader(int s)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Modify user</title></head>\n");
    puttext(s, "<body bgcolor=\"fFfFfF\">\n");
}

void _http_modusersuccess(int s)
{
    _http_moduserheader(s);
    puttext(s, "<h2>Success!</h2>User modified.");
}

void _http_moduser_timelist(int s, int def)
{
    int i;
    char buf[BUFLEN];

    for(i=0; i<24; i++)
    {
        if(i==def)
            sprintf(buf, "<option selected>%d\n", i);
        else
            sprintf(buf, "<option>%d\n", i);

        puttext(s, buf);
    }
}

void _http_moduser_process(int s, struct http_request r)
{
    struct user u;
    int times[2];
    char *tmp, *passwd1, *passwd2;
    char buf[BUFLEN];

    if(conf.udb.u_exists(r.auth.name))
    {
        if(conf.debug>2)
            printf("Getting mod data for user ``%s'' for update\n",
                r.auth.name);

        u=conf.udb.getuser(r.auth.name);
    }
    else
    {
        _http_modusererror(s, "Weird, must've raced.  Go home.");
        return;
    }

    _http_moduserheader(s);
    puttext(s, "<h2>Modified user ");
    puttext(s, r.auth.name);
    puttext(s, "</h2>");

    passwd1=_http_getcgiinfo(r, "passwd1");
    passwd2=_http_getcgiinfo(r, "passwd2");

    if(strcmp(passwd1, passwd2)==0)
    {
        if(strlen(passwd1)>0)
        {
            if(strlen(passwd1)<5)
            {
                puttext(s,
                    "Password must be at least 5 chars, not set.<br>\n");
            }
            else
            {
                u=setpasswd(u, passwd1);
                puttext(s, "Password set.<br>\n");
            }
        }
    }
    else
    {
        puttext(s, "Passwords don't match.<br>\n");
        return;
    }

    tmp=_http_getcgiinfo(r, "early");
    times[0]=atoi(tmp);
    tmp=_http_getcgiinfo(r, "late");
    times[1]=atoi(tmp);

    u.times=pack_timebits(times[0], times[1]);

    sprintf(buf, "Early:  %d<br>\nLate:  %d<br>\nMask:  %x<br>\n",
        times[0], times[1], u.times);

    puttext(s, buf);

    puttext(s, "Storing changes<br>\n");
    conf.udb.storeuser(u);
}

void _http_moduser(int s, struct http_request r)
{
    char buf[1024];

    _http_auth_require(s, r, "user");

    if(conf.debug>2)
        puts("Moduser request");

    if(r.nargs==0)
	_http_modusererror(s, "No data given.");
    else
        _http_moduser_process(s, r);

    puttext(s, buf);
}
