/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * "$Id: tap.h,v 1.9 1998/07/15 07:55:14 dustin Exp $"
 */

#ifndef TAP_H
#define TAP_H 1

#include <stdio.h>
#include <string.h>

/* This really should be defined somewhere... */
extern int errno;

/* macros */

/* because I don't want to make a function to do this */
#define do_checksum(a) tap_sent_checksum(tap_checksum(a))

/* char defs */

#define C_STX 0x02
#define C_ETX 0x03
#define C_EOT 0x04
#define C_ACK 0x06
#define C_CR  0x0d
#define C_NAK 0x15
#define C_ESC 0x1b
#define C_RS  0x1e
#define C_US  0x1f

/* Response codes */

#define RES_DOTAP       110 /* TAP is supported */
#define RES_PROCESSING  111 /* Processing previous input, hold please */
#define RES_MAXPAGE     112 /* Maximum pages per session entered */
#define RES_MAXTIME     113 /* Maximum session time reached */
#define RES_BANNER      114 /* Welcome banner */
#define RES_EXITBAN     115 /* exit messages */
#define RES_SUCCESS     211 /* page sent successfully */
#define RES_TRUNC       212 /* Message truncated, too long */
#define RES_DEFER       213 /* Message accepted, but delivery deferred */
#define RES_TRUNC2      214 /* truncated and sent */
#define RES_TIMEOUT     501 /* Timeout occured waiting for input */
#define RES_UNEXP       502 /* unexpected characters received */
#define RES_EXCSUM      503 /* excessive checksum errors */
#define RES_BADFORMAT   504 /* not allowed for the pager format */
#define RES_NOALPHA     505 /* Not an alpha pager */
#define RES_EXCINV      506 /* excessive invalid pages received */
#define RES_INVLOGSEQ   507 /* invalid login sequence */
#define RES_INVLOGCAT   508 /* invalid login service type and category */
#define RES_INVPAS      509 /* password incorrect */
#define RES_ILLPAGID    510 /* illegal pager ID */
#define RES_INVPAGID    511 /* invalid pager id (no subscriber) */
#define RES_TEMPUNAVAIL 512 /* termporarily unavailable, try again later */
#define RES_TOOLONG     513 /* message too long, rejected */
#define RES_CHECKSUM    514 /* message checksum error */
#define RES_FORMAT      515 /* message format error */
#define RES_QUOTA       516 /* message quota exceeded */
#define RES_TOOLONG2    517 /* rejected for length */

/* tuning params */

#define TIME_1 2
#define TIME_2 1
#define TIME_3 10
#define TIME_4 4
#define TIME_5 8

#define RETRY_1 3
#define RETRY_2 3
#define RETRY_3 3

/* misc */

/* if there's a new rts/cts, let's use it */
#ifdef CNEW_RTSCTS
#define CRTSCTS CNEW_RTSCTS
#endif

/* Flags */
#define TAP_INITCR    0x1
#define TAP_INITDELAY 0x2

/* functions */

int s_modem_connect(int s, char *number);
int s_modem_waitfor(int s, char *what, int timeout);
int s_modem_waitforchar(int s, char what, int timeout);
int s_tap_end(int s);
int s_tap_init(int s, int flags);
int s_tap_send(int s, char *id, char *message);

#endif /* TAP_H */
