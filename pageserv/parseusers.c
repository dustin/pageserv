/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseusers.c,v 1.13 1998/02/26 17:17:51 dustin Exp $
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
#include <readconfig.h>

extern struct config conf;

struct user parseuser(char *line, char *delim, int flags)
{
    struct user u;
    int early, late;
    char *tmp;

    u.times=0;

    memset(&u, 0x00, sizeof(struct user));

    tmp=strtok(line, delim);
    if(tmp==NULL)
    {
        return(u);
    }
    else
    {
        if(strlen(tmp)>(size_t)NAMELEN)
        {
            printf("Name ``%s'' is too long, skipping\n", tmp);
            return(u);
        }
        else
        {
            kw(tmp);
            strcpy(u.name, tmp);
        }
    }

    if(flags & PARSE_GETPASSWD)
    {
	_ndebug(4, ("Getting password field\n"));
        tmp=strtok(NULL, delim);
        if(tmp==NULL)
        {
            return(u);
        }
        else
        {
            if(strlen(tmp)>(size_t)PWLEN)
            {
                printf("ID ``%s'' is too long, skipping\n", tmp);
                return(u);
            }
            else
            {
		kw(tmp);
                strcpy(u.passwd, tmp);
            }
        }
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(u);
    }
    else
    {
        if(strlen(tmp)>(size_t)IDLEN)
        {
            printf("ID ``%s'' is too long, skipping\n", tmp);
            return(u);
        }
        else
        {
	    kw(tmp);
            strcpy(u.pageid, tmp);
        }
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(u);
    }
    else
    {
        if(strlen(tmp)>(size_t)STATLEN)
        {
            printf("Station ID ``%s'' is too long, skipping\n", tmp);
            return(u);
        }
        else
        {
	    kw(tmp);
            strcpy(u.statid, tmp);
        }
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(u);
    }
    else
    {
        early=atoi(tmp);
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(u);
    }
    else
    {
        late=atoi(tmp);
    }

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
    char *delim;

    if( (f=fopen(conf.userdb, "r")) == NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    delim=rcfg_lookup(conf.cf, "databases.textdelim");
    if(delim==NULL)
        delim=" \t";

    /* just initialize the database */
    if( (db=dbm_open(conf.userdb, O_CREAT|O_RDWR, 0644)) ==NULL)
    {
        perror(conf.userdb);
        exit(1);
    }
    dbm_close(db);

    while(fgets(buf, BUFLEN, f))
    {
        if( (buf[0]!='#') && (!isspace(buf[0])) )
        {
            u=parseuser(buf, delim, 0);
            if(strlen(u.statid)>0)
            {
                if(conf.debug>0)
                {
                    printuser(u);
                    puts("--");
                }
                conf.udb.storeuser(u);
                i++;
            }
        }
    }

    fclose(f);

    return(i);
}
