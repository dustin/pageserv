/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.1 1997/03/30 06:16:57 dustin Exp $
 */

#include <stdio.h>

#include <tap.h>
#include <pageserv.h>

int s_modem_waitfor(int s, char *what, int timeout)
{
    char buf[BUFLEN];

    /* printf("Waiting for ``%s'' (%d bytes)\n", what, strlen(what)); */

    for(;;)
    {
        gettext(s, buf);
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

    i=s_modem_waitfor(s, "OK", 10);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);

    i=s_modem_waitfor(s, "CONNECT", 10);
}
