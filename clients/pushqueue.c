/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: pushqueue.c,v 2.6 1998/01/14 05:48:40 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <pageserv.h>

#ifndef HAVE_GETOPT
#error No getopt()!!!
#endif /* HAVE_GETOPT */

static void usage(char *command)
{
    printf("pushqueue version %s by Dustin Sallings\n", VERSION);
    printf("Usage:  %s [-p priority]\n", command);
    puts("priority can be either high or normal");
}

int main(int argc, char **argv)
{
struct queuent q;
int c;

    memset( (char *)&q, 0x00, sizeof(q));

    /* set default */
    q.priority=PR_NORMAL;

    while( (c=getopt(argc, argv, "p:")) != -1)
    {
	switch(c)
	{
	    case 'p':
		if( tolower(argv[2][0])=='h')
		    q.priority=PR_HIGH; break;
	    case '?':
		usage(argv[0]);
		exit(1); break;
	}
    }

    cgettext(q.to, TOLEN);
    cgettext(q.message, BUFLEN);

    if( (pushqueue(q.to, q.message, q.priority)) == 0)
    {
        puts("Looks successful");
	return(0);
    }
    else
    {
        puts("Didn't work.");
	return(75);
    }
}
