/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: ldaptermdb.c,v 1.4 1998/12/29 09:31:56 dustin Exp $
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
	if (ld == NULL)
		return (NULL);

	ret = ldap_bind_s(ld, binddn, bindpw, LDAP_AUTH_SIMPLE);
	if (ret != LDAP_SUCCESS) {
		return (NULL);
	}
	return (ld);
}

static struct terminal
ldap_getterm(char *number)
{
	struct terminal t;
	LDAP *ld;
	LDAPMessage *res=0;
	char filter[8192];
	char **values;
	char *base;
	char *att[]={
		"pageNumber",
		"pageDevice",
		"pagePredial",
		"pageInit",
		"pagePort",
		0
	};

	memset(&t, 0x00, sizeof(t));
	snprintf(filter, 8190, "pageNumber=%s", number);

	base=rcfg_lookup(conf.cf, "databases.ldap.base");

	ld=ldap_getld();
	if(ld==NULL)
		return(t);

	_ndebug(2, ("Doing ldap search on base ``%s'' for ``%s''\n",
		base, filter));

	if (ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE, filter, att, 0, 0, &res)
		==-1) {
		ldap_unbind(ld);
		return(t);
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

	_VALUE_FETCH("pageNumber", t.number, STATLEN);
	_VALUE_FETCH("pageDevice", t.ts, FNSIZE);
	_VALUE_FETCH("pagePredial", t.predial, STATLEN);
	_VALUE_FETCH("pageInit", t.init, INITLEN);
	_VALUE_FETCH_INT("pageFlags", t.flags);
	_VALUE_FETCH_INT("pagePort", t.port);

	values=ldap_get_values(ld, res, "pagePredial");
	if(values && values[0]) {
		strncpy(t.predial, values[0],
		strlen(values[0])<STATLEN?strlen(values[0]):STATLEN-1);
	}
	ldap_value_free(values);

	values=ldap_get_values(ld, res, "pagePredial");
	if(values && values[0]) {
		strncpy(t.predial, values[0],
		strlen(values[0])<STATLEN?strlen(values[0]):STATLEN-1);
	}
	ldap_value_free(values);

	return (t);
}

static int
ldap_t_exists(char *name)
{
	struct terminal t;
	int ret;

	t=ldap_getterm(name);
	if(t.number[0])
		ret=1;
	else
		ret=0;
	return(ret);
}

static char   **
ldap_listterms(void)
{
	LDAP *ld;
	LDAPMessage *res, *e;
	char **values, **ret;
	char *base;
	int index, sizelimit;
	char *att[]={
		"pageNumber",
		0
	};

	ld=ldap_getld();
	if(ld==NULL)
		return(NULL);

	sizelimit=rcfg_lookupInt(conf.cf, "databases.ldap.sizelimit");

#ifdef LDAP_OPT_SIZELIMIT
	ldap_set_option(ld, LDAP_OPT_SIZELIMIT, &sizelimit);
#else
	ld->ld_sizelimit = sizelimit;
#endif

	base=rcfg_lookup(conf.cf, "databases.ldap.base");

	_ndebug(2, ("Doing ldap search on base ``%s'' for ``%s''\n",
		base, "objectclass=pageservTerminal"));

	if(ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE,
		"objectclass=pageservTerminal", att, 0, 0, &res) == -1) {
		ldap_unbind(ld);
		return(NULL);
	}
	_ndebug(2, ("Found %d entries\n", ldap_count_entries(ld, res)));
	ret=malloc( (ldap_count_entries(ld, res)+2)*sizeof(char *));
	index=0;
	for(e=ldap_first_entry(ld, res); e; e=ldap_next_entry(ld, e)) {
		values=ldap_get_values(ld, e, "pageNumber");
		ret[index++]=strdup(values[0]);
		ldap_value_free(values);
	}
	ldap_unbind(ld);
	ret[index]=NULL;
	return(ret);
}

static void
ldap_storeterm(struct terminal t)
{
	return;
}

static void
ldap_erasetermdb(void)
{
	return;
}

void
ldap_termdbInit(void)
{
	conf.tdb.erase=ldap_erasetermdb;
	conf.tdb.list=ldap_listterms;
	conf.tdb.store=ldap_storeterm;
	conf.tdb.get=ldap_getterm;
	conf.tdb.exists=ldap_t_exists;
	conf.tdb.dbinit=ldap_termdbInit;
}

#endif				/* HAVE_LDAP */
