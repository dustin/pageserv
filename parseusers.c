/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseusers.c,v 1.10 1997/03/26 00:24:43 dustin Exp $
 */

#include <stdio.h>
#include <ndbm.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pageserv.h"

struct config conf;

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
            storeuser(db, u);
            i++;
        }
    }

    fclose(f);
    dbm_close(db);

    return(i);
}

void main(void)
{
    int users;

    readconfig(CONFIGFILE);
    if(conf.debug>0)
	showconfig();

    users=parseusers();
    printf("Parsed %d users.\n", users);

    cleanconfig();
}
