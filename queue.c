/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.9 1997/03/24 18:47:22 dustin Exp $
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

extern struct config conf;

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

int storequeue(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], *fn;
    FILE *qf;

    if(check_time(q.priority, q.to))
    {
        fn=newqfile();

        qf=fopen(fn, "w");
        fprintf(qf, "%d\n%s\n%s\n", q.priority, q.to, q.message);
        fclose(qf);

        sprintf(buf, "Queued to %s, thank you\n", fn);
    }
    else
    {
        strcpy(buf, MESG_BADTIME);
    }

    if(! (flags & STORE_QUIET))
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

            printf("%s:\n\tPriority:  %d\tTo %s   %d bytes\n\n",
                q.qid, q.priority, q.to, strlen(q.message));
        }
    }
    closedir(dir);
}


struct queuent dofarkle()
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    char buf[BUFLEN];
    struct queuent q;

    chdir(QUEDIR);
    dir=opendir(".");

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

            break;
        }
    }
    closedir(dir);
    unlink(q.qid);
    return(q);
}
