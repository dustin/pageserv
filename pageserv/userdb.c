/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: userdb.c,v 1.8 1997/04/16 18:23:08 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndbm.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#include <pageserv.h>

extern struct config conf;

/* bounds checker for hour loop things */
#define BC (i<24)

void getnormtimes(int times, int *ret)
{
    int i, allones=0;

    if(bit_set(times, 0))
    {
        /* handle full-timers */
        if( (times & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            ret[0]=ret[1]=0;
            allones=1;
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

void eraseuserdb(void)
{
    datum d;
    DBM *db;

    if( (db=dbm_open(conf.userdb, O_RDWR, 0644)) ==NULL)
    {
	/* There isn't one, just return */
	return;
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_firstkey(db))
    {
	if(conf.debug>2)
	    printf("deleting %s\n", d.dptr);
	dbm_delete(db, d);
    }

    dbm_close(db);
}

void printuser(struct user u)
{
    int times[2];
    printf("ID:        %s\n", u.name);
    printf("Password:  %s\n", u.passwd);
    printf("Pager ID:  %s\n", u.pageid);
    printf("Station:   %s\n", u.statid);

    getnormtimes(u.times, times);
    printf("Normal:    %d to %d\n", times[0], times[1]);
}

void printusers(void)
{
    char buf[BUFLEN];
    struct user u;
    datum d;
    DBM *db;

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) ==NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db))
    {
        strncpy(buf, d.dptr, d.dsize);
        buf[d.dsize]=0x00;
        u=open_getuser(db, buf);
        printuser(u);
        puts("--------");
    }

    dbm_close(db);
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

void storeuser(struct user u)
{
    DBM *db;

    if( (db=dbm_open(conf.userdb, O_RDWR, 0644)) == NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    open_storeuser(db, u);

    dbm_close(db);
}

void open_storeuser(DBM *db, struct user u)
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

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) == NULL)
    {
        perror(conf.userdb);
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

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) ==NULL)
    {
	perror(conf.userdb);
	return(0);
    }

    k.dptr=name;
    k.dsize=strlen(name);

    d=dbm_fetch(db, k);

    dbm_close(db);

    return(d.dptr!=NULL);
}
