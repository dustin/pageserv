/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseusers.c,v 1.7 1997/03/14 16:09:34 dustin Exp $
 */

#include <stdio.h>
#include <ndbm.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pageserv.h"

struct user parseuser(char *line)
{
    struct user u;
    int early, late, i;

    u.times=0;

    sscanf(line, "%s %s %s %d %d", u.name, u.pageid, u.statid,
            &early, &late);

    if(early>23 || early < 0 || late >23 || late < 0)
    {
	u.times=0xFFFFFFFF;
    }
    else
    {
	/* subtract one from early so it will make sense. */
	if(early>0)
	    early--;
	else
	    early=23;

	if(early < late)
	{
	    for(i=early; i<late ; i++)
	    {
		u.times=set_bit(u.times, i);
	    }
	}
	else
	{
	    for(i=early; i<24 ; i++)
	    {
		u.times=set_bit(u.times, i);
	    }

	    for(i=0; i<late ; i++)
	    {
		u.times=set_bit(u.times, i);
	    }
	}
    }

    return(u);
}

void main(void)
{
    FILE *f;
    char buf[BUFLEN];
    struct user u;
    DBM *db;
    int i=0;

    if( (f=fopen(USERDB, "r")) == NULL)
    {
	perror(USERDB);
	exit(1);
    }

    if( (db=dbm_open(USERDB, O_CREAT|O_RDWR, 0644)) ==NULL)
    {
	perror(USERDB);
	exit(1);
    }

    while(fgets(buf, BUFLEN, f))
    {
	if( (buf[0]!='#') && (!isspace(buf[0])) )
	{
	    u=parseuser(buf);
	    storeuser(db, u);
	    i++;
	}
    }

    printf("Parsed %d users.\n", i);

    fclose(f);
    dbm_close(db);
}
