/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: queue.c,v 1.50 1998/03/18 08:33:30 dustin Exp $
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
#include <ctype.h>
#include <assert.h>

#include <pageserv.h>
#include <nettools.h>
#include <tap.h>
#include <readconfig.h>

extern struct config conf;

int sendmail(char *to, char **cc, char *message)
{
    FILE *f;
    char *sm, *from;

    sm=rcfg_lookup(conf.cf, "etc.sendmail");
    if(sm==NULL)
    {
	_ndebug(2, ("etc.sendmail not configured\n"));
	return(-1);
    }

    from=rcfg_lookup(conf.cf, "etc.mailfrom");
    if(from==NULL)
    {
	_ndebug(2, ("etc.mailfrom not configured\n"));
	return(-1);
    }

    f=popen(sm, "w");
    if(f==NULL)
    {
	_ndebug(2, ("Error executing ``%s''\n", sm));
	return(-1);
    }

    fprintf(f, "From: %s\n", from);
    fprintf(f, "To: %s\n", to);
    fprintf(f, "Subject: Pageserv message\n\n");

    fprintf(f, message);

    pclose(f);
}

void dq_notify(struct queuent q, char *message, int flags)
{
    struct user u;
    char *buf=NULL;
    int size=0;

    _ndebug(4, ("Queue:  %s\n", q.qid));

    u=conf.udb.getuser(q.to);
    if(!(u.flags&flags))
    {
	_ndebug(4, ("User ``%s'' didn't have notify for %d (flags=%d)\n",
	      q.to, flags, u.flags));
	return;
    }

    buf=addtostr(&size, buf, "Message from pageserv:\n\n\t");
    buf=addtostr(&size, buf, message);
    buf=addtostr(&size, buf, ":\n\nMessage:\n\n\t");
    buf=addtostr(&size, buf, q.message);

    _ndebug(2, ("Going to sendmail:%s\n", buf));

    sendmail(u.notify, NULL, buf);

    free(buf);
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
        case DEQUE_LOG:
            syslog(conf.log_que|LOG_INFO,
                   "dequeued %s to %s, %s", q.qid, q.to, reason);
            break;
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
    /* make sure we got a valid queue file */
    if(q.to[0]==NULL)
	return(0);

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
	    _ndebug(2, ("Ack, stack smash on ``%s'' + ``%s''\n",fn,filename));
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

    #define readQFline(a, b) \
	fgets(a, b, f); \
	if(a==NULL || strlen(a)==0) \
	{ \
	    q.to[0]=0x00; \
	    fclose(f); \
	    _ndebug(3, ("Removing invalid queue file: %s\n", filename)); \
	    unlink(filename); \
	    return(q); \
        }

    /* priority */
    readQFline(buf, BUFLEN);
    sscanf(buf, "%d", &q.priority);

    /* Whom it's to */
    readQFline(q.to, TOLEN);
    kw(q.to);

    /* The date it was submitted */
    readQFline(buf, BUFLEN);
    q.submitted=atoi(buf);

    /* The qid */
    strcpy(q.qid, fntoqid(fn));

    /* Soonest it can be delivered */
    readQFline(buf, BUFLEN);
    q.soonest=atoi(buf);

    /* Latest it can be delivered */
    readQFline(buf, BUFLEN);
    q.latest=atoi(buf);

    /* The submitter's IP address */
    readQFline(buf, BUFLEN);
    q.rem_addr=atoi(buf);

    /* The actual message data */
    readQFline(q.message, BUFLEN);
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
	dq_notify(q, "Too old", NOTIFY_FAIL);
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
    /* make sure we got a valid queue file */
    if(q->to[0]==NULL)
	return;

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

int _splitsq(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], current[BUFLEN];
    int ret=0, maxlen, p, done=0;

    maxlen=rcfg_lookupInt(conf.cf, "tuning.maxMessageLength");
    if(maxlen==0)
	maxlen=250;

    assert(maxlen<BUFLEN-1);

    _ndebug(3, ("Beginning a truncation loop for %s\n", q.message));

    strcpy(current, q.message);

    do {
	strncpy(buf, current, maxlen);
	p=strlen(buf);
	while(--p>0 && !isspace(buf[p]));
	if(p==0)
	    p=maxlen;

	buf[p]=0x00;

	strcpy(q.message, buf);

        _ndebug(3, ("Trying to queue ``%s''\n", q.message));

	ret+=storequeue(s, q, flags);
	if(ret!=0)
	    return(ret);

	if(current[strlen(q.message)+1]!=NULL)
	{
	    strncpy(buf, &current[strlen(q.message)+1], maxlen);
	    strcpy(current, buf);
	}
	else
	{
	    done=1;
	}
    } while(!done);
    return(ret);
}

int storequeue(int s, struct queuent q, int flags)
{
    char buf[BUFLEN], *fn;
    int ret=0, maxlen;
    FILE *qf;

    _ndebug(3, ("Running storequeue()\n"));

    maxlen=rcfg_lookupInt(conf.cf, "tuning.maxMessageLength");
    if(maxlen==0)
	maxlen=250;

     _ndebug(4, ("Max message length is set to %d\n", maxlen));

    if(strlen(q.message)>maxlen)
    {
	if(rcfg_lookupInt(conf.cf, "tuning.wrapMessages")==1)
	{
            _ndebug(4, ("Doing wrapping.\n"));
	    return(_splitsq(s, q, flags));
	}
	else
	{
            _ndebug(4, ("Doing trunctation.\n"));
	    q.message[maxlen-1]=0x00;
	}
    }

    if(check_time(q))
    {
        fn=newqfile();
        time(&q.submitted); /* store the time */

        qf=fopen(fn, "w");
	if(qf==NULL)
	{
	    _ndebug(1, ("Can't open queue file\n"));
	    return(1);
	}
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
