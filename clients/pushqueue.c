#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pageserv.h>

#define PORTNUM 1029
#define REMHOST "localhost"
/*
#define REMHOST "molly.ipa.net"
*/

void timeout(void)
{
	fputs("Connection timed out.\n", stderr);
	exit(1);
}

int openhost(void)
{
struct hostent *hp;
register int s;
int linger;
struct sockaddr_in sin;

	if((hp=gethostbyname(REMHOST)) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	if((s=socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("socket");
		exit(1);
	}

	sin.sin_family = AF_INET;
	sin.sin_port=htons(PORTNUM);
	bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);

	setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(int));

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0)
	{
		perror("connect");
		exit(1);
	}

	return(s);
}

char *ckw(char *in)
{
    /* bounds checking */
    if(strlen(in)==0)
        return(in);

    while(isspace(in[strlen(in)-1]))
    {
        /* bounds checking */
        if(strlen(in)==0)
            return(in);

        in[strlen(in)-1]=0x00;
    }

    return(in);
}

void cgettext(char *message, int size)
{
    fgets(message, size, stdin);
    ckw(message);
}

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
    strcat(q.to, "\n");
    strcat(q.message, "\n");

    s=openhost();

    send(s, "epage\n", strlen("epage\n"), 0);
    send(s, q.to, strlen(q.to), 0);
    sprintf(buf, "%d\n", q.priority);
    send(s, buf, strlen(buf), 0);
    size=send(s, q.message, strlen(q.message), 0);

    if(size==strlen(q.message))
    {
        puts("Looks like it got there...");
    }
    else
    {
	puts("I don't think it all made it...");
    }

    close(s);
}
