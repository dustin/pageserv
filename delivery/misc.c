/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: misc.c,v 2.8 1998/10/21 00:34:23 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
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

void
cleanmylocks()
{
	DIR    *dir;
	FILE   *f;
	struct dirent *d;
	int     i = 0, pid;

	pid = getpid();

	chdir(conf.qdir);
	dir = opendir(".");

	while ((d = readdir(dir)) != NULL) {
		if (d->d_name[0] == 'l') {
			f = fopen(d->d_name, "r");
			if (f != NULL) {
				fscanf(f, "%d", &i);
				if (i == pid) {
					_ndebug(0, ("Getting rid of %s\n", d->d_name));
					unlink(d->d_name);
				}
				fclose(f);
			}
		}
	}
	closedir(dir);

	checklocks();		/* this is for serial locks */
}

/*
 * This is for logging in the delivery module.  Lets me make changes to
 * the way it works without relinking everything.
 */

void
del_log(char *format,...)
{
	va_list ap;
	char    buf[BUFLEN];

	openlog("pageserv", LOG_PID | LOG_NDELAY, conf.log_que);

	va_start(ap, format);
	vsnprintf(buf, BUFLEN - 1, format, ap);
	va_end(ap);

	syslog(conf.log_que | LOG_INFO, buf);

	closelog();
}

void
runqueue(void)
{
	struct queuent *q;
	char  **termlist;
	struct terminal term;
	int     t, i, s, r;

	conf.udb.dbinit();
	resetdelivertraps();
	termlist = listterms();

	/* two minutes to complete delivery */
	alarm(120);

	for (t = 0; termlist[t] != NULL; t++) {
		_ndebug(0, ("Queue for %s:\n", termlist[t]));
		q = listqueue(termlist[t]);

		for (i = 0; q[i].to[0] != NULL; i++);

		if (i > 0) {
			del_log("starting to process queue %d", i);
			term = getterm(termlist[t]);

			if ((s = any_openterm(term)) >= 0) {
				/* Inside this if is executed if I got the port */

				if (s_tap_init(s, term.flags) < 0) {
					any_closeterm(s, term);
					sleep(5);	/* sleep it off */
					break;
				}
				/* Keep looping through queue stuff until there are no more
				 * requests, in case any where queued while we're already
				 * delivering.  */
				while (q[0].to[0] != NULL) {
					/* too debuggy */
					/* del_log("start of while loop"); */

					for (i = 0; q[i].to[0] != NULL; i++) {
						getqueueinfo(&q[i]);

						/* Too debuggy */
						/* del_log("attempt s_tap_send mess: %s",
						   q[i].message); */
						r = s_tap_send(s, q[i].u.pageid, q[i].message);

						if (r == 0) {
							_ndebug(0, ("Delivery of %s successful\n",
								q[i].qid));
							del_log("delivered %s to %s: %d bytes",
							    q[i].qid, q[i].to, strlen(q[i].message));
							dequeue(q[i].qid);
							usleep(2600);
						} else if (r == 'X' && i == 0) {
							_ndebug(0, ("Delivery of %s probably successful\n",
								q[i].qid));
							del_log("probably delivered %s to %s: %d bytes",
							    q[i].qid, q[i].to, strlen(q[i].message));
							dequeue(q[i].qid);
							usleep(2600);
							/* Must re-connect before sending any more. */
							break;
						} else if (r == C_RS) {
							dq_notify(q[i], "Unsuccessful", NOTIFY_FAIL);
							del_log("failed %s to %s, got RS (fatal)",
							    q[i].qid, q[i].to);
							dequeue(q[i].qid);
						} else {
							del_log("deferred %s to %s, "
							    "will keep trying until it expires",
							    q[i].qid, q[i].to);
							q_unlock(q[i]);
							/* Must re-connect. */
							if (r == 'X') {
								break;
							};
						}
						_ndebug(2, ("\t%d to %s  ``%s''\n", i, q[i].to,
							q[i].message));
					}	/* for loop */

					cleanqueuelist(q);

					/* Must re-connect. */
					if (r == 'X') {
						break;
					};

					q = listqueue(termlist[t]);

				}	/* while loop */

				/* Too debuggy */
				/* del_log("attempting s_tap_end"); */
				s_tap_end(s);
				any_closeterm(s, term);
				sleep(5);	/* sleep it off */
			} else {
				del_log("Failed to get a port");
				_ndebug(2, ("Didn't get the port\n"));
			}
		}
		cleanqueuelist(q);

	}
	cleantermlist(termlist);
	cleanmylocks();
	/* Too debuggy */
	/* del_log("end of runqueue"); */
}

static RETSIGTYPE
del_sigint(int sig)
{
	cleanmylocks();
	del_log("Caught signal %d, exiting...", sig);
	exit(1);
}

void
resetdelivertraps(void)
{
	_ndebug(0, ("Setting signals...\n"));

	signal(SIGINT, del_sigint);
	signal(SIGQUIT, del_sigint);
	signal(SIGTERM, del_sigint);
	signal(SIGHUP, del_sigint);
	signal(SIGALRM, del_sigint);
}
