/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: pushqueue.c,v 2.10 1998/10/27 18:31:27 dustin Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <snppclient.h>

#ifndef HAVE_GETOPT
#error No getopt()!!!
#endif				/* HAVE_GETOPT */

static void
usage(char *command)
{
	printf("pushqueue version %s by Dustin Sallings\n", VERSION);
	printf("Usage:  %s [-p priority] [-H host ] [-P port] [-h n] [-d debug]\n",
	       command);
	puts("priority can be either high or normal");
}

int
main(int argc, char **argv)
{
	int             c;
	struct snpp_client *snpp;
	char           *hostname = "pager", *priority = NULL;
	char            to[1024], msg[1024];
	int             port = SNPP_PORT, debug = 0;
	extern char    *optarg;
	extern int      optind;
	time_t          hold = 0;

	while ((c = getopt(argc, argv, "p:H:P:d:h:")) != -1) {
		switch (c) {
		case 'p':
			priority = optarg;
			break;
		case 'H':
			hostname = optarg;
			break;
		case 'P':
			port = atoi(optarg);
			break;
		case 'h':
			hold = atoi(optarg);
			break;
		case 'd':
			debug = atoi(optarg);
			break;
		case '?':
			usage(argv[0]);
			exit(1);
			break;
		}
	}

	snpp = snpp_connect(hostname, port);
	if (snpp == NULL)
		return (75);

	snpp->debug = debug;

	if (priority)
		snpp->rawsend2(snpp, "priority", priority);

	if(hold)
		snpp->hold(snpp, hold, 0);

	fgets(to, 1024, stdin);
	fgets(msg, 1024, stdin);

	if (snpp->sendAPage(snpp, to, msg) == 0) {
		puts("Looks successful");
		return (0);
	} else {
		puts("Didn't work.");
		return (75);
	}
}
