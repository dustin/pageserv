/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: userdb.c,v 1.8 1997/03/13 19:37:37 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ndbm.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#include "pageserv.h"

/* bounds checker for hour loop things */
#define BC (i<24)

void getnormtimes(int times, int *ret)
{
    int i;

    if(bit_set(times, 0))
    {
	/* handle full-timers */
	if( (times & 0xFFFFFFFF) == 0xFFFFFFFF)
	{
	    ret[0]=ret[1]=0;
	}
	else
	{
            for(i=0; bit_set(times, i) && BC; i++);
	    ret[1]=i;

            for(; bit_set(times, i)==0 && BC; i++);
	    ret[0]=i;
	}
    }
    else
    {
        for(i=0; bit_set(times, i)==0 && BC; i++);
        ret[0]=i;

        for(; bit_set(times, i) && BC; i++);
	ret[1]=i;
    }
}

void printuser(struct user u)
{
    int times[2];
    printf("ID:        %s\nPager ID:  %s\nStation:   %s\n", u.name,
         u.pageid, u.statid);

    getnormtimes(u.times, times);
    printf("Normal:    %d to %d\n", times[0], times[1]);
}

int check_time(int priority, char *whom)
{
    struct tm *t;
    struct user u;
    time_t clock;
    int ret;

    if(priority==PR_HIGH)
    {
	ret=1;
    }
    else
    {
        time(&clock);
        t=localtime(&clock);
        u=getuser(whom);

        ret=bit_set(u.times, t->tm_hour);
    }

    return(ret);

}

void storeuser(DBM *db, struct user u)
{
    datum k, d;
    k.dptr=u.name;
    k.dsize=strlen(u.name);

    d.dptr=(char *)&u;
    d.dsize=sizeof(u);

    dbm_store(db, k, d, DBM_REPLACE);
}

struct user getuser(char *name)
{
    DBM *db;
    struct user u;

    if( (db=dbm_open(USERDB, O_RDONLY, 0644)) == NULL)
    {
	perror(USERDB);
	exit(1);
    }

    u=open_getuser(db, name);

    dbm_close(db);

    return(u);
}

struct user open_getuser(DBM *db, char *name)
{
    datum d, k;
    struct user u;

    memset((void *)&u, 0x00, sizeof(u));

    k.dptr=name;
    k.dsize=strlen(name);
    d=dbm_fetch(db, k);

    if(d.dptr!=NULL)
    {
	memcpy( (void *)&u, (void *)d.dptr, sizeof(u));
    }

    return(u);
}

int u_exists(char *name)
{
    datum d, k;
    DBM *db;

    db=dbm_open(USERDB, O_RDONLY, 0644);

    k.dptr=name;
    k.dsize=strlen(name);

    d=dbm_fetch(db, k);

    dbm_close(db);

    return(d.dptr!=NULL);
}
