/*
 * Copyright (c) 1997 Dustin Sallings
 *
 * $Id: utility.c,v 1.1 1997/03/09 22:56:02 dustin Exp $
 */

#include <ctype.h>

/* kill whitey, eat all the whitespace on the end of a string */

char *kw(char *in)
{
    /* bounds checking */
    if(strlen(in)==0)
        return(in);

    while(isspace(in[strlen(in)-1]))
    {
	/* bounds checking */
        if(strlen(in)==0)
            return(in);

	in[strlen(in)-1]=0x00;
    }

    return(in);
}
