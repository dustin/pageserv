/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: termdb.c,v 1.9 1997/09/04 06:18:48 dustin Exp $
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

void printterm(struct terminal t)
{
    printf("Number:     %s\n", t.number);
    printf("Term serv:  %s\n", t.ts);
    printf("Predial:    %s\n", t.predial);
    printf("Port:       %d\n", t.port);
    printf("Flags:      %d\n", t.flags);
    printf("Contype:    %d\n", t.contype);
}

void erasetermdb(void)
{
    datum d;
    DBM *db;

    if( (db=dbm_open(conf.termdb, O_RDWR, 0644)) ==NULL)
    {
	/* there isn't one, just return */
	return;
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_firstkey(db))
    {
	if(conf.debug>2)
	    printf("Deleting %s\n", d.dptr);
        dbm_delete(db, d);
    }

    dbm_close(db);
}

char **listterms(void)
{
    datum d;
    DBM *db;
    char **ret;
    int size=4, index=0;

    ret=malloc(size * sizeof(char *));

    if( (db=dbm_open(conf.termdb, O_RDONLY, 0644)) ==NULL)
    {
        perror(conf.termdb);
        exit(1);
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db))
    {
        ret[index]=malloc(d.dsize+1);
        strncpy(ret[index], d.dptr, d.dsize);
        ret[index++][d.dsize]=0x00;

        if(index == size-1)
        {
           size<<=1;

            if(conf.debug>2)
            {
                printf("Reallocating, now need %d bytes for %d\n",
                    size*sizeof(char *), size);
            }

            ret=realloc(ret, size*sizeof(char *));
        }
    }
    ret[index]=NULL;

    dbm_close(db);
    return(ret);
}

void cleantermlist(char **list)
{
    int i;

    if(conf.debug>2)
	puts("Freeing terminal list.");

    for(i=0; list[i]!=NULL; i++)
    {
	free(list[i]);
    }
    free(list);
}

void printterms(void)
{
    struct terminal t;
    int i;
    char **list;

    list=listterms();
    for(i=0; list[i]!=NULL; i++)
    {
        t=getterm(list[i]);
        printterm(t);
        puts("--------");
	/* printf("%d:  %s\n", i, list[i]); */
    }

    cleantermlist(list);
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
