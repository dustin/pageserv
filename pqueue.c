/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: pqueue.c,v 1.3 1997/03/26 07:26:17 dustin Exp $
 */

#include "pageserv.h"

struct config conf;

void main(void)
{
    readconfig(CONFIGFILE);
    printqueue();
    cleanconfig();
}
