/*
 * Copyright 1997 Dustin Sallings
 *
 * $Id: pageserv.h,v 1.44 1997/08/09 06:35:38 dustin Exp $
 */

#ifndef PAGESERV_H   /* We don't want this to be */
#define PAGESERV_H 1 /* included more than once. */

/* for DBM type */
#include <ndbm.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include <config.h>
#include <module.h>

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
#define WEBROOT "/tmp"

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

/* log types */

#define QUE_LOG     0
#define SUC_LOG     1
#define FAIL_LOG    2
#define EXP_LOG     3

/* port types */

#define PORT_NET    0
#define PORT_DIRECT 1

/* Function returns */
#define FUNC_UNKNOWN -1

/* PID returns */
#define  PID_NOFILE 1
#define  PID_STALE  2
#define  PID_ACTIVE 3

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
    int  contype;
    int  prot;
    int  port;
};

struct namedfunc {
    char *name;
    void (*func)(void);
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
    int webserver;         /* start the webserver? */
    int webport;           /* webserver port num */

    int snppserver;         /* start the snppserver? */
    int snppport;           /* snppserver port num */
    int gmtoffset;         /* number of hours off of gmt */

    char *servhost;        /* server fqdn */
    char *userdb;          /* path to user database */
    char *termdb;          /* path to terminal database */
    char *qdir;            /* path to queue directory */
    char *pidfile;         /* path to pid file */
    char *webroot;         /* path to webroot */

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

RETSIGTYPE serv_sighup(int sig);
RETSIGTYPE serv_sigint(int sig);
char **listterms(void);
char **listusers(char *term);
char *addtostr(int *size, char *dest, char *str);
char *fntoqid(char *fn);
char *getHostName(unsigned int addr);
char *kw(char *in);
char *newqfile(void);
int _pageserv_socket(void);
int any_closeterm(int s, struct terminal t);
int any_openterm(struct terminal t);
int bit_set(int bmap, int which);
int checkIPAccess(struct sockaddr_in addr, module *m);
int check_time(struct queuent q);
int checkpidfile(char *filename);
int deleteuser(char *name);
int execnamedfunc(char *name, struct namedfunc *f);
int f_exists(char *file);
int getservsocket(int port);
int gettext(int s, char *buf);
int gettextcr(int s, char *buf);
int p_lock(char *dev);
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
int readytodeliver(struct queuent q);
int s_openterm(struct terminal t);
int set_bit(int bmap, int which);
int storequeue(int s, struct queuent q, int flags);
int t_exists(char *number);
int u_exists(char *name);
struct queuent *listqueue(char *number);
struct queuent dofarkle(void);
struct queuent readqueuefile(char *fn);
struct terminal getterm(char *key);
struct terminal open_getterm(DBM *db, char *key);
struct user getuser(char *key);
struct user open_getuser(DBM *db, char *key);
struct user setpasswd(struct user u, char *passwd);
void _pageserv_init(void);
void _pageserv_main(modpass p);
void checklocks(void);
void cleanconfig(void);
void cleanmylocks(void);
void cleanqueuelist(struct queuent *list);
void cleantermlist(char **list);
void cleanuserlist(char **list);
void dequeue(char *qid);
void displayq(struct queuent q);
void erasetermdb(void);
void eraseuserdb(void);
void getnormtimes(int times, int *ret);
void getoptions(int argc, char **argv);
void getqueueinfo( struct queuent *q );
void initmodules(void);
void logConnect(struct sockaddr_in fsin, module *m);
void logqueue(struct queuent q, int type, char *reason);
void open_storeuser(DBM *db, struct user u);
void p_getpasswd(int s, char *to);
void p_login(int s);
void printqueue(void);
void printterm(struct terminal t);
void printterms(void);
void printuser(struct user u);
void printusers(void);
void process(int s, char *cmd);
void quit(int s);
void rdconfig(char *file);
void reaper(void);
void resetdelivertraps(void);
void resetservtraps(void);
void runqueue(void);
void showconfig(void);
void showversion(void);
void storeterm(DBM *db, struct terminal t);
void storeuser(struct user u);
void stringListSort(char **list);

/* client stuff */

char *ckw(char *in);
int pushqueue(char *to, char *message, int priority);
void cgettext(char *message, int size);

#endif /* PAGESERV_H */
