/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: client.c,v 2.1 1997/04/01 22:29:44 dustin Exp $
 */

/*
 * This is the generic client code stuff.  Probably all that will
 * be needed out of this is pushqueue().  There's also ckw() which
 * is basically the same as kw() from utility.c, but I didn't want
 * to include utility.c in this crap.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pageserv.h>

void timeout(void)
{
    fputs("Connection timed out.\n", stderr);
    exit(1);
}

int openhost(void)
{
struct hostent *hp;
register int s;
struct linger l;
struct sockaddr_in sin;

    if((hp=gethostbyname(REMHOST)) == NULL)
    {
#ifdef HAVE_HERROR
        herror("gethostbyname");
#else
        fprintf(stderr, "Error looking up %s\n", REMHOST);
#endif
        exit(1);
    }

    if((s=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        perror("socket");
        exit(1);
    }

    sin.sin_family = AF_INET;
    sin.sin_port=htons(PORT);
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

int pushqueue(char *to, char *message, int priority)
{
    int s, size, ret;
    char buf[BUFLEN];

    s=openhost();

    ckw(to);
    strcat(to, "\n");
    ckw(message);
    strcat(message, "\n");

    send(s, "epage\n", strlen("epage\n"), 0);
    send(s, to, strlen(to), 0);

    switch(priority)
    {
        case PR_HIGH: strcpy(buf, "high\n"); break;

        default:      strcpy(buf, "normal\n"); break;
    }

    send(s, buf, strlen(buf), 0);
    size=send(s, message, strlen(message), 0);

    if(size==strlen(message))
       ret=0;
    else
       ret=size;

    /* Sleep a second, unless I can figure out how to do it right... */
    sleep(1);

    close(s);
    return(ret);
}
