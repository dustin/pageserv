/*
 * Copyright (c) 1997  SPY Internetworking
 *
 * $Id: tap.c,v 2.26 1998/10/21 00:34:24 dustin Exp $
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stropts.h>
#include <sys/conf.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

static int tap_checksum(char *string);
static char *tap_sent_checksum(int sum);

static int
tap_checksum(char *string)
{
	int     i, sum = 0;

	for (i = 0; string[i] != 0; i++) {
		sum += string[i] - ((int) (string[i] / 128) * 128);
	}

	return (sum);
}

static char *
tap_sent_checksum(int sum)
{
	static char charsum[4] =
	{0, 0, 0, 0};

	charsum[2] = 48 + sum - (int) (sum / 16) * 16;
	sum = (int) (sum / 16);
	charsum[1] = 48 + sum - (int) (sum / 16) * 16;
	sum = (int) (sum / 16);
	charsum[0] = 48 + sum - (int) (sum / 16) * 16;

	return (charsum);
}

int
s_tap_init(int s, int flags)
{
	char    buf[BUFLEN];
	int     retry, ok;

	_ndebug(0, ("Initializing TAP\n"));

	if (flags & TAP_INITDELAY) {
		_ndebug(3, ("flag TAP_INITDELAY is set, sleeping\n"));

		sleep(1);
	}
	/* we won't do any of this if flag TAP_INITCR isn't set */
	if (flags & TAP_INITCR) {

		_ndebug(3, ("flag TAP_INITCR is set, doing CR dance\n"));
		usleep(500000);	/* Just to make sure everything's ready */

		ok = 0;
		for (retry = 0; retry < RETRY_1; retry++) {
			puttext(s, "\r");

			if (s_modem_waitfor(s, "ID=", TIME_2) < 0) {
				_ndebug(2, ("\t...timed out, sending another CR\n"));
				del_log("Timed out waiting for ID, retry #%d", retry);
			} else {
				ok = 1;
				break;
			}
		}

		if (!ok) {
			del_log("Too many init timeouts");
			return (-1);
		}
	}
	sprintf(buf, "%cPG1\r", C_ESC);
	puttext(s, buf);

	sprintf(buf, "%c[p", C_ESC);
	if (s_modem_waitfor(s, buf, TIME_3) < 0) {
		_ndebug(2, ("Error:  s_modem_waitfor(%d, \"%s\", %d);\n", s, buf, TIME_3));
		del_log("timed out waiting for TAP init reply");
		return (-1);
	}
	return (0);
}

int
s_tap_end(int s)
{
	char    buf[BUFLEN];

	_ndebug(0, ("ending TAP session\n"));

	sprintf(buf, "%c\r", C_EOT);
	puttext(s, buf);
	return (0);
}

static int
charfound(char *string, char *chars)
{
	int     i;

	for (i = 0; chars[i] != 0x00; i++) {
		if (strchr(string, chars[i]) != NULL) {
			return (chars[i]);
		}
	}
	return (0);
}

int
s_tap_send(int s, char *id, char *message)
{
	char    buf[BUFLEN];
	char    search[] =
	{C_ACK, C_ESC, C_NAK, C_RS, 0x00};
	char    c;
	int     i;

	_ndebug(0, ("Sending message to %s:\n%s\n", id, message));

	/* cheap bounds checking */
	if (strlen(message) + strlen(id) > (BUFLEN - 20)) {
		del_log("ACK!  Length of message+id is too long (%s:%d)!",
		    __FILE__, __LINE__);
		return (-1);
	}
	/* Probably this is important. */
	ioctl(s, I_FLUSH, FLUSHR);

	sprintf(buf, "%c%s%c%s%c%c", C_STX, id, C_CR, message, C_CR, C_ETX);

	/* Stick the checksum on the end */
	strcat(buf, do_checksum(buf));
	strcat(buf, "\r");

	/* It compiles, ship it. */
	puttext(s, buf);

	/* Lets see if we won. */

	c = 0;
	strcpy(buf, "");	/* clean out buf */
	while ((c = charfound(buf, search)) == 0) {
		_ndebug(2, ("attempting read from s\n"));
		i = read(s, buf, 1);
		if (i > 0) {
			buf[i] = 0x00;
			_ndebug(2, ("%s\n", buf));
		} else {
			/* Over so soon? */
			_ndebug(2, ("nothing returned that we like\n", buf));
			/* X == EOF */
			c = 'X';
			break;
		}
	}

	/* This may not be quite as important. */
	ioctl(s, I_FLUSH, FLUSHR);

	/* Another random sleepy thing, just to get the timing right */
	usleep(1700);

	/* I used to do escape here...  hmm...  */
	if (c == C_ACK) {
		_ndebug(2, ("Received character %xh.  Good.\n", c));
		/* Too debuggy */
		/* del_log("Received character %xh.  Good.\n", c); */
		return (0);
	} else {
		_ndebug(2, (":( Received character %xh\n", c));
		/* Too debuggy */
		/* del_log(":( Received character %xh\n", c); */
		return ((int) c);
	}
}
