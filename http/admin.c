/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: admin.c,v 1.5 1997/07/14 06:10:53 dustin Exp $
 */

#include <pageserv.h>
#include <http.h>

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

void _http_adminerror(int s, char *message)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Error:  ");
    puttext(s, message);
    puttext(s, "</title></head><body bgcolor=\"fFfFfF\"><h2>Error:</h2>");
    puttext(s, message);
}

void _http_adminheader(int s)
{
    _http_header_ok(s, 0);
    puttext(s, "<html><head><title>Pageserv Admin</title></head>\n");
    puttext(s, "<body bgcolor=\"fFfFfF\">\n");
}

void _http_doadduser(int s, struct http_request r)
{
    struct user u;
    int times[2];
    char *name, *terminal, *pageid, *pass1, *pass2, *tmp;

    name=_http_getcgiinfo(r, "username");
    pageid=_http_getcgiinfo(r, "pageid");
    terminal=_http_getcgiinfo(r, "terminal");
    terminal=_http_getcgiinfo(r, "terminal");
    pass1=_http_getcgiinfo(r, "passwd1");
    pass2=_http_getcgiinfo(r, "passwd2");

    if(!(name && terminal && pageid && pass1 && pass2))
    {
        _http_adminerror(s, "Invalid form");
    }

    tmp=_http_getcgiinfo(r, "early");
    if(tmp==NULL)
    {
	times[0]=0;
    }
    else
    {
	times[0]=atoi(tmp);
    }

    tmp=_http_getcgiinfo(r, "late");
    if(tmp==NULL)
    {
	times[1]=0;
    }
    else
    {
	times[1]=atoi(tmp);
    }

    strcpy(u.name, name);
    strcpy(u.pageid, pageid);
    strcpy(u.statid, terminal);
    u=setpasswd(u, pass1);
    u.times=pack_timebits(times[0], times[1]);

    if(u_exists(name))
    {
	_http_adminerror(s, "User already exists");
	return;
    }

    if( strcmp(pass1, pass2) != 0)
    {
	_http_adminerror(s, "Passwords differ.");
	return;
    }

    storeuser(u);

    if(u_exists(name))
    {
	_http_adminheader(s);
	puttext(s, "<h2>User successfully added</h2>");
    }
    else
    {
	_http_adminerror(s, "Error adding user");
    }
}

void _http_dodeluser(int s, struct http_request r)
{
     char *user;

     user=_http_getcgiinfo(r, "username");

     if(user == NULL)
     {
	 _http_adminerror(s, "Invalid form");
	 return;
     }

     if(!u_exists(user))
     {
	 _http_adminerror(s, "Can't delete a nonexistant user.");
	 return;
     }

     _http_adminheader(s);
     puttext(s, "<h2>Deleting a user</h2>");

     if(deleteuser(user))
     {
	 puttext(s, "Successful");
     }
     else
     {
	 puttext(s, "ERROR!  User not deleted.");
     }
}

void _http_admin_process(int s, struct http_request r)
{
    int i;
    char *form;
    char *forms[]={
        "adduser",
	"deluser",
        NULL
    };

    form=_http_getcgiinfo(r, "formname");

    if(form==NULL)
        form="invalid";

    for(i=0; forms[i]!=NULL; i++)
    {
        if(strcmp(forms[i], form)==0)
            break;
    }

    switch(i)
    {
        case 0:
            _http_doadduser(s, r);
            break;

	case 1:
	    _http_dodeluser(s, r);
	    break;

        default:
            _http_adminerror(s, "Invalid form");
    }
}

void _http_admin(int s, struct http_request r)
{
    char buf[1024];

    _http_auth_require(s, r, "admin");

    if(conf.debug>2)
        puts("Admin request");

    if(r.nargs==0)
        _http_adminerror(s, "No data sent.");
    else
        _http_admin_process(s, r);

    puttext(s, buf);
}
