/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: snppmain.c,v 1.6 1997/06/30 05:16:14 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <snpp.h>
#include <module.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

module mod_snppserv={
    _snpp_init,
    _snpp_main,
    _snpp_socket,
    "SNPP module",
    0,
    0
};

char *snpp_pageid[SNPP_NID];
char *snpp_message=NULL;
int  snpp_nid=0;
int  snpp_priority=PR_NORMAL;

void snpp_onalarm()
{
    if(conf.debug>2)
        puts("SNPP server received alarm, exiting...");

    exit(0);
}

int _snpp_socket(void)
{
    if(mod_snppserv.listening)
        return(mod_snppserv.s);
    else
        return(-1);
}

void _snpp_init(void)
{
    if(conf.snppserver)
    {
        mod_snppserv.s=getservsocket(conf.snppport);
        if(mod_snppserv.s>=0)
        {
            mod_snppserv.listening=1;
        }
        else
        {
            mod_snppserv.listening=0;
        }
    }
    else
    {
        mod_snppserv.listening=0;
    }
}

void snpp_setpageid(int s, char *id)
{
    if(u_exists(id))
    {
        if(snpp_nid>=SNPP_NID)
	{
	    puttext(s, "552 Maximum Entries exceeded.\n");
	    return;
	}
	else
	{
            snpp_pageid[snpp_nid++]=strdup(id);
            puttext(s, "250 Pager ID Accepted\n");
	}
    }
    else
    {
        puttext(s, "550 No such user\n");
    }
}

void snpp_setmess(int s, char *mess)
{
    if(snpp_message!=NULL)
    {
        puttext(s, "503 Error, message already entered\n");
        return;
    }

    if(mess==NULL || strlen(mess)==0)
    {
        puttext(s, "550 Error, invalid message\n");
        return;
    }

    snpp_message=strdup(mess);
    puttext(s, "250 Message OK\n");
}

void snpp_cleanstuff(void)
{
    /* These are set to NULL, because they're used by RESEt, too */
    int i;

    for(i=0; i<SNPP_NID; i++)
    {
        if(snpp_pageid[i])
	    free(snpp_pageid[i]);
    }
    snpp_nid=0;

    if(snpp_message)
    {
        free(snpp_message);
	snpp_message=NULL;
    }
}

void snpp_send(int s)
{
    struct queuent q;
    int i, j;

    if(snpp_nid==0 || snpp_message==NULL)
    {
        puttext(s, "503 Error, pager ID or message incomplete\n");
        return;
    }

    q.priority=snpp_priority;
    for(i=0, j=0; i<snpp_nid; i++)
    {
        strcpy(q.to, snpp_pageid[i]);
        strcpy(q.message, snpp_message);

        if(storequeue(s, q, STORE_QUIET)==0)
	    j++;
    }

    if(j==snpp_nid)
        puttext(s, "250 Message queued successfully\n");
    else
        puttext(s, "554 Message failed\n");

    if(conf.debug>2)
	printf("j is %d, snpp_nid is %d\n", j, snpp_nid);

    return;
}

void _snpp_main(modpass p)
{
    int s, going=1, c, tries=0;
    char buf[BUFLEN];

    s=p.socket;

    alarm(conf.childlifetime);
    signal(SIGALRM, snpp_onalarm);

    sprintf(buf, "220 Dustin's SNPP gateway version %s ready\n", VERSION);
    puttext(s, buf);

    while(going)
    {
        gettextcr(s, buf);
        c=snpp_parse(buf);
        switch(c)
        {
            case SNPP_HELP:
                snpp_help(s);
                break;

            case SNPP_QUIT:
                puttext(s, "221 Alrighty, then, goodbye\n");
                going=0;
                break;

            case SNPP_PAGE:
                snpp_setpageid(s, snpp_arg(buf));
                break;

            case SNPP_MESS:
                snpp_setmess(s, snpp_arg(buf));
                break;

            case SNPP_SEND:
                snpp_send(s);
                tries++;
                break;

            case SNPP_RESE:
                snpp_cleanstuff();
		alarm(conf.childlifetime);
		puttext(s, "250 Reset OK\n");
                tries++;
                break;

            case -1:
                puttext(s, "500 Unknown command\n");
                break;

            default:
                puttext(s, "500 Command not implemented\n");
                break;
        }

	if(tries>SNPP_MAXTRIES)
	{
	    puttext(s, "421 Too many pages, call back later\n");
	    going=0;
	}
    }
    close(s);
    snpp_cleanstuff();
    exit(0);
}
