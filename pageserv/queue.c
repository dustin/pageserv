/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.19 1997/04/29 17:57:57 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

void cleanmylocks()
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    int i=0, pid;

    pid=getpid();

    chdir(conf.qdir);
    dir=opendir(".");

    while( (d=readdir(dir))!=NULL)
    {
        if(d->d_name[0]=='l')
        {
	    f=fopen(d->d_name, "r");
	    if(f!=NULL)
	    {
		fscanf(f, "%d", &i);
		if(i==pid)
		{
		    if(conf.debug>0)
			printf("Getting rid of %s\n", d->d_name);

		    unlink(d->d_name);
		}
	        fclose(f);
	    }
	}
    }
    closedir(dir);
}

void runqueue(void)
{
    struct queuent *q;
    char **termlist;
    struct terminal term;
    int t, i, s;

    resetdelivertraps();
    termlist=listterms();

    /* four minutes to complete delivery */
    alarm(240);

    for(t=0; termlist[t]!=NULL; t++)
    {
	if(conf.debug>0)
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
		    logqueue(q[i], SUC_LOG, NULL);
		    dequeue(q[i].qid);
		}
		else
		{
		    if(conf.debug>0)
			printf("Delivery of %s unsuccessful\n", q[i].qid);
		    logqueue(q[i], FAIL_LOG, MESG_TAPFAIL);
		}
		if(conf.debug>2)
                    printf("\t%d to %s  ``%s''\n", i, q[i].to, q[i].message);
            }
	    s_tap_end(s);
	    close(s);
	    sleep(1); /* sleep it off */
	}

        cleanqueuelist(q);
    }
    cleantermlist(termlist);
}

void logqueue(struct queuent q, int type, char *reason)
{
    openlog("pageserv", LOG_PID|LOG_NDELAY, LOG_LOCAL7);

    switch(type)
    {
	case QUE_LOG:
            syslog(conf.log_que, "%s got paged %d bytes qid %s", q.to,
	        strlen(q.message), q.qid); break;
	case SUC_LOG:
            syslog(conf.log_que, "delivered %s %d bytes qid %s", q.to,
	        strlen(q.message), q.qid); break;
	case FAIL_LOG:
            syslog(conf.log_que, "failed %s to %s: %s", q.qid,
	        q.to, reason); break;
	case EXP_LOG:
            syslog(conf.log_que, "%s to %s expired, dequeuing", q.qid,
	        q.to ); break;
    }
    closelog();
}

void dequeue(char *qid)
{
    char buf[BUFLEN], *tmp;

    switch(qid[0])
    {
	case 'q':
	    strcpy(buf, conf.qdir);
	    strcat(buf, "/");
	    strcat(buf, qid);
	    break;

	case '/':
	    strcpy(buf, qid);
	    break;

        default:
	    if(conf.debug>0)
		printf("I don't know what the hell to do with %s\n", qid);
	    qid[0]=NULL;
    }

    if(qid[0]!=NULL)
    {
	if(conf.debug>0)
	    printf("Unlinking file: %s\n", buf);
        unlink(buf);

        /* Nasty lock handler */

	tmp=buf+strlen(buf)-1;
	while(*tmp!='q' && *tmp!='/') tmp--;
	if(*tmp=='q')
	{
	    *tmp='l';
	    if(conf.debug>0)
		printf("Unlinking file: %s\n", buf);
	    unlink(buf);
	} /* lock handler */
    } /* all delete handler */
}

int gq_checkit(struct queuent q, char *number)
{
    if( (strcmp(q.u.statid, number))!=0)
    {
	/* Splat means grab 'em all */
	if(strcmp(number, "*") == 0)
	{
	    return(1);
	}
    }

    if( readytodeliver(q) == 0)
    {
	return(0);
    }

    if( q_islocked(q) == 1 )
    {
	return(0);
    }

    return(1);
}

int q_lock(struct queuent q)
{
    char buf[BUFLEN];
    char tmp[FNSIZE];
    FILE *f;

    strcpy(buf, conf.qdir);
    strcat(buf, "/");
    strcpy(tmp, q.qid);
    tmp[0]='l';
    strcat(buf, tmp);

    if(conf.debug>0)
	printf("Locked %s with %s\n", q.qid, buf);

    f=fopen(buf, "w");
    if(f==NULL)
    {
	return(-1);
    }

    fprintf(f, "%d\n", getpid());

    fclose(f);
    return(0);
}

int q_unlock(struct queuent q)
{
    char buf[BUFLEN];
    char tmp[FNSIZE];

    strcpy(buf, conf.qdir);
    strcat(buf, "/");
    strcpy(tmp, q.qid);
    tmp[0]='l';
    strcat(buf, tmp);

    if(conf.debug>0)
	printf("Unlocking %s with %s\n", q.qid, buf);

    return(unlink(buf));
}

int q_islocked(struct queuent q)
{
    char buf[BUFLEN];
    char tmp[FNSIZE];

    strcpy(buf, conf.qdir);
    strcat(buf, "/");
    strcpy(tmp, q.qid);
    tmp[0]='l';
    strcat(buf, tmp);

    return( (access(buf, F_OK)) == 0);
}

void cleanqueuelist(struct queuent *list)
{
    free(list);
}

/* quicksort to sort the queuelist */

int queuecompare(struct queuent q1, struct queuent q2)
{
    /* check priorities */

    if(q1.priority != q2.priority)
    {
	return(q1.priority < q2.priority);
    }

    return(q1.submitted < q2.submitted);
}

void queuesort(struct queuent *q, int l, int r)
{
    int i, j;
    struct queuent v, t;

    if(r>l)
    {
	v=q[r]; i=l-1; j=r;

	do
	{
	    /*
	    do{ i++; } while(q[i].submitted<v.submitted);
	    do{ j--; } while(q[j].submitted>v.submitted);
	    */
	    do{ i++; } while(queuecompare(q[i], v));
	    do{ j--; } while(!queuecompare(q[j], v));
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
    struct dirent *d;
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

		if(strcmp(number, "*")!=0)
		    q_lock(q);

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
	strcat(filename, "/");
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
    strcpy(q.qid, fntoqid(fn));


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
    int fuz, ret=1;
    time_t t;

    time(&t);
    fuz=(t-q.submitted)/60;

    if(conf.debug>2)
        printf("fuz is %d\n", fuz);

    if(fuz>conf.maxqueuetime)
    {
	if(conf.debug>2)
	    printf("Deleting %s, too old...\n", q.qid);

	logqueue(q, EXP_LOG, NULL);
	dequeue(q.qid);
	ret=0;
    }

    return(ret);
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

char *fntoqid(char *fn)
{
    int i;

    for(i=strlen(fn)-1; i>0; i--)
    {
	if(fn[i]=='q')
	    break;
    }

    if(conf.debug>2)
	printf("fntoqid(%s) found %s\n", fn, fn+i);

    return(fn+i);
}

int storequeue(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], *fn;
    int ret=0;
    FILE *qf;

    if(conf.debug>3)
        puts("Running storequeue()");

    if(check_time(q.priority, q.to))
    {
        fn=newqfile();
        time(&q.submitted); /* store the time */

        qf=fopen(fn, "w");
        fprintf(qf, "%d\n%s\n%s\n%d\n", q.priority, q.to, q.message,
	    (int)q.submitted);
        fclose(qf);

	strcpy(q.qid, fntoqid(fn));

        sprintf(buf, "Queued to %s, thank you\n", q.qid);

	if(conf.debug>1)
	    printf("Queued %s for %s\n", fn, q.to);

	if(conf.debug>2)
	    printf("Qid is now %s\n", q.qid);
        logqueue(q, QUE_LOG, NULL);
    }
    else
    {
        strcpy(buf, MESG_BADTIME);

	if(conf.debug>1)
	    printf("Attempted to page %s, bad time\n", q.to);

	ret=1;
    }

    if(! (flags & STORE_QUIET))
        puttext(s, buf);

    return(ret);
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
    struct queuent *q, retq;

    /* This looks like it's more than necessary, but I want to get the
       right order. */

    q=listqueue("*");
    retq=*q;
    cleanqueuelist(q);

    dequeue(retq.qid);
    return(retq);
}
