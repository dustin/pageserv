/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseusers.c,v 1.8 1997/07/09 07:26:22 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ndbm.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pageserv.h>

extern struct config conf;

struct user parseuser(char *line)
{
    struct user u;
    int early, late, i;

    u.times=0;

    memset(&u.passwd, 0x00, PWLEN);

    sscanf(line, "%s %s %s %d %d", u.name, u.pageid, u.statid,
            &early, &late);

    u.times=pack_timebits(early, late);

    return(u);
}

int parseusers(void)
{
    FILE *f;
    char buf[BUFLEN];
    struct user u;
    DBM *db;
    int i=0;

    if( (f=fopen(conf.userdb, "r")) == NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    if( (db=dbm_open(conf.userdb, O_CREAT|O_RDWR, 0644)) ==NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    while(fgets(buf, BUFLEN, f))
    {
        if( (buf[0]!='#') && (!isspace(buf[0])) )
        {
            u=parseuser(buf);
	    if(conf.debug>0)
	    {
		printuser(u);
		puts("--");
	    }
            open_storeuser(db, u);
            i++;
        }
    }

    fclose(f);
    dbm_close(db);

    return(i);
}
