/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.4 1997/04/01 02:22:21 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pageserv.h>

extern struct config conf;

struct queuent readqueuefile(char *fn)
{
    char filename[FNSIZE];
    char buf[BUFLEN];
    FILE *f;
    struct queuent q;

    if(fn[0]!='/')
    {
	strcpy(filename, fn);
    }
    else
    {
	strcpy(filename, conf.qdir);
	strcat(filename, fn);
    }

    if(NULL== (f=fopen(filename, "r")))
    {
	perror(filename);
	exit(1);
    }

    fgets(buf, BUFLEN, f);
    sscanf(buf, "%d", &q.priority);
    fgets(q.to, TOLEN, f);
    kw(q.to);
    fgets(q.message, BUFLEN, f);
    kw(q.message);

    fgets(buf, BUFLEN, f);
    q.submitted=atoi(buf);
    strcpy(q.qid, fn);


    fclose(f);
    return(q);
}

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
        time(&q.submitted); /* store the time */

        qf=fopen(fn, "w");
        fprintf(qf, "%d\n%s\n%s\n%d\n", q.priority, q.to, q.message,
	    q.submitted);
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

void displayq(struct queuent q)
{
    printf("%s:\t%s\t\tPriority:  %d\tTo %s   %d bytes\n\n",
        q.qid, ctime(&q.submitted), q.priority, q.to,
        strlen(q.message));
}

void printqueue(void)
{
    struct queuent *q;
    int i;

    q=listqueue("*");

    for(i=0; q[i].to[0] != 0x00; i++)
    {
	displayq(q[i]);
    }
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
