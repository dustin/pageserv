/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: module.h,v 1.1 1997/04/16 06:10:38 dustin Exp $
 */

#ifndef _MODULE_H
#define _MODULE_H

typedef struct {
    int socket;
} modpass;

typedef struct {
    void (*init)(void);
    void (*handler)(modpass in);

    int (*socket)(void);

    char *name;

    int listening;
    int s;
} module;

#endif
