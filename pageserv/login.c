/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: login.c,v 1.6 1997/04/11 22:58:06 dustin Exp $
 * $State: Exp $
 */

#include <config.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

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

/* real password, test password */
#define checkpass(a, b) (strcmp(a, (char *)crypt(b, a)) == 0)

void makesalt(char *s, int n, long v)
{
    char *c="./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    while(--n>=0)
    {
	*s++=c[v&0x3f];
	v>>=6;
    }
}

struct user setpasswd(struct user u, char *passwd)
{
    char salt[3]={0x00, 0x00, 0x00};
    time_t t;

    time(&t);
    srand(t);

    makesalt(salt, 2, rand());
    strcpy(u.passwd, (char *)crypt(passwd, salt));

    return(u);
}

int getynresult(int s, char *str)
{
    int flag;
    char buf[BUFLEN];

    switch(tolower(str[0]))
    {
	case 'y':
	    flag=1;
	    break;
	case 'n':
	    flag=2;
	    break;
	default:
	    sprintf(buf, "Um, I said y or n, you said %c.  \
Please hang up and try again\n", str[0]);
	    puttext(s, buf);
	    exit(0);
    }

    return(flag);
}

void login_usermain(int s, struct user u)
{
    char buf[BUFLEN];
    int times[2];
    int flag, changes=0, i;

    /* reset alarm, give him some time to get stuff done */
    alarm(conf.childlifetime);

    getnormtimes(u.times, times);

    sprintf(buf, "Your normal times are from %d to %d.\n", times[0], times[1]);
    puttext(s, buf);
    puttext(s, "Would you like to change this?  [yn]:  ");

    gettextcr(s, buf);
    flag=getynresult(s, buf);

    if(flag==1)
    {
	/* zero it out */
	u.times=0;

	puttext(s, "Earliest time to receive normal pages [0-23] (24 hour):  ");
	gettextcr(s, buf);
	sscanf(buf, "%d", &times[0]);

	puttext(s, "Latest time to receive normal pages [0-23] (24 hour):  ");
	gettextcr(s, buf);
	sscanf(buf, "%d", &times[1]);

	if(times[0]==times[1])
	{
	    puttext(s, "You will receive normal priority pages at any time\n");
	    u.times=0xFFFFFFFF;
	}
	else
	{
	    if(times[0]>0)
		times[0]--;
            else
		times[0]=23;

	    if(times[0]<times[1])
	    {
		for(i=times[0]; i<times[1] ; i++)
		{
		    u.times=set_bit(u.times, i);
		}
	    }
	    else
	    {
		for(i=times[0]; i<24 ; i++)
		{
		    u.times=set_bit(u.times, i);
		}

		for(i=0; i<times[1] ; i++)
		{
		    u.times=set_bit(u.times, i);
		}
	    } /* end of early >= late */
	}     /* end of specific time setting stuff */
	changes++;
    }         /* setting times */

    puttext(s, "Would you like to change your password?  [yn]:  ");
    gettextcr(s, buf);
    flag=getynresult(s, buf);

    if(flag==1)
    {
	puttext(s, "Enter new password here:  ");
	gettextcr(s, buf);

	u=setpasswd(u, buf);

	changes++;
    } /* password change */

    if(changes>0)
    {
	sprintf(buf, "There were %d changes, storing data.\n", changes);
	puttext(s, buf);
	storeuser(u);
    }
    else
    {
	puttext(s, "No changes made.\n");
    }

    puttext(s, "Thank you for shopping\n");
}

void p_login(int s)
{
    char buf[BUFLEN];
    struct user u;

    puttext(s, PROMPT_UN);
    gettextcr(s, buf);

    if(u_exists(buf))
    {
	u=getuser(buf);
    }
    else
    {
	puttext(s, MESG_NOUSER);
	exit(0);
    }

    if(strlen(u.passwd)<13)
    {
	puttext(s, "Password has not been setup for this account.\n");
	exit(0);
    }

    puttext(s, PROMPT_PW);
    gettextcr(s, buf);

    if(checkpass(u.passwd, buf))
    {
	login_usermain(s, u);
    }
    else
    {
	puttext(s, MESG_BADPASSWD);
	exit(0);
    }
}
