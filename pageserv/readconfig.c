/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: readconfig.c,v 1.12 1997/04/14 04:36:43 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

/* to get the mode names array from pageserv.h */
#define IWANT_MODENAMES 1

#include <pageserv.h>

extern struct config conf;

#define COMMANDS "DTPW"

int isin(char c, char *s)
{
 char *p;

    for(p=s; *p; p++)
    {
       if(*p==c)
	return(1);
    }

    return(0);
}

char *getarg(char *line)
{
   char *b, *e;

   b=line+2;

   if(*b==0x00)
   {
      return(NULL);
   }
   e=line+(strlen(line)-1);

   while(isspace(*b)) b++;
   while(isspace(*e)) e--;

   e++;
   *e=0x00;

   return(b);
}

void W_command(char s, char *arg)
{
    switch(s)
    {
	case 'D': conf.webroot=strdup(arg); break;
	case 'r': conf.webserver=atoi(arg); break;
	case 'p': conf.webport=atoi(arg); break;
    }
}

void D_command(char s, char *arg)
{
    switch(s)
    {
	case 'H': conf.servhost=strdup(arg); break;
	case 'u': conf.userdb=strdup(arg); break;
	case 't': conf.termdb=strdup(arg); break;
	case 'q': conf.qdir=strdup(arg); break;
	case 'p': conf.pidfile=strdup(arg); break;
	case 'd': conf.debug=atoi(arg); break;
    }
}

void T_command(char s, char *arg)
{
    switch(s)
    {
	case 'l': conf.childlifetime=atoi(arg); break;
	case 'Q': conf.maxqueuetime=atoi(arg); break;
	case 'c': conf.maxconattempts=atoi(arg); break;
	case 's': conf.conattemptsleep=atoi(arg); break;
    }
}

void P_command(char s, char *arg)
{
    switch(s)
    {
	case 'f': conf.farkle=atoi(arg); break;
    }
}

void docommand(int l, char c, char s, char *arg)
{
    if(!*arg)
    {
	fprintf(stderr, "ERR:  No arg for %c%c command line %d\n", c, s, l);
	exit(1);
    }

    switch(c)
    {
	case 'D': D_command(s, arg);  break;
	case 'T': T_command(s, arg);  break;
	case 'P': P_command(s, arg);  break;
	case 'W': W_command(s, arg);  break;
    }
}

void setdefaults(void)
{
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

void readconfig(char *file)
{
    FILE *f;
    char line[100];
    int linenum=0;

    /* set int defaults */
    memset( (char *)&conf, 0x00, sizeof(conf));

    conf.childlifetime=CHILD_LIFETIME;
    conf.maxqueuetime=MAX_QUEUETIME;
    conf.farkle=DEFAULT_FARKLE;
    conf.log_que=LOG_LOCAL7|LOG_INFO;
    conf.maxconattempts=MAX_CONATTEMPTS;
    conf.conattemptsleep=CONATTEMPTSSLEEP;
    conf.webport=WEBPORT;
    conf.webserver=0;

    f=fopen(file, "r");
    if(f==NULL)
    {
	perror(file);
	exit(1);
    }

    for(;;)
    {
	fgets(line, 100, f);
	if(feof(f))
	    break;
	linenum++;

        if( isin(line[0], COMMANDS))
	{
	    docommand(linenum, line[0], line[1], getarg(line));
	}
    }

    setdefaults();

    fclose(f);
}

void cleanconfig(void)
{
    if(conf.servhost) free(conf.servhost);
    if(conf.userdb) free(conf.userdb);
    if(conf.termdb) free(conf.termdb);
    if(conf.qdir) free(conf.qdir);
    if(conf.pidfile) free(conf.pidfile);
    if(conf.webroot) free(conf.webroot);
}

void showconfig(void)
{
    puts("Configuration:");
    printf("\tServer:       %s\n", conf.servhost);
    printf("\tRunning mode: %s\n", modenames[conf.mode]);
    printf("\tUser db:      %s\n", conf.userdb);
    printf("\tTerm db:      %s\n", conf.termdb);
    printf("\tQueue dir:    %s\n", conf.qdir);
    printf("\tPID file:     %s\n", conf.pidfile);
    printf("\tWebRoot:      %s\n", conf.webroot);
    printf("\tWebserver:    %d\n", conf.webserver);
    printf("\tWeb port:     %d\n", conf.webport);
    printf("\tFarkle        %d\n", conf.farkle);
    printf("\tChild life:   %d\n", conf.childlifetime);
    printf("\tMax queue tm: %d\n", conf.maxqueuetime);
    printf("\tLog facility: %d\n", conf.log_que);
    printf("\tDebug:        %d\n", conf.debug);
}
