/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: urlparse.c,v 1.5 1997/07/07 08:47:52 dustin Exp $
 */

#include <pageserv.h>
#include <http.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

extern struct config conf;

char _http_X2c(char *u)
{
    char c;
    c=( (u[0]>='A'? ((u[0]&0xdf)-'A')+10 : (u[0]-'0')) << 4);
    c+=(u[1]>='A'?((u[1]&0xdf)-'A')+10 : (u[1]-'0'));
    return(c);
}

void _http_unescape(char *u)
{
    int i, j;

    for(i=0, j=0; u[j]; i++, j++)
    {
        if( (u[i]=u[j]) == '%')
        {
           u[i]=_http_X2c(&u[++j]);
           j++;
        }
    }
    u[i]=0x00;
}

void _http_addtolist(struct http_request *r, char *n, char *v)
{
    struct http_list *list, *tmp;

    list=(struct http_list *)malloc(sizeof(struct http_list));
    list->next=NULL;
    list->name=strdup(n);
    list->value=strdup(v);

    if(r->largs==NULL)
    {
        r->largs=list;
    }
    else
    {
        tmp=r->largs;
        while(tmp->next!=NULL)
            tmp=tmp->next;
        tmp->next=list;
    }
}

char *_http_getcgiinfo(struct http_request r, char *what)
{
    struct http_list *tmp;

    tmp=r.largs;

    while(tmp!=NULL)
    {
        if(strcmp(tmp->name, what)==0)
            break;
        tmp=tmp->next;
    }

    if(tmp==NULL)
        return(NULL);
    else
        return(tmp->value);
}

void _http_parseargs(int s, struct http_request *r)
{
    int i, count;
    char *buf, *pair, *v, *st;
    char **list;

    if(r->method == HTTP_POST)
    {
        r->args=(char *)malloc(sizeof(char)*r->length+1);
        r->args[r->length]=0x00;

        i=0;
        while(i<r->length)
            i+=recv(s, r->args+i, r->length-i, 0);

        if(conf.debug>2)
            printf("Got POST data:  ``%s'' (%d/%d bytes)\n", r->args,
                i, r->length);
    }

    buf=r->args;

    if(buf==NULL)
        return;

    for(i=0; buf[i]; i++)
        if(buf[i]=='+')
            buf[i]=' ';

    list=(char **)malloc(256*sizeof(char **));
    count=0;
    pair=strtok(buf, "&");
    while(pair)
    {
        list[count++]=strdup(pair);
        if( (count%256) == 0)
            list=(char **)realloc(list, (count+256)*sizeof(char **));

        pair=strtok(NULL, "&");
    }
    list[count]=NULL;

    for(i=0; i<count; i++)
    {
        if( (st=strchr(list[i], '=')) )
        {
            *st=0x00;
            _http_unescape(v=st+1);
        }
        else
        {
            _http_unescape(v="");
        }

        _http_addtolist(r, list[i], v);
        free(list[count]);
    }
    free(list);
}
