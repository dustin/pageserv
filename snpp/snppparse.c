/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: snppparse.c,v 1.1 1997/04/29 05:14:25 dustin Exp $
 */

#define IWANTMETHODNAMES
#include <snpp.h>

#include <string.h>

int snpp_parse(char *cmd)
{
    int i, ret=-1;
    for(i=0; methodnames[i]!=NULL; i++)
    {
	if(strncasecmp(cmd, methodnames[i], strlen(methodnames[i]))==0)
	{
	    ret=i;
	    break;
	}
    }

    return(ret);
}
