/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: userdb.c,v 1.5 1997/03/12 16:02:25 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ndbm.h>
#include <fcntl.h>

#include "pageserv.h"

void printuser(struct user u)
{
    printf("ID:        %s\nPager ID:  %s\nStation:   %s\n", u.name,
         u.pageid, u.statid);
    printf("Times:     %x\n", u.times);
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
