/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: login.c,v 1.4 1997/04/11 07:12:16 dustin Exp $
 * $State: Exp $
 */

#include <config.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pageserv.h>

extern struct config conf;

/* real password, test password */
#define checkpass(a, b) (strcmp(a, crypt(b, a)) == 0)

void p_login(int s)
{
    char buf[BUFLEN];
    struct user u;

    puttext(s, PROMPT_UN);
    gettextcr(s, buf);

    if(u_exists(buf))
    {
	u=getuser(buf);
    }
    else
    {
	puttext(s, MESG_NOUSER);
	exit(0);
    }

    if(strlen(u.passwd)<13)
    {
	puttext(s, "Password has not been setup for this account.\n");
	exit(0);
    }

    puttext(s, PROMPT_PW);
    gettextcr(s, buf);

    if(checkpass(u.passwd, buf))
    {
	puttext(s, "Good password\n");
    }
    else
    {
	puttext(s, MESG_BADPASSWD);
	exit(0);
    }
}
