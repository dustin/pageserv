/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parseterms.c,v 1.8 1998/01/22 10:40:32 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndbm.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pageserv.h>
#include <readconfig.h>

extern struct config conf;

struct terminal parseterm(char *line)
{
    struct terminal t;
    char *tmp, *delim;

    memset(&t, 0x00, sizeof(struct terminal));

    delim=rcfg_lookup(conf.cf, "databases.textdelim");
    if(delim==NULL)
        delim=" ";

    tmp=strtok(line, delim);
    if(tmp==NULL)
    {
        return(t);
    }
    else
    {
        if(strlen(tmp)>(size_t)STATLEN)
        {
            printf("Terminal ``%s'' is too long, skipping\n", tmp);
            return(t);
        }
        else
        {
            strcpy(t.number, tmp);
        }
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(t);
    }
    else
    {
        t.flags=atoi(tmp);
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(t);
    }
    else
    {
        if(strlen(tmp)>(size_t)FNSIZE)
        {
            printf("Port ``%s'' is too long, skipping\n", tmp);
            return(t);
        }
        else
        {
            strcpy(t.ts, tmp);
        }
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(t);
    }
    else
    {
        t.port=atoi(tmp);
    }

    tmp=strtok(NULL, delim);
    if(tmp==NULL)
    {
        return(t);
    }
    else
    {
        if(strlen(tmp)>(size_t)STATLEN)
        {
            printf("Predial ``%s'' is too long, skipping\n", tmp);
            return(t);
        }
        else
        {
            strcpy(t.predial, tmp);
        }
    }

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
	    if(strlen(t.predial)>0)
	    {
	        if(conf.debug>0)
	        {
                    printterm(t);
		    puts("--");
	        }

                storeterm(db, t);
                i++;
	    }
        }
    }

    fclose(f);
    dbm_close(db);

    return(i);
}
