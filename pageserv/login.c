/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: login.c,v 1.3 1997/04/11 03:52:37 dustin Exp $
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
