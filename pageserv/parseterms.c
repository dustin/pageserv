/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseterms.c,v 1.3 1997/04/04 22:21:04 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ndbm.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pageserv.h>

extern struct config conf;

struct terminal parseterm(char *line)
{
    struct terminal t;

    sscanf(line, "%s %d %s %d %s", t.number, &t.prot, t.ts, &t.port,
        t.predial);

    return(t);
}

int parseterms(void)
{
    FILE *f;
    char buf[BUFLEN];
    struct terminal t;
    DBM *db;
    int i=0;

    if( (f=fopen(conf.termdb, "r")) == NULL)
    {
        perror(conf.termdb);
        exit(1);
    }

    if( (db=dbm_open(conf.termdb, O_CREAT|O_RDWR, 0644)) ==NULL)
    {
        perror(conf.termdb);
        exit(1);
    }

    while(fgets(buf, BUFLEN, f))
    {
        if( (buf[0]!='#') && (!isspace(buf[0])) )
        {
            t=parseterm(buf);
	    if(conf.debug>0)
	    {
		printterm(t);
		puts("--");
	    }

            storeterm(db, t);
            i++;
        }
    }

    fclose(f);
    dbm_close(db);

    return(i);
}
