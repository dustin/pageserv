/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: module.h,v 1.2 1997/08/09 06:35:36 dustin Exp $
 */

#ifndef _MODULE_H
#define _MODULE_H

#include <netinet/in.h>

typedef struct {
    int socket;
    struct sockaddr_in fsin;
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
