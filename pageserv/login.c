/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: login.c,v 1.1 1997/04/10 06:23:44 dustin Exp $
 * $State: Exp $
 */

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

void p_login(int s)
{
    puttext(s, "Logins not *quite* supported yet.\n");
    exit(1);
}
