/*
 * Copyright (c) 1996-1998  Dustin Sallings
 *
 * $Id: snppclient.h,v 1.4 1998/10/27 18:31:36 dustin Exp $
 */

#ifndef _SNPP_CLIENT_H
#define _SNPP_CLIENT_H 1

#include <config.h>

#undef _ndebug
#define _ndebug(a, b) if(snpp->debug>=a) printf b

#define SNPP_BUF_LEN 4096  /* This number cannot be smaller than 16 or so */
#define SNPP_BUF_THRESH (SNPP_BUF_LEN*4) /* shrink it if it gets this big */

#ifndef SNPP_PORT
# define SNPP_PORT 1031
#endif

/* HOLDuntil flags */
#define SNPP_COMPAT 1 /* YYMMDDHHMMSS[+/-GMT] */

/* Client structure */

struct snpp_client {
    int debug;

    char *hostname;
    int port;
    int socket;
    int status;

    /* Read crap, stolen from my IMAP library */

    struct {
	int buf_begin;
	int buf_current;
	int buf_end;
	int buf_size;
	char *buffer;
    } indata;

    /* ``member functions'' */

    int (*reset)(struct snpp_client *snpp);
    void (*quit)(struct snpp_client *snpp);
    void (*destroy)(struct snpp_client *snpp);

    void (*rawsend)(struct snpp_client *snpp, char *what);
    void (*rawsend2)(struct snpp_client *snpp, char *what, char *whatelse);

    int (*page)(struct snpp_client *snpp, char *whom);
    int (*message)(struct snpp_client *snpp, char *message);
    int (*send)(struct snpp_client *snpp);
	int (*hold)(struct snpp_client *snpp, time_t when, int flags);

    int (*sendAPage)(struct snpp_client *snpp, char *towhom, char *message);

};

/* The thing the outside world sees */
struct snpp_client *snpp_connect(char *hostname, int port);
char *_killwhitey(char *what);

#if !defined(HAVE_SNPRINTF)
int snprintf(char *s, size_t n, const char *format, ...);
#endif

#endif /* _SNPP_CLIENT_H */
