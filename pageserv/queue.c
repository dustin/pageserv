/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.3 1997/03/31 23:12:06 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pageserv.h>

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

/* This is a dummy until I decide to do something with it. */

int readytodeliver(struct queuent q)
{
    return(1);
}

void getqueueinfo( struct queuent *q )
{
    q->u=getuser(q->to);
}

char *newqfile(void)
{
   int pid;
   static char fn[FNSIZE];

   pid=getpid();

   sprintf(fn, "%s/q%s%d", conf.qdir, newtmp(), pid);

   while(f_exists(fn))
   {
       sprintf(fn, "%s/q%s%d", conf.qdir, newtmp(), pid);
   }

   return(fn);
}

int storequeue(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], *fn;
    FILE *qf;

    if(conf.debug>3)
        puts("Running storequeue()");

    if(check_time(q.priority, q.to))
    {
        fn=newqfile();

        qf=fopen(fn, "w");
        fprintf(qf, "%d\n%s\n%s\n", q.priority, q.to, q.message);
        fclose(qf);

        sprintf(buf, "Queued to %s, thank you\n", fn);

	if(conf.debug>1)
	    printf("Queued %s for %s\n", fn, q.to);
    }
    else
    {
        strcpy(buf, MESG_BADTIME);

	if(conf.debug>1)
	    printf("Attempted to page %s, bad time\n", q.to);
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

    chdir(conf.qdir);
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

    chdir(conf.qdir);
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

    chdir(conf.qdir);
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
