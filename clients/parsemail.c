/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parsemail.c,v 2.3 1998/01/10 01:32:21 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <pageserv.h>

#define LINELEN 2048

static void usage(char *name)
{
    fprintf(stderr, "Usage:\n%s [-p priority] <to>\n", name);
}

static char *getdata(int l, char *line)
{
    char *ret;

    ret=(char *)malloc(strlen(line));
    strcpy(ret, line+l);
    ckw(ret);
    return(ret);
}


void main(int argc, char **argv)
{
    char line[LINELEN];
    int priority, c;
    char *subject="(no subject)", *from=NULL, *to=NULL;
    extern int optind;

    priority=PR_NORMAL;

    while( (c=getopt(argc, argv, "p:")) != -1)
    {
	switch(c)
	{
	    case 'p':
		if(tolower(argv[2][0])=='h')
		    priority=PR_HIGH; break;
	    case '?':
		usage(argv[0]); exit(1); break;
	}
    }

    if(optind>=argc)
    {
	fputs("Error, too few arguments\n", stderr);
	usage(argv[0]);
	exit(1);
    }

    to=argv[optind++];

    while(!feof(stdin))
    {
	if(fgets(line, LINELEN, stdin)==NULL)
            break;

	if(strncmp(line, "From: ", 6) == 0)
	{
	    from=getdata(6, line);
	}
	else if(strncmp(line, "Subject: ", 9) == 0)
	{
	    subject=getdata(9, line);
	}
    }

    sprintf(line, "Mail: %s -- %s", from, subject);

    pushqueue(to, line, priority);

    free(from);
    free(subject);
}
