#include <stdio.h>

#include <pageserv.h>

void main(int argc, char **argv)
{
struct queuent q;
int s, ack, size;
char buf[BUFLEN];

    memset( (char *)&q, 0x00, sizeof(q));

    if(argc>2)
    {
	if(strncmp(argv[1], "-p", 2) == 0)
	{
	    if( tolower(argv[2][0])=='h')
	    {
		q.priority=htonl(PR_HIGH);
	    }
	}
    }
    else
    {
        q.priority=htonl(PR_NORMAL);
    }

    cgettext(q.to, TOLEN);
    cgettext(q.message, BUFLEN);

    if( (pushqueue(q.to, q.message, q.priority)) == 0)
	puts("Looks successful");
    else
        puts("Didn't work.");
}
