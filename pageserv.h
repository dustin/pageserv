/*
 * Copyright 1997 Dustin Sallings
 *
 * $Id: pageserv.h,v 1.22 1997/03/28 02:05:37 dustin Exp $
 */

/* for DBM type */
#include <ndbm.h>

#include "config.h"

/* Remote host for clients */
#define REMHOST "localhost"

/* Port number to run on/connect to */
#define PORT 1029

/* Config file location */
#define CONFIGFILE "sample.config"

#define BUFLEN 300
#define TOLEN 40
#define FNSIZE 40

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

/* Default child lifetime */
#define CHILD_LIFETIME 120

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

/* store flags */
#define STORE_NORMAL 0x00
#define STORE_QUIET  0x01

/* type defs */

struct queuent {
    int priority;
    char to[TOLEN];
    char message[BUFLEN];
    char qid[FNSIZE];
};

struct user {
    char name[NAMELEN];
    char pageid[IDLEN];
    char statid[STATLEN];
    int  times;
};

struct config {
    int debug;
    int childlifetime;
    char *servhost;
    char *userdb;
    char *termdb;
    char *qdir;
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
int parseusers(void);
int queuedepth(void);
int set_bit(int bmap, int which);
int storequeue(int s, struct queuent q, int flags);
int u_exists(char *name);
struct queuent dofarkle();
struct user getuser(char *key);
struct user open_getuser(DBM *db, char *key);
void childmain(int s);
void cleanconfig(void);
void getnormtimes(int times, int *ret);
void printqueue(void);
void printuser(struct user u);
void process(int s, char *cmd);
void quit(int s);
void readconfig(char *file);
void reaper(void);
void showconfig(void);
void storeuser(DBM *db, struct user u);

/* client stuff */

char *ckw(char *in);
int pushqueue(char *to, char *message, int priority);
void cgettext(char *message, int size);
