/*
 * Copyright (c) 1997 Dustin Sallings
 *
 * $Id: sockets.c,v 1.6 1997/08/09 06:48:36 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>

#include <pageserv.h>
#include <nettools.h>
#include <readconfig.h>

extern struct config conf;

int checkIPAccess(struct sockaddr_in addr, module *m)
{
    return 1;
}

char *getHostName(unsigned int addr)
{
    struct hostent *h;
    char *name;

    h=gethostbyaddr((void *)&addr, sizeof(unsigned int), AF_INET);
    if(h==NULL)
	name=nmc_intToDQ(addr);
    else
	name=h->h_name;

    return(name);
}

void logConnect(struct sockaddr_in fsin, module *m)
{
    char *ip_addr, *hostname;

    openlog("pageserv", LOG_PID|LOG_NDELAY, conf.log_que);

    if(rcfg_lookupInt(conf.cf, "log.hostnames") ==1)
	hostname=getHostName(fsin.sin_addr.s_addr);
    else
        hostname=nmc_intToDQ(fsin.sin_addr.s_addr);

    ip_addr=nmc_intToDQ(fsin.sin_addr.s_addr);

    syslog(conf.log_que|LOG_INFO, "connect from %s (%s) for %s",
	   hostname, ip_addr, m->name);

    if(conf.debug>2)
	printf("connect from %s (%s) for %s\n", hostname, ip_addr,
	       m->name);

    closelog();
}

int getservsocket(int port)
{
    int reuse=1, s;
    struct sockaddr_in sin;

    signal(SIGPIPE, SIG_IGN);

    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("server: socket");
        exit(1);
    }

    memset((char *) &sin, 0x00, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
        (char *)&reuse, sizeof(int));

    if( bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
        perror("server: bind");
        exit(1);
    }

    if(listen(s, 5) < 0)
    {
        perror("server: listen");
        exit(1);
    }

    return(s);
}
