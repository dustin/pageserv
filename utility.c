/*
 * Copyright (c) 1997 Dustin Sallings
 *
 * $Id: utility.c,v 1.6 1997/03/30 01:32:17 dustin Exp $
 */

#include <ctype.h>
#include <unistd.h>

#include "pageserv.h"

extern struct config conf;

/* kill whitey, eat all the whitespace on the end of a string */

int gettext(int s, char *buf)
{
    int size;
    if( (size=recv(s, buf, BUFLEN-1, 0)) >0)
    {
        buf[size]=0x00;
        kw(buf);
        return(size);
    }
    else
    {
        /* Pipe breaking bastard */
        exit(0);
    }

    if(conf.debug>1)
        printf("gettext() received:\n\t``%s''\n", buf);

    return(size);
}

int gettextcr(int s, char *buf)
{
    int size=1;

    /* eat any extra CR's and LF's */
    while( (recv(s, buf, 1, 0)) >0)
    {
        if(buf[size-1]!='\r' && buf[size-1]!='\n')
            break;
    }

    while( (size+=recv(s, buf+size, 1, 0)) >0)
    {
        buf[size]=0x00;
        if(buf[size-1]=='\r' || buf[size-1]=='\n')
            break;
    }

    kw(buf);

    if(conf.debug>1)
        printf("gettextcr() received:\n\t``%s''\n", buf);

    return(size);
}


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
    map>>=bit;
    map&=1;

    return(map);
}

int set_bit(int map, int bit)
{
    int blah;

    blah=1;
    blah<<=bit;

    map|=blah;

    return(map);
}
