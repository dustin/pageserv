/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: serial.c,v 2.1 1997/06/19 08:24:34 dustin Exp $
 * $State: Exp $
 */

/*
 * Client delivery local serial code
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

int p_openterm(struct terminal t)
{
    char buf[BUFLEN];
    int s;

    strcpy(buf, t.predial);
    strcat(buf, t.number);
    s=p_openport(t.ts);
    s_modem_connect(s, buf);
    return(s);
}

int p_openport(char *port)
{
    int s;
    struct termios tm, tm_old;

    s=open(port, O_RDWR|O_NOCTTY, 0);
    if(conf.debug>2)
	printf("Serial port attached, fd %d\n", s);

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
