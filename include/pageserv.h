/*
 * Copyright 1997 Dustin Sallings
 *
 * $Id: pageserv.h,v 1.7 1997/04/01 05:41:44 dustin Exp $
 */

#ifndef PAGESERV_H   /* We don't want this to be */
#define PAGESERV_H 1 /* included more than once. */

/* for DBM type */
#include <ndbm.h>
#include <sys/types.h>

#include "config.h"

/* Remote host for clients */
#define REMHOST "localhost"

/* Port number to run on/connect to */
#define PORT 1029

/* Config file location */
#define CONFIGFILE "/tmp/sample.config"

#define BUFLEN 1000
#define TOLEN 40
#define FNSIZE 256

#define NAMELEN 15
#define IDLEN   9
#define STATLEN 20

/* Messages */
#define MESG_QUIT "Good bye, thanks for shopping\n"
#define MESG_NOQUEUE "No pages\n\n\n"
#define MESG_NOUSER "No such user\n\n\n"
#define MESG_BADTIME "User is not accepting normal priority pages now\n\n\n"
#define MESG_WELCOME "Welcome to Dustin's pager server version %s.\n"

/* prompts */
#define PROMPT_CMD  "CMD: "
#define PROMPT_ID   "ID: "
#define PROMPT_MESS "Message: "
#define PROMPT_PRI  "Priority: "

/* paths */
#define QUEDIR "/tmp"
#define USERDB "/tmp/userdb"
#define TERMDB "/tmp/termdb"
#define PIDFILE "/tmp/pageserv.pid"

/* Default child lifetime */
#define CHILD_LIFETIME 120

/* Max queue time before deletion */
#define MAX_QUEUETIME  180

/* The protocol */

#define P_UNKNOWN -1
#define P_MASH     0
#define P_FARKLE   1
#define P_DEPTH    2
#define P_QUIT     3
#define P_EPAGE    4
#define P_APAGE    5

#define P_MAX      5

/* priorities */

#define PR_HIGH   5
#define PR_NORMAL 10

/* run modes */

#define MODE_DAEMON 0    /* daemonize */
#define MODE_REHASH 1    /* rehash databases */
#define MODE_LDB    2    /* list databases */
#define MODE_PQ     3    /* print queue */
#define MODE_VERS   4    /* print version info */
#define MODE_RUNQ   5    /* print version info */

#ifdef IWANT_MODENAMES
static char *modenames[]={
    "daemon",
    "rehashing",
    "listing database",
    "print queue",
    "printing version info",
    "running queue"
};
#endif

/* store flags */
#define STORE_NORMAL 0x00
#define STORE_QUIET  0x01

/* type defs */

struct user {
    char name[NAMELEN];
    char pageid[IDLEN];
    char statid[STATLEN];
    int  times;
};

struct queuent {
    int priority;
    char to[TOLEN];
    char message[BUFLEN];
    char qid[FNSIZE];
    time_t submitted;
    struct user u;
};

struct terminal {
    char number[STATLEN];
    char ts[FNSIZE];
    char predial[STATLEN];
    int  contype;
    int  prot;
    int  port;
};

struct config {
    int mode;           /* execution mode */
    int debug;
    int childlifetime;
    int maxqueuetime;
    char *servhost;
    char *userdb;
    char *termdb;
    char *qdir;
    char *pidfile;
};

/* macros */

#define puttext(a, b) send(a, b, strlen(b), 0)

/* prototypes */

char *kw(char *in);
char *newqfile(void);
int bit_set(int bmap, int which);
int check_time(int priority, char *whom);
int f_exists(char *file);
int gettext(int s, char *buf);
int gettextcr(int s, char *buf);
int initialize(void);
int parseterms(void);
int parseusers(void);
int queuedepth(void);
int readytodeliver(struct queuent q);
int set_bit(int bmap, int which);
int storequeue(int s, struct queuent q, int flags);
int t_exists(char *number);
int u_exists(char *name);
struct queuent *listqueue(char *number);
struct queuent dofarkle();
struct queuent readqueuefile(char *fn);
struct terminal getterm(char *key);
struct terminal open_getterm(DBM *db, char *key);
struct user getuser(char *key);
struct user open_getuser(DBM *db, char *key);
void childmain(int s);
void cleanconfig(void);
void cleanqueuelist(struct queuent *list);
void getnormtimes(int times, int *ret);
void getoptions(int argc, char **argv);
void getqueueinfo( struct queuent *q );
void printqueue(void);
void printterm(struct terminal t);
void printterms(void);
void printuser(struct user u);
void printusers(void);
void process(int s, char *cmd);
void quit(int s);
void readconfig(char *file);
void reaper(void);
void resetservtraps(void);
void runqueue(void);
void serv_sighup();
void serv_sigint();
void showconfig(void);
void showversion(void);
void storeterm(DBM *db, struct terminal t);
void storeuser(DBM *db, struct user u);

/* client stuff */

char *ckw(char *in);
int pushqueue(char *to, char *message, int priority);
void cgettext(char *message, int size);

#endif /* PAGESERV_H */
