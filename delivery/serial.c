/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: serial.c,v 2.12 1998/02/27 04:30:59 dustin Exp $
 */

/*
 * Client delivery local serial code
 */

#include <pageserv.h>
#include <tap.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#else
# ifdef HAVE_TERMIOS_H
#  include <termios.h>
# endif
#endif

#ifndef PATH_LOCKS
#define PATH_LOCKS "/var/spool/locks/"
#endif

extern struct config conf;

static int p_lock(char *dev);
static int p_openport(char *port);

void checklocks(void)
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    int i, pid;
    char path[80];

    pid=getpid();

    dir=opendir(PATH_LOCKS);

    while( (d=readdir(dir))!=NULL)
    {
	strcpy(path, PATH_LOCKS);
	strcat(path, d->d_name);

	if( (f=fopen(path, "r"))!=NULL)
	{
	    fscanf(f, "%d", &i);

	    if(i==pid)
	    {
		_ndebug(2, ("Found a lockfile:  %s\n", path));

		if(unlink(path)<0)
		{
		    _ndebug(1, ("Couldn't delete lockfile:  %s\n", path));
		}
	    }

	    fclose(f);
	}
    }
    closedir(dir);
}

int p_unlock(char *dev)
{
    char lock[50];
    char lockfile[80];

    if(strncmp(dev, "/dev/", 5)==0)
	strcpy(lock, dev+5);
    else
	strcpy(lock, dev);

    strcpy(lockfile, PATH_LOCKS);
    strcat(lockfile, "LCK..");
    strcat(lockfile, lock);

    _ndebug(2, ("lockfile is %s, killin' it\n", lockfile));

    return(unlink(lockfile));
}

static int p_lock(char *dev)
{
    FILE *f;
    char lock[50];
    char lockfile[80];

    if(strncmp(dev, "/dev/", 5)==0)
	strcpy(lock, dev+5);
    else
	strcpy(lock, dev);

    strcpy(lockfile, PATH_LOCKS);
    strcat(lockfile, "LCK..");
    strcat(lockfile, lock);

    _ndebug(2, ("lockfile is %s\n", lockfile));

    if(access(lockfile, F_OK)==0)
    {
	_ndebug(2, ("Port is already locked.\n"));
	return(-1);
    }
    else
    {
	if( (f=fopen(lockfile, "w")) == NULL)
	{
	    _ndebug(2, ("Cannot create lockfile.\n"));
	    return(-1);
	}

	fprintf(f, "%10d\n", (int)getpid());

	fclose(f);
	return(0);
    }

    /*NOTREACHED*/
    return(-1);
}

int p_openterm(struct terminal t)
{
    char buf[BUFLEN];
    int s;

    strcpy(buf, t.predial);
    strcat(buf, t.number);
    if( (s=p_openport(t.ts))>=0)
    {
        if( s_modem_connect(s, buf) < 0)
	{
	    close(s);
            p_unlock(t.ts);
	    s=-1;
	}
    }

    return(s);
}

static int p_openport(char *port)
{
    int s;
    struct termios tm;

    _ndebug(3, ("Called p_openport(%s);\n", port));

    if(p_lock(port)<0)
    {
	_ndebug(2, ("p_openport():  Resource is locked.\n"));

	return(-1);
    }
    s=open(port, O_RDWR|O_NOCTTY, 0);

    _ndebug(2, ("Serial port attached for ``%s'', fd %d\n", port, s));

    if(tcgetattr(s, &tm)<0)
	perror("tcgetattr");

    if (cfsetispeed(&tm, B2400) == -1)
	perror("cfsetispeed");
    if (cfsetospeed(&tm, B2400) == -1)
	perror("cfsetospeed");

    tm.c_cc[VMIN]=1;
    tm.c_cc[VTIME]=0;

    tm.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
	INLCR | IGNCR | ICRNL | IXON | ICANON);
    tm.c_iflag |= (INPCK | IGNBRK | ISTRIP | IXON | IXOFF);

    tm.c_oflag=0;
    tm.c_lflag=0;

    tm.c_cflag &= ~(CSTOPB | CSIZE | PARODD);
    tm.c_cflag |= (CREAD | CS7 | PARENB | CLOCAL);

    if (tcflush(s, TCIFLUSH) == -1)
	perror("tcflush");

    if (tcsetattr(s,TCSANOW, &tm))
	perror("tcsetattr");

    return(s);
}
