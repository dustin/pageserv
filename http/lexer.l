/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: lexer.l,v 1.1 1997/07/10 04:07:15 dustin Exp $
 */

%{
#include <config.h>
#include <pageserv.h>
#include <http.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

extern struct config conf;

int _parse_sock;
struct http_request _parse_r;

%}

THING   $[_A-Za-z][_A-Za-z0-9]*
FUNC    &[_A-Za-z][_A-Za-z0-9]*

%%
{THING} { _http_lex_showthing(yytext); }
{FUNC}  { _http_lex_dofunc(yytext); }
.|\n    { puttext(_parse_sock, yytext); }

%%

int yywrap(void)
{
    return(1);
}

void _http_lex_dofunc(char *func)
{
    puttext(_parse_sock, "``");
    puttext(_parse_sock, func);
    puttext(_parse_sock, "''");
}

void _http_lex_showthing(char *thing)
{
    int i;
    static char *things[]={
	"$version",
	NULL
    };

    for(i=0; things[i]!=NULL; i++)
    {
	if(strcmp(things[i], thing)==0)
	    break;
    }

    switch(i)
    {
	case 0:
	    puttext(_parse_sock, VERSION);
	    break;

	default:
	    puttext(_parse_sock, thing);
    }
}

void _http_parse(int s, struct http_request r)
{
    char buf[2048];
    struct stat st;

    /* Set the globals */
    _parse_sock=s;
    _parse_r=r;

    strcpy(buf, conf.webroot);
    strcat(buf, r.request);
    if(buf[strlen(buf)-1]=='/')
    {
        strcat(buf, "index.html");
    }

    http_checkauth(s, r, buf);

    if(r.version>0)
    {
        stat(buf, &st);
        _http_header_ok(s, (int)st.st_size+strlen(HTTP_FOOTER)+1);
    }

    yyin=fopen(buf, "r");

    yylex();

    if(yyin)
	fclose(yyin);
}
