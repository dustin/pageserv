/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseusers.c,v 1.4 1997/04/11 03:45:52 dustin Exp $
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

struct user parseuser(char *line)
{
    struct user u;
    int early, late, i;

    u.times=0;

    memset(&u.passwd, 0x00, PWLEN);

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
            storeuser(db, u);
            i++;
        }
    }

    fclose(f);
    dbm_close(db);

    return(i);
}
