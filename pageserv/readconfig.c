/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: readconfig.c,v 1.29 1997/10/10 08:24:21 dustin Exp $
 */

#include <readconfig.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

/* to get the mode names array from pageserv.h */
#define IWANT_MODENAMES 1

#include <pageserv.h>

extern struct config conf;

void setdefaults(void)
{
    /* This is a *little* inclear, but here I list all known types of
     * user database, so that I can just check to see what's configured
     * in
     */
    struct { char *name;
	     void (*func)(void);
    } userdbtypes[]={
#ifdef HAVE_NIS
	{ "nis", nis_userdbInit },
#endif /* have NIS */
	{ "dbm", dbm_userdbInit },
#ifdef HAVE_SQL
        { "sql", sql_userdbInit },
#endif /* have SQL */
	{ NULL, NULL }
    };
    char *userdb;
    int i=0;

    if(conf.servhost == NULL)
	conf.servhost=REMHOST;

    if(conf.userdb == NULL)
	conf.userdb= USERDB;

    if(conf.termdb == NULL)
	conf.termdb= TERMDB;

    if(conf.qdir == NULL)
	conf.qdir= QUEDIR;

    if(conf.pidfile == NULL)
	conf.pidfile= PIDFILE;

    if(conf.webroot == NULL)
	conf.webroot= WEBROOT;

    userdb=rcfg_lookup(conf.cf, "databases.userdbType");

    /* Check to make sure this is configured in somewhere */
    if(userdb)
    {
        for(i=0; userdbtypes[i].name != NULL; i++)
        {
	    if(strcmp(userdbtypes[i].name, userdb)==0)
	        break;
        }
    }

    if( (userdbtypes[i].func == NULL) || (userdb==NULL) )
    {
	fputs("Unknown, or unconfigured user database type, using dbm\n",
              stderr);
	dbm_userdbInit();
    }
    else
    {
	userdbtypes[i].func();
    }
}

#ifndef HAVE_GETOPT
#error No getopt()!!!
#endif /* HAVE_GETOPT */

void showversion(void)
{
    printf("Pageserv by Dustin Sallings\nVersion %s\n", VERSION);
}

void showusage(char *cmd)
{
    showversion();

    printf("Usage:  %s -b{d|r|q|k|p} [-dx] or\n", cmd);
    printf("        %s -p{l|q} [-dx]   or\n", cmd);
    printf("        %s -v [-dx]\n", cmd);

    puts("\n-b is run modes, can be one of the following:");
    puts("\td: starts a pager server");
    puts("\tr: rehashes databases");
    puts("\tq: runs the queue");
    puts("\tk: kills an already running daemon");
    puts("\tp: sets a user's password");

    puts("\n-p is print modes");
    puts("\tl: lists databases");
    puts("\tq: prints queue");

    puts("\n-d sets the debugging level to x");

    puts("\n-v shows version information");

    puts("\nWith no arguments, a server is started as if -bd were used");
}

void getoptions(int argc, char **argv)
{
    int c;
    extern char *optarg;

    while( (c=getopt(argc, argv, "vb:p:d:")) != -1)
    {
	switch(c)
	{
	    case 'b':
		switch(optarg[0])
		{
		    case 'd':
			conf.mode=MODE_DAEMON; break;
		    case 'r':
			conf.mode=MODE_REHASH; break;
		    case 'q':
			conf.mode=MODE_RUNQ; break;
		    case 'k':
			conf.mode=MODE_KILL; break;
		    case 'p':
			conf.mode=MODE_PWCH; break;
                    default:
			printf("Unknown run mode %c\n", optarg[0]);
			exit(1); break;
		} break;
	    case 'p':
		switch(optarg[0])
		{
		    case 'l':
			conf.mode=MODE_LDB; break;
                    case 'q':
			conf.mode=MODE_PQ; break;
                    default:
			printf("Unknown print mode %c\n", optarg[0]);
			exit(1); break;
		} break;
	    case 'd':
		conf.debug=atoi(optarg); break;
	    case 'v':
		conf.mode=MODE_VERS; break;
	    case '?':
		showusage(argv[0]); exit(1); break;
	}
    }
}

int getlogFacility(char *facility)
{
    int i;
    static struct { char *name; int value; } fcs[]={
      { "auth",         LOG_AUTH   },
      { "cron",         LOG_CRON   },
      { "daemon",       LOG_DAEMON },
      { "kern",         LOG_KERN   },
      { "lpr",          LOG_LPR    },
      { "mail",         LOG_MAIL   },
      { "news",         LOG_NEWS   },
      { "security",     LOG_AUTH   },
      { "syslog",       LOG_SYSLOG },
      { "user",         LOG_USER   },
      { "uucp",         LOG_UUCP   },
      { "local0",       LOG_LOCAL0 },
      { "local1",       LOG_LOCAL1 },
      { "local2",       LOG_LOCAL2 },
      { "local3",       LOG_LOCAL3 },
      { "local4",       LOG_LOCAL4 },
      { "local5",       LOG_LOCAL5 },
      { "local6",       LOG_LOCAL6 },
      { "local7",       LOG_LOCAL7 },
      { NULL,           LOG_LOCAL7 } /* Default is to log local7 */
    };

    if(facility==NULL)
        return(LOG_LOCAL7);

    for(i=0; fcs[i].name!=NULL; i++)
    {
        if(strcmp(fcs[i].name, facility)==0)
	    break;
    }

    return(fcs[i].value);
}

void rdconfig(char *file)
{
    char *tmp;
    struct confType *cf;

    /* set int defaults */
    memset( (char *)&conf, 0x00, sizeof(conf));

    conf.childlifetime=CHILD_LIFETIME;
    conf.maxqueuetime=MAX_QUEUETIME;
    conf.farkle=DEFAULT_FARKLE;

    conf.maxconattempts=MAX_CONATTEMPTS;
    conf.conattemptsleep=CONATTEMPTSSLEEP;
    conf.gmtoffset=0;
    conf.webport=WEBPORT;
    conf.webserver=0;
    conf.snppport=SNPPPORT;
    conf.snppserver=0;

    /* set stuff out of the config crap */

    conf.cf=rcfg_readconfig(file);
    cf=conf.cf;

    if(cf==NULL)
    {
	perror(file);
	exit(1);
    }

    /* Hook up with some log facility action */
    conf.log_que=getlogFacility(rcfg_lookup(cf, "log.facility"));

    tmp=rcfg_lookup(cf, "databases.userdb");
    if(tmp)
	conf.userdb=tmp;

    tmp=rcfg_lookup(cf, "databases.termdb");
    if(tmp)
	conf.termdb=tmp;

    tmp=rcfg_lookup(cf, "etc.pidfile");
    if(tmp)
	conf.pidfile=tmp;

    tmp=rcfg_lookup(cf, "etc.queuedir");
    if(tmp)
	conf.qdir=tmp;

    tmp=rcfg_lookup(cf, "etc.gmtOffset");
    if(tmp)
	conf.gmtoffset=atoi(tmp);

    tmp=rcfg_lookup(cf, "protocols.farkle");
    if(tmp)
	conf.farkle=atoi(tmp);

    tmp=rcfg_lookup(cf, "webserver.run");
    if(tmp)
	conf.webserver=atoi(tmp);

    tmp=rcfg_lookup(cf, "webserver.docroot");
    if(tmp)
	conf.webroot=tmp;

    tmp=rcfg_lookup(cf, "webserver.port");
    if(tmp)
	conf.webport=atoi(tmp);

    tmp=rcfg_lookup(cf, "snpp.run");
    if(tmp)
	conf.snppserver=atoi(tmp);

    tmp=rcfg_lookup(cf, "snpp.port");
    if(tmp)
	conf.snppport=atoi(tmp);

    tmp=rcfg_lookup(cf, "tuning.debug");
    if(tmp)
	conf.debug=atoi(tmp);

    tmp=rcfg_lookup(cf, "tuning.childLifetime");
    if(tmp)
	conf.childlifetime=atoi(tmp);

    tmp=rcfg_lookup(cf, "tuning.queueLifeTime");
    if(tmp)
	conf.maxqueuetime=atoi(tmp);

    tmp=rcfg_lookup(cf, "tuning.modemGrabAttempts");
    if(tmp)
	conf.maxconattempts=atoi(tmp);

    tmp=rcfg_lookup(cf, "tuning.modemGrabSleep");
    if(tmp)
	conf.conattemptsleep=atoi(tmp);


    setdefaults();
    initmodules();
}

void cleanconfig(void)
{
    if(conf.servhost) free(conf.servhost);
    if(conf.userdb) free(conf.userdb);
    if(conf.termdb) free(conf.termdb);
    if(conf.qdir) free(conf.qdir);
    if(conf.pidfile) free(conf.pidfile);
    if(conf.webroot) free(conf.webroot);
    if(conf.modules) free(conf.modules);
}

void showconfig(void)
{
    int i;
    module *m;
    char *tmp;

    puts("Configuration:");
    printf("\tServer:       %s\n", conf.servhost);
    printf("\tRunning mode: %s\n", modenames[conf.mode]);
    printf("\tUser db:      %s\n", conf.userdb);

    /* Find out the configured user database type */
    tmp=rcfg_lookup(conf.cf, "databases.userdbType");
    tmp=tmp?tmp:"dbm";
    printf("\tUser db type: %s\n", tmp);

    printf("\tTerm db:      %s\n", conf.termdb);
    printf("\tQueue dir:    %s\n", conf.qdir);
    printf("\tPID file:     %s\n", conf.pidfile);
    printf("\tGMT offset:   %d\n", conf.gmtoffset);
    printf("\tWebRoot:      %s\n", conf.webroot);
    printf("\tWebserver:    %d\n", conf.webserver);
    printf("\tWeb port:     %d\n", conf.webport);
    printf("\tSNPPserver:   %d\n", conf.snppserver);
    printf("\tSNPP port:    %d\n", conf.snppport);
    printf("\tFarkle        %d\n", conf.farkle);
    printf("\tChild life:   %d\n", conf.childlifetime);
    printf("\tMax queue tm: %d\n", conf.maxqueuetime);
    printf("\tLog facility: %d\n", conf.log_que);
    printf("\tDebug:        %d\n", conf.debug);

    puts("Modules:");

    printf("\tNumber:       %d\n", conf.nmodules);

    m=conf.modules;
    for(i=0; i<conf.nmodules; i++)
    {
	printf("\t%s\n", m->name);
	m++;
    }
}
