/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.2 1997/03/11 06:48:15 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

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

   return(fn);
}

int storequeue(int s, char *whom, char *message)
{
    char buf[BUFLEN], *fn;
    FILE *q;

    fn=newqfile();

    q=fopen(fn, "w");

    fprintf(q, "%s\n%s\n", whom, message);

    fclose(q);

    sprintf(buf, "Queued to %s, thank you\n", fn);
    puttext(s, buf);

    return(0);
}
