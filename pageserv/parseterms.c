/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseterms.c,v 1.11 1998/12/28 02:56:57 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pageserv.h>
#include <readconfig.h>

extern struct config conf;

struct terminal
parseterm(char *line)
{
	struct terminal t;
	char           *tmp, *delim;

	memset(&t, 0x00, sizeof(struct terminal));

	delim = rcfg_lookup(conf.cf, "databases.textdelim");
	if (delim == NULL)
		delim = " \t";

	tmp = strtok(line, delim);
	if (tmp == NULL) {
		return (t);
	} else {
		if (strlen(tmp) > (size_t) STATLEN) {
			printf("Terminal ``%s'' is too long, skipping\n", tmp);
			return (t);
		} else {
			strcpy(t.number, tmp);
		}
	}

	tmp = strtok(NULL, delim);
	if (tmp == NULL) {
		return (t);
	} else {
		t.flags = atoi(tmp);
	}

	tmp = strtok(NULL, delim);
	if (tmp == NULL) {
		return (t);
	} else {
		if (strlen(tmp) > (size_t) FNSIZE) {
			printf("Port ``%s'' is too long, skipping\n", tmp);
			return (t);
		} else {
			strcpy(t.ts, tmp);
		}
	}

	tmp = strtok(NULL, delim);
	if (tmp == NULL) {
		return (t);
	} else {
		t.port = atoi(tmp);
	}

	tmp = strtok(NULL, delim);
	if (tmp == NULL) {
		return (t);
	} else {
		if (strlen(tmp) > (size_t) STATLEN) {
			printf("Predial ``%s'' is too long, skipping\n", tmp);
			return (t);
		} else {
			strcpy(t.predial, tmp);
		}
	}

	tmp = strtok(NULL, delim);
	if (tmp == NULL) {
		_ndebug(2, ("Using default init %s\n", DEF_INIT));
		strcpy(t.init, DEF_INIT);
	} else {
		if (strlen(tmp) > (size_t) INITLEN) {
			printf("Init ``%s'' is too long, skipping\n", tmp);
			return (t);
		} else {
			strcpy(t.init, tmp);
		}
	}

	return (t);
}

int
parseterms(void)
{
	FILE           *f;
	char            buf[BUFLEN];
	struct terminal t;
	int             i = 0;

	if ((f = fopen(conf.termdb, "r")) == NULL) {
		perror(conf.termdb);
		exit(1);
	}
	while (fgets(buf, BUFLEN, f)) {
		if ((buf[0] != '#') && (!isspace(buf[0]))) {
			t = parseterm(buf);
			if (strlen(t.predial) > 0) {
				if (conf.debug > 0) {
					printterm(t);
					puts("--");
				}
				conf.tdb.store(t);
				i++;
			}
		}
	}

	fclose(f);

	return (i);
}
