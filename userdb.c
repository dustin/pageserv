/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: userdb.c,v 1.1 1997/03/12 06:00:36 dustin Exp $
 */

#include <stdio.h>
#include <ndbm.h>

#include "pageserv.h"

void printuser(struct user u)
{
    printf("ID:        %s\nPager ID:  %s\nStation:   %s\n", u.name,
         u.pageid, u.statid);
    printf("Early:     %d\nLate:      %d\n", u.early, u.late);
}

void storeuser(DBM *db, struct user u)
{
    datum k, d;
    k.dptr=u.name;
    k.dsize=strlen(u.name);

    d.dptr=&u;
    d.dsize=sizeof(u);

    dbm_store(db, k, d, DBM_REPLACE);
}
