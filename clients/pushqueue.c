#include <stdio.h>

#include <pageserv.h>

#ifndef HAVE_GETOPT
#error No getopt()!!!
#endif /* HAVE_GETOPT */

void usage(char *command)
{
    printf("pushqueue version %s by Dustin Sallings\n", VERSION);
    printf("Usage:  %s [-p priority]\n", command);
    puts("priority can be either high or normal");
}

void main(int argc, char **argv)
{
struct queuent q;
int s, c, ack, size;
char buf[BUFLEN];

    memset( (char *)&q, 0x00, sizeof(q));

    /* set default */
    q.priority=htonl(PR_NORMAL);

    while( (c=getopt(argc, argv, "p:")) != -1)
    {
	switch(c)
	{
	    case 'p':
		if( tolower(argv[2][0])=='h')
		    q.priority=htonl(PR_HIGH); break;
	    case '?':
		usage(argv[0]);
		exit(1); break;
	}
    }

    cgettext(q.to, TOLEN);
    cgettext(q.message, BUFLEN);

    if( (pushqueue(q.to, q.message, q.priority)) == 0)
        puts("Looks successful");
    else
        puts("Didn't work.");
}
