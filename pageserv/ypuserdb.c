/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: ypuserdb.c,v 1.13 1998/06/29 01:07:05 dustin Exp $
 */

#include <config.h>

/* This code should only be compiled if there is NIS support */
#ifdef HAVE_NIS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include <fcntl.h>

#ifdef HAVE_RPCSVC_YPCLNT_H
#include <rpcsvc/ypclnt.h>
#endif

#include <pageserv.h>
#include <readconfig.h>

/* This is a little dangerous, as I'm not doing memory debugging on it
   because of NIS */

#undef malloc
#undef free
#undef strdup
#undef realloc
#undef calloc

extern struct config conf;

static char *domainname=NULL;

static int nis_u_exists(char *name)
{
    char *data=NULL;
    int yperr, len;

    yperr=yp_match(domainname, conf.userdb, name,
	  strlen(name), &data, &len);

    if(data!=NULL && (!yperr) )
	    free(data);

    return(!yperr);
}

static struct user nis_getuser(char *name)
{
    struct user u;
    char *data;
    int yperr, len;

    yperr=yp_match(domainname, conf.userdb, name,
	  strlen(name), &data, &len);

    if(!yperr)
    {
	u=parseuser(data, ":", PARSE_GETPASSWD);
	free(data);
    }

    if(strlen(u.statid)<(size_t)1)
    {
	_ndebug(2, ("nis_getuser:  Parser didn't like my data for ``%s'',"
		    "eating the evidence.\n", name));
	memset(&u, 0x00, sizeof(struct user));
    }

    return(u);
}

static char **nis_listusers(char *term)
{
    char **ret;
    struct user u;
    int size, index=0, keylen, vallen, yperr;
    char *key, *val;
    char tmp[BUFLEN];

    size=rcfg_lookupInt(conf.cf, "tuning.utableguess");
    if(size<2)
	size=4;

    _ndebug(2, ("Table size guess is %d\n", size));

    ret=malloc(size * sizeof(char *));

    yperr=yp_first(domainname, conf.userdb, &key,
          &keylen, &val, &vallen);

    while(!yperr)
    {
	strncpy(u.name, key, keylen);
	u.name[keylen]=0x00;

        _ndebug(4, ("nis_listusers found key ``%s''\n", u.name));

	if(vallen>BUFLEN)
	{
	    _ndebug(2, ("My buffer runneth over.\n"));
	    memset(&u, 0x00, sizeof(struct user));
	}
	else
	{
	    strncpy(tmp, val, vallen);
	    tmp[vallen]=0x00;
	    u=parseuser(tmp, ":", PARSE_GETPASSWD);
	}

	if( (strcmp(u.statid, term) == 0) || (strcmp(term, "*") == 0) )
	{
	    ret[index]=malloc(keylen+1);
	    strncpy(ret[index], key, keylen);
	    ret[index++][keylen]=0x00;
	}
	else
	{
	    _ndebug(2, ("user didn't match pattern\n"));
	}

	free(key);
	free(val);

	if(index == size-1)
	{
	    size<<=1;

	    _ndebug(2, ("Reallocating, now need %d bytes for %d\n",
			size*sizeof(char *), size));

	    ret=realloc(ret, size*sizeof(char *));
	    assert(ret);
	}

	_ndebug(4, ("Index is %d, grabbing a new key\n", index));

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

    if(domainname==NULL)
    {
        domainname=rcfg_lookup(conf.cf, "etc.nisdomain");
        if(domainname==NULL)
        {
	    yp_get_default_domain(&domainname);
        }
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
