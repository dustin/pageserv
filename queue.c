/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.3 1997/03/11 19:36:35 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
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

int storequeue(int s, int priority, char *whom, char *message)
{
    char buf[BUFLEN], *fn;
    FILE *q;

    fn=newqfile();

    q=fopen(fn, "w");

    fprintf(q, "%d\n%s\n%s\n", priority, whom, message);

    fclose(q);

    sprintf(buf, "Queued to %s, thank you\n", fn);
    puttext(s, buf);

    return(0);
}

int queuedepth(void)
{
    DIR *dir;
    struct dirent *d;
    int i=0;

    chdir(QUEDIR);
    dir=opendir(".");

    while( (d=readdir(dir))!=NULL)
    {
	if(d->d_name[0]=='q')
	    i++;
    }
    closedir(dir);
    return(i);
}

void printqueue(void)
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    char buf[BUFLEN];
    struct queuent q;

    chdir(QUEDIR);
    dir=opendir(".");

    printf("There are %d items in the queue.\n\n", queuedepth());

    while( (d=readdir(dir))!=NULL)
    {
	if(d->d_name[0]=='q')
	{
	    f=fopen(d->d_name, "r");

	    fgets(buf, BUFLEN, f);
	    sscanf(buf, "%d", &q.priority);
	    fgets(q.to, TOLEN, f);
	    fgets(q.message, BUFLEN, f);
	    kw(q.to);
	    kw(q.message);
	    strcpy(q.qid, d->d_name);

	    fclose(f);

	    printf("%s:\n\tTo %s   %d bytes\n\n", q.qid, q.to,
		strlen(q.message));
	}
    }
    closedir(dir);
}
