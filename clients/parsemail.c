/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: parsemail.c,v 1.1 1997/03/14 00:52:51 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <pageserv.h>

#define LINELEN 2048

char *getdata(int l, char *line)
{
    char *ret;

    ret=(char *)malloc(strlen(line));
    strcpy(ret, line+l);
    return(ret);
}


void main(int argc, char **argv)
{
    char line[LINELEN];
    int priority;
    char *subject=NULL, *from=NULL, *to=NULL;

    if(argc<2)
    {
	exit(0);
    }

    priority=PR_NORMAL;
    to=argv[1];

    while(!feof(stdin))
    {
	fgets(line, LINELEN, stdin);

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
