/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: client.c,v 2.6 1998/06/03 15:39:11 dustin Exp $
 */

/*
 * This is the generic client code stuff.  Probably all that will
 * be needed out of this is pushqueue().  There's also ckw() which
 * is basically the same as kw() from utility.c, but I didn't want
 * to include utility.c in this crap.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <assert.h>

#include <snppclient.h>

static void _snpp_destroy_struct(struct snpp_client *snpp)
{
    if(snpp==NULL)
	return;

    if(snpp->hostname)
	free(snpp->hostname);

    if(snpp->socket>=0)
	(void)close(snpp->socket); /* We don't care, 'cuz we're killin' it */

    if(snpp->indata.buffer)
	free(snpp->indata.buffer);

    free(snpp);
}

static int _getclientsocket(char *host, int port)
{
    struct hostent *hp;
    int success, i, flag;
    register int s=-1;
    struct linger l;
    struct sockaddr_in sin;

    if(host==NULL || port==0)
        return(-1);

    if((hp=gethostbyname(host)) == NULL) {
#ifdef HAVE_HERROR
        herror("gethostbyname");
#else
        fprintf(stderr, "Error looking up %s\n", host);
#endif
        return(-1);
    }

    success=0;

    /* of course, replace that 1 with the max number of con attempts */
    for(i=0; i<1; i++) {
        if((s=socket(AF_INET, SOCK_STREAM, 0))<0) {
            perror("socket");
            return(-1);
        }

        sin.sin_family = AF_INET;
        sin.sin_port=htons(port);
        (void)memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);

        l.l_onoff  = 1;
        l.l_linger = 60;
        setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l));

        flag=1;
        if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
            sizeof(int)) <0) {
            fprintf(stderr, "Nagle algorithm not dislabled.\n");
        }

        if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0) {
            sleep(1);
        } else {
            success=1;
            break;
        }
    }

    if(!success)
       s=-1;

    return(s);
}

char *_killwhitey(char *in)
{
    /* bounds checking */
    if(strlen(in)==0)
        return(in);

    while(isspace(in[strlen(in)-1])) {
        /* bounds checking */
        if(strlen(in)==0)
            return(in);

        in[strlen(in)-1]=0x00;
    }

    return(in);
}

static int _snpp_checkrange(struct snpp_client *snpp, int bottom)
{
    return( snpp->status>=bottom && snpp->status<=bottom+100);
}

static int _snpp_fill_buffer(struct snpp_client *snpp)
{
    char *p;
    int size;

    /* Make sure we have a socket... */
    if(snpp->socket<0)
        return(-1);

    _ndebug(3, ("Looks like we need to fill the buffer some...\n"));

    if(snpp->indata.buf_size>=SNPP_BUF_THRESH) {
        _ndebug(4, ("Buffer got too big (%d), shrinking\n",
                    snpp->indata.buf_size));
        snpp->indata.buf_size=SNPP_BUF_THRESH;
        snpp->indata.buffer=realloc(snpp->indata.buffer,
                                    snpp->indata.buf_size);
        assert(snpp->indata.buffer); /* realloc failed */
        _ndebug(4, ("Buffer is now %d bytes\n", snpp->indata.buf_size));
    }

    snpp->indata.buf_begin=0;
    snpp->indata.buf_end=0;
    snpp->indata.buffer[0]=0x00;

    while(snpp->indata.buf_end<snpp->indata.buf_size &&
          snpp->indata.buffer[snpp->indata.buf_end-1]!='\n') {
         size=recv(snpp->socket, snpp->indata.buffer+snpp->indata.buf_end,
                                 (snpp->indata.buf_size -
                                 snpp->indata.buf_end)-4, 0);
         if(size<1) {
             perror("recv");
             printf("Socket:  %d\n", snpp->socket);
             abort();
         }
         snpp->indata.buf_end+=size;
         /* See if we need a bigger buffer */
         if(snpp->indata.buf_end > ((snpp->indata.buf_size -
                                     snpp->indata.buf_end)-8) ) {
             _ndebug(4, ("Need to allocate a larger receive buffer\n"));
             snpp->indata.buffer=realloc(snpp->indata.buffer,
                                         (snpp->indata.buf_size<<=1));
             assert(snpp->indata.buffer); /* realloc failed */
             _ndebug(4, ("Buffer is now %d bytes\n", snpp->indata.buf_size));
         }
        }
    assert(snpp->indata.buf_end<snpp->indata.buf_size); /* check boundries */
    snpp->indata.buffer[snpp->indata.buf_end]=0x00;

    p=strchr(snpp->indata.buffer, '\n');
    assert(p); /* We read for newlines, this *HAS* to be here */
    *p=NULL;
    return(snpp->indata.buf_end);
}

static char *_snpp_read_cr(struct snpp_client *snpp)
{
    char *p, *ret;
    if(snpp->indata.buf_end == snpp->indata.buf_begin) {
        /* Make sure we get something when we do this. */
        if(_snpp_fill_buffer(snpp)<0)
            return(NULL);
    }

    ret=(snpp->indata.buffer+snpp->indata.buf_begin);

    if(strlen(snpp->indata.buffer) < (size_t)snpp->indata.buf_end) {
        snpp->indata.buf_begin+=(strlen(snpp->indata.buffer+
                                        snpp->indata.buf_begin)+1);
        p=strchr(snpp->indata.buffer+snpp->indata.buf_begin, '\n');

        /* If p is NULL, we're out of data, will need to read again for
           more */
        if(p==NULL)
            snpp->indata.buf_end=snpp->indata.buf_begin=0;
        else
            *p=NULL;
    }

    _ndebug(2, ("<< %s\n", _killwhitey(ret)));

    return(ret);
}

static void _snpp_getstatus(struct snpp_client *snpp, char *in)
{
    assert(snpp);
    assert(in);

    snpp->status=atoi(in);
}

static void _snpp_senddata(struct snpp_client *snpp, char *data)
{
    char *p;
    int size=0;

    assert(snpp);
    assert(data);

    p=strdup(data);
    while(size<strlen(p)) {
        size+=send(snpp->socket, p+size, strlen(p+size), 0);
    }

    _ndebug(2, (">> %s\n", _killwhitey(p)));

    free(p);
}

static void _snpp_client_send(struct snpp_client *snpp, char *msg)
{
    char *mymsg, *p;

    assert(snpp);
    assert(msg);

    mymsg=malloc(strlen(msg)+16);
    strcpy(mymsg, msg);
    p=_killwhitey(mymsg);
    strcat(mymsg, "\r\n");

    _snpp_senddata(snpp, mymsg);

    p=strdup(_snpp_read_cr(snpp));

    _snpp_getstatus(snpp, p);

    free(p);
    free(mymsg);
}

static void _snpp_client_send2(struct snpp_client *snpp, char *a, char *b)
{
    char *p, *mya, *myb;

    assert(snpp);
    assert(a);
    assert(b);

    p=malloc(sizeof(char) * (strlen(a) + strlen(b) + 16));
    assert(p);

    mya=strdup(a);
    myb=strdup(b);

    strcpy(p, _killwhitey(mya));
    strcat(p, " ");
    strcat(p, _killwhitey(myb));

    free(mya);
    free(myb);

    _snpp_client_send(snpp, p);

    free(p);
}

static int _snpp_page(struct snpp_client *snpp, char *whom)
{
    assert(snpp);
    assert(whom);

    _snpp_client_send2(snpp, "page", whom);
    return(_snpp_checkrange(snpp, 200));
}

static int _snpp_message(struct snpp_client *snpp, char *message)
{
    assert(snpp);
    assert(message);

    _snpp_client_send2(snpp, "message", message);
    return(_snpp_checkrange(snpp, 200));
}

static int _snpp_send(struct snpp_client *snpp)
{
    _snpp_client_send(snpp, "send");
    return(_snpp_checkrange(snpp, 200));
}

static void _snpp_quit(struct snpp_client *snpp)
{
    _snpp_client_send(snpp, "quit");
    snpp->destroy(snpp);
}

static int _imap_sendapage(struct snpp_client *snpp, char *whom, char *msg)
{
    int r=0;

    r+=snpp->page(snpp, whom);
    r+=snpp->message(snpp, msg);
    r+=snpp->send(snpp);

    return(r);
}

struct snpp_client *snpp_connect(char *hostname, int port)
{
    struct snpp_client *snpp;

    assert(hostname); /* hostname can't be NULL */

    snpp=malloc(sizeof(struct snpp_client));
    assert(snpp);

    snpp->indata.buf_begin=0;
    snpp->indata.buf_current=0;
    snpp->indata.buf_end=0;
    snpp->indata.buf_size=SNPP_BUF_LEN;

    snpp->indata.buffer=malloc(sizeof(char) * SNPP_BUF_LEN);
    assert(snpp->indata.buffer);

    /* ``member functions'' */
    snpp->destroy=_snpp_destroy_struct;
    snpp->quit=_snpp_quit;
    snpp->send=_snpp_send;
    snpp->page=_snpp_page;
    snpp->message=_snpp_message;

    snpp->sendAPage=_imap_sendapage;

    snpp->rawsend=_snpp_client_send;
    snpp->rawsend2=_snpp_client_send2;

    snpp->debug=0;
    snpp->hostname=strdup(hostname);
    snpp->port=port;
    snpp->socket=_getclientsocket(hostname, port);

    if(snpp->socket<0) {
	free(snpp->hostname);
	free(snpp->indata.buffer);
	free(snpp);
	return(NULL);
    }

    _snpp_getstatus(snpp, _snpp_read_cr(snpp));

    return(snpp);
}
