%{
/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: lexer.l,v 1.11 1998/01/10 01:32:43 dustin Exp $
 */

#include <config.h>
#include <pageserv.h>
#include <http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

static int _parse_sock;
static struct http_request _parse_r;
static int _parse_strlen=0;
static char *_parse_sendstr;
static void _http_lex_dofunc(char *func);
static void _http_lex_showthing(char *thing);

%}

THING   $[_A-Za-z][_A-Za-z0-9]*
FUNC    &[_A-Za-z][_A-Za-z0-9]*

%%
{THING} { _http_lex_showthing(yytext); }
{FUNC}  { _http_lex_dofunc(yytext); }

[^\$&\n]* {
        _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr,  yytext); }

.|\n    { _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr,  yytext); }

%%

#define EARLY  0
#define LATE   1

#ifdef yywrap
#undef yywrap
#endif

int yywrap(void)
{
    return(1);
}

static void _parse_maketimelist(int frwhen)
{
    int i, times[2];
    char buf[BUFLEN];
    struct user u;

    if(_parse_r.auth.name == NULL)
        return;

    u=conf.udb.getuser(_parse_r.auth.name);

    getnormtimes(u.times, times);

    for(i=0; i<24; i++)
    {
        if(i==times[frwhen])
            sprintf(buf, "<option selected>%d\n", i);
        else
            sprintf(buf, "<option>%d\n", i);

        _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, buf);
    }
}

static void _parse_listTerms(void)
{
    char **terms;
    char buf[1024];
    int i;

    _http_auth_require(_parse_sock, _parse_r, "admin");

    terms=listterms();
    stringListSort(terms);

    for(i=0; terms[i]!=NULL; i++)
    {
        sprintf(buf, "<option>%s\n", terms[i]);
        _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, buf);
    }

    cleantermlist(terms);
}

static void _parse_selectTimeE(void)
{
    _parse_maketimelist(EARLY);
}

static void _parse_selectTimeL(void)
{
    _parse_maketimelist(LATE);
}

static void _parse_tableUserList(void)
{
    char **users;
    char **terms;
    char buf[1024];
    struct user u;
    int i, j;

    _http_auth_require(_parse_sock, _parse_r, "admin");

    terms=listterms();
    stringListSort(terms);

    _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr,
        "<table border=\"3\">\n");

    _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr,
        "<tr><td>Username</td><td>Terminal</td></tr>\n");

    for(j=0; terms[j]!=NULL; j++)
    {
        users=conf.udb.listusers(terms[j]);
        stringListSort(users);

        for(i=0; users[i]!=NULL; i++)
        {
            u=conf.udb.getuser(users[i]);
            sprintf(buf, "<tr><td>%s</td><td>%s</td></tr>\n", users[i],
                u.statid);
            _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, buf);
        }

        cleanuserlist(users);
    }

    _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, "</table>");

    cleantermlist(terms);
}

static void _http_lex_dofunc(char *func)
{
    struct namedfunc f[]={
        { "&selectTimeE", _parse_selectTimeE },
        { "&selectTimeL", _parse_selectTimeL },
        { "&selectListTerms", _parse_listTerms },
        { "&tableUserList", _parse_tableUserList },
        { NULL, NULL }
    };

    if(conf.debug>2)
        printf("_http_lex_dofunc got %s\n", func);

    if(execnamedfunc(func, f)<0)
        _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, func);
}

static void _http_lex_showthing(char *thing)
{
    int i;
    static char *parse_vars[]={
        "$version",
        "$user",
        NULL
    };

    if(conf.debug>2)
        printf("_http_lex_showthing got %s\n", thing);

    for(i=0; parse_vars[i]!=NULL; i++)
    {
        if(strcmp(parse_vars[i], thing)==0)
            break;
    }

    switch(i)
    {
        case 0:
            _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, VERSION);
            break;
        case 1:
            _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr,
                _parse_r.auth.name);
            break;

        default:
            _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, thing);
    }
}

void _http_parse(int s, struct http_request r)
{
    char buf[2048];

    /* Set the globals */
    _parse_sock=s;
    _parse_r=r;

    _parse_sendstr=(char *)malloc(sizeof(char)*DEFAULT_STRLEN);
    _parse_strlen=DEFAULT_STRLEN;

    strcpy(buf, conf.webroot);
    strcat(buf, r.request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

    http_checkauth(s, r, buf);

    yyin=fopen(buf, "r");

    yylex();

    if(yyin)
        fclose(yyin);

    _parse_sendstr=addtostr(&_parse_strlen, _parse_sendstr, HTTP_FOOTER);


    if(r.version>0)
    {
        _http_header_ok(s, strlen(_parse_sendstr));
    }

    if(_parse_sendstr)
    {
        puttext(s, _parse_sendstr);
        free(_parse_sendstr);
    }
}
