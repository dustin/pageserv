/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parsemail.c,v 2.5 1998/03/18 16:31:59 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* local debug stuff */
#if (PDEBUG>0)
# ifndef _ndebug
#  define _ndebug(a, b) if(PDEBUG > a ) printf b;
# endif
#endif

/* In case it didn't make it */
#ifndef _ndebug
#define _ndebug(a, b)
#endif

#include <pageserv.h>

#define LINELEN 2048

static void usage(char *name)
{
    fprintf(stderr, "Usage:\n%s [-p priority] [-t tag] <to>\n", name);
}

static char *getdata(int l, char *line)
{
    char *ret;

    ret=(char *)malloc(strlen(line));
    strcpy(ret, line+l);
    ckw(ret);
    return(ret);
}


int main(int argc, char **argv)
{
    char line[LINELEN];
    int priority, c, r;
    char *subject="(no subject)", *from=NULL, *to=NULL, *tag="Mail";
    extern int optind;

    priority=PR_NORMAL;

    while( (c=getopt(argc, argv, "p:t:")) != -1)
    {
	switch(c)
	{
	    case 'p':
		if(tolower(optarg[0])=='h')
		    priority=PR_HIGH; break;
            case 't':
                tag=optarg; break;
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

    sprintf(line, "%s: %s -- %s", tag, from, subject);

    _ndebug(2, ("Page to send:  %s\n", line));

    if( pushqueue(to, line, priority) == 0)
	r=0;
    else
	r=75;

    free(from);
    free(subject);
    return(r);
}
