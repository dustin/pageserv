/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: userdb.c,v 1.26 1999/02/25 22:45:36 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndbm.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#include <pageserv.h>
#include <readconfig.h>

extern struct config conf;

/* bounds checker for hour loop things */
#define BC (i<24)

void cleanuserlist(char **list)
{
    int i;

    _ndebug(2, ("Freeing user list.\n"));

    for(i=0; list[i]!=NULL; i++)
    {
        free(list[i]);
    }
    free(list);
}

static char **dbm_listusers(char *term)
{
    datum d, val;
    DBM *db;
    char **ret;
    struct user u;
    int size, index=0;

    size=rcfg_lookupInt(conf.cf, "tuning.utableguess");
    if(size<2)
        size=4;

    _ndebug(2, ("Table size guess is %d\n", size));

    ret=malloc(size * sizeof(char *));

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) ==NULL)
    {
        if(conf.debug>2)
            perror(conf.userdb);
        return(NULL);
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db))
    {
        val=dbm_fetch(db, d);
        memcpy( (void *)&u, (void *)val.dptr, sizeof(u));

        if( (strcmp(u.statid, term) == 0) || (strcmp(term, "*") == 0) )
        {
            ret[index]=malloc(d.dsize+1);
            strncpy(ret[index], d.dptr, d.dsize);
            ret[index++][d.dsize]=0x00;
        }

        if(index == size-1)
        {
           size<<=1;

	    _ndebug(2, ("Reallocating, now need %d bytes for %d\n",
			size*sizeof(char *), size));

            ret=realloc(ret, size*sizeof(char *));
        }
    }
    ret[index]=NULL;

    dbm_close(db);
    return(ret);
}

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

static void dbm_eraseuserdb(void)
{
    datum d;
    DBM *db;
    char *tmp;

    if( (db=dbm_open(conf.userdb, O_RDWR|O_CREAT, 0644)) ==NULL)
    {
        /* There isn't one, just return */
        _ndebug(2, ("No user database.\n"));
        return;
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_firstkey(db))
    {
        if(conf.debug>2)
        {
            tmp=strdup((char *)d.dptr);
            tmp[d.dsize]=0x00;
            _ndebug(2, ("deleting %s\n", tmp));
            free(tmp);
        }
        dbm_delete(db, d);
    }

    dbm_close(db);
}

static int dbm_u_exists(char *name)
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

static int dbm_deleteuser(char *name)
{
    datum d;
    DBM *db;

    if( (db=dbm_open(conf.userdb, O_RDWR, 0644)) == NULL)
    {
        return(-1);
    }

    d.dptr=name;
    d.dsize=strlen(name);

    dbm_delete(db, d);
    dbm_close(db);

    /* do a lookup and return the result */
    return(!dbm_u_exists(name));
}

void printuser(struct user u)
{
    int times[2];
    printf("ID:        %s\n", u.name);
    printf("Password:  %s\n", u.passwd);
    printf("Pager ID:  %s\n", u.pageid);
    printf("Station:   %s\n", u.statid);
    printf("Notify:    %s\n", u.notify);
    printf("Flags:     %d%s", u.flags, (u.flags>0)?" -- ":"");

    /* print out the flags */
    printf("%s", (u.flags&NOTIFY_SUCC)?"NOTIFY_SUCC ":"");
    printf("%s", (u.flags&NOTIFY_FAIL)?"NOTIFY_FAIL ":"");

    puts("");

    getnormtimes(u.times, times);
    printf("Normal:    %d to %d\n", times[0], times[1]);
}

static struct user dbm_open_getuser(DBM *db, char *name)
{
    datum d, k;
    struct user u;

    memset((void *)&u, 0x00, sizeof(u));

    k.dptr=name;
    k.dsize=strlen(name);
    d=dbm_fetch(db, k);

    if(d.dptr!=NULL) {
        memcpy( (void *)&u, (void *)d.dptr, sizeof(u));
    }

    return(u);
}

void dumpuser(struct user u, char *delim)
{
    int times[2];

    if(strcmp(u.name, u.pageid)!=0)
    {
        getnormtimes(u.times, times);
        printf("%s%s%s%s%s%s%d%s%d\n", u.name, delim,
				       u.pageid, delim,
				       u.statid, delim,
				       times[0], delim,
				       times[1]);
    }

}

void dumpuserdb(void)
{
    struct user u;
    char **users, *delim;
    int i;
    time_t t;

    conf.udb.dbinit();

    delim=rcfg_lookup(conf.cf, "databases.textdelim");
    if(delim==NULL)
	delim="\t";

    t=time(NULL);
    printf("# Database dump created on %s", ctime(&t));
    printf("# Using text delimiter ``%s''\n\n", delim);

    users=conf.udb.listusers("*");

    for(i=0; users[i]; i++)
    {
        u=conf.udb.getuser(users[i]);
        dumpuser(u, delim);
    }

    cleanuserlist(users);
}

void printusers(void)
{
    struct user u;
    char **users;
    int i;

    users=conf.udb.listusers("*");

	if(users==NULL)
		return;

    for(i=0; users[i]; i++)
    {
        u=conf.udb.getuser(users[i]);
        printuser(u);
        puts("--------");
    }

    cleanuserlist(users);
}

int check_time(struct queuent q)
{
    struct tm *t;
    struct user u;
    time_t clock;
    int ret;

    if(q.priority==PR_HIGH)
    {
        ret=1;
    }
    else
    {
        time(&clock);
        clock=(clock>q.soonest ? clock : q.soonest);
        t=localtime(&clock);
        u=conf.udb.getuser(q.to);

        ret=bit_set(u.times, t->tm_hour);
    }

    return(ret);

}

static void dbm_open_storeuser(DBM *db, struct user u)
{
    datum k, d;
    k.dptr=u.name;
    k.dsize=strlen(u.name);

    d.dptr=(char *)&u;
    d.dsize=sizeof(u);

    dbm_store(db, k, d, DBM_REPLACE);
}

static void dbm_storeuser(struct user u)
{
    DBM *db;

    if( (db=dbm_open(conf.userdb, O_RDWR|O_CREAT, 0644)) == NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    dbm_open_storeuser(db, u);

    if(rcfg_lookupInt(conf.cf, "databases.userdbrhash"))
    {
        if(strlen(u.pageid)<(size_t)NAMELEN)
        {
            _ndebug(3, ("Doing reverse (%s) for %s\n", u.pageid,
                        u.name));
            strcpy(u.name, u.pageid);
            dbm_open_storeuser(db, u);
        }
        else
        {
            _ndebug(3, ("Ignoring reverse for %s (too long)\n",
                        u.name));
        }
    }


    dbm_close(db);
}

static struct user dbm_getuser(char *name)
{
    DBM *db;
    struct user u;

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) == NULL) {
        perror(conf.userdb);
        exit(1);
    }

    u=dbm_open_getuser(db, name);

    dbm_close(db);

    return(u);
}

void dbm_userdbInit(void)
{
    if(conf.udb.listusers==dbm_listusers)
    {
	_ndebug(2, ("dbm_userdbInit has already been called\n"));
	return;
    }

    conf.udb.listusers=dbm_listusers;
    conf.udb.eraseuserdb=dbm_eraseuserdb;
    conf.udb.deleteuser=dbm_deleteuser;
    conf.udb.storeuser=dbm_storeuser;
    conf.udb.getuser=dbm_getuser;
    conf.udb.u_exists=dbm_u_exists;
    conf.udb.parseusers=parseusers;
}
