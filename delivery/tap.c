/*
 * Copyright (c) 1997  SPY Internetworking
 *
 * $Id: tap.c,v 2.1 1997/03/30 06:16:58 dustin Exp $
 */

#include <pageserv.h>
#include <tap.h>

void chardump(char *s)
{
    int i;

    for(i=0; s[i]!=0; i++)
    {
	printf("%d:\t%2d (%c)\n", i, (int)s[i], s[i]);
    }
}

int s_tap_init(int s)
{
    char buf[BUFLEN];
    int i;
    char c;

    puts("Initializing TAP");

    i=puttext(s, "\r");
    s_modem_waitfor(s, "ID=", 2);

    sprintf(buf, "%cPG1\r", C_ESC);
    puttext(s, buf);

    sprintf(buf, "%c[p", C_ESC);
    s_modem_waitfor(s, buf, 10);
}

int s_tap_end(int s)
{
    char buf[BUFLEN];
    char c;

    puts("ending TAP session");

    sprintf(buf, "%c\r", C_EOT);
    puttext(s, buf);
}

int charfound(char *string, char *chars)
{
    int i;

    for(i=0; chars[i]!=0x00; i++)
    {
	if(strchr(string, chars[i])!=NULL)
	{
	    return(chars[i]);
	}
    }
    return(0);
}

int s_tap_send(int s, char *id, char *message)
{
    char buf[BUFLEN];
    char search[]={C_ACK, C_NAK, C_RS, 0x00};
    char c;
    int i;

    printf("Sending message to %s:\n%s\n", id, message);

    sprintf(buf, "%c%s%c%s%c%c", C_STX, id, C_CR, message, C_CR, C_ETX);

    /* Stick the checksum on the end */
    strcat(buf, do_checksum(buf));
    strcat(buf, "\r");

    puttext(s, buf);

    /* Lets see if we won. */

    c=0;
    while( (c=charfound(buf, search)) ==0 )
    {
	i=recv(s, buf, BUFLEN, 0);
	buf[i]=0x00;
    }

    if(c==C_ACK)
    {
	return(0);
    }
    else
    {
	printf(":( Received character %x\n", c);
	return(1);
    }
}
