/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: checksum.c,v 1.4 1997/03/30 01:34:53 dustin Exp $
 */

#include <stdio.h>

#include "tap.h"

int tap_checksum(char *string)
{
    int i, sum=0;

    for(i=0; string[i]!=0; i++)
    {
	sum+=string[i] - ( (int)(string[i]/128) * 128);
    }

    return(sum);
}

char *tap_sent_checksum(int sum)
{
    static char charsum[4]={0,0,0,0};

    charsum[2]=48+ sum - (int)(sum/16) * 16;
    sum = (int)(sum/16);
    charsum[1]=48+ sum - (int)(sum/16) * 16;
    sum = (int)(sum/16);
    charsum[0]=48+ sum - (int)(sum/16) * 16;

    return(charsum);
}
