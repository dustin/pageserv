/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: network.c,v 1.2 1997/03/30 01:34:55 dustin Exp $
 */

/*
 * Client delivery network code
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../pageserv.h"

void net_timeout(void)
{
    fputs("Connection timed out.\n", stderr);
    exit(1);
}

int openhost(char *host, int port)
{
struct hostent *hp;
register int s;
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

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0)
    {
        perror("connect");
        exit(1);
    }

    return(s);
}
