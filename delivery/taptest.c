/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: taptest.c,v 2.4 1997/04/01 22:29:55 dustin Exp $
 */

#include <stdio.h>

#include <tap.h>
#include <pageserv.h>

struct config conf;

void main(void)
{
    int s;

    conf.debug=3;

/*
    s=s_openhost("liv-lex-1.ipa.net.", 6001);
    s_modem_connect(s, "9,783.6426");
    puts("I got connected.  :)");
    s_tap_init(s);

    if( s_tap_send(s, "7888802", "This is a TAP test, please ignore") == 0)
	puts(":) Message was successful");
    else
	puts(":( Message was unsuccessful");

    if( s_tap_send(s, "7888802", "This is another TAP test, please ignore (i love you)") == 0)
	puts(":) Message was successful");
    else
	puts(":( Message was unsuccessful");

    s_tap_end(s);
    puts("Disco-necting");
    close(s);
*/

    sleep(5);

    s=s_openhost("liv-lex-1.ipa.net.", 6001);
    s_modem_connect(s, "9,1,501.219.1805");
    puts("I got connected.  :)");
    s_tap_init(s);

    if( s_tap_send(s, "3123362", "This is a TAP test.") == 0)
	puts(":) Message was successful");
    else
	puts(":( Message was unsuccessful");

    usleep(2600);

    if( s_tap_send(s, "3123362", "This is another TAP test.") == 0)
	puts(":) Message was successful");
    else
	puts(":( Message was unsuccessful");

    s_tap_end(s);
    puts("Disco-necting");
    close(s);
}
