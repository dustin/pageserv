/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * "$Id: http.h,v 1.9 1997/04/17 20:31:18 dustin Exp $"
 */

#ifndef HTTP_H
#define HTTP_H 1

/* for NULL */
#include <stdio.h>

/* macros */
#define REQUESTSIZE 1024

#define HTTP_FOOTER "<p><hr width=\"50%\" align=\"left\">\n\
<font size=\"-2\">Copyright &copy;  1997  Dustin Sallings</font>\n\
</body></html>\n"

/* methods defs */
#define HTTP_GET     0
#define HTTP_PUT     1
#define HTTP_POST    2
#define HTTP_HEAD    3
#define HTTP_DELETE  4
#define HTTP_TRACE   5
#define HTTP_OPTIONS 6

#ifdef IWANTMETHODNAMES
static char *methodnames[]={
    "GET",
    "PUT",
    "POST",
    "HEAD",
    "DELETE",
    "TRACE",
    "OPTIONS",
    NULL
};

static char *miscnames[]={
    "Content-Length: ",
    NULL
};

#endif

#define HTTP_CONTENTLENGTH 0

#ifdef IWANTDOCINFO

static char *docnames[]={
    "/usermod",
    "/sendpage"
};

#define DOC_USERMOD 0
#define DOC_SENDPAGE 1

#define NDOCS    2
#define NOTADOC  NDOCS

#endif /* IWANTDOCINFO */

struct http_list {
    char *name;
    char *value;
    struct http_list *next;
};

struct http_request {
    char request[REQUESTSIZE];
    char args[REQUESTSIZE];
    struct http_list *largs;
    int length;
    int version;
    int docnum;
    int special;
    int method;
};

/* functions */

int  _http_socket(void);
struct http_request http_parserequest(int s);
void _http_init(void);
void _http_main(modpass p);
void http_error(int s, struct http_request r);
void http_footer(int s);
void http_header_notfound(int s, struct http_request r);
void http_header_ok(int s);
void http_process(int s, struct http_request r);
void http_process_get(int s, struct http_request r);

#endif /* HTTP_H */
