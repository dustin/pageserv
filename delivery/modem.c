/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.6 1997/06/19 08:24:33 dustin Exp $
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
    }
    while(c!=what);
}

/*
 * This should be replaced, I want something that just keeps reading from
 * the buffer, one byte at a time until it sees something looking like what
 * it's looking for, then start pulling stuff in until it finds the string,
 * or runs out of time.
 */

int s_modem_waitfor(int s, char *what, int timeout)
{
    char buf[BUFLEN];

    /* printf("Waiting for ``%s'' (%d bytes)\n", what, strlen(what)); */

    for(;;)
    {
        gettext(s, buf);

	if(conf.debug>2)
	    puts(buf);

        if( (strstr(what, buf)!=NULL))
        {
	    /* printf("Got %s\n", what); */
            return(0);
        }
    }
}

int s_modem_connect(int s, char *number)
{
    char buf[BUFLEN];
    int i;

    i=puttext(s, "atz\r\n");
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    i=s_modem_waitforchar(s, 'K', 10);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    i=s_modem_waitforchar(s, 'T', 10);
    if(conf.debug>3)
        printf("Wrote %d bytes\n", i);

    usleep(2600);

    return(0);
}
