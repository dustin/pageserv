/*
 * Copyright (c) 1997 Dustin Sallings
 *
 * $Id: utility.c,v 1.10 1997/07/10 06:47:45 dustin Exp $
 * $State: Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <pageserv.h>

extern struct config conf;

int addtostr(int size, char *dest, char *str)
{
    int new=0;

    if(size==0)
    {
	if(conf.debug>5)
	    puts("Doing initial malloc");

	size=DEFAULT_STRLEN;
	dest=(char *)malloc(size*sizeof(char));
	if(dest==NULL)
	{
	    perror("malloc");
	    exit(1);
        }
	new=1;
    }

    if(strlen(dest)+strlen(str)>=size)
    {
	if(conf.debug>4)
	    printf("Realloc'in to %d bytes, need more than %d bytes\n",
		size<<1, size);

	size<<=1;
	realloc(dest, size*sizeof(char));
	if(dest==NULL)
	{
	    perror("realloc");
	    exit(1);
	}
    }

    if(new)
	strcpy(dest, str);
    else
        strcat(dest, str);

    return(size);
}

int pack_timebits(int early, int late)
{
    int t=0, i;
    int times[2];

    times[0]=early;
    times[1]=late;

    if( (times[0]==times[1])
	|| (early>23 || early < 0 || late >23 || late < 0) )
    {
        t=0xffffffff;
    }
    else
    {
        if(times[0]==0)
            times[0]=23;

        if(times[0]<times[1])
        {
            for(i=times[0]; i<times[1]; i++)
                t=set_bit(t, i);
        }
        else
        {
            for(i=times[0]; i<24; i++)
                t=set_bit(t, i);

            for(i=0; i<times[1]; i++)
                t=set_bit(t, i);
        }
    }

    return(t);
}

/* This is a function instead of a macro because as a macro it
 * forces all constant strings to be duplicated at compile time
 */

int puttext(int s, char *txt)
{
    write(s, txt, strlen(txt));
}

int gettext(int s, char *buf)
{
    int size;
    if( (size=read(s, buf, BUFLEN-1)) >0)
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
    int size=1, len=0;

    /* eat any extra CR's and LF's */
    while( (len=read(s, buf, 1)) >0)
    {
	if(buf[size-1]=='\r')
	{
	    size=0;
	    break;
	}
	else if(buf[size-1]=='\n')
	{
	    size=0;
            break;
	}
	else
	{
	    break;
	}
    }

    if(len==0)
    {
	if(conf.debug>0)
	    puts("Broken pipe?");
	exit(0);
    }

    while( (len=read(s, buf+size, 1)) >0)
    {
        if(len==0)
        {
	    if(conf.debug>0)
	        puts("Broken pipe?");
	    exit(0);
        }

	size+=len;
        buf[size]=0x00;
        if(buf[size-1]=='\r' || buf[size-1]=='\n')
            break;
    }

    kw(buf);

    if(conf.debug>1)
        printf("gettextcr() received:\n\t``%s''\n", buf);

    return(size);
}

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
