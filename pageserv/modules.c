/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modules.c,v 1.1 1997/04/16 06:10:52 dustin Exp $
 */

#include <pageserv.h>
#include <module.h>

#include <stdlib.h>

extern struct config conf;

extern module mod_webserv;

void initmodules(void)
{
    int i;

    module *m[]={
        &mod_webserv,
        NULL
    };
    module *tmp;

    for(i=0; m[i]; i++);

    tmp=(module *)malloc( sizeof(module)*(i+1) );

    for(i=0; m[i]; i++)
	memcpy(&tmp[i], m[i], sizeof(module));

    conf.nmodules=i;
    conf.modules=tmp;
}
