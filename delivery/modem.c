/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: modem.c,v 2.17 1998/03/11 07:18:17 dustin Exp $
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <tap.h>
#include <pageserv.h>

extern struct config conf;

int any_closeterm(int s, struct terminal t)
{
    puttext(s, "+++atz\n");
    close(s);
    sleep(5);
    if(p_unlock(t.ts)<0)
    {
	_ndebug(2, ("Weird, didn't get the lock back for %s\n", t.ts));
    }
    return(0);
}

int any_openterm(struct terminal term)
{
    int s=-1;

    if(term.ts[0]=='/')
        s=p_openterm(term);
    else
        s=s_openterm(term);

    _ndebug(2, ("Looks like the modem for today will be %d\n", s));

    return(s);
}

/*
 * This won't be very accurate, but it'll serve the purpose.
 */

int _waitondesc(int fd, int timeout)
{
    fd_set fdset, tfdset;
    struct timeval t;
    int ret;
    time_t start;

    FD_ZERO(&tfdset);
    FD_SET(fd, &tfdset);

    fdset=tfdset;

    t.tv_sec=timeout;
    t.tv_usec=0;

    start=time(NULL);
    if( select(fd+1, &fdset, NULL, NULL, &t) > 0)
    {
	_ndebug(5, ("_waitondesc got some, recalculating timeout\n"));
        ret=time(NULL)-start;
    }
    else
    {
	_ndebug(5, ("_waitondesc timed out\n"));
	ret=timeout;
    }

    return(ret);
}

int s_modem_waitforchar(int s, char what, int timeout)
{
    char c;

    do
    {
	timeout-=_waitondesc(s, timeout);
	if(timeout<1)
	{
	    _ndebug(2, ("s_modem_waitfor timed out, returning -1\n"));
	    return(-1);
        }

        if(read(s, &c, 1)<=0)
            return(-1);
	_ndebug(2, ("%c", c));
    }
    while(c!=what);
    return(0);
}

int s_modem_waitfor(int s, char *what, int timeout)
{
    int i, size;
    char c;

    i=0;

    while(i < (int)strlen(what))
    {
	timeout-=_waitondesc(s, timeout);
	if(timeout<1)
	{
	    _ndebug(2, ("s_modem_waitfor timed out, returning -1\n"));
	    return(-1);
	}

	size=read(s, &c, 1);
	if(size>0)
	{
	    _ndebug(2, ("%c", c));

	    if(c==what[i])
	        i++;
	    else
	        i=0;
	}
	else
	{
	    return(-1);
	}
    }
    return(0);
}

int dexpect(int s, char **what, int timeout)
{
    int which, size, i[1024], found;
    char c;

    if(what==NULL || what[0]==NULL)
        return(-1);

    for(which=0; what[which]; which++) i[which]=0;

    found=-1;

    while(found<0)
    {
        timeout-=_waitondesc(s, timeout);
        if(timeout<1)
        {
            _ndebug(2, ("dexpect timed out, returning -1\n"));
            return(-1);
        }

	_ndebug(5, ("Timeout is now %d\n", timeout));

        size=read(s, &c, 1);
        if(size>0)
        {
            _ndebug(2, ("%c", c));

            for(which=0; what[which]!=NULL; which++)
            {
                _ndebug(5, ("Checking %s\n", what[which]));
                if(c==what[which][i[which]])
                {
                    _ndebug(4, ("dexpect found another character in %s (%d)\n",
                                what[which], i[which]));
                    i[which]++;

                    if(i[which]==strlen(what[which]))
                    {
                        _ndebug(3, ("dexpect found %s\n", what[which]));
                        found=which;
                    }
                }
                else
                {
                    i[which]=0;
                }
            }
        }
        else
        {
            return(-1);
        }
    }
    return(found);
}

/*
 * Dial and connect to ``number'' over file descriptor ``s''
 */
int s_modem_connect(int s, char *number)
{
    char buf[BUFLEN];
    int i, dialtimeout;
    char *constr[]={
	"CONNECT",
	"BUSY",
	"NO CARRIER",
	"NO DIALTONE",
	NULL
    };

    dialtimeout=rcfg_lookupInt(conf.cf, "tuning.modemDialTimeout");
    if(dialtimeout==0)
	dialtimeout=120;

    i=puttext(s, "atz\r\n");
    _ndebug(3, ("Wrote %d bytes\n", i));

    i=s_modem_waitfor(s, "OK", 10);
    if(i<0)
    {
	_ndebug(2, ("init failed\n"));
	return(-1);
    }

    /* Take a nap before trying to write again */

    usleep(2600);

    sprintf(buf, "atdt%s\r\n", number);
    i=puttext(s, buf);
    _ndebug(3, ("Wrote %d bytes\n", i));

    i=dexpect(s, constr, dialtimeout);
    if(i!=0)
    {
	_ndebug(2, ("connect failed: %s\n", i>0?constr[i]:"timeout"));
	return(-1);
    }

    usleep(2600);

    return(0);
}
