/*
 * Copyright (c) 1997  SPY Internetworking
 *
 * $Id: tap.c,v 2.20 1998/07/13 23:41:39 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

static int tap_checksum(char *string);
static char *tap_sent_checksum(int sum);

static int tap_checksum(char *string)
{
    int i, sum=0;

    for(i=0; string[i]!=0; i++) {
	sum+=string[i] - ( (int)(string[i]/128) * 128);
    }

    return(sum);
}

static char *tap_sent_checksum(int sum)
{
    static char charsum[4]={0,0,0,0};

    charsum[2]=48+ sum - (int)(sum/16) * 16;
    sum = (int)(sum/16);
    charsum[1]=48+ sum - (int)(sum/16) * 16;
    sum = (int)(sum/16);
    charsum[0]=48+ sum - (int)(sum/16) * 16;

    return(charsum);
}

void chardump(char *s)
{
    int i;

    for(i=0; s[i]!=0; i++) {
	printf("%d:\t%2d (%c)\n", i, (int)s[i], s[i]);
    }
}

int s_tap_init(int s, int flags)
{
    char buf[BUFLEN];

    _ndebug(0, ("Initializing TAP\n"));

    /* If configured to do so, pimp slap it with a CR right away */

    if(flags & TAP_INITDELAY) {
	_ndebug(3, ("flag TAP_INITDELAY is set, sleeping\n"));

	sleep(1);
    }

    if(flags & TAP_INITCR) {
	_ndebug(3, ("flag TAP_INITCR is set, sending a CR\n"));

	usleep(500000);
	puttext(s, "\r");
    }

    if( s_modem_waitfor(s, "ID=", 2) < 0) {
	_ndebug(2, ("Error:  s_modem_waitfor(%d, \"ID=\", 2);\n", s));
	return(-1);
    }

    sprintf(buf, "%cPG1\r", C_ESC);
    puttext(s, buf);

    sprintf(buf, "%c[p", C_ESC);
    if( s_modem_waitfor(s, buf, 10) < 0) {
	_ndebug(2, ("Error:  s_modem_waitfor(%d, \"%s\", 2);\n", s, buf));
	return(-1);
    }
    return(0);
}

int s_tap_end(int s)
{
    char buf[BUFLEN];

    _ndebug(0, ("ending TAP session\n"));

    sprintf(buf, "%c\r", C_EOT);
    puttext(s, buf);
    return(0);
}

static int charfound(char *string, char *chars)
{
    int i;

    for(i=0; chars[i]!=0x00; i++) {
	if(strchr(string, chars[i])!=NULL) {
	    return(chars[i]);
	}
    }
    return(0);
}

int s_tap_send(int s, char *id, char *message)
{
    char buf[BUFLEN];
    char search[]={C_ACK, C_ESC, C_NAK, 0x00};
    char c;
    int i;

    _ndebug(0, ("Sending message to %s:\n%s\n", id, message));

    sprintf(buf, "%c%s%c%s%c%c", C_STX, id, C_CR, message, C_CR, C_ETX);

    /* Stick the checksum on the end */
    strcat(buf, do_checksum(buf));
    strcat(buf, "\r");

    puttext(s, buf);

    /* Lets see if we won. */

    c=0;
    while( (c=charfound(buf, search)) ==0 ) {
	i=read(s, buf, BUFLEN);
	if(i>0) {
	    buf[i]=0x00;
	    _ndebug(2, ("%s\n", buf));
	}
    }

    usleep(1700);

    if(c==C_ACK || c==C_ESC) {
	return(0);
    } else {
	_ndebug(2, (":( Received character %xh\n", c));
	return(1);
    }
}
