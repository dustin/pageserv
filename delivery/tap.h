/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * "$Id: tap.h,v 1.1 1997/03/29 19:21:30 dustin Exp $"
 */

#ifndef TAP_H
#define TAP_H 1

/* char defs */

#define C_STX 2
#define C_ETX 3
#define C_CR  13

int do_checksum(char *string);
void revstring(char *string);
char *sent_checksum(int sum);

#endif /* TAP_H */
