/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: termdb.c,v 1.2 1997/03/29 00:48:55 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ndbm.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#include "pageserv.h"

extern struct config conf;

void printterm(struct terminal t)
{
    printf("Number:     %s\n", t.number);
    printf("Term serv:  %s\n", t.ts);
    printf("Predial:    %s\n", t.predial);
    printf("Port:       %d\n", t.port);
    printf("Protocol:   %d\n", t.prot);
}

void printterms(void)
{
    char buf[BUFLEN];
    struct terminal t;
    datum d;
    DBM *db;

    if( (db=dbm_open(conf.termdb, O_RDONLY, 0644)) ==NULL)
    {
        perror(conf.termdb);
        exit(1);
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db))
    {
        strncpy(buf, d.dptr, d.dsize);
        buf[d.dsize]=0x00;
        t=open_getterm(db, buf);
        printterm(t);
        puts("--------");
    }

    dbm_close(db);
}

void storeterm(DBM *db, struct terminal t)
{
    datum k, d;
    k.dptr=t.number;
    k.dsize=strlen(t.number);

    d.dptr=(char *)&t;
    d.dsize=sizeof(t);

    dbm_store(db, k, d, DBM_REPLACE);
}

struct terminal getterm(char *number)
{
    DBM *db;
    struct terminal t;

    if( (db=dbm_open(conf.termdb, O_RDONLY, 0644)) == NULL)
    {
        perror(conf.termdb);
        exit(1);
    }

    t=open_getterm(db, number);

    dbm_close(db);

    return(t);
}

struct terminal open_getterm(DBM *db, char *number)
{
    datum d, k;
    struct terminal t;

    memset((void *)&t, 0x00, sizeof(t));

    k.dptr=number;
    k.dsize=strlen(number);
    d=dbm_fetch(db, k);

    if(d.dptr!=NULL)
    {
        memcpy( (void *)&t, (void *)d.dptr, sizeof(t));
    }

    return(t);
}

int t_exists(char *number)
{
    datum d, k;
    DBM *db;

    db=dbm_open(conf.termdb, O_RDONLY, 0644);

    k.dptr=number;
    k.dsize=strlen(number);

    d=dbm_fetch(db, k);

    dbm_close(db);

    return(d.dptr!=NULL);
}
