/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: serial.c,v 2.3 1997/06/22 07:56:39 dustin Exp $
 * $State: Exp $
 */

/*
 * Client delivery local serial code
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>

#include <pageserv.h>
#include <tap.h>

#ifndef PATH_LOCKS
#define PATH_LOCKS "/var/spool/locks/"
#endif

extern struct config conf;

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
		if(conf.debug>2)
		    printf("Found a lockfile:  %s\n", path);

		unlink(path);
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

    if(conf.debug>2)
	printf("lockfile is %s, killin' it\n", lockfile);

    unlink(lockfile);
    return(0);
}

int p_lock(char *dev)
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

    if(conf.debug>2)
	printf("lockfile is %s\n", lockfile);

    if(access(lockfile, F_OK)==0)
    {
	if(conf.debug>2)
	    puts("Port is already locked.");
	return(-1);
    }
    else
    {
	if( (f=fopen(lockfile, "w")) == NULL)
	{
	    if(conf.debug>2)
		perror("Cannot create lockfile");
	    return(-1);
	}

	fprintf(f, "%10d\n", getpid());

	fclose(f);
	return(0);
    }

    return(-1); /* this will never happen */
}

int p_openterm(struct terminal t)
{
    char buf[BUFLEN];
    int s;

    strcpy(buf, t.predial);
    strcat(buf, t.number);
    if( (s=p_openport(t.ts))>=0)
        s_modem_connect(s, buf);

    return(s);
}

int p_openport(char *port)
{
    int s;
    struct termios tm, tm_old;

    if(conf.debug>3)
	printf("Called p_openport(%s);\n", port);

    if(p_lock(port)<0)
    {
	if(conf.debug>2)
	    puts("p_openport():  Resource is locked.");

	return(-1);
    }
s=open(port, O_RDWR|O_NOCTTY, 0);

    if(conf.debug>2)
	printf("Serial port attached for ``%s'', fd %d\n", port, s);

    if(tcgetattr(s, &tm)<0)
	perror("tcgetattr");

    tm_old=tm;

    tm.c_cc[VMIN]=1;
    tm.c_cc[VTIME]=0;

    tm.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
	INLCR | IGNCR | ICRNL | IXON | ICANON);
    tm.c_iflag |= (IGNBRK | IGNPAR | ISTRIP);

    tm.c_oflag=0;
    tm.c_lflag=0;

    tm.c_cflag &= ~(CSTOPB | CSIZE | PARODD | CLOCAL);
    tm.c_cflag |= (HUPCL | CREAD | CS7 | PARENB);

    if (cfsetispeed(&tm, B2400) == -1)
	perror("cfsetispeed");
    if (cfsetospeed(&tm, B2400) == -1)
	perror("cfsetospeed");

    if (tcflush(s, TCIFLUSH) == -1)
	perror("tcflush");

    if (tcsetattr(s,TCSANOW, &tm))
	perror("tcsetattr");

    return(s);
}
