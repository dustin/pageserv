/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: misc.c,v 2.3 1998/07/11 06:16:04 dustin Exp $
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
#include <signal.h>

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

    while( (d=readdir(dir))!=NULL) {
        if(d->d_name[0]=='l') {
            f=fopen(d->d_name, "r");
            if(f!=NULL) {
                fscanf(f, "%d", &i);
                if(i==pid) {
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
    for(t=0; termlist[t]!=NULL; t++) {
        _ndebug(0, ("Queue for %s:\n", termlist[t]));
        q=listqueue(termlist[t]);

        for(i=0; q[i].to[0] != NULL; i++);

        if(i>0) {
            term=getterm(termlist[t]);

            if( (s=any_openterm(term))>=0) {
                /* Inside this if is executed if I got the port */

                s_tap_init(s, term.flags);

                /* Keep looping through queue stuff until there are no more
                   requests, in case any where queued while we're already
                   delivering.  */
                while(q[0].to[0] != NULL) {

                    for(i=0; q[i].to[0] != NULL; i++) {
                        getqueueinfo(&q[i]);

                        if( (s_tap_send(s, q[i].u.pageid, q[i].message)) == 0){
                            _ndebug(0, ("Delivery of %s successful\n",
                                q[i].qid));
                            logqueue(q[i], SUC_LOG, NULL);
			    dq_notify(q[i], "Successfully delivered",
				      NOTIFY_SUCC);
                            dequeue(q[i].qid);
                            usleep(2600);
                        } else {
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
            } else {
                _ndebug(2, ("Didn't get the port\n"));
            }
        }

        cleanqueuelist(q);

    }
    cleantermlist(termlist);
    cleanmylocks();
}

static RETSIGTYPE del_sigint(int sig)
{
    cleanmylocks();
    exit(1);
}

void resetdelivertraps(void)
{
    _ndebug(0, ("Setting signals...\n"));

    signal(SIGINT, del_sigint);
    signal(SIGQUIT, del_sigint);
    signal(SIGTERM, del_sigint);
    signal(SIGHUP, del_sigint);
    signal(SIGALRM, del_sigint);
}
