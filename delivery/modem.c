/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.3 1997/04/01 22:29:51 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <tap.h>
#include <pageserv.h>

extern struct config conf;

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

    i=s_modem_waitfor(s, "OK", 10);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);

    i=s_modem_waitfor(s, "CONNECT", 10);
    return(0);
}
