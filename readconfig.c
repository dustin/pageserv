/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: readconfig.c,v 1.1 1997/03/26 00:24:44 dustin Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "pageserv.h"

extern struct config conf;

#define COMMANDS "D"

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

void d_command(char s, char *arg)
{
    switch(s)
    {
	case 'H': conf.servhost=strdup(arg); break;
	case 'u': conf.userdb=strdup(arg); break;
	case 'd': conf.debug=atoi(arg); break;
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
	case 'D': d_command(s, arg);  break;
    }
}

void setdefaults(void)
{
    if(conf.servhost == NULL)
	conf.servhost=REMHOST;

    if(conf.userdb == NULL)
	conf.userdb= USERDB;
}

void readconfig(char *file)
{
    FILE *f;
    char line[100];
    int linenum=0;

    /* clear the thingy */

    memset( (char *)&conf, 0x00, sizeof(conf));

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
}

void showconfig(void)
{
    puts("Configuration:");
    printf("\tServer:       %s\n\tUser db:      %s\n\tdebug:        %d\n",
	conf.servhost, conf.userdb, conf.debug);
}
