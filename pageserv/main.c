/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.57 1999/02/25 22:57:43 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <readconfig.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

struct config conf;

static void writepid(int pid)
{
	FILE *f;
	int r;

	r=checkpidfile(conf.pidfile);

	switch(r) {
		case PID_NOFILE:
			break;
		case PID_STALE:
			puts("Stale PID file found, overriding.");
			break;
		case PID_ACTIVE:
			puts("Active PID file found, exiting...");
			kill(pid, SIGTERM);
			exit(1);
	}

	if(NULL ==(f=fopen(conf.pidfile, "w")) ) {
	perror(conf.pidfile);
	return;
	}

	fprintf(f, "%d\n", pid);

	fclose(f);
}

static void detach(void)
{
   int pid, i;
   char *tmp;

   pid=fork();

   if(pid>0) {
	   printf("Running on PID %d\n", pid);
	   writepid(pid);
	   exit(0);
   }

   setsid();

   /* close uneeded file descriptors */

   for(i=0; i<256; i++)
		close(i);

   tmp=rcfg_lookup(conf.cf, "etc.working_directory");
   if(tmp==NULL)
	   tmp="/";

   chdir(tmp);
   umask(7);
}

static int deliveryd_checkmom(int oldpid)
{
	int pid, i;
	FILE *f;

	for(i=0; i<3; i++) {
		f=fopen(conf.pidfile, "r");
		if(f!=NULL)
		break;
	sleep(2);
	}

	if(f==NULL)
	return(-1);

	if(fscanf(f, "%d", &pid)<1)
	pid=-1;

	fclose(f);

	if(oldpid!=0) {
	if(pid!=oldpid)
		pid=-1;
	}

	return(pid);
}

/*
 * This is a hack.
 */
static int deliveryd_main(void)
{
	int t, stat, ppid, pid;

	/*
	 * We return here because we're still the main server daemon until
	 * we fork().
	 */
	if( (pid=fork())>0)
	return(pid);

	for(t=0; t<255; t++)
	close(t);

	page_log("deliveryd started");

	/*
	 * Defaultify everything, keep the thing from deleting its pid file.
	 */
	resethappytraps();

	t=rcfg_lookupInt(conf.cf, "etc.deliverysleep");

	/*
	 * Racy condition, pid file went away before we got here, we have to
	 * know the pid so we can exit, so if it's not there, we go away.
	 */
	ppid=deliveryd_checkmom(0);
	if(ppid<0) {
	   page_log("deliveryd exiting, didn't get initial pid");

	   /*
	* Let's sleep for another 60 seconds, to keep the thing from
		* looping too fast
	*/
	   sleep(60);
	   exit(0);
	}

	if(t<1)
		t=DEFAULT_DELSLEEP;

	for(;;) {
	sleep(t);
	if(readyqueue()>0) {
		if( (pid=fork())==0) {
			runqueue_main();
		exit(0);
		} else {
				page_log("delivering pages on pid %d", pid);
			wait(&stat);
		}
	}

	/* See if it's ``our time'' */
		if(deliveryd_checkmom(ppid)<0) {
			 page_log("deliveryd exiting, parent isn't %d", ppid);
		exit(0);
	}
	}
}

static void daemon_main(void)
{
	struct sockaddr_in fsin;
	int i, s, fromlen, pid, upper=0, deliveryd;
	fd_set fdset, tfdset;
	struct timeval t;
	module *m;
	modpass p;

	if(conf.debug==0)
		detach();
	else
	writepid(getpid());

	/* *NOW* let's initialize the databases, shall we? */
	conf.udb.dbinit();
	conf.tdb.dbinit();

	if(rcfg_lookupInt(conf.cf, "etc.deliveryd")==0)
		deliveryd=-1;
	else
		deliveryd=0;

	resetservtraps(); /* set signal traps */

	FD_ZERO(&tfdset);

	m=conf.modules;
	for(i=0; i<conf.nmodules; i++) {
		_ndebug(2, ("Loading module ``%s''\n", m->name));

		m->init();

		s=m->socket();
		if(s>=0) {
			_ndebug(2, ("Module is listening, fdsetting %d\n", s));

		page_log("%s is listening on %d", m->name, m->socket());

		if(s >upper)
			upper=s;

			FD_SET(s, &tfdset);
		}

		m++;
	}

	page_log("Pageserv %s initialized on pid %d (deliveryd=%d)",
		 VERSION, getpid(), deliveryd+1);

	upper++;  /* one more, just because */

	for(;;) {
		fdset=tfdset;
		t.tv_sec=conf.childlifetime;
		t.tv_usec=0;
		fromlen=sizeof(fsin);

		if( select(upper, &fdset, NULL, NULL, &t) > 0) {
			m=conf.modules;
			for(i=0; i<conf.nmodules; i++) {
				if( (m->socket()>0) && (FD_ISSET(m->socket(), &fdset)) ) {
					_ndebug(2, ("Got a connection for ``%s''\n", m->name));

					if( (p.socket=accept(m->socket(),
						(struct sockaddr *)&fsin, &fromlen)) >=0 ) {
						logConnect(fsin, m);

						if(checkIPAccess(fsin, m) == 0) {
						_ndebug(2, ("Hanging up on connection because "
						"of ACL\n"));
						pid=1;
					} else {
						pid=fork();
					}

					if(pid==0) {
						p.fsin=fsin;
						m->handler(p);
					} else {
						close(p.socket);

						/* This used to be if(conf.debug>2 && pid>1) */
						_ndebug(2, ("Spawned a child, pid %d\n", pid));
					}
				}
			}
			m++;  /* Look at the next module */
		} /* end of module scan */

		} /* end of select loop */
		reaper();

	/* Check deliveryd */
	if(deliveryd>=0) {
		if(deliveryd==0) {
		deliveryd=deliveryd_main();
		} else {
			if(kill(deliveryd, 0)!=0) {
			deliveryd=deliveryd_main();
			}
		}
	}
	}
}

static char *getcachepw(char *name, char **names, char **passwords)
{
	int i;
	char *p=NULL;

	for(i=0; names[i]; i++) {
	if(strcmp(name, names[i])==0) {
		p=passwords[i];
		break;
	}
	}
	return(p);
}

static void rehash_main(void)
{
	int i;
	char **names, **names2, **passwords=NULL, *tmp;
	struct user u;

	conf.udb.dbinit();
	conf.tdb.dbinit();

	names=conf.udb.listusers("*");
	if(names) {
		for(i=0; names[i]; i++);

		passwords=(char **)malloc(i*sizeof(char *));
		for(i=0; names[i]; i++) {
		u=conf.udb.getuser(names[i]);
		passwords[i]=strdup(u.passwd);
		}
	}

	conf.udb.eraseuserdb();
	conf.tdb.erase();

	i=conf.udb.parseusers();
	printf("Parsed %d users.\n", i);

	if(names) {
		names2=conf.udb.listusers("*");

		for(i=0; names2[i]; i++) {
		tmp=getcachepw(names2[i], names, passwords);
		if(tmp) {
			u=conf.udb.getuser(names2[i]);
			strcpy(u.passwd, tmp);
			conf.udb.storeuser(u);
		}
		}
	}

	i=parseterms();
	printf("Parsed %d terminal servers.\n", i);

	if(names) {
		for(i=0; names[i]; i++)
			free(passwords[i]);
		free(passwords);
		cleanuserlist(names);
	}
}

static void killserver(void)
{
	int pid, r;
	FILE *f;

	r=checkpidfile(conf.pidfile);

	if(r==PID_NOFILE) {
	puts("Error, no PID file found, is it running?");
	exit(1);
	}

	f=fopen(conf.pidfile, "r");
	if(f==NULL) {
	perror(conf.pidfile);
	exit(1);
	}

	if( fscanf(f, "%d", &pid) == EOF) {
	puts("Error:  No PID found in pidfile");
	exit(1);
	}

	kill(pid, SIGTERM);
	sleep(1);

	r=checkpidfile(conf.pidfile);

	switch(r) {
	case PID_NOFILE:
		puts("Successfully shut down.");
		break;
	case PID_STALE:
		if(unlink(conf.pidfile)<0) {
		puts("Server shut down and left its pid file, I could not "
			 "remove it.");
		perror(conf.pidfile);
		} else {
		puts("Server shut down and left its pid file, I removed it");
		}
		break;
	case PID_ACTIVE:
		puts("Server could not be shut down");
		break;
	}

	fclose(f);
	exit(0);
}

static void ldb_main(void)
{
	conf.udb.dbinit();
	conf.tdb.dbinit();
	puts("Users:\n------------");
	printusers();
	puts("\nTerminals:\n------------");
	printterms();
}

static void changepasswd(void)
{
	char buf[BUFLEN];
	struct user u;

	conf.udb.dbinit();
	fputs("User's password to change:  ", stdout);
	fflush(stdout);
	fgets(buf, BUFLEN, stdin);
	kw(buf);

	if(!conf.udb.u_exists(buf))
	{
	puts("No such user.");
	exit(1);
	}

	u=conf.udb.getuser(buf);

	fputs("User's new password:  ", stdout);
	fflush(stdout);
	p_getpasswd(1, buf);
	putchar('\n');

	u=setpasswd(u, buf);
	conf.udb.storeuser(u);

	puts("Password set.");
}

void runqueue_main(void)
{
#if (DYNLIBS)
	void *lib;
	void (*runqueue)(void);
	char *libname;

	libname=rcfg_lookup(conf.cf, "libs.delivery");

	if(libname==NULL)
	{
	_ndebug(2, ("libs.delivery isn't in the config file.\n"));
	page_log("libs.delivery isn't in the config file");
	return;
	}

	lib=dlopen(libname, RTLD_LAZY);
	if(lib==NULL)
	{
		_ndebug(2, ("dlopen: %s\n", dlerror()));
	page_log("dlopen: %s\n", dlerror());
	return;
	}

	runqueue=dlsym(lib, "runqueue");
	if(runqueue==NULL)
	{
	_ndebug(2, ("dlsym: %s\n", dlerror()));
	page_log("dlsym: %s\n", dlerror());
	return;
	}
#endif

	runqueue();

#if (DYNLIBS)
	if(lib)
		dlclose(lib);
#endif
}

int main(int argc, char **argv)
{

	rdconfig(CONFIGFILE);
	getoptions(argc, argv);
	if(conf.debug>0)
		showconfig();
	switch(conf.mode)
	{
	case MODE_DAEMON:
		daemon_main(); break;

	case MODE_REHASH:
		rehash_main(); break;

	case MODE_LDB:
		ldb_main(); break;

	case MODE_PQ:
			printqueue(); break;

	case MODE_VERS:
		showversion(); break;

		case MODE_RUNQ:
		runqueue_main(); break;

		case MODE_KILL:
		killserver(); break;

		case MODE_PWCH:
		changepasswd(); break;

		case MODE_DUMPUSERS:
			dumpuserdb(); break;
	}
	cleanconfig();
	return(0);
}
