/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: protocol.c,v 1.6 1997/03/12 17:49:48 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "pageserv.h"

int gettext(int s, char *buf)
{
    int size;
    if( (size=recv(s, buf, BUFLEN-1, 0)) >0)
    {
	buf[size]=0x00;
	kw(buf);
	return(size);
    }
    else
    {
	/* Pipe breaking bastard */
	exit(0);
    }
}

void p_epage(int s)
{
    char buf1[BUFLEN], buf2[BUFLEN], buf3[BUFLEN];
    int priority;

    puttext(s, PROMPT_ID);
    gettext(s, buf1);
    puttext(s, PROMPT_PRI);
    gettext(s, buf2);
    puttext(s, PROMPT_MESS);
    gettext(s, buf3);

    if(tolower(buf2[0])=='h')
    {
	priority=PR_HIGH;
    }
    else
    {
	priority=PR_NORMAL;
    }

    if(u_exists(buf1))
    {
        storequeue(s, priority, buf1, buf3);
    }
    else
    {
	puttext(s, MESG_NOUSER);
    }
}

void p_queueup(int s)
{
    char buf1[BUFLEN], buf2[BUFLEN];

    puttext(s, PROMPT_ID);
    gettext(s, buf1);
    puttext(s, PROMPT_MESS);
    gettext(s, buf2);

    if(u_exists(buf1))
    {
        storequeue(s, PR_NORMAL, buf1, buf2);
    }
    else
    {
	puttext(s, MESG_NOUSER);
    }
}

void p_depth(int s)
{
    char buf[BUFLEN];

    sprintf(buf, "%d items in the queue\n", queuedepth());
    puttext(s, buf);
}

void p_farkle(int s)
{
    struct queuent q;
    struct user u;
    char buf[BUFLEN];

    if(queuedepth()>0)
    {
	q=dofarkle();
	u=getuser(q.to);
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

void process(int s, char *cmd)
{
    static char *commands[P_MAX+1]={
	"mash", "farkle",
	"depth", "quit", "epage"
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
        send(s, buf, strlen(buf), 0);
	exit(0);
    }

    /* process command */

    switch(c)
    {
	case P_MASH:
	    p_queueup(s); break;

	case P_DEPTH:
	    p_depth(s); break;

	case P_FARKLE:
	    p_farkle(s); break;

	case P_EPAGE:
	    p_epage(s); break;

        case P_QUIT:
	    quit(s);
    }
}
