/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.8 1997/04/01 20:16:09 dustin Exp $
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

void runqueue(void)
{
    struct queuent *q;
    char **termlist;
    struct terminal term;
    int t, i, s;

    termlist=listterms();

    for(t=0; termlist[t]!=NULL; t++)
    {
	printf("Queue for %s:\n", termlist[t]);
        q=listqueue(termlist[t]);

        for(i=0; q[i].to[0] != NULL; i++);

	if(i>0)
	{
	    term=getterm(termlist[t]);

            s=s_openterm(term);
	    s_tap_init(s);

            for(i=0; q[i].to[0] != NULL; i++)
	    {
		getqueueinfo(&q[i]);
		if( (s_tap_send(s, q[i].u.pageid, q[i].message)) == 0)
		{
		    if(conf.debug>0)
			printf("Delivery of %s successful\n", q[i].qid);
		    unlink(q[i].qid);
		}
		else
		{
		    if(conf.debug>0)
			printf("Delivery of %s unsuccessful\n", q[i].qid);
		}
                printf("\t%d to %s  ``%s''\n", i, q[i].to, q[i].message);
            }
	    s_tap_end(s);
	    sleep(1); /* sleep it off */
	}

        cleanqueuelist(q);
    }
    cleantermlist(termlist);
}

int gq_checkit(struct queuent q, char *number)
{
    if( (strcmp(q.u.statid, number))!=0)
    {
	/* Splat means grab 'em all */
	if(strcmp(number, "*") != 0)
	{
            return(0);
	}
    }

    if( readytodeliver(q) == 0)
    {
	return(0);
    }

    return(1);
}

void cleanqueuelist(struct queuent *list)
{
    free(list);
}

/* quicksort to sort the queuelist */

void queuesort(struct queuent *q, int l, int r)
{
    int i, j;
    struct queuent v, t;

    if(r>l)
    {
	v=q[r]; i=l-1; j=r;

	do
	{
	    do{ i++; } while(q[i].submitted<v.submitted);
	    do{ j--; } while(q[j].submitted>v.submitted);
	    t=q[i]; q[i]=q[j]; q[j]=t;
	} while(j>i);

	q[j]=q[i]; q[i]=q[r]; q[r]=t;

	queuesort(q, l, i-1);
	queuesort(q, i+1, r);
    }
}

struct queuent *listqueue(char *number)
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    char buf[BUFLEN];
    struct queuent q;
    int index=0, size=4;
    struct queuent *list;

    chdir(conf.qdir);
    dir=opendir(".");

    list=(void *)malloc(size*sizeof(struct queuent));

    while( (d=readdir(dir))!=NULL)
    {
        if(d->d_name[0]=='q')
        {
            q=readqueuefile(d->d_name);

	    getqueueinfo(&q);

            if(gq_checkit(q, number))
	    {
		if(index == size-1)
		{
		    size<<=1;

		    if(conf.debug>2)
		    {
		        printf("Reallocating, now need %d bytes for %d\n",
			    size*sizeof(struct queuent *), size);
		    }

		    list=realloc(list, size*sizeof(struct queuent));
		}
		list[index++]=q;
	    }
        }
    }
    closedir(dir);
    list[index].to[0]=NULL;

    /* sort the list */
    if(index>0)
        queuesort(list, 0, index-1);

    return(list);
}

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
    int i, j;

    q=listqueue("*");

    for(j=0; q[j].to[0] != 0x00; j++);

    printf("There are %d items in the queue\n\n", j);

    for(i=0; i<j; i++)
	displayq(q[i]);

    cleanqueuelist(q);
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
            q=readqueuefile(d->d_name);

            break;
        }
    }
    closedir(dir);
    unlink(q.qid);
    return(q);
}
