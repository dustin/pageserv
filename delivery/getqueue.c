/*
 * Copyright (c) 1997  Dustin Sallings
 *
 * $Id: getqueue.c,v 2.4 1997/04/01 02:22:16 dustin Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <pageserv.h>
#include <tap.h>

extern struct config conf;

void runqueue(void)
{
    struct queuent *q;
    int i;

    q=listqueue("219.1805");

    for(i=0; q[i].to[0] != 0x00 ; i++)
    {
	printf("%d to %s  ``%s''\n", i, q[i].to, q[i].message);
    }

    cleanqueuelist(q);
}

int gq_checkit(struct queuent q, char *number)
{
    if( (strcmp(q.u.statid, number))!=0)
    {
	/* Splat means grab 'em all */
	if(strcmp(number, "*") != 0)
	{
            return(0);
	}
    }

    if( readytodeliver(q) == 0)
    {
	return(0);
    }

    return(1);
}

int queuecompare(const struct queuent *a, const struct queuent *b)
{
    return( a->submitted > b->submitted );
}

void cleanqueuelist(struct queuent *list)
{
    free(list);
}

struct queuent *listqueue(char *number)
{
    DIR *dir;
    FILE *f;
    struct dirent *d;
    char buf[BUFLEN];
    struct queuent q;
    int index=0, size=4;
    struct queuent *list;

    chdir(conf.qdir);
    dir=opendir(".");

    list=(void *)malloc(size*sizeof(struct queuent));

    while( (d=readdir(dir))!=NULL)
    {
        if(d->d_name[0]=='q')
        {
            q=readqueuefile(d->d_name);

	    getqueueinfo(&q);

            if(gq_checkit(q, number))
	    {
		if(index == size-1)
		{
		    size<<=1;

		    if(conf.debug>2)
		    {
		        printf("Reallocating, now need %d bytes for %d\n",
			    size*sizeof(struct queuent *), size);
		    }

		    list=realloc(list, size*sizeof(struct queuent));
		}
		list[index++]=q;
	    }
            /* unlink(q.qid); */
        }
    }
    list[index].to[0]==0x00;
    closedir(dir);

    /* sort the list */
    qsort(list, index, sizeof(struct queuent), queuecompare);
    return(list);
}
