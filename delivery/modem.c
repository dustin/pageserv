/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.13 1998/01/01 09:40:41 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <tap.h>
#include <pageserv.h>

extern struct config conf;

int any_closeterm(int s, struct terminal t)
{
    puttext(s, "+++atz\n");
    close(s);
    sleep(5);
    p_unlock(t.ts);
    return(0);
}

int any_openterm(struct terminal term)
{
    int s=-1;

    if(term.ts[0]=='/')
        s=p_openterm(term);
    else
        s=s_openterm(term);

    _ndebug(2, ("Looks like the modem for today will be %d\n", s));

    return(s);
}

int s_modem_waitforchar(int s, char what, int timeout)
{
    char c;
    do
    {
        read(s, &c, 1);
	_ndebug(2, ("%c", c));
    }
    while(c!=what);
    return(0);
}

int s_modem_waitfor(int s, char *what, int timeout)
{
    int i, size;
    char c;

    i=0;

    while(i<strlen(what))
    {
	size=read(s, &c, 1);
	if(size>0)
	{
	    _ndebug(2, ("%c", c));

	    if(c==what[i])
	        i++;
	    else
	        i=0;
	}
    }
    return(0);
}

int s_modem_connect(int s, char *number)
{
    char buf[BUFLEN];
    int i;

    i=puttext(s, "atz\r\n");
    _ndebug(3, ("Wrote %d bytes\n", i));

    i=s_modem_waitfor(s, "OK", 10);
    _ndebug(3, ("Wrote %d bytes\n", i));

    /* Take a nap before trying to write again */

    usleep(2600);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);
    _ndebug(3, ("Wrote %d bytes\n", i));

    i=s_modem_waitfor(s, "CONNECT", 10);
    _ndebug(3, ("Wrote %d bytes\n", i));

    usleep(2600);

    return(0);
}
