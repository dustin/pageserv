/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: sqluserdb.c,v 1.3 1998/01/27 01:32:41 dustin Exp $
 */

#include <config.h>

/* This code should only be compiled if there is SQL support */
#ifdef HAVE_SQL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pageserv.h>
#include <readconfig.h>

#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif

extern struct config conf;

#ifdef HAVE_POSTGRES
static PGresult *do_sql_pg(char *query)
{
    PGconn *conn;
    PGresult *res;

    conn=PQsetdb(
	rcfg_lookup(conf.cf, "databases.sql.host"),
	rcfg_lookup(conf.cf, "databases.sql.port"),
	rcfg_lookup(conf.cf, "databases.sql.options"),
	rcfg_lookup(conf.cf, "databases.sql.tty"),
        rcfg_lookup(conf.cf, "databases.sql.db"));

    if(PQstatus(conn)==CONNECTION_BAD)
    {
	/* Keen logging thing goes here */
        _ndebug(2, (PQerrorMessage(conn)));
	return(NULL);
    }

    _ndebug(2, ("Doing query:\n%s\n", query));

    res=PQexec(conn, query);

    if(PQresultStatus(res)!=PGRES_TUPLES_OK)
    {
	/* Keen logging thing goes here */
        _ndebug(2, (PQerrorMessage(conn)));
	return(NULL);
    }

    PQfinish(conn);

    return(res);
}
#endif

static int sql_u_exists(char *name)
{
    char buf[BUFLEN];
    PGresult *res;

    sprintf(buf, "select count(*) from %s where %s='%s';",
	rcfg_lookup(conf.cf, "databases.sql.udb.table"),
	rcfg_lookup(conf.cf, "databases.sql.udb.name"),
	name);

    res=do_sql_pg(buf);

    if(res!=NULL)
    {
	return(atoi(PQgetvalue(res, 0, 0)));
    }
    else
    {
	return(0);
    }
}

static int sql_deleteuser(char *name)
{
    return(0);
}

static void sql_storeuser(struct user u)
{
    return;
}

static void sql_eraseuserdb(void)
{
    return;
}

static int sql_parseusers(void)
{
    return 0;
}

static struct user sql_getuser(char *name)
{
    struct user u;
    memset(&u, 0x00, sizeof(u));
    return(u);
}

static char **sql_listusers(char *term)
{
    return(NULL);
}

/* Reference from NIS so we can remember what all we need to do */
#if 0

struct user nis_getuser(char *name)
{
    struct user u;
    char *data;
    int yperr, len, times[2];

    memset(&u, 0x00, sizeof(u));

    yperr=yp_match(domainname, conf.userdb, name,
	  strlen(name), &data, &len);

    if(!yperr)
    {
	strcpy(u.name, strtok(data, ":"));
	strcpy(u.passwd, strtok(NULL, ":"));
	strcpy(u.pageid, strtok(NULL, ":"));
	strcpy(u.statid, strtok(NULL, ":"));
	times[0]=atoi(strtok(NULL, ":"));
	times[1]=atoi(strtok(NULL, ":"));
	u.times=pack_timebits(times[0], times[1]);

	free(data);
    }

    return(u);
}

char **nis_listusers(char *term)
{
    char **ret;
    struct user u;
    int size=4, index=0, keylen, vallen, yperr;
    char *key, *val;

    ret=malloc(size * sizeof(char *));

    yperr=yp_first(domainname, conf.userdb, &key,
          &keylen, &val, &vallen);

    while(!yperr)
    {
	strncpy(u.name, key, keylen);
	u.name[keylen]=0x00;
	u=conf.udb.getuser(u.name);

	if( (strcmp(u.statid, term) == 0) || (strcmp(term, "*") == 0) )
	{
	    ret[index]=malloc(keylen+1);
	    strncpy(ret[index], key, keylen);
	    ret[index++][keylen]=0x00;
	}

	free(key);
	free(val);

	if(index == size-1)
	{
	    size<<=1;

            _ndebug(2, ("Reallocating, now need %d bytes for %d\n"
			size*sizeof(char *), size));

	    ret=realloc(ret, size*sizeof(char *));
	}

	yperr=yp_next(domainname, conf.userdb, key, keylen,
	      &key, &keylen, &val, &vallen);
    }
    ret[index]=NULL;
    return(ret);
}

int nis_deleteuser(char *name)
{
    return(0);
}

void nis_storeuser(struct user u)
{
    return;
}

void nis_eraseuserdb(void)
{
    return;
}

int nis_parseusers(void)
{
    return 0;
}

void nis_userdbInit(void)
{
    char *data;
    int len, yperr;

    domainname=rcfg_lookup(conf.cf, "etc.nisdomain");
    if(domainname==NULL)
    {
	yp_get_default_domain(&domainname);
    }

    yp_bind(domainname);

    yperr=yp_match(domainname, conf.userdb, "blah",
	  4, &data, &len);

    if(yperr==YPERR_MAP)
    {
	fprintf(stderr,
		"Warning, table %s not found in NIS, setting to %s\n",
		conf.userdb, NIS_DEFAULTUDB);
	conf.userdb=NIS_DEFAULTUDB;
    }

    conf.udb.u_exists=nis_u_exists;
    conf.udb.getuser=nis_getuser;
    conf.udb.listusers=nis_listusers;
    conf.udb.storeuser=nis_storeuser;
    conf.udb.deleteuser=nis_deleteuser;
    conf.udb.eraseuserdb=nis_eraseuserdb;
    conf.udb.parseusers=nis_parseusers;
}
#endif /* 0 */

void sql_userdbInit(void)
{
    conf.udb.u_exists=sql_u_exists;
    conf.udb.getuser=sql_getuser;
    conf.udb.listusers=sql_listusers;
    conf.udb.storeuser=sql_storeuser;
    conf.udb.deleteuser=sql_deleteuser;
    conf.udb.eraseuserdb=sql_eraseuserdb;
    conf.udb.parseusers=sql_parseusers;
}

#endif /* HAVE_SQL */
