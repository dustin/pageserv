/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: network.c,v 2.14 1998/07/15 07:54:51 dustin Exp $
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

static int s_openhost(char *host, int port);

int s_openterm(struct terminal t)
{
    char buf[BUFLEN];
    int s;

    strcpy(buf, t.predial);
    strcat(buf, t.number);
    s=s_openhost(t.ts, t.port);

    if(s_modem_connect(s, buf) < 0) {
        close(s);
	s=-1;
    }

    return(s);
}

static int s_openhost(char *host, int port)
{
    struct hostent *hp;
    int i, flag;
    register int s=-1;
    struct linger l;
    struct sockaddr_in sin;

    if((hp=gethostbyname(host)) == NULL) {
        del_log("Error looking up %s\n", host);
        return(-1);
    }

    for(i=0; i<conf.maxconattempts; i++) {
        if((s=socket(AF_INET, SOCK_STREAM, 0))<0) {
	    del_log("socket: ", strerror(errno));
            return(-1);
        }

        sin.sin_family = AF_INET;
        sin.sin_port=htons(port);
        bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);

        l.l_onoff  = 1;
        l.l_linger = 60;
        setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l));

	flag=1;
	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
	    sizeof(int)) <0) {
	    _ndebug(0, ("Nagle algorithm not dislabled.\n"));
	}

        if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0) {
	    _ndebug(2, ("Error getting modem, attempt %d, sleeping...\n",
	        i+1));

	    sleep(conf.conattemptsleep);
        } else {
	    break; /* We're connected */
	}
    }

    return(s);
}
