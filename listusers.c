/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: listusers.c,v 1.4 1997/03/14 21:33:29 dustin Exp $
 */

#include <stdio.h>
#include <ndbm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pageserv.h"

void main(void)
{
    char buf[BUFLEN];
    struct user u;
    datum d;
    DBM *db;

    if( (db=dbm_open(USERDB, O_RDONLY, 0644)) ==NULL)
    {
        perror(USERDB);
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
}
