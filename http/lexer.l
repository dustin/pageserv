/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: lexer.l,v 1.3 1997/07/10 06:55:44 dustin Exp $
 */

%{
#include <config.h>
#include <pageserv.h>
#include <http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

int _parse_sock;
struct http_request _parse_r;
int _parse_strlen=0;
char *_parse_sendstr;

%}

THING   $[_A-Za-z][_A-Za-z0-9]*
FUNC    &[_A-Za-z][_A-Za-z0-9]*

%%
{THING} { _http_lex_showthing(yytext); }
{FUNC}  { _http_lex_dofunc(yytext); }
.|\n    { _parse_strlen=addtostr(_parse_strlen, _parse_sendstr,  yytext); }

%%

#define EARLY  0
#define LATE   1

int yywrap(void)
{
    return(1);
}

void _parse_maketimelist(int frwhen)
{
    int i, times[2];
    char buf[BUFLEN];
    struct user u;

    if(_parse_r.auth.name == NULL)
        return;

    u=getuser(_parse_r.auth.name);

    getnormtimes(u.times, times);

    for(i=0; i<24; i++)
    {
	if(i==times[frwhen])
	    sprintf(buf, "<option selected>%d\n", i);
	else
	    sprintf(buf, "<option>%d\n", i);

	_parse_strlen=addtostr(_parse_strlen, _parse_sendstr, buf);
    }
}

void _http_lex_dofunc(char *func)
{
    int i;
    static char *parse_funcs[]={
	"&selectTimeE",
	"&selectTimeL",
	NULL
    };

    if(conf.debug>2)
	printf("_http_lex_dofunc got %s\n", func);

    for(i=0; parse_funcs[i]!=NULL; i++)
    {
	if(strcmp(parse_funcs[i], func)==0)
	    break;
    }

    switch(i)
    {
	case 0:
	    _parse_maketimelist(EARLY);
	    break;

	case 1:
	    _parse_maketimelist(LATE);
	    break;

	default:
	    _parse_strlen=addtostr(_parse_strlen, _parse_sendstr, func);
    }
}

void _http_lex_showthing(char *thing)
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
	    _parse_strlen=addtostr(_parse_strlen, _parse_sendstr, VERSION);
	    break;
        case 1:
	    _parse_strlen=addtostr(_parse_strlen, _parse_sendstr,
	        _parse_r.auth.name);
            break;

	default:
	    _parse_strlen=addtostr(_parse_strlen, _parse_sendstr, thing);
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

    _parse_strlen=addtostr(_parse_strlen, _parse_sendstr, HTTP_FOOTER);


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
