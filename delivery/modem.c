/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.8 1997/06/20 13:46:21 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <tap.h>
#include <pageserv.h>

extern struct config conf;

int any_openterm(struct terminal term)
{
    int s;

    if(conf.debug>2)
	printf("any_openterm contype is %d\n", term.contype);

    switch(term.contype)
    {
	case PORT_NET:
            s=s_openterm(term);
	    break;
	case PORT_DIRECT:
	    s=p_openterm(term);
	    break;
	default:
	    printf("Unkown con type:  %d\n", term.contype);
    }

    return(s);
}

int s_modem_waitforchar(int s, char what, int timeout)
{
    char c;
    do
    {
        read(s, &c, 1);
	if(conf.debug>2)
	    putchar(c);
    }
    while(c!=what);
}

int s_modem_waitfor(int s, char *what, int timeout)
{
    int i;
    char c;

    i=0;

    while(i<strlen(what))
    {
	read(s, &c, 1);
	if(conf.debug>2)
	    putchar(c);

	if(c==what[i])
	    i++;
	else
	    i=0;
    }
    return(0);
}

int s_modem_connect(int s, char *number)
{
    char buf[BUFLEN];
    int i;

    i=puttext(s, "atz\r\n");
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    i=s_modem_waitfor(s, "OK", 10);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    i=s_modem_waitfor(s, "CONNECT", 10);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    usleep(2600);

    return(0);
}
