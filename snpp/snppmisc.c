/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: snppmisc.c,v 1.7 1998/01/20 04:29:47 dustin Exp $
 */

#include <snpp.h>
#include <pageserv.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

static void snpp_helpsend(int s, char *message)
{
    char buf[BUFLEN];

    strcpy(buf, "214 ");
    strcat(buf, message);
    strcat(buf, "\n");
    puttext(s, buf);
}

void snpp_help(int s)
{
    int i;

    static char *snpp_helptext[]={
    "Welcome to Dustin's Pager Server SNPP Module.",
    "The following commands are recognized here:\n214 ",
    "HELP",
    "\tThis help",
    "PAGE <id>",
    "\tPage goes to named ID",
    "MESSage <message>",
    "\tThe message to send",
    "PRIORITY",
    "\tSet the priority of the message",
    "HOLDuntil YYMMDDHHMMSS[+/-GMT]",
    " ...or",
    "HOLDuntil YYYYMMDDHHMMSS[+/-GMT]",
    "\tDelay delivery until the given time",
    "SEND",
    "\tSend the queued message",
    "RESEt",
    "\tReset all input crap, get ready to go again.",
    "LOGIn <username> <passwd>",
    "\tLogin for special features, etc...",
    "QUIT",
    "\tExits",
    NULL
};
    for(i=0; snpp_helptext[i]!=NULL; i++)
    {
	snpp_helpsend(s, snpp_helptext[i]);
    }
    puttext(s, "250 End of help\n");
}

char *snpp_arg(char *s)
{
    while(*s && *s!=' ') s++;
    if(*s==' ') s++;
    return(s);
}
