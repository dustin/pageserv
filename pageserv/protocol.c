/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: protocol.c,v 1.2 1997/03/30 05:59:52 dustin Exp $
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

/* This is broken, but pretty */

void p_apage(int s)
{
    struct queuent q;
    int size=0, ack;

    if(conf.debug>1)
	puts("Entering p_apage");

    size+=recv(s, (char *)&q, sizeof(q), 0);
    printf("Received %d bytes, wanted %d\n", size, sizeof(q));

    ack=htonl(1);

    send(s, (char *)&ack, sizeof(ack), 0);

    q.priority=ntohl(q.priority);

    if(u_exists(q.to))
    {
        ack=storequeue(s, q, STORE_QUIET);
    }
}

void p_epage(int s)
{
    char buf1[BUFLEN], buf2[BUFLEN], buf3[BUFLEN];
    struct queuent q;

    if(conf.debug>1)
	puts("Entering p_epage");

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

    if(u_exists(buf1))
    {
        storequeue(s, q, STORE_NORMAL);
    }
    else
    {
        puttext(s, MESG_NOUSER);
    }
}

void p_mash(int s)
{
    char buf1[BUFLEN], buf2[BUFLEN];
    struct queuent q;

    if(conf.debug>1)
	puts("Entering p_mash");

    puttext(s, PROMPT_ID);
    gettextcr(s, buf1);
    puttext(s, PROMPT_MESS);
    gettextcr(s, buf2);

    q.priority=PR_NORMAL;

    strcpy(q.to, buf1);
    strcpy(q.message, buf2);

    if(u_exists(buf1))
    {
        storequeue(s, q, STORE_NORMAL);
    }
    else
    {
        puttext(s, MESG_NOUSER);
    }
}

void p_depth(int s)
{
    char buf[BUFLEN];

    if(conf.debug>1)
	puts("Entering p_depth");

    sprintf(buf, "%d items in the queue\n", queuedepth());
    puttext(s, buf);
}

void p_farkle(int s)
{
    struct queuent q;
    struct user u;
    char buf[BUFLEN];

    if(conf.debug>1)
	puts("Entering p_farkle");

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

void p_quit(int s)
{
    if(conf.debug>1)
	puts("Entering p_quit");

    puttext(s, MESG_QUIT);
    exit(0);
}

void process(int s, char *cmd)
{
    static char *commands[P_MAX+1]={
        "mash", "farkle",
        "depth", "quit", "epage", "apage"
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

	if(conf.debug>1)
	    puts(buf);

        send(s, buf, strlen(buf), 0);
        exit(0);
    }

    if(conf.debug>1)
	printf("Received ``%s'' command\n", cmd);

    /* process command */

    switch(c)
    {
        case P_MASH:
            p_mash(s); break;

        case P_DEPTH:
            p_depth(s); break;

        case P_FARKLE:
            p_farkle(s); break;

        case P_EPAGE:
            p_epage(s); break;

        case P_APAGE:
            p_apage(s); break;

        case P_QUIT:
            p_quit(s);
    }
}