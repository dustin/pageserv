/*
 * Copyright 1997 Dustin Sallings
 *
 * $Id: pageserv.h,v 1.71 1998/06/29 01:06:38 dustin Exp $
 */

#ifndef PAGESERV_H   /* We don't want this to be */
#define PAGESERV_H 1 /* included more than once. */

/* Debug stuff */
#ifndef PDEBUG
#define PDEBUG 1
#endif

#if (PDEBUG>0)
# ifndef _ndebug
#  define _ndebug(a, b) if(conf.debug > a ) printf b;
# endif
#endif

/* In case it didn't make it */
#ifndef _ndebug
#define _ndebug(a, b)
#endif

/* An exit routine for _mod_pageserv */
#ifndef _mod_pageserv_exit
#define _mod_pageserv_exit(a, b) close(a); exit(b);
#endif

/* for DBM type */
#include <ndbm.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include <config.h>
#include <module.h>

#include <mymalloc.h>

/* Remote host for clients */
#define REMHOST "pager"

/* Port number to run on/connect to */
#define PORT 1029
#define WEBPORT 1030
#define SNPPPORT 1031

/* Config file location */

#ifndef CONFIGFILE
#define CONFIGFILE "/usr/local/etc/pageserv.conf"
#endif /* CONFIGFILE */

#define BUFLEN 1024
#define TOLEN 40
#define FNSIZE 256

#define DEFAULT_STRLEN 1024

#define NAMELEN  15
#define PWLEN    14
#define IDLEN    15
#define STATLEN  20
#define INITLEN  80
#define EMAILLEN 80

/* Misc flags */
#define NOTIFY_SUCC 0x0001
#define NOTIFY_FAIL 0x0002

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
#define WEBROOT "/tmp"

/* Default NIS databases */
#define NIS_DEFAULTUDB "pageserv.users"

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

/* Default delivery scan sleep */
#define DEFAULT_DELSLEEP 60

/* Default modem init string */
#define DEF_INIT "atz"

/* The protocol */

#define P_UNKNOWN -1
#define P_MASH     0
#define P_FARKLE   1
#define P_DEPTH    2
#define P_QUIT     3
#define P_EPAGE    4
#define P_LOGIN    5

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
#define MODE_RUNQ   5    /* Run the queue */
#define MODE_KILL   6    /* Kill off the daemon. */
#define MODE_PWCH   7    /* Password change */
#define MODE_DUMPUSERS 8  /* dump user databases */

/* log types */

#define QUE_LOG     0
#define SUC_LOG     1
#define FAIL_LOG    2
#define EXP_LOG     3
#define DEQUE_LOG   4

/* Function returns */
#define FUNC_UNKNOWN -1

/* PID returns */
#define  PID_NOFILE 1
#define  PID_STALE  2
#define  PID_ACTIVE 3

/* Parser flags */
#define PARSE_GETPASSWD 1

#ifdef IWANT_MODENAMES
static char *modenames[]={
    "daemon",
    "rehashing",
    "listing database",
    "print queue",
    "printing version info",
    "running queue",
    "killing server",
    "changing password"
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
    char notify[EMAILLEN];
    int  flags;
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
    unsigned int rem_addr;
    struct user u;
};

struct terminal {
    char number[STATLEN];
    char ts[FNSIZE];
    char predial[STATLEN];
    char init[INITLEN];
    int  flags;
    int  port;
};

struct namedfunc {
    char *name;
    void (*func)(void);
};

struct userDB {
    char **(*listusers)(char *term);
    void (*eraseuserdb)(void);
    int  (*deleteuser)(char *name);
    void (*storeuser)(struct user u);
    struct user (*getuser)(char *name);
    int (*u_exists)(char *name);
    int (*parseusers)(void);
    void (*dbinit)(void);
};

struct config {
    int mode;              /* execution mode */
    int debug;             /* debug level */
    int deliveryd;         /* deliveryd pid */
    int childlifetime;     /* child lifetime */
    int maxqueuetime;      /* max queue time */
    int farkle;            /* farkle toggle */
    int log_que;           /* logging facility */
    int maxconattempts;    /* maximum attempts to connect to modem server */
    int conattemptsleep;   /* sleep between tries */
    int pageserv;          /* oldschool module */
    int pageport;          /* port to run it on */
    int webserver;         /* start the webserver? */
    int webport;           /* webserver port num */

    int snppserver;        /* start the snppserver? */
    int snppport;          /* snppserver port num */

    char *servhost;        /* server fqdn */
    char *userdb;          /* path to user database */
    char *termdb;          /* path to terminal database */
    char *qdir;            /* path to queue directory */
    char *pidfile;         /* path to pid file */
    char *webroot;         /* path to webroot */

    struct userDB udb;     /* The user database handlers */

    struct confType *cf;   /* The *real* config stuff */

    module *modules;       /* All of the loaded modules */
    int    nmodules;       /* number of modules */
};

/* Let's see if loadable modules are going to be used */

#if defined(HAVE_DLSYM) && defined(HAVE_DLOPEN) && defined(HAVE_DLCLOSE)
#define USE_LM 1
#else
#undef  USE_LM
#endif

/* prototypes */

char **listterms(void);
char *addtostr(int *size, char *dest, char *str);
char *fntoqid(char *fn);
char *getHostName(unsigned int addr);
char *kw(char *in);
char *newqfile(void);
int any_closeterm(int s, struct terminal t);
int any_openterm(struct terminal t);
int bit_set(int bmap, int which);
int checkIPAccess(struct sockaddr_in addr, module *m);
int check_time(struct queuent q);
int checkpidfile(char *filename);
int dexpect(int s, char **what, int timeout);
int execnamedfunc(char *name, struct namedfunc *f);
int f_exists(char *file);
int findGMTOffset(void);
int getservsocket(int port);
int gettext(int s, char *buf);
int gettextcr(int s, char *buf);
int p_openterm(struct terminal t);
int p_unlock(char *dev);
int pack_timebits(int early, int late);
int parseterms(void);
int parseusers(void);
int puttext(int s, char *txt);
int q_islocked(struct queuent q);
int q_lock(struct queuent q);
int q_unlock(struct queuent q);
int queuedepth(void);
int readyqueue(void);
int readytodeliver(struct queuent q);
int s_openterm(struct terminal t);
int storequeue(int s, struct queuent q, int flags);
int t_exists(char *number);
struct queuent *listqueue(char *number);
struct queuent dofarkle(void);
struct queuent readqueuefile(char *fn);
struct terminal getterm(char *key);
struct terminal open_getterm(DBM *db, char *key);
struct user parseuser(char *line, char *delim, int flags);
struct user setpasswd(struct user u, char *passwd);
void checklocks(void);
void cleanconfig(void);
void cleanmylocks(void);
void cleanqueuelist(struct queuent *list);
void cleantermlist(char **list);
void cleanuserlist(char **list);
void dbm_userdbInit(void);
void dequeue(char *qid);
void displayq(struct queuent q);
void dq_notify(struct queuent q, char *message, int flags);
void dumpuserdb(void);
void erasetermdb(void);
void getnormtimes(int times, int *ret);
void getoptions(int argc, char **argv);
void getqueueinfo( struct queuent *q );
void initmodules(void);
void logConnect(struct sockaddr_in fsin, module *m);
void logqueue(struct queuent q, int type, char *reason);
void p_getpasswd(int s, char *to);
void p_login(int s);
void printqueue(void);
void printterm(struct terminal t);
void printterms(void);
void printuser(struct user u);
void printusers(void);
void process(int s, char *cmd, modpass p);
void quit(int s);
void rdconfig(char *file);
void reaper(void);
void resetdelivertraps(void);
void resethappytraps(void);
void resetservtraps(void);
void runqueue(void);
void runqueue_main(void);
void showconfig(void);
void showversion(void);
void sql_userdbInit(void);
void storeterm(DBM *db, struct terminal t);
void stringListSort(char **list);

/* This doesn't even get defined if HAVE_NIS isn't set */
#ifdef HAVE_NIS
void nis_userdbInit(void);
#endif

#endif /* PAGESERV_H */
