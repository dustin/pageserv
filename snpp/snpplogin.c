/*
 * Copyright (c)  1996-1998  Dustin Sallings
 *
 * $Id: snpplogin.c,v 1.2 1998/01/20 06:00:49 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <snpp.h>
#include <nettools.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_TERMIO_H
#include <termio.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>

extern struct config conf;

/* real password, test password */
#define checkpass(a, b) (strcmp(a, (char *)crypt(b, a)) == 0)

char *snpp_login(int s, char *what)
{
    char *u, *p;
    struct user usr;

    u=strdup(what);
    for(p=u; *p&&*p!=' '; p++);
    if(*p)
    {
	*p=NULL;
	p++;
    }

    _ndebug(3, ("Login:  ``%s''\nPasswd:  ``%s''\n", u, p));

    if(conf.udb.u_exists(u))
    {
	usr=conf.udb.getuser(u);
    }
    else
    {
	free(u);
	puttext(s, INVALIDLOGIN);
	return(NULL);
    }

    if(strlen(usr.passwd)<(size_t)13)
    {
	free(u);
	puttext(s, INVALIDLOGIN);
	return(NULL);
    }

    if(checkpass(usr.passwd, p))
    {
	_ndebug(2, ("SNPP login for user ``%s''\n", usr.name));
	puttext(s, LOGINOK);
    }
    else
    {
	_ndebug(2, ("SNPP invalid login for user ``%s''\n", usr.name));
	free(u);
	u=NULL;
	puttext(s, INVALIDLOGIN);
    }
    return(u);
}

static void snpp_displayqlong(int s, char *quename, char *username)
{
    struct queuent q;
    char buf[BUFLEN];

    q=readqueuefile(quename);

    if(q.to[0]==0x00)
    {
	puttext(s, "550 No such queue entry\n");
	return;
    }

    if( strcmp(q.to, username)!=0 )
    {
	puttext(s, "550 That's not yours.\n");
	return;
    }

    sprintf(buf, "214 Pri: %d, size: %d, from: %s, sent: %s",
	    q.priority, strlen(q.message), nmc_intToDQ(q.rem_addr),
	    ctime(&q.submitted));
    puttext(s, buf);
    puttext(s, "214 ");
    puttext(s, q.message);
    puttext(s, "\n250 End of queue entry\n");
}

static void snpp_displayq(int s, struct queuent q)
{
    char buf[BUFLEN];

    sprintf(buf, "214 %s: Pri: %d, size: %d, sent: %s", q.qid,
            q.priority, strlen(q.message), ctime(&q.submitted));

    puttext(s, buf);
}

void snpp_showUserQ(int s, char *user, char *arg)
{
    struct queuent *q;
    int i;

    if(user==NULL)
    {
	puttext(s, "550 Must LOGIn first\n");
	return;
    }

    if(strlen(arg)>(size_t)0)
    {
	snpp_displayqlong(s, arg, user);
	return;
    }

    _ndebug(2, ("Showing queue for ``%s''\n", user));

    q=listqueue("*");
    for(i=0; q[i].to[0] != 0x00; i++)
    {
	_ndebug(3, ("Found queue entry for ``%s''\n", q[i].to));
	if( strcmp(q[i].to, user)==0 )
	{
	    snpp_displayq(s, q[i]);
	}
    }

    puttext(s, "250 End of queue\n");
}
