/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: ldapuserdb.c,v 1.4 1998/12/27 15:12:17 dustin Exp $
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

static struct user
ldap_getuser(char *name)
{
	LDAP           *ld;
	LDAPMessage    *res = 0;
	char            filter[8192];
	char          **values;
	char           *base;
	struct user     u;
	char           *att[] = {
		"uid",
		"pagePasswd",
		"pageId",
		"pageStatId",
		"pageNotify",
		"pageFlags",
		"pageTimes",
		0
	};

	memset(&u, 0x00, sizeof(struct user));
	snprintf(filter, 8190, "uid=%s", name);

	base=rcfg_lookup(conf.cf, "databases.ldap.base");

	ld = ldap_getld();
	if(ld==NULL)
		return(u);

	if (ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE, filter, att, 0, 0, &res)
	    == -1) {
		return (u);
		ldap_unbind(ld);
	}
	values = ldap_get_values(ld, res, "uid");
	if (values && values[0]) {
		strncpy(u.name, values[0],
			strlen(values[0]) < NAMELEN ? strlen(values[0]) : NAMELEN - 1);
	}
	ldap_value_free(values);
	values = ldap_get_values(ld, res, "pagepasswd");
	if (values && values[0]) {
		strncpy(u.passwd, values[0],
		 strlen(values[0]) < PWLEN ? strlen(values[0]) : PWLEN - 1);
	}
	ldap_value_free(values);
	values = ldap_get_values(ld, res, "pageid");
	if (values && values[0]) {
		strncpy(u.pageid, values[0],
		 strlen(values[0]) < IDLEN ? strlen(values[0]) : IDLEN - 1);
	}
	ldap_value_free(values);
	values = ldap_get_values(ld, res, "pagestatid");
	if (values && values[0]) {
		strncpy(u.statid, values[0],
			strlen(values[0]) < STATLEN ? strlen(values[0]) : STATLEN - 1);
	}
	ldap_value_free(values);
	values = ldap_get_values(ld, res, "pagenotify");
	if (values && values[0]) {
		strncpy(u.notify, values[0],
			strlen(values[0]) < EMAILLEN ? strlen(values[0]) : EMAILLEN - 1);
	}
	ldap_value_free(values);
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
	LDAPMessage *res;
	char **values;
	char  *base;
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

	if(ldap_search_st(ld, base, LDAP_SCOPE_SUBTREE, filter, att, 0, 0, &res)
		== -1) {
		ldap_unbind(ld);
		return(NULL);
	}
	values=ldap_get_values(ld, res, "uid");
	ldap_unbind(ld);
	return(values);
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
