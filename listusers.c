/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: listusers.c,v 1.5 1997/03/26 00:24:41 dustin Exp $
 */

#include <stdio.h>
#include <ndbm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pageserv.h"

struct config conf;

void main(void)
{
    char buf[BUFLEN];
    struct user u;
    datum d;
    DBM *db;

    readconfig(CONFIGFILE);
    if(conf.debug>0)
	showconfig();

    if( (db=dbm_open(conf.userdb, O_RDONLY, 0644)) ==NULL)
    {
        perror(conf.userdb);
        exit(1);
    }

    for(d=dbm_firstkey(db); d.dptr!=NULL; d=dbm_nextkey(db))
    {
        strncpy(buf, d.dptr, d.dsize);
        buf[d.dsize]=0x00;
        u=open_getuser(db, buf);
        printuser(u);
        puts("--------");
    }

    dbm_close(db);
    cleanconfig();
}
