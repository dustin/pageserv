/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: serial.c,v 2.21 2000/09/12 17:37:28 dustin Exp $
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
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#endif

#ifndef PATH_LOCKS
#define PATH_LOCKS "/var/spool/locks/"
#endif

extern struct config conf;

extern int errno;

static int p_lock(char *dev);
static int p_openport(char *port);

void
checklocks(void)
{
	DIR    *dir;
	FILE   *f;
	struct dirent *d;
	int     i, pid;
	char    path[80];

	pid = getpid();

	dir = opendir(PATH_LOCKS);

	while ((d = readdir(dir)) != NULL) {
		strcpy(path, PATH_LOCKS);
		strcat(path, d->d_name);

		if ((f = fopen(path, "r")) != NULL) {
			fscanf(f, "%d", &i);

			if (i == pid) {
				_ndebug(2, ("Found a lockfile:  %s\n", path));

				if (unlink(path) < 0) {
					_ndebug(1, ("Couldn't delete lockfile:  %s\n", path));
				}
			}
			fclose(f);
		}
	}
	closedir(dir);
}

int
p_unlock(char *dev)
{
	char    lock[50];
	char    lockfile[80];

	if (strncmp(dev, "/dev/", 5) == 0)
		strcpy(lock, dev + 5);
	else
		strcpy(lock, dev);

	strcpy(lockfile, PATH_LOCKS);
	strcat(lockfile, "LCK..");
	strcat(lockfile, lock);

	_ndebug(2, ("lockfile is %s, killin' it\n", lockfile));

	return (unlink(lockfile));
}

/* Return -1 if we can't lock */
static int
p_lock(char *dev)
{
	FILE   *f;
	char    lock[50];
	char    lockfile[80];
	int     lock_status, ret = -1;

	if (strncmp(dev, "/dev/", 5) == 0)
		strcpy(lock, dev + 5);
	else
		strcpy(lock, dev);

	strcpy(lockfile, PATH_LOCKS);
	strcat(lockfile, "LCK..");
	strcat(lockfile, lock);

	_ndebug(2, ("lockfile is %s\n", lockfile));

	lock_status = checkpidfile(lockfile);

	switch (lock_status) {
	case PID_ACTIVE:
		_ndebug(2, ("Active lockfile found.\n"));
		ret = -1;
		break;
	case PID_NOT_OWNER:
		_ndebug(2, ("Found a valid lockfile, but I'm not the owner.\n"));
		ret = -1;
		break;
	case PID_STALE:
		_ndebug(2, ("Found stale lockfile.\n"));
		if(unlink(lockfile)<0) {
			/* We can't pass through if we couldn't unlink */
			return(-1);
		}
		/* pass through */
	case PID_NOFILE:
		if ((f = fopen(lockfile, "w")) == NULL) {
			_ndebug(2, ("Cannot create lockfile.\n"));
			ret = -1;
		}
		fprintf(f, "%10d\n", (int) getpid());

		fclose(f);
		ret = 0;
		break;
	default:
		ret = -1;
	}

	return (ret);
}

int
p_openterm(struct terminal t)
{
	char    buf[BUFLEN];
	int     s;

	strcpy(buf, t.predial);
	strcat(buf, t.number);
	if ((s = p_openport(t.ts)) >= 0) {
		if (s_modem_connect(s, buf) < 0) {
			close(s);
			p_unlock(t.ts);
			s = -1;
		}
	}
	return (s);
}

static int
p_openport(char *port)
{
	int     s=0;
	struct termios tm;
	int flags=0;

	_ndebug(3, ("Called p_openport(%s);\n", port));

	if (p_lock(port) < 0) {
		_ndebug(2, ("p_openport():  Resource is locked.\n"));
		return (-1);
	}
#ifdef O_NONBLOCK
	s = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
#else
	s = open(port, O_RDWR | O_NOCTTY, 0);
#endif

	if (s < 0) {
		_ndebug(2, ("Error opening device:  errno: %d\n", errno));
		return (-1);
	}
	_ndebug(2, ("Serial port attached for ``%s'', fd %d\n", port, s));

	if (tcgetattr(s, &tm) < 0)
		perror("tcgetattr");

	if (cfsetispeed(&tm, B38400) == -1)
		perror("cfsetispeed");
	if (cfsetospeed(&tm, B38400) == -1)
		perror("cfsetospeed");

	tm.c_cc[VMIN] = 1;
	tm.c_cc[VTIME] = 0;

	tm.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
	    INLCR | IGNCR | ICRNL | IXON | ICANON);
	tm.c_iflag |= (INPCK | IGNBRK | ISTRIP | IXON | IXOFF);

	tm.c_oflag = 0;
	tm.c_lflag = 0;

	tm.c_cflag &= ~(CSTOPB | CSIZE | PARODD);
	tm.c_cflag |= (CREAD | CS7 | PARENB | CLOCAL);

	if (tcflush(s, TCIFLUSH) == -1)
		del_log("tcflush: %s", strerror(errno));

	if (tcsetattr(s, TCSANOW, &tm))
		del_log("tcsetattr", strerror(errno));

/* If we opened it non-blocking, we've gotta disable blocking. */
#ifdef O_NONBLOCK
	flags = fcntl(s, F_GETFL);
	if(fcntl(s, F_SETFL, flags & ~O_NONBLOCK)<0) {
		del_log("Could not disable NON_BLOCKING I/O, will continue anyway.");
	}
#endif

	return (s);
}
