/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parsemail.c,v 2.9 1998/06/03 16:12:21 dustin Exp $
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

extern char *optarg;
extern int optind, opterr;

#include <snppclient.h>

#define LINELEN 2048

static void usage(char *name)
{
    fprintf(stderr, "Usage:\n%s [-p priority] [-H SNPPserver] [-P port] "
                    "[-t tag] <to>\n", name);
}

static char *getdata(int l, char *line)
{
    char *ret;

    ret=(char *)malloc(strlen(line));
    strcpy(ret, line+l);
    return(_killwhitey(ret));
}


int main(int argc, char **argv)
{
    char line[LINELEN];
    int c, r, port=1041;
    char *subject=NULL, *from=NULL, *to=NULL, *tag="Mail";
    char *priority=NULL, *hostname="pager";
    extern int optind;
    struct snpp_client *snpp;

    while( (c=getopt(argc, argv, "H:P:p:t:")) != -1)
    {
	switch(c)
	{
	    case 'P':
		port=atoi(optarg);
		break;
	    case 'H':
		hostname=optarg;
		break;
	    case 'p':
		priority=optarg;
		break;
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
	exit(75);
    }

    snpp=snpp_connect(hostname, port);
    if(snpp==NULL) {
	fputs("Error connecting to pager server\n", stderr);
	return(75);
    }

    to=argv[optind++];

    while(!feof(stdin))
    {
	if(fgets(line, LINELEN, stdin)==NULL)
            break;

	if(strncmp(line, "From: ", 6) == 0)
	{
	    if(from==NULL)
	        from=getdata(6, line);
	}
	else if(strncmp(line, "Subject: ", 9) == 0)
	{
	    if(subject==NULL)
	        subject=getdata(9, line);
	}
    }

    sprintf(line, "%s: %s -- %s", tag, from, subject);

    _ndebug(2, ("Page to send:  %s\n", line));

    if(priority)
	snpp->rawsend2(snpp, "priority", priority);

    if( snpp->sendAPage(snpp, to, line) == 0) {
	r=0;
    } else {
        printf("Page failed for some unknown reason, deferring...\n");
	r=75;
    }

    free(from);
    free(subject);
    return(r);
}
