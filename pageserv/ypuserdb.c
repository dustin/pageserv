/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: ypuserdb.c,v 1.6 1998/01/10 01:33:14 dustin Exp $
 */

#include <config.h>

/* This code should only be compiled if there is NIS support */
#ifdef HAVE_NIS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_RPCSVC_YPCLNT_H
#include <rpcsvc/ypclnt.h>
#endif

#include <pageserv.h>
#include <readconfig.h>

extern struct config conf;

static char *domainname;

static int nis_u_exists(char *name)
{
    char *data;
    int yperr, len;

    yperr=yp_match(domainname, conf.userdb, name,
	  strlen(name), &data, &len);

    if(!yperr)
	free(data);

    return(!yperr);
}

static struct user nis_getuser(char *name)
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

static char **nis_listusers(char *term)
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

	    if(conf.debug>2)
	    {
		printf("Reallocating, now need %d bytes for %d\n",
		    size*sizeof(char *), size);
	    }

	    ret=realloc(ret, size*sizeof(char *));
	}

	yperr=yp_next(domainname, conf.userdb, key, keylen,
	      &key, &keylen, &val, &vallen);
    }
    ret[index]=NULL;
    return(ret);
}

static int nis_deleteuser(char *name)
{
    return(0);
}

static void nis_storeuser(struct user u)
{
    return;
}

static void nis_eraseuserdb(void)
{
    return;
}

static int nis_parseusers(void)
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

#endif
