/*
 * Copyright 1997 Dustin Sallings
 *
 * $Id: pageserv.h,v 1.19 1997/04/11 03:45:44 dustin Exp $
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
#define CONFIGFILE "/tmp/pageserv.conf"

#define BUFLEN 1000
#define TOLEN 40
#define FNSIZE 256

#define NAMELEN 15
#define PWLEN   14
#define IDLEN   9
#define STATLEN 20

/* Messages */
#define MESG_QUIT "Good bye, thanks for shopping\n"
#define MESG_NOQUEUE "No pages\n\n\n"
#define MESG_NOUSER "No such user\n\n\n"
#define MESG_BADTIME "User is not accepting normal priority pages now\n\n\n"
#define MESG_WELCOME "Welcome to Dustin's pager server version %s.\n"
#define MESG_NOFARKLE "Sorry, but farkle is not supported.\n\n\n"
#define MESG_TAPFAIL "TAP failure, will keep trying until it expires"
#define MESG_BADPASSWD "Password incorrect\n"

/* prompts */
#define PROMPT_CMD  "CMD: "
#define PROMPT_ID   "ID: "
#define PROMPT_MESS "Message: "
#define PROMPT_PRI  "Priority: "
#define PROMPT_UN   "Username: "
#define PROMPT_PW   "Password: "

/* paths */
#define QUEDIR "/tmp"
#define USERDB "/tmp/userdb"
#define TERMDB "/tmp/termdb"
#define PIDFILE "/tmp/pageserv.pid"

/* Default child lifetime */
#define CHILD_LIFETIME 120

/* Max queue time before deletion */
#define MAX_QUEUETIME  180

/* Don't farkle by default */
#define DEFAULT_FARKLE 0

/* Default maximum modem connection tries */
#define MAX_CONATTEMPTS 15

/* Default sleep between attempts */
#define CONATTEMPTSSLEEP 5

/* The protocol */

#define P_UNKNOWN -1
#define P_MASH     0
#define P_FARKLE   1
#define P_DEPTH    2
#define P_QUIT     3
#define P_EPAGE    4
#define P_APAGE    5
#define P_LOGIN    6

#define P_MAX      6

/* priorities */

#define PR_HIGH   5
#define PR_NORMAL 10

/* run modes */

#define MODE_DAEMON 0    /* daemonize */
#define MODE_REHASH 1    /* rehash databases */
#define MODE_LDB    2    /* list databases */
#define MODE_PQ     3    /* print queue */
#define MODE_VERS   4    /* print version info */
#define MODE_RUNQ   5    /* Run the queue */
#define MODE_KILL   6    /* Kill off the daemon. */

/* log types */

#define QUE_LOG     0
#define SUC_LOG     1
#define FAIL_LOG    2
#define EXP_LOG     3

#ifdef IWANT_MODENAMES
static char *modenames[]={
    "daemon",
    "rehashing",
    "listing database",
    "print queue",
    "printing version info",
    "running queue",
    "killing server"
};
#endif

/* store flags */
#define STORE_NORMAL 0x00
#define STORE_QUIET  0x01

/* type defs */

struct user {
    char name[NAMELEN];
    char passwd[PWLEN];
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
    time_t soonest;
    time_t latest;
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
    int mode;              /* execution mode */
    int debug;             /* debug level */
    int childlifetime;     /* child lifetime */
    int maxqueuetime;      /* max queue time */
    int farkle;            /* farkle toggle */
    int log_que;           /* logging facility */
    int maxconattempts;    /* maximum attempts to connect to modem server */
    int conattemptsleep;   /* sleep between tries */
    char *servhost;        /* server fqdn */
    char *userdb;          /* path to user database */
    char *termdb;          /* path to terminal database */
    char *qdir;            /* path to queue directory */
    char *pidfile;         /* path to pid file */
};

/* macros */

#define puttext(a, b) send(a, b, strlen(b), 0)

/* prototypes */

RETSIGTYPE serv_sighup(int sig);
RETSIGTYPE serv_sigint(int sig);
char **listterms(void);
char *fntoqid(char *fn);
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
int s_openterm(struct terminal t);
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
void cleantermlist(char **list);
void dequeue(char *qid);
void displayq(struct queuent q);
void erasetermdb(void);
void eraseuserdb(void);
void getnormtimes(int times, int *ret);
void getoptions(int argc, char **argv);
void getqueueinfo( struct queuent *q );
void logqueue(struct queuent q, int type, char *reason);
void p_login(int s);
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
void showconfig(void);
void showversion(void);
void storeterm(DBM *db, struct terminal t);
void storeuser(DBM *db, struct user u);

/* client stuff */

char *ckw(char *in);
int pushqueue(char *to, char *message, int priority);
void cgettext(char *message, int size);

#endif /* PAGESERV_H */
