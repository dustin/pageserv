/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: protocol.c,v 1.12 1998/04/15 17:14:21 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pageserv.h>

extern struct config conf;

static void p_epage(int s, modpass p)
{
    char buf1[BUFLEN], buf2[BUFLEN], buf3[BUFLEN];
    struct queuent q;

    memset(&q, 0x00, sizeof(q));

    _ndebug(1, ("Entering p_epage\n"));

    puttext(s, PROMPT_ID);
    gettextcr(s, buf1);
    puttext(s, PROMPT_PRI);
    gettextcr(s, buf2);
    puttext(s, PROMPT_MESS);
    gettextcr(s, buf3);

    if(tolower(buf2[0])=='h')
    {
        q.priority=PR_HIGH;
    }
    else
    {
        q.priority=PR_NORMAL;
    }

    strcpy(q.to, buf1);
    strcpy(q.message, buf3);

    q.rem_addr=ntohl(p.fsin.sin_addr.s_addr);

    if(conf.udb.u_exists(buf1))
    {
        storequeue(s, q, STORE_NORMAL);
    }
    else
    {
        puttext(s, MESG_NOUSER);
    }
}

static void p_mash(int s, modpass p)
{
    char buf1[BUFLEN], buf2[BUFLEN];
    struct queuent q;

    _ndebug(1, ("Entering p_mash\n"));

    puttext(s, PROMPT_ID);
    gettextcr(s, buf1);
    puttext(s, PROMPT_MESS);
    gettextcr(s, buf2);

    q.priority=PR_NORMAL;

    strcpy(q.to, buf1);
    strcpy(q.message, buf2);

    if(conf.udb.u_exists(buf1))
    {
        storequeue(s, q, STORE_NORMAL);
    }
    else
    {
        puttext(s, MESG_NOUSER);
    }
}

static void p_depth(int s, modpass p)
{
    char buf[BUFLEN];

    _ndebug(1, ("Entering p_depth\n"));

    sprintf(buf, "%d items in the queue\n", queuedepth());
    puttext(s, buf);
}

static void p_farkle(int s, modpass p)
{
    struct queuent q;
    struct user u;
    char buf[BUFLEN];

    if(conf.farkle==0)
    {
	_ndebug(1, ("Tried to farkle, configured out...\n"));
	puttext(s, MESG_NOFARKLE);
	return;
    }

    _ndebug(1, ("Entering p_farkle\n"));

    if(queuedepth()>0)
    {
        q=dofarkle();
        u=conf.udb.getuser(q.to);
        if(u.name[0]!=0x00)
        {
            sprintf(buf, "%s\n%s\n%s\n", u.pageid, u.statid, q.message);
            puttext(s, buf);
        }
    }
    else
    {
        puttext(s, MESG_NOQUEUE);
    }
}

static void p_quit(int s, modpass p)
{
    _ndebug(1, ("Entering p_quit\n"));

    puttext(s, MESG_QUIT);
    _mod_pageserv_exit(s, 0);
}

void process(int s, char *cmd, modpass p)
{
    static char *commands[P_MAX+1]={
        "mash", "farkle", "depth", "quit",
	"epage", "login"
    };

    char buf[BUFLEN];
    int i, c;

    c=-1;

    for(i=0; i<=P_MAX; i++)
    {
        if(strcmp(cmd, commands[i])==0)
        {
            c=i;
            break;
        }
    }

    if(c == P_UNKNOWN)
    {
        sprintf(buf, "Unknown command: %s\n", cmd);

	_ndebug(1, ("%s", buf));

        send(s, buf, strlen(buf), 0);
        _mod_pageserv_exit(s, 0);
    }

    _ndebug(1, ("Received ``%s'' command\n", cmd));

    /* process command */

    switch(c)
    {
        case P_MASH:
            p_mash(s, p); break;

        case P_DEPTH:
            p_depth(s, p); break;

        case P_FARKLE:
            p_farkle(s, p); break;

        case P_EPAGE:
            p_epage(s, p); break;

        case P_LOGIN:
            p_login(s); break;

        case P_QUIT:
            p_quit(s, p);
    }
}
