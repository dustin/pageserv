/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: checksum.c,v 1.3 1997/03/29 20:36:12 dustin Exp $
 */

#include <stdio.h>

#include "tap.h"

int tap_checksum(char *string)
{
    int tmp, i, sum=0;

    for(i=0; string[i]!=0; i++)
    {
	printf("Got char %d\n", (int)string[i]);
	tmp=string[i] - ( (int)(string[i]/128) * 128);
	sum+=tmp;
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

void main(void)
{
    char data[]={2, 49, 50, 51, 13, 65, 66, 67, 13, 3, 0};
    int sum=0;

    sum=do_checksum(data);
    printf("The sum is %d\n", sum);

    printf("Transmit is %s\n", sent_checksum(sum));
}
