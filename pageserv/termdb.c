/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: termdb.c,v 1.14 1998/12/28 02:57:01 dustin Exp $
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

static DBM *dbm_open_termdb(flags)
{
	DBM *db;
    if( (db=dbm_open(conf.termdb, flags, 0644)) == NULL)
        perror(conf.termdb);
	return(db);
}

void printterm(struct terminal t)
{
    printf("Number:     %s\n", t.number);
    printf("Term serv:  %s\n", t.ts);
    printf("Predial:    %s\n", t.predial);
    printf("Port:       %d\n", t.port);
    printf("Flags:      %d\n", t.flags);
}

static void dbm_erasetermdb(void)
{
    datum d;
    DBM *db;
    char *tmp;

	db=dbm_open_termdb(O_RDWR);
	if(db==NULL)
		return;

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_firstkey(db)) {
        if(conf.debug>2) {
            tmp=strdup((char *)d.dptr);
            tmp[d.dsize]=0x00;
            _ndebug(2, ("deleting %s\n", tmp));
            free(tmp);
        }
        dbm_delete(db, d);
    }

    dbm_close(db);
}

static char **dbm_listterms(void)
{
    datum d;
    DBM *db;
    char **ret;
    int size=4, index=0;

    ret=malloc(size * sizeof(char *));

	db=dbm_open_termdb(O_RDONLY);
	if(db==NULL)
		return(NULL);

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db)) {
        ret[index]=malloc(d.dsize+1);
        strncpy(ret[index], d.dptr, d.dsize);
        ret[index++][d.dsize]=0x00;

        if(index == size-1) {
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

void cleantermlist(char **list)
{
	int i;

	_ndebug(2, ("Freeing terminal list.\n"));

	for(i=0; list[i]!=NULL; i++)
		free(list[i]);
	free(list);
}

void printterms(void)
{
    struct terminal t;
    int i;
    char **list;

    list=conf.tdb.list();
	if(list==NULL)
		return;
    for(i=0; list[i]!=NULL; i++) {
        t=conf.tdb.get(list[i]);
        printterm(t);
        puts("--------");
    }

    cleantermlist(list);
}

static void dbm_open_storeterm(DBM *db, struct terminal t)
{
    datum k, d;
    k.dptr=t.number;
    k.dsize=strlen(t.number);

    d.dptr=(char *)&t;
    d.dsize=sizeof(t);

    dbm_store(db, k, d, DBM_REPLACE);
}

static void dbm_storeterm(struct terminal t)
{
	DBM *db;
	db=dbm_open_termdb(O_RDWR|O_CREAT);
	if(db==NULL)
		return;
	dbm_open_storeterm(db, t);
	dbm_close(db);
}

static struct terminal dbm_open_getterm(DBM *db, char *number)
{
    datum d, k;
    struct terminal t;

    memset((void *)&t, 0x00, sizeof(t));

    k.dptr=number;
    k.dsize=strlen(number);
    d=dbm_fetch(db, k);

    if(d.dptr!=NULL)
        memcpy( (void *)&t, (void *)d.dptr, sizeof(t));

    return(t);
}

static struct terminal dbm_getterm(char *number)
{
    DBM *db;
    struct terminal t;
	memset(&t, 0x00, sizeof(t));
	db=dbm_open_termdb(O_RDONLY);
	if(db==NULL)
		return(t);
    t=dbm_open_getterm(db, number);
    dbm_close(db);
    return(t);
}

static int dbm_t_exists(char *number)
{
    datum d, k;
    DBM *db;

	db=dbm_open_termdb(O_RDONLY);
	if(db==NULL)
		return(0);

    k.dptr=number;
    k.dsize=strlen(number);

    d=dbm_fetch(db, k);

    dbm_close(db);

    return(d.dptr!=NULL);
}

void dbm_termdbInit(void)
{
	conf.tdb.erase=dbm_erasetermdb;
	conf.tdb.list=dbm_listterms;
	conf.tdb.store=dbm_storeterm;
	conf.tdb.get=dbm_getterm;
	conf.tdb.exists=dbm_t_exists;
	conf.tdb.dbinit=dbm_termdbInit;
}
