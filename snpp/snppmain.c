/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: snppmain.c,v 1.23 1998/04/12 01:04:47 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <snpp.h>
#include <module.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <assert.h>

#include <readconfig.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <sys/socket.h>
#include <netinet/in.h>

extern struct config conf;

static void _snpp_init(void);
static void _snpp_main(modpass p);
static int _snpp_socket(void);

module mod_snppserv={
    _snpp_init,
    _snpp_main,
    _snpp_socket,
    "SNPP module",
    0,
    0
};

static char **snpp_pageid=NULL;
static char *snpp_message=NULL;
static char *snpp_logname=NULL;
static int  snpp_nid=0;
static int  snpp_priority=PR_NORMAL;
static int  snpp_holdtime=0;
static int  snpp_lastsend=0;

static void snpp_onalarm()
{
    _ndebug(2, ("SNPP server received alarm, exiting...\n"));

    exit(0);
}

static int _snpp_socket(void)
{
    if(mod_snppserv.listening)
        return(mod_snppserv.s);
    else
        return(-1);
}

static void _snpp_init(void)
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

static void snpp_holduntil(int s, char *time)
{
    int i, gmt, offset=0;
    int vals[6];
    char tmp[5], buf[1024];
    struct tm tm, *tmptm;
    time_t t;

    memset(&tm, 0x00, sizeof(tm));

    if(time==NULL)
    {
        puttext(s, "550 Invalid Delivery Date/Time (try help)\n");
        return;
    }

    for(i=0; time[i] && time[i]!=' ' && time[i]!='-' && time[i]!='+'; i++);

    if(time[i])
    {
        offset=atoi(time+i);
	time[i]=0x00;
        _ndebug(2, ("User supplied offset is %d\n", offset));
    }

    if(strlen(time) < (size_t)12)
    {
        puttext(s, "550 Invalid Delivery Date/Time (try help)\n");
        return;
    }

    if(strlen(time)==(size_t)12)
    {
        for(i=0; i<12; i+=2)
        {
            tmp[0]=time[i];
	    tmp[1]=time[i+1];
	    tmp[2]=0x00;

	    vals[i/2]=atoi(tmp);
        }
    }
    else
    {
	/* Get year */
	strncpy(tmp, time, 4);
	vals[0]=atoi(tmp)-1900;

        /* do the rest, yeah, this looks funny */
        for(i=4; i<14; i+=2)
        {
            tmp[0]=time[i];
	    tmp[1]=time[i+1];
	    tmp[2]=0x00;

	    vals[(i-2)/2]=atoi(tmp);
        }
    }

    tm.tm_year=vals[0];
    tm.tm_mon=vals[1]-1;
    tm.tm_mday=vals[2];
    tm.tm_hour=vals[3];
    tm.tm_min=vals[4];
    tm.tm_sec=vals[5];

    t=mktime(&tm);

    if(t<(time_t)0)
    {
        puttext(s, "550 Invalid Delivery Date/Time (try help)\n");
	return;
    }

    gmt=findGMTOffset();
    _ndebug(2, ("Adding %d for GMT offset\n", gmt));

    t+=gmt;
    tmptm=localtime(&t);

    t-=(3600*offset);

    if(t>0)
    {
        snpp_holdtime=t;
	tmptm=gmtime(&t);
	strcpy(buf, "250 Page will be held until ");
	strcat(buf, asctime(tmptm));
	buf[strlen(buf)-1]=0x00;
	strcat(buf, " GMT\n");
	puttext(s, buf);
    }
}

static void snpp_setpageid(int s, char *id)
{
    static int size, max;

    if(snpp_pageid==NULL)
    {
	_ndebug(4, ("Initializing snpp_pageid\n"));
	size=4;
	max=rcfg_lookupInt(conf.cf, "modules.snpp.maxbroadcast");
	snpp_pageid=malloc(size*sizeof(char **));
    }

    _ndebug(5, ("Trying conf.udb.u_exists(%s) (%d bytes) for %d\n", id,
        strlen(id), snpp_nid));

    if(conf.udb.u_exists(id))
    {
        if((max>0) && (snpp_nid>=max) )
        {
            puttext(s, "552 Maximum Entries exceeded.\n");
            return;
        }
        else
        {
	    if(snpp_nid==size-1)
	    {
		size<<=1;
		_ndebug(4, ("Growing snpp_pageid to %d\n", size));
		snpp_pageid=realloc(snpp_pageid, size*sizeof(char **));
		assert(snpp_pageid);
	    }
            snpp_pageid[snpp_nid++]=strdup(id);
            puttext(s, "250 Pager ID Accepted\n");
        }
    }
    else
    {
        puttext(s, "550 No such user\n");
    }
}

static void snpp_setmess(int s, char *mess)
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

static void snpp_setpriority(int s, char *pri)
{
    if( (pri!=NULL) && (strlen(pri)> (size_t)0) )
    {
        if( tolower(pri[0]) == 'h')
	    snpp_priority=PR_HIGH;

        puttext(s, "250 Priority OK\n");
    }
    else
    {
	puttext(s, "500 You forgot your argument.\n");
    }
}

static void snpp_cleanstuff(void)
{
    /* These are set to NULL, because they're used by RESEt, too */
    int i;


    if(snpp_pageid!=NULL)
    {
        for(i=0; i<snpp_nid; i++)
        {
            if(snpp_pageid[i])
                free(snpp_pageid[i]);
        }
        free(snpp_pageid);
        snpp_pageid=NULL;
    }

    snpp_nid=0;
    snpp_holdtime=0;
    snpp_lastsend=0;
    snpp_priority=PR_NORMAL;

    if(snpp_message)
    {
        free(snpp_message);
        snpp_message=NULL;
    }

    if(snpp_logname)
    {
	free(snpp_logname);
	snpp_logname=NULL;
    }
}

static void snpp_send(int s, struct sockaddr_in fsin)
{
    struct queuent q;
    int i, j;

    if(snpp_nid==0 || snpp_message==NULL)
    {
        puttext(s, "503 Error, pager ID or message incomplete\n");
        return;
    }

    if(snpp_lastsend!=0)
    {
	puttext(s, "554 Message already sent\n");
	return;
    }

    memset(&q, 0x00, sizeof(q));

    q.rem_addr=ntohl(fsin.sin_addr.s_addr);

    q.priority=snpp_priority;
    q.soonest=snpp_holdtime;
    for(i=0, j=0; i<snpp_nid; i++)
    {
        strcpy(q.to, snpp_pageid[i]);
        strcpy(q.message, snpp_message);

        if(storequeue(s, q, STORE_QUIET)==0)
            j++;
    }

    if(j==snpp_nid)
    {
        puttext(s, "250 Message queued successfully\n");
	snpp_lastsend=1;
    }
    else
    {
        puttext(s, "554 Message failed\n");
	snpp_lastsend=0;
    }

    _ndebug(2, ("j is %d, snpp_nid is %d\n", j, snpp_nid));

    return;
}

static void _snpp_main(modpass p)
{
    int s, going=1, c, tries=0, maxtries;
    char buf[BUFLEN];

    s=p.socket;

    alarm(conf.childlifetime);
    signal(SIGALRM, snpp_onalarm);

    /*
     * The maximum number of resets/sends
     */
    maxtries=rcfg_lookupInt(conf.cf, "modules.snpp.maxtries");
    if(maxtries==0)
	maxtries=SNPP_MAXTRIES;

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
                snpp_send(s, p.fsin);
                break;

            case SNPP_RESE:
                snpp_cleanstuff();
                alarm(conf.childlifetime);
                puttext(s, "250 Reset OK\n");
                tries++;
                break;

            case SNPP_PRIORITY:
                snpp_setpriority(s, snpp_arg(buf));
                break;

            case SNPP_HOLD:
                snpp_holduntil(s, snpp_arg(buf));
		break;

	    case SNPP_LOGI:
		snpp_logname=snpp_login(s, snpp_arg(buf));
		break;

	    case SNPP_SHOWQ:
		snpp_showUserQ(s, snpp_logname, snpp_arg(buf));
		break;

	    case SNPP_DEQUE:
		snpp_delUserQ(s, snpp_logname, snpp_arg(buf));
		break;

            case -1:
                puttext(s, "500 Unknown command\n");
                break;

            default:
                puttext(s, "500 Command not implemented\n");
                break;
        }

        if(tries>maxtries)
        {
            puttext(s, "421 Too many pages, call back later\n");
            going=0;
        }
    }
    close(s);
    snpp_cleanstuff();
    exit(0);
}
