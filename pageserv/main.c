/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: main.c,v 1.47 1998/01/25 11:12:18 dustin Exp $
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
#include <syslog.h>
#include <fcntl.h>
#include <sys/wait.h>

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

    switch(r)
    {
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

    if(NULL ==(f=fopen(conf.pidfile, "w")) )
    {
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

   if(pid>0)
   {
       printf("Running on PID %d\n", pid);
       writepid(pid);
       exit(0);
   }

   setsid();

   /* close uneeded file descriptors */

   for(i=0; i<256; i++)
   {
        close(i);
   }

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

    for(i=0; i<3; i++)
    {
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

    if(oldpid!=0)
    {
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

    openlog("pageserv", LOG_PID|LOG_NDELAY, conf.log_que);
    syslog(conf.log_que|LOG_INFO, "deliveryd started");

    resetdelivertraps();

    t=rcfg_lookupInt(conf.cf, "etc.deliverysleep");

    /*
     * Racy condition, pid file went away before we got here, we have to
     * know the pid so we can exit, so if it's not there, we go away.
     */
    ppid=deliveryd_checkmom(0);
    if(ppid<0)
    {
       syslog(conf.log_que|LOG_NOTICE,
	      "deliveryd exiting, didn't get initial pid");
       closelog();

       /*
	* Let's sleep for another 60 seconds, to keep the thing from
        * looping too fast
	*/
       sleep(60);
       exit(0);
    }

    if(t<1)
        t=DEFAULT_DELSLEEP;

    for(;;)
    {
	sleep(t);
	if(readyqueue()>0)
	{
	    if( (pid=fork())==0)
	    {
		closelog();
	        runqueue();
		exit(0);
	    }
	    else
	    {
                syslog(conf.log_que|LOG_INFO, "delivering pages on pid %d",
		       pid);
	        wait(&stat);
	    }
	}

	/* See if it's ``our time'' */
        if(deliveryd_checkmom(ppid)<0)
	{
             syslog(conf.log_que|LOG_NOTICE,
	            "deliveryd exiting, parent isn't %d", ppid);
	    closelog();
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

    /* *NOW* let's initialize the database, shall we? */
    conf.udb.dbinit();

    if(rcfg_lookupInt(conf.cf, "etc.deliveryd")==0)
    {
	deliveryd=-1;
    }
    else
    {
	deliveryd=0;
    }

    resetservtraps(); /* set signal traps */

    FD_ZERO(&tfdset);

    m=conf.modules;
    for(i=0; i<conf.nmodules; i++)
    {
        _ndebug(2, ("Loading module ``%s''\n", m->name));

	m->init();

	s=m->socket();
        if(s>=0)
	{
            _ndebug(2, ("Module is listening, fdsetting %d\n", s));

	    if(s >upper)
	        upper=s;

            FD_SET(s, &tfdset);
        }

	m++;
    }

    upper++;  /* one more, just because */

    for(;;)
    {
        fdset=tfdset;
        t.tv_sec=conf.childlifetime;
        t.tv_usec=0;
        fromlen=sizeof(fsin);

        if( select(upper, &fdset, NULL, NULL, &t) > 0)
        {
	    m=conf.modules;
	    for(i=0; i<conf.nmodules; i++)
	    {
		if(FD_ISSET(m->socket(), &fdset))
		{
                    _ndebug(2, ("Got a connection for ``%s''\n", m->name));

		    if( (p.socket=accept(m->socket(),
			(struct sockaddr *)&fsin, &fromlen)) >=0 )
		    {
			logConnect(fsin, m);

			if(checkIPAccess(fsin, m) == 0)
			{
			    _ndebug(2, ("Hanging up on connection because "
					"of ACL\n"));
			    pid=1;
			}
			else
			{
			    pid=fork();
			}

			if(pid==0)
			{
			    p.fsin=fsin;
			    m->handler(p);
			}
			else
			{
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
	if(deliveryd>=0)
	{
	    if(deliveryd==0)
	    {
		deliveryd=deliveryd_main();
	    }
	    else
	    {
	        if(kill(deliveryd, 0)!=0)
	        {
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

    for(i=0; names[i]; i++)
    {
	if(strcmp(name, names[i])==0)
	{
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

    names=conf.udb.listusers("*");
    if(names)
    {
        for(i=0; names[i]; i++);

        passwords=(char **)malloc(i*sizeof(char *));
        for(i=0; names[i]; i++)
        {
	    u=conf.udb.getuser(names[i]);
	    passwords[i]=strdup(u.passwd);
        }
    }

    conf.udb.eraseuserdb();
    erasetermdb();

    i=conf.udb.parseusers();
    printf("Parsed %d users.\n", i);

    if(names)
    {
        names2=conf.udb.listusers("*");

        for(i=0; names2[i]; i++)
        {
	    tmp=getcachepw(names2[i], names, passwords);
	    if(tmp)
	    {
	        u=conf.udb.getuser(names2[i]);
	        strcpy(u.passwd, tmp);
	        conf.udb.storeuser(u);
	    }
        }
    }

    i=parseterms();
    printf("Parsed %d terminal servers.\n", i);

    if(names)
    {
        for(i=0; names[i]; i++)
        {
            free(passwords[i]);
        }
        free(passwords);
        cleanuserlist(names);
    }
}

static void killserver(void)
{
    int pid;
    FILE *f;

    if(access(conf.pidfile, F_OK)!=0)
    {
	puts("Error, no PID file found, is it running?");
	exit(1);
    }

    f=fopen(conf.pidfile, "r");
    if(f==NULL)
    {
	perror(conf.pidfile);
	exit(1);
    }

    if( fscanf(f, "%d", &pid) == EOF)
    {
	puts("Error:  No PID found in pidfile");
	exit(1);
    }

    kill(pid, SIGTERM);
    sleep(1);

    if(access(conf.pidfile, F_OK)==0)
    {
	puts("Error, pid file still exists, may not have shut down properly");
	exit(1);
    }
    else
    {
	puts("Successfully shut down.");
	exit(1);
    }

    fclose(f);
    exit(0);
}

static void ldb_main(void)
{
    conf.udb.dbinit();
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
	    runqueue(); break;

        case MODE_KILL:
	    killserver(); break;

        case MODE_PWCH:
	    changepasswd(); break;
    }
    cleanconfig();
    return(0);
}
