/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.1 1997/03/11 06:02:07 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pageserv.h"

char *newtmp(void)
{
    static char tmp[3];

    if(tmp[0]==0)
    {
	strncpy(tmp, "AA" , 2);
    }
    else
    {
        tmp[1]++;

        if(tmp[1]>'Z')
        {
	    tmp[0]++;
	    tmp[1]='A';
        }
    }

    return tmp;
}

char *newqfile(void)
{
   int pid;
   static char fn[FNSIZE];

   pid=getpid();

   sprintf(fn, "%s/q%s%d", QUEDIR, newtmp(), pid);

   while(f_exists(fn))
   {
       sprintf(fn, "%s/q%s%d", QUEDIR, newtmp(), pid);
   }

   printf("Will queue to %s\n", fn);
   return(fn);
}
