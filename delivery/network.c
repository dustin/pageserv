/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: network.c,v 2.9 1998/01/01 09:40:42 dustin Exp $
 * $State: Exp $
 */

/*
 * Client delivery network code
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

int s_openterm(struct terminal t)
{
    char buf[BUFLEN];
    int s;

    strcpy(buf, t.predial);
    strcat(buf, t.number);
    s=s_openhost(t.ts, t.port);
    s_modem_connect(s, buf);
    return(s);
}

void net_timeout(void)
{
    fputs("Connection timed out.\n", stderr);
    exit(1);
}

int s_openhost(char *host, int port)
{
struct hostent *hp;
int success, i, flag;
register int s=-1;
struct linger l;
struct sockaddr_in sin;

    if((hp=gethostbyname(host)) == NULL)
    {
#ifdef HAVE_HERROR
        herror("gethostbyname");
#else
        fprintf(stderr, "Error looking up %s\n", host);
#endif
        exit(1);
    }

    success=0;

    for(i=0; i<conf.maxconattempts; i++)
    {
        if((s=socket(AF_INET, SOCK_STREAM, 0))<0)
        {
            perror("socket");
            exit(1);
        }

        sin.sin_family = AF_INET;
        sin.sin_port=htons(port);
        bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);

        l.l_onoff  = 1;
        l.l_linger = 60;
        setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l));

	flag=1;
	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
	    sizeof(int)) <0)
        {
	    _ndebug(0, ("Nagle algorithm not dislabled.\n"));
	}

        if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0)
        {
	    _ndebug(2, ("Error getting modem, attempt %d, sleeping...\n",
	        i+1));

	    sleep(conf.conattemptsleep);
        }
	else
	{
	    success=1;
	    break;
	}
    }

    return(s);
}
