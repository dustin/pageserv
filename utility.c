/*
 * Copyright (c) 1997 Dustin Sallings
 *
 * $Id: utility.c,v 1.3 1997/03/12 16:02:26 dustin Exp $
 */

#include <ctype.h>
#include <unistd.h>

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

int f_exists(char *file)
{
    return(access(file, F_OK)==0);
}

int bit_set(int map, int bit)
{
    map<<=bit;
    map&=1;

    return(map);
}

int set_bit(int map, int bit)
{
    int blah;

    blah=1;
    blah<<=bit;

    printf("%2d:  map is %x blah is %x ored is %x\n", bit, map, blah, map|blah);
    map|=blah;

    return(map);
}
