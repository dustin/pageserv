/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: protocol.c,v 1.3 1997/03/11 19:47:52 dustin Exp $
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

void p_queueup(int s)
{
    char buf1[BUFLEN], buf2[BUFLEN];

    puttext(s, PROMPT_ID);
    gettext(s, buf1);
    puttext(s, PROMPT_MESS);
    gettext(s, buf2);

    storequeue(s, PR_NORMAL, buf1, buf2);
}

void p_depth(int s)
{
    char buf[BUFLEN];

    sprintf(buf, "%d items in the queue\n", queuedepth());
    puttext(s, buf);
}

void process(int s, char *cmd)
{
    static char *commands[4]={
	"mash", "farkle",
	"depth", "quit"
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

        case P_QUIT:
	    quit(s);
    }
}
