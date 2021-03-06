/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * "$Id: http.h,v 1.16 1998/01/10 01:32:50 dustin Exp $"
 */

#ifndef HTTP_H
#define HTTP_H 1

/* for NULL */
#include <stdio.h>
#include <module.h>

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
    "Content-length: ",
    "Authorization: Basic ",
    NULL
};

#endif

#define HTTP_CONTENTLENGTH  0
#define HTTP_CONTENTLENGTH2 1
#define HTTP_AUTHBASIC      2

struct http_list {
    char *name;
    char *value;
    struct http_list *next;
};

struct http_authinfo {
    char *name;
    char *pass;
    char *string;
    int  authtype;
};

struct http_request {
    char request[REQUESTSIZE];
    struct http_authinfo auth;
    char *args;
    struct http_list *largs;
    int nargs;
    int length;
    int version;
    int docnum;
    int special;
    int method;
};

/* functions */

char *_http_getcgiinfo(struct http_request r, char *what);
struct http_request http_parserequest(int s);
void _http_admin(int s, struct http_request r);
void _http_auth_require(int s, struct http_request r, char *authname);
void _http_error(int s,  struct http_request r);
void _http_footer(int s);
void _http_free_request(struct http_request r);
void _http_header(int s);
void _http_header_needauth(int s, char *authname, struct http_request r);
void _http_header_notfound(int s, struct http_request r);
void _http_header_ok(int s, int size);
void _http_init_request(struct http_request *r);
void _http_moduser(int s, struct http_request r);
void _http_parse(int s, struct http_request r);
void _http_parseargs(int s, struct http_request *r);
void _http_sendpage(int s, struct http_request r, modpass p);
void http_checkauth(int s, struct http_request r, char *path);
void http_process(int s, struct http_request r, modpass p);

#ifdef IWANTDOCINFO

static char *docnames[]={
    "/moduser",
    "/sendpage",
    "/admin"
};

#define DOC_MODUSER 0
#define DOC_SENDPAGE 1
#define DOC_ADMIN 2

#define NDOCS    3
#define NOTADOC  NDOCS

#endif /* IWANTDOCINFO */

#define DEFAULT_EARLY 7
#define DEFAULT_LATE  22

#endif /* HTTP_H */
