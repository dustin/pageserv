/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.41 1998/01/20 05:57:52 dustin Exp $
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
#include <nettools.h>
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
		    _ndebug(0, ("Getting rid of %s\n", d->d_name));
                    unlink(d->d_name);
                }
                fclose(f);
            }
        }
    }
    closedir(dir);

    checklocks(); /* this is for serial locks */
}

void runqueue(void)
{
    struct queuent *q;
    char **termlist;
    struct terminal term;
    int t, i, s;

    conf.udb.dbinit();
    resetdelivertraps();
    termlist=listterms();

    /* four minutes to complete delivery */
    alarm(240);

    for(t=0; termlist[t]!=NULL; t++)
    {
	_ndebug(0, ("Queue for %s:\n", termlist[t]));
        q=listqueue(termlist[t]);

        for(i=0; q[i].to[0] != NULL; i++);

        if(i>0)
        {
            term=getterm(termlist[t]);

            if( (s=any_openterm(term))>=0)
            {
                /* Inside this if is executed if I got the port */

                s_tap_init(s, term.flags);

                /* Keep looping through queue stuff until there are no more
                   requests, in case any where queued while we're already
                   delivering.  */

                while(q[0].to[0] != NULL)
                {

                    for(i=0; q[i].to[0] != NULL; i++)
                    {
                        getqueueinfo(&q[i]);

                        if( (s_tap_send(s, q[i].u.pageid, q[i].message)) == 0)
                        {
			    _ndebug(0, ("Delivery of %s successful\n",
			        q[i].qid));
                            logqueue(q[i], SUC_LOG, NULL);
                            dequeue(q[i].qid);
                            usleep(2600);
                        }
                        else
                        {
			    _ndebug(0, ("Delivery of %s unsuccessful\n",
			        q[i].qid));
                            logqueue(q[i], FAIL_LOG, MESG_TAPFAIL);
                            q_unlock(q[i]);
                        }
			_ndebug(2, ("\t%d to %s  ``%s''\n", i, q[i].to,
			    q[i].message));
                    } /* for loop */

                    cleanqueuelist(q);
                    q=listqueue(termlist[t]);

                } /* while loop */

                s_tap_end(s);
                puttext(s, "+++atz\n");
                any_closeterm(s, term);
                sleep(5); /* sleep it off */
            } /* got port */
            else
            {
		_ndebug(2, ("Didn't get the port\n"));
            }
        }

        cleanqueuelist(q);

    }
    cleantermlist(termlist);
    cleanmylocks();
}

void logqueue(struct queuent q, int type, char *reason)
{
    openlog("pageserv", LOG_PID|LOG_NDELAY, conf.log_que);

    switch(type)
    {
        case QUE_LOG:
            syslog(conf.log_que|LOG_INFO, "queued %s to %s:  %d bytes",
                q.qid, q.to, strlen(q.message)); break;
        case SUC_LOG:
            syslog(conf.log_que|LOG_INFO, "delivered %s to %s: %d bytes",
                q.qid, q.to, strlen(q.message)); break;
        case FAIL_LOG:
            syslog(conf.log_que|LOG_NOTICE, "failed %s to %s: %s", q.qid,
                q.to, reason); break;
        case EXP_LOG:
            syslog(conf.log_que|LOG_NOTICE, "expired %s to %s:  dequeuing",
	        q.qid, q.to ); break;
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
	    _ndebug(0, ("I don't know what the hell to do with %s\n", qid));
            qid[0]=NULL;
    }

    if(qid[0]!=NULL)
    {
	_ndebug(0, ("Unlinking file: %s\n", buf));
        unlink(buf);

        /* Nasty lock handler */

        tmp=buf+strlen(buf)-1;
        while(*tmp!='q' && *tmp!='/') tmp--;
        if(*tmp=='q')
        {
            *tmp='l';
	    _ndebug(0, ("Unlinking file: %s\n", buf));
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
        else
        {
            return(0);
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

    _ndebug(0, ("Locked %s with %s\n", q.qid, buf));

    f=fopen(buf, "w");
    if(f==NULL)
    {
        return(-1);
    }

    fprintf(f, "%d\n", (int)getpid());

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

    _ndebug(0, ("Unlocking %s with %s\n", q.qid, buf));

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

		    _ndebug(2, ("Reallocating, now need %d bytes for %d\n",
			size*sizeof(struct queuent *), size));

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

    q.to[0]=0x00;

    if(fn[0]=='/')
    {
	if(strlen(fn)>=(size_t)FNSIZE)
	{
	    _ndebug(2, ("Ack, stack smash on ``%s''\n", fn));
	    return(q);
	}
        strcpy(filename, fn);
    }
    else
    {
	if(strlen(conf.qdir)>=(size_t)FNSIZE+5)
	{
	    _ndebug(2, ("Ack, stack smash on ``%s''\n", fn));
	    return(q);
	}
        strcpy(filename, conf.qdir);
        strcat(filename, "/");
	if(strlen(fn)+strlen(filename)>=(size_t)FNSIZE)
	{
	    _ndebug(2, ("Ack, stack smash on ``%s'' + ``%s''\n", fn));
	    return(q);
	}
        strcat(filename, fn);
    }

    _ndebug(4, ("reading in queue for ``%s''\n", filename));

    if(NULL== (f=fopen(filename, "r")))
    {
        perror(filename);
	return(q);
    }

    /* priority */
    fgets(buf, BUFLEN, f);
    sscanf(buf, "%d", &q.priority);

    /* Whom it's to */
    fgets(q.to, TOLEN, f);
    kw(q.to);

    /* The date it was submitted */
    fgets(buf, BUFLEN, f);
    q.submitted=atoi(buf);

    /* The qid */
    strcpy(q.qid, fntoqid(fn));

    /* Soonest it can be delivered */
    fgets(buf, BUFLEN, f);
    q.soonest=atoi(buf);

    /* Latest it can be delivered */
    fgets(buf, BUFLEN, f);
    q.latest=atoi(buf);

    /* The submitter's IP address */
    fgets(buf, BUFLEN, f);
    q.rem_addr=atoi(buf);

    /* The actual message data */
    fgets(q.message, BUFLEN, f);
    kw(q.message);

    fclose(f);
    return(q);
}

char *newtmp(void)
{
    static char *tmp;
    static int size=0;
    int i;

    if(size==0)
    {
        size=4;
        tmp=(char *)malloc(sizeof(char *)*size);
        tmp[0]='A';
        tmp[1]=0x00;
    }
    else
    {
        tmp[0]++;
        for(i=0; tmp[i]; i++)
        {
            if(tmp[i]>'Z')
            {
                if(i==size-1)
                {
                    size<<=1;
                    _ndebug(4, ("newtmp() Reallocating to %d bytes\n", size));
                    tmp=(char *)realloc(tmp, size);
                }
                tmp[i]='A';
                if(tmp[i+1])
                {
                    tmp[i+1]++;
                }
                else
                {
                    tmp[i+1]='A';
                    tmp[i+2]=0x00;
                }
            }
        }
    }
    return(tmp);
}

int readytodeliver(struct queuent q)
{
    int fuz, ret=1;
    time_t t, lt;

    if(q.submitted>q.soonest)
    {
        lt=q.submitted;
    }
    else
    {
        lt=q.soonest;
    }

    time(&t);
    fuz=(t-lt)/60;

    _ndebug(2, ("fuz is %d\n", fuz));

    if(fuz>conf.maxqueuetime)
    {
	_ndebug(2, ("Deleting %s, too old...\n", q.qid));

        logqueue(q, EXP_LOG, NULL);
        dequeue(q.qid);
        ret=0;
    }

    if(fuz<0)
    {
        ret=0;
    }

    return(ret);
}

void getqueueinfo( struct queuent *q )
{
    q->u=conf.udb.getuser(q->to);
}

char *newqfile(void)
{
   int pid;
   static char fn[FNSIZE];

   pid=getpid();

   sprintf(fn, "%s/q%d%s", conf.qdir, pid, newtmp());

   while(f_exists(fn))
   {
       sprintf(fn, "%s/q%d%s", conf.qdir, pid, newtmp());
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

    _ndebug(2, ("fntoqid(%s) found %s\n", fn, fn+i));

    return(fn+i);
}

int storequeue(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], *fn;
    int ret=0;
    FILE *qf;

    _ndebug(3, ("Running storequeue()\n"));

    if(check_time(q))
    {
        fn=newqfile();
        time(&q.submitted); /* store the time */

        qf=fopen(fn, "w");
        fprintf(qf, "%d\n%s\n%d\n%d\n%d\n%u\n%s\n",
		q.priority,
		q.to,
                (int)q.submitted,
		(int)q.soonest,
		(int)q.latest,
		q.rem_addr,
		q.message);
        fclose(qf);

        strcpy(q.qid, fntoqid(fn));

        sprintf(buf, "Queued to %s, thank you\n", q.qid);

	_ndebug(1, ("Queued %s for %s\n", fn, q.to));
	_ndebug(2, ("Qid is now %s\n", q.qid));

        logqueue(q, QUE_LOG, NULL);
    }
    else
    {
        strcpy(buf, MESG_BADTIME);

	_ndebug(1, ("Attempted to page %s, bad time\n", q.to));

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

int readyqueue(void)
{
    struct queuent *q;
    int i, j=0;

    conf.udb.dbinit();
    q=listqueue("*");

    for(i=0; q[i].to[0] != 0x00; i++)
	if(readytodeliver(q[i]))
	    j++;

    cleanqueuelist(q);
    return(j);
}

void displayq(struct queuent q)
{
    struct tm *t;

    printf("%s:\t%s\t\tPriority:  %d\tTo %s   %d bytes\n",
        q.qid, ctime(&q.submitted), q.priority, q.to,
        strlen(q.message));

    printf("\t\tFrom:  %s (%s)\n", getHostName(htonl(q.rem_addr)),
	   nmc_intToDQ(q.rem_addr));

    if(q.soonest>0)
    {
        t=localtime(&q.soonest);
        printf("\t\tScheduled no sooner than %s", asctime(t));
    }

    puts("");
}

void printqueue(void)
{
    struct queuent *q;
    int i, j;

    conf.udb.dbinit();
    q=listqueue("*");

    for(j=0; q[j].to[0] != 0x00; j++);

    if(j==1)
        printf("There is 1 item in the queue, %d ready to deliver\n\n",
		readyqueue());
    else
        printf("There are %d items in the queue, %d ready to deliver\n\n",
	       j, readyqueue());

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
