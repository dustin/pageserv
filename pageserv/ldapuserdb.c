/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: ldapuserdb.c,v 1.9 1998/12/29 03:24:52 dustin Exp $
 */

#include <config.h>

/* This code should only be compiled if there is LDAP support */
#ifdef HAVE_LDAP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include <lber.h>
#include <ldap.h>

#include <fcntl.h>

#include <pageserv.h>
#include <readconfig.h>

extern struct config conf;

static LDAP    *
ldap_getld(void)
{
	LDAP           *ld;
	int             ret;
	char           *binddn, *server, *bindpw;

	binddn=rcfg_lookup(conf.cf, "databases.ldap.binddn");
	bindpw=rcfg_lookup(conf.cf, "databases.ldap.bindpw");
	server=rcfg_lookup(conf.cf, "databases.ldap.server");

	ld = ldap_open(server, 0);
	if (ld == NULL) {
		_ndebug(2, ("ldap_open(%s) failed\n", server));
		return (NULL);
	}

	ret = ldap_bind_s(ld, binddn, bindpw, LDAP_AUTH_SIMPLE);
	if (ret != LDAP_SUCCESS) {
		_ndebug(2, ("ldap_bind_s(%p, %s, %s, %d) failed\n", ld, binddn,
			bindpw, LDAP_AUTH_SIMPLE));
		return (NULL);
	}
	return (ld);
}

static struct user
ldap_getuser(char *name)
{
	LDAP           *ld;
	LDAPMessage    *res = 0;
	char            filter[8192];
	char          **values;
	char           *base;
	struct user     u;
	int				times[2];
	char           *att[] = {
		"uid",
		"pagePasswd",
		"pageId",
		"pageStatId",
		"pageNotify",
		"pageFlags",
		"pageEarly",
		"pageLate",
		0
	};

	memset(&u, 0x00, sizeof(struct user));
	snprintf(filter, 8190, "uid=%s", name);

	base=rcfg_lookup(conf.cf, "databases.ldap.base");

	ld = ldap_getld();
	if(ld==NULL)
		return(u);

	_ndebug(2, ("Doing ldap search on base ``%s'' for ``%s''\n",
		base, filter));

	if (ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE, filter, att, 0, 0, &res)
	    == -1) {
		ldap_unbind(ld);
		return (u);
	}

	/* attribut_name destination size */
	#define _VALUE_FETCH(a, b, c) { \
		values=ldap_get_values(ld, res, a); \
		if(values && values[0]) { \
			strncpy(b, values[0], \
				strlen(values[0])<c ? strlen(values[0]):c-1); \
		} \
		ldap_value_free(values); \
	}

	/* attribut_name destination */
	#define _VALUE_FETCH_INT(a, b) { \
		values=ldap_get_values(ld, res, a); \
		if(values && values[0]) { \
			b=atoi(values[0]); \
		} \
		ldap_value_free(values); \
	}

	_VALUE_FETCH("uid", u.name, NAMELEN);
	_VALUE_FETCH("pagePasswd", u.passwd, PWLEN);
	_VALUE_FETCH("pageId", u.pageid, IDLEN);
	_VALUE_FETCH("pageStatid", u.statid, STATLEN);
	_VALUE_FETCH("pageNotify", u.notify, EMAILLEN);

	/* get early and late times */

	times[0]=times[1]=0;

	_VALUE_FETCH_INT("pageEarly", times[0]);
	_VALUE_FETCH_INT("pageLate", times[1]);

	u.times=pack_timebits(times[0], times[1]);

	ldap_unbind(ld);
	return (u);
}

static int
ldap_u_exists(char *name)
{
	struct user u;
	int ret;

	u=ldap_getuser(name);
	if(u.pageid[0])
		ret=1;
	else
		ret=0;
	return(ret);
}

static char   **
ldap_listusers(char *term)
{
	LDAP *ld;
	LDAPMessage *res, *e;
	char **values, **ret;
	char  *base;
	int index;
	char *att[] = {
		"uid",
		0
	};
	char filter[8192];

	if(strlen(term)>4096)
		return(NULL);

	snprintf(filter, 8190, "pagestatid=%s", term);
	base=rcfg_lookup(conf.cf, "databases.ldap.base");

	ld=ldap_getld();
	if(ld==NULL)
		return(NULL);

	_ndebug(2, ("Doing ldap search on base ``%s'' for ``%s''\n",
		base, filter));

	if(ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE, filter, att, 0, 0, &res)
		== -1) {
		ldap_unbind(ld);
		return(NULL);
	}
	_ndebug(2, ("Found %d entries\n", ldap_count_entries(ld, res)));
	index=0;
	ret=malloc( (ldap_count_entries(ld, res)+2)*sizeof(char *));
	for(e=ldap_first_entry(ld, res); e; e=ldap_next_entry(ld, e)) {
		values=ldap_get_values(ld, e, "uid");
		ret[index++]=strdup(values[0]);
		ldap_value_free(values);
	}
	ldap_unbind(ld);
	ret[index]=NULL;
	return(ret);
}

static int
ldap_deleteuser(char *name)
{
	return (0);
}

static void
ldap_storeuser(struct user u)
{
	return;
}

static void
ldap_eraseuserdb(void)
{
	return;
}

static int
ldap_parseusers(void)
{
	return 0;
}

void
ldap_userdbInit(void)
{
	_ndebug(2, ("LDAP setting up\n"));
	conf.udb.u_exists = ldap_u_exists;
	conf.udb.getuser = ldap_getuser;
	conf.udb.listusers = ldap_listusers;
	conf.udb.storeuser = ldap_storeuser;
	conf.udb.deleteuser = ldap_deleteuser;
	conf.udb.eraseuserdb = ldap_eraseuserdb;
	conf.udb.parseusers = ldap_parseusers;
}

#endif				/* HAVE_LDAP */
