/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * "$Id: snpp.h,v 1.7 1998/01/20 04:29:41 dustin Exp $"
 */

#ifndef SNPP_H
#define SNPP_H 1

/* for NULL */
#include <stdio.h>
#include <sys/types.h>
#include <module.h>

#define SNPP_MAXTRIES 250
#define SNPP_NID      50

/* Text crap */
#define LOGINOK       "250 Login Accepted\n"
#define ILLEGALLOGIN  "421 Illegal Access Attempt\n"
#define INVALIDLOGIN  "550 Error, Invalid LoginID or Password\n"

/* methods defs */
#define SNPP_PAGE    0
#define SNPP_MESS    1
#define SNPP_RESE    2
#define SNPP_SEND    3
#define SNPP_QUIT    4
#define SNPP_HELP    5
#define SNPP_DATA    6
#define SNPP_LOGI    7
#define SNPP_LEVE    8
#define SNPP_ALER    9
#define SNPP_COVE   10
#define SNPP_HOLD   11
#define SNPP_CALL   12
#define SNPP_SUBJ   13
#define SNPP_2WAY   14
#define SNPP_PING   15
#define SNPP_EXPT   16
#define SNPP_NOQUEUE 17
#define SNPP_ACKR   18
#define SNPP_RTYP   19
#define SNPP_MCRE   20
#define SNPP_MSTA   21
#define SNPP_KTAG   22
#define SNPP_PRIORITY 23
#define SNPP_SHOWQ  24

#ifdef IWANTMETHODNAMES

static char *methodnames[]={
    "PAGE",
    "MESS",
    "RESE",
    "SEND",
    "QUIT",
    "HELP",
    "DATA",
    "LOGI",
    "LEVE",
    "ALER",
    "COVE",
    "HOLD",
    "CALL",
    "SUBJ",
    "2WAY",
    "PING",
    "EXPT",
    "NOQUEUE",
    "ACKR",
    "RTYP",
    "MCRE",
    "MSTA",
    "KTAG",
/* The following are not standard SNPP commands */
    "PRIORITY",
    "SHOWQ",
    NULL
};

#endif

char *snpp_arg(char *s);
char *snpp_login(int s, char *what);
int snpp_parse(char *cmd);
void snpp_help(int s);
void snpp_showUserQ(int s, char *user, char *arg);

#endif /* SNPP_H */
